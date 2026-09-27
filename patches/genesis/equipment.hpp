#pragma once
#include "progress.hpp"
#include "../hud_ownership/lifecycle.hpp"
namespace rev_genesis {
// Synchronous inventory identity only: does not pin the weapon across engine
// callbacks, assert its dynamic type, or grant permission to activate a scanner.
Mode equipment(ProgressHost,rev_hud::Reader,u32 widget,u32* weapon) noexcept;
}
