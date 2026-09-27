#pragma once
#include "../hud_ownership/ownership.hpp"
namespace rev_genesis {
using rev_hud::u32;
using rev_hud::Actor;
using rev_hud::Session;
using rev_hud::Mode;
enum class Operation : u32 { Read, Set, Add };
struct Owner {
    Session session{};
    u32 actor=0;
};
struct ProgressHost {
    void* context=nullptr;
    // Must certify widget, live registry binding, actor lifetime and owner thread.
    // Unregistered stock widgets return Stock; revoked clones return Hidden.
    Mode (*resolve)(void*,u32 widget,Owner*) noexcept=nullptr;
};
class Progress {
public:
    explicit Progress(ProgressHost host):host_(host){}
    Mode access(u32 widget,Operation op,u32 argument,u32* result) noexcept;
    void reset() noexcept;
private:
    ProgressHost host_{};
    Session session_{};
    u32 value_=0;
    bool bound_=false,busy_=false;
};
// One process-lifetime production binding; absent binding preserves stock.
bool bind_progress(Progress&) noexcept;
}
extern "C" unsigned int rev_genesis_progress(unsigned int widget,
    unsigned int operation,unsigned int argument,unsigned int* result) noexcept;
