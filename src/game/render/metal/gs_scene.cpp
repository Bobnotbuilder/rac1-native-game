#include "gs_scene.hpp"
#include <algorithm>
#include <cmath>
namespace rac1::metal {
namespace { uint32_t field(uint64_t v,unsigned start,unsigned bits) { return uint32_t(v>>start)&((uint32_t{1}<<bits)-1); } }
bool prepare_gs_scene(std::span<const gs::Primitive> primitives,const SceneView& view,SceneFrame& out,std::string& error) {
    auto fail=[&](const char* reason){error=reason;return false;};
    if(!view.width || !view.height || view.width>8192 || view.height>8192 ||
       !std::isfinite(view.depth_a) || !std::isfinite(view.depth_b)) return fail("GS_INVALID_SCENE_VIEW");
    SceneFrame next; next.width=view.width;next.height=view.height;
    for(const auto& p:primitives) {
        const auto& s=p.st;
        if(p.sub || p.fx) return fail("GS_SUBSTITUTED_OR_EXTERNAL_EFFECT_STREAM_REQUIRES_MRP_ORDERING");
        if(p.type!=1 && p.type!=3 && p.type!=6 && p.type!=7) return fail("GS_PRIMITIVE_PATH_NOT_IMPLEMENTED");
        if(p.v.size()!=(p.type==3?3u:2u)) return fail("GS_INVALID_PRIMITIVE_VERTEX_COUNT");
        if(field(s.frame,0,9)!=view.screen_fbp) return fail("GS_RENDER_TARGET_PATH_NOT_IMPLEMENTED");
        if(field(s.fba,0,1) || s.pabe || s.dthe || !s.colclamp) return fail("GS_EXTENDED_FRAGMENT_STATE_NOT_IMPLEMENTED");
        if(p.mip_n || p.mip_lo) return fail("GS_MIP_SELECTION_PATH_NOT_IMPLEMENTED");
        const bool textured=field(s.prim,4,1)!=0 || p.type==7;
        if(textured && (field(s.clamp,0,2)>1 || field(s.clamp,2,2)>1)) return fail("GS_REGION_CLAMP_PATH_NOT_IMPLEMENTED");
        if(textured && (!p.tex || p.tex->w<1 || p.tex->h<1 ||
            uint64_t(p.tex->w)*p.tex->h!=p.tex->px.size())) return fail("GS_DRAW_TIME_TEXTURE_REQUIRED");
        if(textured && p.type!=7 &&
           (p.tex->w!=int(1u<<std::min(10u,field(s.tex0,26,4))) ||
            p.tex->h!=int(1u<<std::min(10u,field(s.tex0,30,4))))) return fail("GS_TEXTURE_SHAPE_DOES_NOT_MATCH_STATE");
        SceneDraw draw; draw.first=uint32_t(next.vertices.size()); draw.lines=p.type==1;
        auto& u=draw.uniform;
        u.texture[0]=textured;u.texture[1]=p.type==7?1:field(s.tex0,35,2);u.texture[2]=p.type==7?1:field(s.tex0,34,1);
        u.texture[3]=field(s.prim,5,1);
        u.alpha[0]=field(s.test,0,1);u.alpha[1]=field(s.test,1,3);u.alpha[2]=field(s.test,4,8);u.alpha[3]=field(s.test,12,2);
        for(unsigned i=0;i<4;++i)u.blend[i]=field(s.alpha,2*i,2);
        u.flags[0]=field(s.prim,6,1);u.flags[1]=field(s.alpha,32,8);u.flags[2]=field(s.test,14,1);u.flags[3]=field(s.test,15,1);
        if(u.flags[0] && std::any_of(u.blend,u.blend+4,[](auto x){return x==3;})) return fail("GS_RESERVED_BLEND_SELECTOR");
        draw.depth_compare=field(s.test,17,2);
        draw.depth_test=view.depth_valid && field(s.test,16,1) && draw.depth_compare!=1;
        draw.depth_write=draw.depth_test && !field(s.zbuf,32,1);
        if(draw.depth_write && u.alpha[0] && u.alpha[3]!=0) return fail("GS_ALPHA_FAIL_SPLIT_DEPTH_PASS_REQUIRED");
        for(unsigned c=0;c<4;++c){u.mask[c]=field(s.frame,32+c*8,8);u.fog[c]=c<3?field(s.fogcol,c*8,8)/255.f:0;}
        const auto x0=std::min(view.width,field(s.scissor,0,11)),y0=std::min(view.height,field(s.scissor,32,11));
        const auto x1=std::min(view.width,field(s.scissor,16,11)+1),y1=std::min(view.height,field(s.scissor,48,11)+1);
        if(x1<=x0 || y1<=y0) continue;
        if(!p.vu && !p.wide && (p.type==6 || p.type==3)) {
            float bx0=p.v.front().x,bx1=bx0,by0=p.v.front().y,by1=by0;
            for(const auto& v:p.v){bx0=std::min(bx0,v.x);bx1=std::max(bx1,v.x);by0=std::min(by0,v.y);by1=std::max(by1,v.y);}
            if(std::ceil(std::max(bx0,float(x0)))>=std::min(bx1,float(x1)) ||
               std::ceil(std::max(by0,float(y0)))>=std::min(by1,float(y1))) continue;
        }
        draw.scissor[0]=x0;draw.scissor[1]=y0;draw.scissor[2]=x1-x0;draw.scissor[3]=y1-y0;
        draw.image=p.tex;draw.linear=field(s.tex1,5,1);draw.repeat_s=field(s.clamp,0,2)==0;draw.repeat_t=field(s.clamp,2,2)==0;
        auto make=[&](const gs::Vertex& v) {
            SceneVertex r;
            r.position[0]=2*v.x/view.width-1;r.position[1]=1-2*v.y/view.height;
            r.position[2]=draw.depth_test?float(std::clamp(view.depth_a+view.depth_b*double(v.z),0.0,1.0)):0.5f;r.position[3]=1;
            r.colour[0]=v.r/255.f;r.colour[1]=v.g/255.f;r.colour[2]=v.b/255.f;r.colour[3]=v.a/255.f;
            const bool fst=field(s.prim,8,1)!=0 || p.type==7;
            r.tex[0]=fst?v.u/float(p.tex?p.tex->w:1):v.s;
            r.tex[1]=fst?v.v/float(p.tex?p.tex->h:1):v.t;
            r.tex[2]=fst || v.q==0?1:v.q;r.fog[0]=v.fog/255.f;
            return r;
        };
        if(p.type==6 || p.type==7) {
            const auto a=make(p.v[0]),b=make(p.v[1]);
            auto c0=b,c1=b,c2=b;
            c0.position[0]=a.position[0];c0.position[1]=a.position[1];
            c0.tex[0]=a.tex[0]/a.tex[2]*b.tex[2];c0.tex[1]=a.tex[1]/a.tex[2]*b.tex[2];
            c1.position[1]=a.position[1];c1.tex[1]=c0.tex[1];
            c2.position[0]=a.position[0];c2.tex[0]=c0.tex[0];
            next.vertices.insert(next.vertices.end(),{c0,c1,c2,c1,b,c2});
        } else for(const auto& v:p.v)next.vertices.push_back(make(v));
        for(size_t i=draw.first;i<next.vertices.size();++i) for(float x:next.vertices[i].position)
            if(!std::isfinite(x))return fail("GS_NONFINITE_VERTEX");
        for(size_t i=draw.first;i<next.vertices.size();++i) for(float x:next.vertices[i].tex)
            if(!std::isfinite(x))return fail("GS_NONFINITE_TEXTURE_COORDINATE");
        draw.count=uint32_t(next.vertices.size())-draw.first;
        next.draws.push_back(std::move(draw));
    }
    out=std::move(next);error.clear();return true;
}
} // namespace rac1::metal
