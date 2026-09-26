#pragma once
#include "pipeline_frame_provider.hpp"
namespace rev_hud {
enum class ViewScopeFault : u32 { None, Protocol, Memory, Layout, Changed };
struct NativeViewAccess {
    Reader memory{};
    u32 (*thread_id)(void*) noexcept = nullptr;
};
class NativeViewScope {
public:
    explicit NativeViewScope(NativeViewAccess access) : access_(access) {}
    NativeViewScope(const NativeViewScope&) = delete;
    NativeViewScope& operator=(const NativeViewScope&) = delete;
    bool enter(const ManagerSite&, const ManagerFrame&, u32 context) noexcept;
    bool leave() noexcept;
    bool active() const noexcept { return active_; }
    ViewScopeFault fault() const noexcept { return fault_; }
    PipelineServices services() noexcept;
private:
    struct Anchor {
        u32 context=0,camera=0,sub0=0,view_word=0;
        u32 region[4]{};
    };
    NativeViewAccess access_{};
    Anchor anchor_{};
    u32 thread_=0,phase_=0;
    bool active_=false;
    ViewScopeFault fault_=ViewScopeFault::None;
    bool read(u32,u32&) const noexcept;
    bool draw_anchor(const ManagerFrame&,u32,Anchor&) const noexcept;
    bool same_draw(const Anchor&) const noexcept;
    bool fail(ViewScopeFault f) noexcept { fault_=f; active_=false; return false; }
    void clear() noexcept { anchor_={};thread_=phase_=0;active_=false; }
};
}
