// Real UIKit/Metal host entry point. This file must link the complete generated/native
// game; it is NEVER packaged as an import-screen-only substitute for that executable.
#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>
#import <AVFAudio/AVFAudio.h>
#import <CommonCrypto/CommonDigest.h>
#include "game_session.hpp"
#include <fstream>
#include <memory>
using namespace rac1;
namespace {
NSString* hash_file(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);if(!file)return nil;
    CC_SHA256_CTX context;CC_SHA256_Init(&context);char bytes[65536];
    while(file){file.read(bytes,sizeof bytes);const auto n=file.gcount();if(n)CC_SHA256_Update(&context,bytes,CC_LONG(n));}
    if(!file.eof())return nil;unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256_Final(digest,&context);
    NSMutableString* hex=[NSMutableString stringWithCapacity:64];for(auto b:digest)[hex appendFormat:@"%02x",b];return hex;
}
// Executed before the game starts, on a background queue. Compare against private build-time
// identities, not a user-editable manifest alone. No input or hash is uploaded/logged.
bool verified_data(const platform::apple::GameDataPaths& data,NSDictionary* identity,std::string& error) {
    NSString* iso=identity[@"disc_sha256"],*manifest=identity[@"runtime_manifest_sha256"];
    if(![iso isKindOfClass:NSString.class] || iso.length!=64 || ![manifest isKindOfClass:NSString.class] || manifest.length!=64){error="PRIVATE_AOT_INPUT_IDENTITY_REQUIRED";return false;}
    if(![hash_file(data.iso) isEqualToString:iso]){error="ISO_DOES_NOT_MATCH_COMPILED_GAME";return false;}
    const auto receipt=data.root/"assets/ios-runtime-assets.json";
    if(![hash_file(receipt) isEqualToString:manifest]){error="ASSET_RECEIPT_DOES_NOT_MATCH_COMPILED_GAME";return false;}
    NSData* bytes=[NSData dataWithContentsOfFile:@(receipt.c_str())];
    NSDictionary* record=bytes?[NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil]:nil;
    NSDictionary* files=[record isKindOfClass:NSDictionary.class]?record[@"file_hashes"]:nil;
    if(![files isKindOfClass:NSDictionary.class] || files.count==0){error="VERIFIED_ASSET_HASHES_REQUIRED";return false;}
    for(NSString* name in files){
        if(![name isKindOfClass:NSString.class] || name.isAbsolutePath || [name.pathComponents containsObject:@".."]){error="INVALID_ASSET_RECEIPT_PATH";return false;}
        std::error_code ec;const auto file=std::filesystem::canonical(data.converted/name.UTF8String,ec);
        const auto relative=ec?std::filesystem::path{}:file.lexically_relative(data.converted);
        if(ec || relative.empty() || *relative.begin()==".." || ![hash_file(file) isEqualToString:files[name]]){error="IMPORTED_RUNTIME_ASSET_HASH_MISMATCH";return false;}
    }
    return true;
}
}
@interface RAC1ViewController : UIViewController<MTKViewDelegate> {
    std::unique_ptr<ios::GameSession> game_;
    std::unique_ptr<metal::SceneEncoder> renderer_;
    MTKView* metal_;UILabel* status_;UIButton* check_;
    id<MTLCommandQueue> queue_;id<MTLTexture> retained_frame_,depth_;
    dispatch_semaphore_t slots_;
    BOOL checking_,active_,interrupted_;
}
- (void)setForeground:(BOOL)active;
@end
@implementation RAC1ViewController
- (void)viewDidLoad {
    [super viewDidLoad];self.view.backgroundColor=UIColor.blackColor;
    metal_=[[MTKView alloc]initWithFrame:CGRectZero device:MTLCreateSystemDefaultDevice()];
    // This host explicitly blits its retained GS framebuffer into the drawable.
    // Metal prohibits blit destinations on framebuffer-only drawable textures.
    metal_.framebufferOnly=NO;
    metal_.colorPixelFormat=MTLPixelFormatBGRA8Unorm;metal_.autoResizeDrawable=NO;
    metal_.drawableSize=CGSizeMake(512,416);metal_.preferredFramesPerSecond=60;
    metal_.paused=YES;metal_.delegate=self;[self.view addSubview:metal_];
    status_=[[UILabel alloc]init];status_.numberOfLines=0;status_.textColor=UIColor.whiteColor;status_.textAlignment=NSTextAlignmentCenter;
    status_.text=@"Copy your prepared assets folder into Files → RAC1 Native → RAC1NativeData. Game data stays on this iPhone.";
    [self.view addSubview:status_];check_=[UIButton buttonWithType:UIButtonTypeSystem];
    [check_ setTitle:@"Check Imported Game Data" forState:UIControlStateNormal];
    [check_ addTarget:self action:@selector(checkData) forControlEvents:UIControlEventTouchUpInside];[self.view addSubview:check_];
    slots_=dispatch_semaphore_create(3);
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(audioInterruption:) name:AVAudioSessionInterruptionNotification object:nil];
}
- (void)viewDidAppear:(BOOL)animated { [super viewDidAppear:animated];if(!checking_ && !game_)[self checkData]; }
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];const CGRect safe=UIEdgeInsetsInsetRect(self.view.bounds,self.view.safeAreaInsets);
    // Preserve geometry in the first faithful GS path. Native widened projection/MRP remains a
    // separate unpassed gate: never stretch a 4:3 scene to claim fullscreen support.
    const CGFloat w=MIN(safe.size.width,safe.size.height*4/3),h=w*3/4;
    metal_.frame=CGRectMake(CGRectGetMidX(safe)-w/2,CGRectGetMidY(safe)-h/2,w,h);
    status_.frame=CGRectInset(safe,20,40);check_.frame=CGRectMake(safe.origin.x,safe.origin.y+safe.size.height-48,safe.size.width,44);
}
- (void)showFailure:(const std::string&)reason {
    metal_.paused=YES;if(game_)game_->suspend();status_.hidden=NO;
    status_.text=[NSString stringWithFormat:@"RAC1 stopped safely: %s",reason.c_str()];
    // Bounded error classification only; no ISO bytes, filenames, memory dumps or input identity.
    NSLog(@"[RAC1] STARTUP_OR_FRAME_BLOCKER=%s",reason.c_str());
}
- (void)checkData {
    if(checking_ || game_)return;checking_=YES;check_.enabled=NO;
    const auto documents=[[[NSFileManager defaultManager]URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask]firstObject];
    const std::filesystem::path root=std::filesystem::path(documents.fileSystemRepresentation)/"RAC1NativeData";
    std::error_code directory_error;std::filesystem::create_directories(root,directory_error);
    if(directory_error){checking_=NO;check_.enabled=YES;[self showFailure:std::string("APP_DATA_DIRECTORY_UNAVAILABLE")];return;}
    NSString* identityPath=[[NSBundle mainBundle]pathForResource:@"private-input-identity" ofType:@"json"];
    NSData* identityData=identityPath?[NSData dataWithContentsOfFile:identityPath]:nil;
    NSDictionary* identity=identityData?[NSJSONSerialization JSONObjectWithData:identityData options:0 error:nil]:nil;
    status_.text=@"Verifying local game data against this app's AOT build. No game data is uploaded.";
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
        platform::apple::GameDataPaths paths;std::string error;
        const bool valid=platform::apple::resolve_game_data(root,paths,error) && verified_data(paths,identity,error);
        dispatch_async(dispatch_get_main_queue(),^{
            std::string issue=error;
            checking_=NO;check_.enabled=YES;
            if(!valid){[self showFailure:issue];return;}
            auto library=[metal_.device newDefaultLibrary];
            if(!library){[self showFailure:std::string("PRECOMPILED_METAL_LIBRARY_REQUIRED")];return;}
            game_=std::make_unique<ios::GameSession>();
            if(!game_->configure(paths,issue)){[self showFailure:issue];return;}
            renderer_=std::make_unique<metal::SceneEncoder>(metal_.device,library);queue_=[metal_.device newCommandQueue];
            check_.hidden=YES;status_.hidden=YES;[self updateActivity];
        });
    });
}
- (void)updateActivity {
    const BOOL run=active_ && !interrupted_ && game_ && renderer_;
    if(!run){metal_.paused=YES;if(game_)game_->suspend();return;}
    std::string error;if(!game_->resume(error)){[self showFailure:error];return;}
    metal_.paused=NO;
}
- (void)setForeground:(BOOL)active {active_=active;[self updateActivity];}
- (void)audioInterruption:(NSNotification*)notification {
    const auto kind=[notification.userInfo[AVAudioSessionInterruptionTypeKey]unsignedIntegerValue];
    interrupted_=kind==AVAudioSessionInterruptionTypeBegan;[self updateActivity];
}
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size { (void)view;(void)size;retained_frame_=nil;depth_=nil; }
- (void)drawInMTKView:(MTKView*)view {
    if(!game_ || view.paused || dispatch_semaphore_wait(slots_,DISPATCH_TIME_NOW)!=0)return;
    auto drawable=view.currentDrawable;
    if(!drawable){dispatch_semaphore_signal(slots_);return;}
    std::string error;metal::SceneFrame frame;bool clear=true;
    if(!game_->step(error) || !game_->scene(frame,clear,error)){dispatch_semaphore_signal(slots_);[self showFailure:error];return;}
    if(!retained_frame_) {auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:drawable.texture.pixelFormat
        width:drawable.texture.width height:drawable.texture.height mipmapped:NO];
        d.usage=MTLTextureUsageRenderTarget;d.storageMode=MTLStorageModePrivate;
        retained_frame_=[view.device newTextureWithDescriptor:d];
        d.pixelFormat=MTLPixelFormatDepth32Float;depth_=[view.device newTextureWithDescriptor:d];clear=true;}
    // A persistent framebuffer preserves Held fades; never LOAD uninitialized rotating drawables.
    auto command=[queue_ commandBuffer];
    if(!renderer_->encode(command,retained_frame_,depth_,frame,clear,error)){dispatch_semaphore_signal(slots_);[self showFailure:error];return;}
    auto blit=[command blitCommandEncoder];
    [blit copyFromTexture:retained_frame_ sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0)
        sourceSize:MTLSizeMake(retained_frame_.width,retained_frame_.height,1) toTexture:drawable.texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
    [blit endEncoding];[command presentDrawable:drawable];
    const auto semaphore=slots_;[command addCompletedHandler:^(id<MTLCommandBuffer>){dispatch_semaphore_signal(semaphore);}];
    [command commit];
}
@end
@interface RAC1SceneDelegate : UIResponder<UIWindowSceneDelegate>
@property(nonatomic,strong)UIWindow* window;
@end
@implementation RAC1SceneDelegate
- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    (void)session;(void)options;self.window=[[UIWindow alloc]initWithWindowScene:(UIWindowScene*)scene];
    self.window.rootViewController=[[RAC1ViewController alloc]init];[self.window makeKeyAndVisible];
}
- (void)sceneDidBecomeActive:(UIScene*)scene {(void)scene;[(RAC1ViewController*)self.window.rootViewController setForeground:YES];}
- (void)sceneWillResignActive:(UIScene*)scene {(void)scene;[(RAC1ViewController*)self.window.rootViewController setForeground:NO];}
@end
@interface RAC1AppDelegate : UIResponder<UIApplicationDelegate>
@end
@implementation RAC1AppDelegate
- (UISceneConfiguration*)application:(UIApplication*)app configurationForConnectingSceneSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    (void)app;(void)options;auto config=[[UISceneConfiguration alloc]initWithName:@"RAC1 Native" sessionRole:session.role];
    config.delegateClass=RAC1SceneDelegate.class;return config;
}
@end
int main(int argc,char** argv){@autoreleasepool{return UIApplicationMain(argc,argv,nil,NSStringFromClass(RAC1AppDelegate.class));}}
