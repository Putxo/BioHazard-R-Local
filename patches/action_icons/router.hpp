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
    SnapshotMode (*snapshot)(void*,Frame*) noexcept=nullptr;
    Calls calls{};
};
enum class Result : u32 { Stock, Skipped, Drawn, Refused, RestoreFailed };
// Borrowed native objects only. Never clones or frees embedded ActionIcon GUIs.
// Stock priority is retained; this component isolates draw ownership/claim only.
class Router {
public:
    explicit Router(Host h):host_(h){}
    bool ready() const noexcept;
    Result draw(u32 icon,u32 context) noexcept;
    bool faulted() const noexcept {return faulted_;}
private:
    Host host_;
    Frame current_{};
    u32 manager_=0,claims_[2]{};
    bool busy_=false,faulted_=false;
    bool word(u32,u32&) const noexcept;
    bool manager(u32&) const noexcept;
};
bool bind(Router&) noexcept;
}
extern "C" void rev_action_draw(unsigned int icon,unsigned int context) noexcept;
