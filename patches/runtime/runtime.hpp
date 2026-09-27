#pragma once
#include "../hud_ownership/begin_activator.hpp"
#include "../hud_ownership/p1_view_mask.hpp"
#include "../menu_routing/menu_owner.hpp"
#include "../action_icons/router.hpp"
#include "../action_icons/priority_driver.hpp"
#include "draw_schedule.hpp"
#include "../genesis/progress.hpp"
#include "../genesis/camera.hpp"
#include "../genesis/equipment.hpp"
#include "../genesis/filter_resource.hpp"
#include "../genesis/target_lifetime.hpp"
#include "../genesis/effect_lifetime.hpp"
#include "../genesis/effects.hpp"
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
    // Native bool predicate used by January's actor aim-visibility routine.
    bool (*aim_weapon_hidden)(void*,u32 actor) noexcept=nullptr;
    bool (*weapon_class)(void*,u32 weapon,u32* out) noexcept=nullptr;
    void (*scope_activate)(void*,u32 unit,u32 weapon,u32 flag) noexcept=nullptr;
    rev_genesis::FilterCopyHost filter_copy{};
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
    rev_genesis::EffectLifetime& genesis_effects() noexcept {return effects_;}
    rev_genesis::TargetViews& genesis_target_views() noexcept {return target_views_;}
    Mode genesis_target_focus(u32 target,u32* out) noexcept;
    Mode genesis_target_position(u32 target,u32 destination) noexcept;
    void genesis_detect(u32 manager) noexcept;
    Mode genesis_detector_camera(u32 manager,u32* out) noexcept;
    Mode genesis_detector_actor(u32* out) noexcept;
    Mode genesis_detector_widget(u32* out) noexcept;
    bool genesis_candidate(u32 target) noexcept;
    Mode genesis_set_focus(u32 target,u32 value) noexcept;
    Mode genesis_set_position(u32 target,u32 source) noexcept;
    void genesis_remove_target(u32 target) noexcept;
    Mode genesis_complete(u32 widget,u32 target) noexcept;
    bool genesis_scan_aborted(u32 widget) noexcept;
    bool genesis_notify(u32 target,u32 world_owner,u32 delegate) noexcept;
    void aim_visibility(u32 actor,u32 hide,u32 flag=0) noexcept;
    Mode scope_actor(u32 widget,u32* out) noexcept;
    u32 genesis_filter_loaded(u32 widget,u32 resource) noexcept;
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
    rev_genesis::EffectLifetime effects_;
    rev_genesis::Effects effects_owned_{},effects_stock_{};
    bool scanner_effects(u32 unit,u32 stock,bool prepare) noexcept;
    rev_genesis::TargetViews target_views_;
    rev_genesis::TargetViewHost target_view_host() noexcept;
    u32 scanner_scope_=0;
    rev_genesis::ViewFrame scanner_frame_{};
    bool scanner_copy_failed_=false;
    bool detector_busy_=false,detector_producing_=false;
    bool completion_busy_=false,completion_removed_=false;
    u32 scanner_abort_=0;
    bool aim_busy_=false,aim_faulted_=false;
    struct AimIntent {rev_genesis::Owner owner{};u32 ticket=0,revision=0,hide=0,flag=0;};
    AimIntent aim_intent_{};
    u32 scope_applied_=0,scope_ticket_=0,scope_weapon_=0,scope_class_=0;
    bool scope_feed(u32 unit,u32 actor) noexcept;
    struct Notification {u32 generation=0,owner=0,invoke=0;};
    Notification notifications_[rev_genesis::TargetLifetime::Capacity]{};
    Mode detector_mode() noexcept;
    bool scanner_begin(u32) noexcept;
    bool scanner_end(u32) noexcept;
    bool scanner_current() noexcept;
    bool scanner_weapon_current(u32 widget) noexcept;
    Mode scanner_sample(u32 target,rev_genesis::ViewSample*) noexcept;
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
