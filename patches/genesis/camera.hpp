#pragma once
#include "progress.hpp"
#include "../hud_ownership/lifecycle.hpp"
namespace rev_genesis {
// A CameraManage is not an actor or a cDraw. Resolve only the registered
// scanner's camera; the host must certify the owning thread and actor lifetime.
Mode camera(ProgressHost,rev_hud::Reader,u32 widget,u32 manager,u32* out) noexcept;
}
