#include "january_admission.hpp"
namespace rev_hud {
bool JanuaryAdmission::on_thread() const noexcept {
    return thread_ && services_.thread_id &&
        services_.thread_id(services_.memory.context)==thread_;
}
bool JanuaryAdmission::image() noexcept {
    return services_.image_ok && services_.image_ok(services_.memory.context);
}
bool JanuaryAdmission::structural() noexcept {
    return on_thread() && image() && gate_.in_event && gate_.view_active &&
        !gate_.in_event(gate_.context) && !gate_.view_active(gate_.context) &&
        services_.structural_safe &&
        services_.structural_safe(services_.memory.context);
}
bool JanuaryAdmission::readable() noexcept {
    if(!on_thread()) return reject(AdmissionFault::Thread);
    if(!image()) return reject(AdmissionFault::Image);
    const bool event=gate_.in_event && gate_.in_event(gate_.context);
    const bool view=gate_.view_active && gate_.view_active(gate_.context);
    const bool safe=services_.structural_safe &&
        services_.structural_safe(services_.memory.context);
    if(safe || (event && view)){last_=AdmissionFault::None;return true;}
    return reject(AdmissionFault::Unsafe);
}
bool JanuaryAdmission::start(u32 thread) noexcept {
    if(thread_ || !thread || !services_.memory.word || !services_.thread_id ||
       !services_.image_ok || !services_.structural_safe || !services_.write_word ||
       !services_.fresh_allocation || !gate_.phase || !gate_.unit ||
       !gate_.in_event || !gate_.view_active)
        return reject(AdmissionFault::Config);
    if(services_.thread_id(services_.memory.context)!=thread)
        return reject(AdmissionFault::Thread);
    thread_=thread;last_=AdmissionFault::None;return true;
}
bool JanuaryAdmission::permit(NativeOp op,WidgetKind kind,u32 unit,u32 phase,u32 context) noexcept {
    if(!on_thread()) return reject(AdmissionFault::Thread);
    if(!image()) return reject(AdmissionFault::Image);
    switch(op){
    case NativeOp::Read:
        return readable();
    case NativeOp::Phase:
        if(gate_.phase(gate_.context,kind,unit,phase,context)){
            last_=AdmissionFault::None;return true;
        }
        return reject(AdmissionFault::Phase);
    case NativeOp::Write:
        if(gate_.unit(gate_.context,kind,unit)){
            last_=AdmissionFault::None;return true;
        }
        return reject(AdmissionFault::Phase);
    case NativeOp::Structural:
    case NativeOp::Initialize:
    case NativeOp::Destroy:
        if(structural()){last_=AdmissionFault::None;return true;}
        return reject(AdmissionFault::Unsafe);
    }
    return reject(AdmissionFault::Config);
}
bool JanuaryAdmission::fresh(u32 address,u32 size) noexcept {
    if(!address || !size || address>Invalid-size)
        return reject(AdmissionFault::Allocation);
    if(!structural()) return reject(AdmissionFault::Unsafe);
    if(!services_.fresh_allocation(services_.memory.context,address,size))
        return reject(AdmissionFault::Allocation);
    last_=AdmissionFault::None;return true;
}
JanuaryHost JanuaryAdmission::callbacks() noexcept {
    JanuaryHost h{};
    h.memory={this,[](void* c,u32 a,u32* out) noexcept {
        auto& ad=*static_cast<JanuaryAdmission*>(c);
        if(!out || !ad.readable() || !ad.services_.memory.word) return false;
        if(!ad.services_.memory.word(ad.services_.memory.context,a,out))
            return ad.reject(AdmissionFault::Memory);
        ad.last_=AdmissionFault::None;return true;
    }};
    h.write=[](void* c,u32 a,u32 v) noexcept {
        auto& ad=*static_cast<JanuaryAdmission*>(c);
        if(!ad.on_thread() || !ad.image() || !ad.gate_.in_event(ad.gate_.context) ||
           !ad.gate_.view_active(ad.gate_.context) || !ad.services_.write_word)
            return ad.reject(AdmissionFault::Memory);
        if(!ad.services_.write_word(ad.services_.memory.context,a,v))
            return ad.reject(AdmissionFault::Memory);
        ad.last_=AdmissionFault::None;return true;
    };
    h.permit=[](void* c,NativeOp op,WidgetKind k,u32 u,u32 p,u32 x) noexcept {
        return static_cast<JanuaryAdmission*>(c)->permit(op,k,u,p,x);
    };
    h.fresh_allocation=[](void* c,u32 a,u32 n) noexcept {
        return static_cast<JanuaryAdmission*>(c)->fresh(a,n);
    };
    return h;
}
}
