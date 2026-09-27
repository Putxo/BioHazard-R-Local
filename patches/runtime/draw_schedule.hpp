#pragma once
#include "../action_icons/router.hpp"
namespace rev_runtime {
// Selects the engine's existing immediate sUnit draw path for local View0/1.
// Does not change thread counts, global flags, draw lists or renderer resources.
unsigned int choose_draw_schedule(const rev_action::Host&,unsigned int context,unsigned int stock) noexcept;
bool bind_draw_schedule(rev_action::Host) noexcept;
}
extern "C" unsigned int rev_runtime_draw_schedule(unsigned int context,unsigned int stock) noexcept;
