#pragma once
#include "../hud_ownership/lifecycle.hpp"
namespace rev_action {
using namespace rev_hud;
enum class SnapshotMode : u32 { Stock, Local, Unavailable };
struct Frame { u32 number=0; Session session{}; };
struct Calls {
    void* context=nullptr;
    u32 (*member)(void*,u32 command) noexcept=nullptr;
    void (*draw)(void*,u32 icon,u32 render_context) noexcept=nullptr;
};
struct Host {
    Reader memory{};
    bool (*write)(void*,u32,u32) noexcept=nullptr;
    // Local certifies the single owning thread. Off-thread local calls must
    // return Unavailable, before the router accesses any mutable state.
    SnapshotMode (*snapshot)(void*,Frame*) noexcept=nullptr;
    Calls calls{};
    bool (*priority)(void*,const Frame&,u32 manager,u32 member,u32* out) noexcept=nullptr;
};
enum class Result : u32 { Stock, Skipped, Drawn, Refused, RestoreFailed };
// Borrowed native objects only. Never clones or frees embedded ActionIcon GUIs.
// Stock priority is retained; this component isolates draw ownership/claim only.
class Router {
public:
    explicit Router(Host h):host_(h){}
    bool ready() const noexcept;
    Result draw(u32 icon,u32 context) noexcept;
    // Used only by the two audited sUnit draw-list filters, not the global getter.
    u32 mask(u32 unit,u32 original) noexcept;
    u32 draw_priority(u32 manager,u32 original) noexcept;
    bool faulted() const noexcept {return faulted_;}
private:
    Host host_;
    Frame current_{};
    u32 manager_=0,claims_[2]{};
    bool busy_=false,faulted_=false;
    bool priority_active_=false;
    u32 draw_priority_=0;
    bool word(u32,u32&) const noexcept;
    bool manager(u32&) const noexcept;
};
bool bind(Router&) noexcept;
}
extern "C" void rev_action_draw(unsigned int icon,unsigned int context) noexcept;
extern "C" unsigned int rev_action_mask(unsigned int unit,unsigned int original) noexcept;
extern "C" unsigned int rev_action_draw_priority(unsigned int manager,unsigned int original) noexcept;
