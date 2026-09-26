#include "native_view_scope.hpp"
namespace rev_hud {
namespace {
constexpr u32 CDrawVtable=0x04F79014;
constexpr u32 CameraGlobal=0x05799D3C;
constexpr u32 CameraVtable=0x04EC9AA8;
constexpr u32 LocalActive=0x057D9188;
constexpr u32 LocalSub0=0x057D9184;
constexpr u32 View1Offset=0x30+0x190;
bool rect_valid(const u32 r[4]) noexcept {
    const i32 left=static_cast<i32>(r[0]), top=static_cast<i32>(r[1]);
    const i32 right=static_cast<i32>(r[2]), bottom=static_cast<i32>(r[3]);
    return right>left && bottom>top;
}
}
bool NativeViewScope::read(u32 a,u32& out) const noexcept {
    return a && a<=Invalid-3 && access_.memory.word &&
        access_.memory.word(access_.memory.context,a,&out);
}
bool NativeViewScope::draw_anchor(const ManagerFrame& frame,u32 context,Anchor& out) const noexcept {
    if (!context || context>Invalid-0x15B || !frame.session.active || !frame.session.epoch ||
        !frame.session.sub0.address || !frame.session.sub0.lifetime ||
        frame.session.sub0.serial<0 || frame.session.sub0.think_mode!=1) return false;
    u32 vt=0,view=0,active=0,sub=0,camera=0,camera_vt=0,state=0,display=0;
    if (!read(context,vt) || vt!=CDrawVtable ||
        !read(context+0x158,view) || (view&0xFF)!=1 ||
        !read(LocalActive,active) || active!=1 ||
        !read(LocalSub0,sub) || sub!=frame.session.sub0.address ||
        !read(CameraGlobal,camera) || !camera || camera>Invalid-(View1Offset+0x24) ||
        !read(camera,camera_vt) || camera_vt!=CameraVtable ||
        !read(camera+View1Offset+0x10,state) ||
        (state&0xFF)!=1 || ((state>>8)&0xFF)!=1 || ((state>>24)&0xFF)!=3 ||
        !read(camera+View1Offset+0x14,display) || (display&0xFF)!=0) return false;
    u32 region[4]{},draw[4]{};
    for(u32 i=0;i<4;++i)
        if(!read(camera+View1Offset+0x18+i*4,region[i]) ||
           !read(context+0xBC+i*4,draw[i]) || region[i]!=draw[i]) return false;
    if(!rect_valid(region)) return false;
    out={context,camera,sub,view,{region[0],region[1],region[2],region[3]}};
    return true;
}
bool NativeViewScope::same_draw(const Anchor& expected) const noexcept {
    u32 vt=0,view=0,active=0,sub=0,camera=0,camera_vt=0,state=0,display=0;
    if (!expected.context || !expected.camera ||
        !read(expected.context,vt) || vt!=CDrawVtable ||
        !read(expected.context+0x158,view) || view!=expected.view_word ||
        !read(LocalActive,active) || active!=1 ||
        !read(LocalSub0,sub) || sub!=expected.sub0 ||
        !read(CameraGlobal,camera) || camera!=expected.camera ||
        !read(camera,camera_vt) || camera_vt!=CameraVtable ||
        !read(camera+View1Offset+0x10,state) ||
        (state&0xFF)!=1 || ((state>>8)&0xFF)!=1 || ((state>>24)&0xFF)!=3 ||
        !read(camera+View1Offset+0x14,display) || (display&0xFF)!=0) return false;
    for(u32 i=0;i<4;++i) {
        u32 a=0,b=0;
        if(!read(camera+View1Offset+0x18+i*4,a) ||
           !read(expected.context+0xBC+i*4,b) ||
           a!=expected.region[i] || b!=expected.region[i]) return false;
    }
    return true;
}
bool NativeViewScope::enter(const ManagerSite& site,const ManagerFrame& frame,u32 context) noexcept {
    if (active_ || fault_!=ViewScopeFault::None || !access_.thread_id)
        return fail(ViewScopeFault::Protocol);
    const u32 thread=access_.thread_id(access_.memory.context);
    if(!thread) return fail(ViewScopeFault::Protocol);
    if(site.phase!=8 && site.phase!=9 && site.phase!=11) return fail(ViewScopeFault::Protocol);
    if(!frame.frame || !frame.session.active) return fail(ViewScopeFault::Layout);
    Anchor next{};
    if(site.phase==11) {
        if(!draw_anchor(frame,context,next)) return fail(ViewScopeFault::Layout);
    } else if(context) return fail(ViewScopeFault::Protocol);
    thread_=thread;phase_=site.phase;anchor_=next;active_=true;return true;
}
bool NativeViewScope::leave() noexcept {
    if(!active_ || fault_!=ViewScopeFault::None || !access_.thread_id)
        return fail(ViewScopeFault::Protocol);
    if(access_.thread_id(access_.memory.context)!=thread_) return fail(ViewScopeFault::Changed);
    if(phase_==11 && !same_draw(anchor_)) return fail(ViewScopeFault::Changed);
    clear();return true;
}
PipelineServices NativeViewScope::services() noexcept {
    PipelineServices s{};
    s.memory={this,[](void* c,u32 a,u32* out) noexcept {
        auto& v=*static_cast<NativeViewScope*>(c);
        return out && v.read(a,*out);
    }};
    s.thread_id=[](void* c) noexcept -> u32 {
        auto& v=*static_cast<NativeViewScope*>(c);
        return v.access_.thread_id ? v.access_.thread_id(v.access_.memory.context) : 0;
    };
    s.enter_scope=[](void* c,const ManagerSite& site,const ManagerFrame& frame,u32 context) noexcept {
        return static_cast<NativeViewScope*>(c)->enter(site,frame,context);
    };
    s.leave_scope=[](void* c) noexcept { return static_cast<NativeViewScope*>(c)->leave(); };
    return s;
}
}
