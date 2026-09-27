#pragma once
#include "../action_icons/router.hpp"
namespace rev_runtime {
// Selects the engine's existing immediate sUnit draw path for local View0/1.
// Does not change thread counts, global flags, draw lists or renderer resources.
unsigned int choose_draw_schedule(const rev_action::Host&,unsigned int context,unsigned int stock) noexcept;
// Select the native non-cached model draw branch for a concrete Hunter in a
// certified local view. No command/resource/model fields are changed here.
unsigned int hunter_uncached(const rev_action::Host&,unsigned int model,unsigned int context,unsigned int stock) noexcept;
bool bind_draw_schedule(rev_action::Host) noexcept;
}
extern "C" unsigned int rev_runtime_draw_schedule(unsigned int context,unsigned int stock) noexcept;
extern "C" unsigned int rev_runtime_hunter_uncached(unsigned int model,unsigned int context,unsigned int stock) noexcept;
