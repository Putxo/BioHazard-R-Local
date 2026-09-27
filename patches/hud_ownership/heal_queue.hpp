#pragma once
#include "ownership.hpp"
namespace rev_hud {
// Owner-thread queue for a visual notification. Health/inventory stay native.
// Multiple notifications before the next HUD update coalesce into one restart.
class HealQueue {
public:
    static bool session_valid(const Session&) noexcept;
    bool request(const Session&,u32 frame,u32 actor) noexcept;
    bool consume(const Session&,u32 frame,u32 actor) noexcept;
    void clear() noexcept { pending_=false;frame_=0;session_={}; }
private:
    Session session_{};
    u32 frame_=0;
    bool pending_=false;
};
}
