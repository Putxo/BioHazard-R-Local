#pragma once
#include "../hud_ownership/begin_activator.hpp"
#include "../hud_ownership/p1_view_mask.hpp"
#include "../menu_routing/menu_owner.hpp"
#include "../action_icons/router.hpp"
#include "../action_icons/priority_driver.hpp"
#include "draw_schedule.hpp"
#include "../genesis/progress.hpp"
#include "../genesis/camera.hpp"
#include "../genesis/target_lifetime.hpp"
#include "../genesis/target_views.hpp"
#include "../hud_ownership/heal_queue.hpp"

namespace rev_runtime {
using namespace rev_hud;
struct Host {
    Reader memory{};
    bool (*write)(void*,u32,u32) noexcept = nullptr;
    u32 (*thread)(void*) noexcept = nullptr;
    bool (*image)(void*) noexcept = nullptr;
    u32 (*self)(void*) noexcept = nullptr;
    JanuaryCalls native{};
    rev_action::Calls action{};
    u32 (*action_rank)(void*,u32) noexcept=nullptr;
};
// One process-lifetime graph. Construct and start before ANY installed gateway
// is reachable. No callback is invoked during member construction.
class Runtime {
public:
    Runtime(Registry&, Host, u32 count=LegacyWidgetKinds) noexcept;
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    bool start() noexcept;
    bool bind() noexcept;
    bool started() const noexcept { return started_; }
    bool structural(u32 frame,u32 owner,u32 frame_base) noexcept;
    LifetimeSource& source() noexcept { return source_; }
    PipelineClock& clock() noexcept { return clock_; }
    StructuralWindow& window() noexcept { return window_; }
    Lifecycle& lifecycle() noexcept { return lifecycle_; }
    BeginActivator& activator() noexcept { return activator_; }
    ManagerDriver& driver() noexcept { return driver_; }
    rev_menu::MenuOwnerRouter& menu() noexcept { return menu_; }
    rev_action::Router& action() noexcept { return action_; }
    rev_action::PriorityDriver& priority() noexcept { return priority_; }
    rev_genesis::Progress& genesis_progress() noexcept {return genesis_;}
    rev_genesis::TargetLifetime& genesis_targets() noexcept {return targets_;}
    rev_genesis::TargetViews& genesis_target_views() noexcept {return target_views_;}
    Mode genesis_camera(u32 widget,u32 manager,u32* out) noexcept {
        return rev_genesis::camera(genesis_host(),host_.memory,widget,manager,out);
    }
    bool heal_event(u32 actor) noexcept; // true means call stock activation
    u32 draw_schedule(u32 context,u32 stock) noexcept {
        return choose_draw_schedule(action_host(),context,stock);
    }
private:
    HealQueue heal_queue_;
    bool heal_feed(u32 unit,u32 actor) noexcept;
    Host host_;
    Registry& registry_;
    rev_genesis::Progress genesis_;
    rev_genesis::TargetLifetime targets_;
    rev_genesis::TargetViews target_views_;
    rev_genesis::TargetViewHost target_view_host() noexcept;
    rev_genesis::ProgressHost genesis_host() noexcept;
    const u32 count_;
    bool attempted_=false,started_=false,bind_attempted_=false;
    u32 thread_=0;
    LifetimeSource source_;
    PipelineClock clock_;
    NativeViewScope view_;
    StructuralWindow window_;
    JanuaryAllocationLedger ledger_;
    AllocationAdmissionBridge allocation_;
    JanuaryAdmission admission_;
    JanuaryBackend backend_;
    Lifecycle lifecycle_;
    PipelineFrameProvider frames_;
    ManagerDriver driver_;
    NativeCoordinatorBindings coordinator_bindings_;
    StructuralCoordinator coordinator_;
    NativeBeginBindings begin_bindings_;
    BeginActivator activator_;
    P1ViewMask mask_;
    rev_menu::MenuOwnerRouter menu_;
    rev_action::PriorityDriver priority_;
    rev_action::Router action_;
    rev_action::Host action_host() noexcept;
    rev_action::PriorityHost priority_host() noexcept;
    AdmissionGate gate() noexcept;
    AdmissionServices services() noexcept;
    bool select_managers() noexcept;
    bool word(u32,u32&) noexcept;
};
}
