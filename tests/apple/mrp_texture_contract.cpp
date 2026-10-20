#ifdef NDEBUG
#error Texture contracts require assertions
#endif
#include <game/render/metal/mrp_texture.hpp>
#include <cassert>
#include <cstdio>
using namespace rac1;
int main() {
    mrp::Texture t; t.width=16; t.height=16;
    t.mips={{},std::vector<uint8_t>(64,17),std::vector<uint8_t>(16,31)};
    t.mips[0].resize(256);
    for (unsigned i=0;i<256;++i) {
        t.mips[0][i]=uint8_t(i);
        t.palette[i]=i | ((255-i)<<8) | (i<<16) | ((i%2 ? 128u : 255u)<<24);
    }
    const mrp::Texture* inputs[]={&t,&t};
    metal::TextureArrayShape shape;
    std::string error; std::vector<uint8_t> rgba;
    assert(metal::validate_texture_array(inputs,3,shape,error));
    assert(shape.width==16 && shape.height==16 && shape.layers==2 && shape.levels==3);
    assert(metal::decode_texture_mip(t,0,rgba,error));
    for (unsigned i=0;i<256;++i) {
        const unsigned swizzle=(i&~0x18u)|((i&8)<<1)|((i&16)>>1);
        const uint32_t want=t.palette[swizzle];
        for (unsigned c=0;c<4;++c) assert(rgba[i*4+c]==uint8_t(want>>(8*c)));
    }
    assert(metal::decode_texture_mip(t,1,rgba,error) && rgba.size()==64*4);
    const auto before=rgba;
    t.mips[1].pop_back();
    assert(!metal::decode_texture_mip(t,1,rgba,error) && rgba==before);
    assert(!metal::decode_texture_mip(t,1,rgba,error,metal::MissingMipPolicy::UpstreamZeroFill) && rgba==before);
    const auto old_shape=shape;
    assert(!metal::validate_texture_array(inputs,3,shape,error));
    assert(shape.layers==old_shape.layers && shape.levels==old_shape.levels);
    assert(!metal::validate_texture_array({},1,shape,error));
    const mrp::Texture* empty[]={nullptr};
    assert(!metal::validate_texture_array(empty,1,shape,error));
    t.width=0; assert(!metal::decode_texture_mip(t,0,rgba,error));
    t.width=16; assert(!metal::decode_texture_mip(t,32,rgba,error));
    mrp::Texture one; one.width=1; one.height=1; one.mips={{0},{0}};
    assert(metal::decode_texture_mip(one,0,rgba,error));
    assert(!metal::decode_texture_mip(one,1,rgba,error));
    const mrp::Texture* different[]={&t,&one};
    assert(!metal::validate_texture_array(different,1,shape,error));
    t.mips.resize(1);
    assert(!metal::validate_texture_array(inputs,3,shape,error));
    assert(metal::validate_texture_array(inputs,3,shape,error,metal::MissingMipPolicy::UpstreamZeroFill));
    assert(shape.zero_filled_mips==4);
    assert(metal::decode_texture_mip(t,2,rgba,error,metal::MissingMipPolicy::UpstreamZeroFill));
    assert(rgba==std::vector<uint8_t>(16*4,0));
    std::puts("PASS: all 256 CSM1 palette entries, raw alpha, mips, array shape and fail-closed malformed inputs");
}
