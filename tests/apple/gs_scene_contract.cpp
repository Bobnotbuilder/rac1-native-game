#ifdef NDEBUG
#error GS scene assertions must remain enabled
#endif
#include <game/render/metal/gs_scene.hpp>
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace rac1;
int main() {
    gs::Primitive p;p.type=6;p.st.colclamp=1;p.st.scissor=511ull<<16|415ull<<48;
    p.v.resize(2);p.v[0].x=0;p.v[0].y=0;p.v[0].r=13;
    p.v[1].x=512;p.v[1].y=416;p.v[1].r=255;p.v[1].a=128;
    std::vector<gs::Primitive> input{p};metal::SceneFrame result;std::string error;
    assert(metal::prepare_gs_scene(input,{},result,error));
    assert(result.vertices.size()==6 && result.draws.size()==1);
    assert(result.vertices[0].position[0]==-1 && result.vertices[0].position[1]==1);
    assert(result.vertices[4].position[0]==1 && result.vertices[4].position[1]==-1);
    for(const auto& v:result.vertices)assert(v.colour[0]==1); // sprite takes SECOND colour
    assert(result.draws[0].scissor[2]==512 && result.draws[0].scissor[3]==416);
    p.st.frame=1;input[0]=p;
    assert(!metal::prepare_gs_scene(input,{},result,error) && error=="GS_RENDER_TARGET_PATH_NOT_IMPLEMENTED");
    assert(result.vertices.size()==6); // rejected frame cannot replace prior valid data
    p.st.frame=0;p.mip_n=1;input[0]=p;
    assert(!metal::prepare_gs_scene(input,{},result,error) && error=="GS_MIP_SELECTION_PATH_NOT_IMPLEMENTED");
    p.mip_n=0;p.st.prim=1ull<<6;p.st.alpha=3;input[0]=p;
    assert(!metal::prepare_gs_scene(input,{},result,error) && error=="GS_RESERVED_BLEND_SELECTOR");
    p.st.alpha=0;p.st.test=1ull|1ull<<12|1ull<<16|3ull<<17;input[0]=p;
    metal::SceneView depth;depth.depth_valid=true;
    assert(!metal::prepare_gs_scene(input,depth,result,error) && error=="GS_ALPHA_FAIL_SPLIT_DEPTH_PASS_REQUIRED");
    p.st.test=0;p.st.prim=0;p.v[0].x=511.5;p.v[1].x=512;input[0]=p;
    assert(metal::prepare_gs_scene(input,{},result,error) && result.draws.empty());
    p.v[0].x=0;p.v[1].x=512;p.v[0].q=NAN;input[0]=p;
    assert(!metal::prepare_gs_scene(input,{},result,error) && error=="GS_NONFINITE_TEXTURE_COORDINATE");
    p.v[0].q=1;p.v.resize(1);input[0]=p;
    assert(!metal::prepare_gs_scene(input,{},result,error) && error=="GS_INVALID_PRIMITIVE_VERTEX_COUNT");
    std::puts("PASS: original GS stream screen geometry, second-vertex sprite colour, scissor, narrow-edge exclusion, atomic frame rejection; unsupported target/mip/depth/selector paths remain explicit blockers");
}
