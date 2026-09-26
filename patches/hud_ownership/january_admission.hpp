#pragma once
#include "january_backend.hpp"
#include "native_view_scope.hpp"
namespace rev_hud {
struct AdmissionGate {
    void* context=nullptr;
    bool (*phase)(void*,WidgetKind,u32,u32,u32) noexcept=nullptr;
    bool (*unit)(void*,WidgetKind,u32) noexcept=nullptr;
    bool (*in_event)(void*) noexcept=nullptr;
    bool (*view_active)(void*) noexcept=nullptr;
};
// Concrete adapter to the actual ManagerDriver + NativeViewScope. The object
// itself must outlive JanuaryAdmission.
struct DriverViewAdmission {
    ManagerDriver& driver;
    NativeViewScope& view;
    AdmissionGate callbacks() noexcept {
        return {this,
            [](void* c,WidgetKind k,u32 u,u32 p,u32 x) noexcept {
                auto& a=*static_cast<DriverViewAdmission*>(c);
                return a.view.active() && a.driver.phase_scope(k,u,p,x);
            },
            [](void* c,WidgetKind k,u32 u) noexcept {
                auto& a=*static_cast<DriverViewAdmission*>(c);
                return a.view.active() && a.driver.phase_unit_scope(k,u);
            },
            [](void* c) noexcept {
                return static_cast<DriverViewAdmission*>(c)->driver.in_event();
            },
            [](void* c) noexcept {
                return static_cast<DriverViewAdmission*>(c)->view.active();
            }};
    }
};
struct AdmissionServices {
    Reader memory{};
    u32 (*thread_id)(void*) noexcept=nullptr;
    bool (*image_ok)(void*) noexcept=nullptr;
    bool (*structural_safe)(void*) noexcept=nullptr;
    bool (*write_word)(void*,u32,u32) noexcept=nullptr;
    bool (*fresh_allocation)(void*,u32,u32) noexcept=nullptr;
};
enum class AdmissionFault : u32 { None, Config, Thread, Image, Unsafe, Phase, Memory, Allocation };
class JanuaryAdmission {
public:
    JanuaryAdmission(AdmissionGate gate, AdmissionServices services)
        : gate_(gate), services_(services) {}
    bool start(u32 owning_thread) noexcept;
    JanuaryHost callbacks() noexcept;
    AdmissionFault last_fault() const noexcept { return last_; }
private:
    AdmissionGate gate_{};
    AdmissionServices services_{};
    u32 thread_=0;
    AdmissionFault last_=AdmissionFault::None;
    bool on_thread() const noexcept;
    bool image() noexcept;
    bool structural() noexcept;
    bool readable() noexcept;
    bool permit(NativeOp,WidgetKind,u32,u32,u32) noexcept;
    bool fresh(u32,u32) noexcept;
    bool reject(AdmissionFault f) noexcept { last_=f;return false; }
};
}
