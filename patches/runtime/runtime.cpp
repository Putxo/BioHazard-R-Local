#include "runtime.hpp"
namespace rev_runtime {
namespace {Runtime* HealSink=nullptr;}
Runtime::Runtime(Registry& registry,Host host,u32 count) noexcept
    : host_(host), registry_(registry), genesis_(genesis_host()),
      targets_({host.memory,host.thread}), count_(count),
      source_(registry,{host.memory.context,host.memory.word,host.thread,host.self}),
      clock_({host.memory.context,host.thread}),
      view_({host.memory,host.thread}),
      window_(clock_,{host.memory.context,host.thread},
          {this,[](void* p,u32 f,u32 o,u32 b) noexcept {
              return static_cast<Runtime*>(p)->structural(f,o,b);
          }}),
      ledger_(structural_allocation_gate(window_),host.native),
      allocation_(structural_allocation_gate(window_),ledger_,services()),
      admission_(gate(),allocation_.callbacks()),
      backend_(admission_.callbacks(),ledger_.callbacks(),count,&registry,
        {this,[](void* p,u32 u,u32 a) noexcept {return static_cast<Runtime*>(p)->heal_feed(u,a);}}),
      lifecycle_(registry,backend_.callbacks(),count),
      frames_(clock_,source_,view_.services()),
      driver_(lifecycle_,frames_.callbacks()),
      coordinator_bindings_(source_,lifecycle_,backend_,driver_,ledger_),
      coordinator_(host.memory,coordinator_bindings_.callbacks(),count),
      begin_bindings_(coordinator_,driver_),
      activator_(clock_,begin_bindings_.callbacks()),
      mask_({host.memory,host.write},count),
      menu_({this,[](void* p,u32 a,u32* out) noexcept {
          return out && static_cast<Runtime*>(p)->word(a,*out);
      },[](void* p) noexcept -> u32 {
          u32 v=0;return static_cast<Runtime*>(p)->word(0x057D9188,v)?v:0;
      },[](void* p) noexcept -> u32 {
          u32 v=0;return static_cast<Runtime*>(p)->word(0x057D9184,v)?v:0;
      }}), priority_(priority_host()), action_(action_host()) {}

rev_genesis::ProgressHost Runtime::genesis_host() noexcept {
    return {this,[](void* p,u32 widget,rev_genesis::Owner* out) noexcept {
        auto& r=*static_cast<Runtime*>(p);
        const auto route=r.registry_.resolve(widget,WidgetKind::Scanner);
        if(route.mode!=Mode::Local)return route.mode;
        auto h=r.action_host();rev_action::Frame f{};
        if(!out || r.count_<7 || r.lifecycle_.state()!=LifeState::Live ||
           widget!=r.lifecycle_.unit(6) || route.member!=1 || route.view!=1 ||
           h.snapshot(h.memory.context,&f)!=rev_action::SnapshotMode::Local ||
           route.actor!=f.session.sub0.address)return Mode::Hidden;
        *out={f.session,route.actor};return Mode::Local;
    }};
}
bool Runtime::heal_event(u32 actor) noexcept {
    auto h=action_host();rev_action::Frame f{};
    const auto mode=h.snapshot(h.memory.context,&f);
    if(mode==rev_action::SnapshotMode::Stock)return true;
    if(mode!=rev_action::SnapshotMode::Local || !HealQueue::session_valid(f.session))return false;
    if(actor==f.session.self.address)return true;
    if(count_>=6 && lifecycle_.state()==LifeState::Live && lifecycle_.unit(5))
        (void)heal_queue_.request(f.session,f.number,actor);
    return false;
}
bool Runtime::heal_feed(u32 unit,u32 actor) noexcept {
    auto h=action_host();rev_action::Frame f{};
    if(!unit || unit!=lifecycle_.unit(5) ||
       h.snapshot(h.memory.context,&f)!=rev_action::SnapshotMode::Local ||
       actor!=f.session.sub0.address)return false;
    if(heal_queue_.consume(f.session,f.number,actor))
        host_.native.method0(host_.native.context,0x01C88B6C,unit);
    return true;
}
rev_action::Host Runtime::action_host() noexcept {
    return {{this,[](void* p,u32 a,u32* v) noexcept {
        return v && static_cast<Runtime*>(p)->word(a,*v);
    }},[](void* p,u32 a,u32 v) noexcept {
        auto& r=*static_cast<Runtime*>(p);
        return r.host_.write && r.host_.write(r.host_.memory.context,a,v);
    },[](void* p,rev_action::Frame* f) noexcept {
        using M=rev_action::SnapshotMode;
        auto& r=*static_cast<Runtime*>(p);
        u32 active=0;
        if(!f || !r.started_ || !r.word(0x057D9188,active))return M::Unavailable;
        if(!active)return M::Stock;
        PipelineStamp stamp{};LifeSnapshot life{};
        if(!r.clock_.capture(&stamp) || !r.source_.capture(&life))return M::Unavailable;
        *f={stamp.frame,life.session};return M::Local;
    },host_.action,[](void* p,const rev_action::Frame& f,u32 m,u32 member,u32* out) noexcept {
        return static_cast<Runtime*>(p)->priority_.priority(f,m,member,out);
    }};
}
rev_action::PriorityHost Runtime::priority_host() noexcept {
    return {action_host(),[](void* p) noexcept {
        auto& r=*static_cast<Runtime*>(p);
        return r.started_&&r.host_.thread&&r.thread_==r.host_.thread(r.host_.memory.context);
    },[](void* p,u32 raw) noexcept -> u32 {
        auto& r=*static_cast<Runtime*>(p);
        return r.host_.action_rank?r.host_.action_rank(r.host_.memory.context,raw):0;
    }};
}

AdmissionGate Runtime::gate() noexcept {
    // Store this, rather than accessing driver_ before its construction.
    return {this,[](void* p,WidgetKind k,u32 u,u32 ph,u32 c) noexcept {
        auto& r=*static_cast<Runtime*>(p);
        return r.view_.active() && r.driver_.phase_scope(k,u,ph,c);
    },[](void* p,WidgetKind k,u32 u) noexcept {
        auto& r=*static_cast<Runtime*>(p);
        return r.view_.active() && r.driver_.phase_unit_scope(k,u);
    },[](void* p) noexcept {return static_cast<Runtime*>(p)->driver_.in_event();},
      [](void* p) noexcept {return static_cast<Runtime*>(p)->view_.active();}};
}
AdmissionServices Runtime::services() noexcept {
    AdmissionServices s{};s.memory=host_.memory;s.thread_id=host_.thread;
    s.image_ok=host_.image;s.write_word=host_.write;
    // Structural and fresh-allocation permissions come from the real window
    // and ledger via AllocationAdmissionBridge, never an always-true callback.
    return s;
}
bool Runtime::word(u32 a,u32& out) noexcept {
    return a && a<=Invalid-3 && host_.memory.word &&
        host_.memory.word(host_.memory.context,a,&out);
}
bool Runtime::start() noexcept {
    if(attempted_)return false;
    attempted_=true;
    const auto& n=host_.native;
    if(!valid_widget_count(count_) || !host_.memory.word || !host_.write || !host_.thread || !host_.image ||
       !host_.self || !n.allocate || !n.construct || !n.method0 || !n.method1 ||
       !n.singleton || !n.contains || (count_>4 && !n.damage) || !host_.image(host_.memory.context))return false;
    const u32 thread=host_.thread(host_.memory.context);
    thread_=thread;
    started_=thread && targets_.start(thread) && source_.start(thread) && clock_.start(thread) &&
        window_.start(thread) && admission_.start(thread);
    return started_;
}
bool Runtime::bind() noexcept {
    if(!started_ || bind_attempted_)return false;
    bind_attempted_=true;
    // A failed bind must prevent installing/reaching gateways. The graph must
    // remain alive even on failure: some process-lifetime sinks may be bound.
    if(HealSink)return false;
    HealSink=this;
    if(!rev_genesis::bind_progress(genesis_) || !rev_genesis::bind_targets(targets_))return false;
    return bind_lifetime_sink(source_) && bind_pipeline_sink(clock_) &&
        bind_structural_sink(window_) && bind_manager_sink(driver_) &&
        bind_begin_activator_sink(activator_) && bind_p1_mask_sink(mask_) &&
        rev_menu::bind_menu_owner_sink(menu_) && host_.action_rank &&
        rev_action::bind_priority(priority_) && rev_action::bind(action_) &&
        bind_draw_schedule(action_host());
}
bool Runtime::select_managers() noexcept {
    // sIDCockpit[0], verified through 01BE32C5 -> 01CB57A0 and
    // 01C11AEE -> 01CB6A80. Read the actual owner, never guess from a GUI phase.
    u32 owner=0,cockpit=0,minimap=0,again=0,c2=0,m2=0;
    if(!word(0x055623C4,owner) || !owner || owner>Invalid-0x2F ||
       !word(owner+0x28,cockpit) || !word(owner+0x2C,minimap) ||
       !source_.select_managers(cockpit,minimap) ||
       !word(0x055623C4,again) || again!=owner ||
       !word(owner+0x28,c2) || c2!=cockpit ||
       !word(owner+0x2C,m2) || m2!=minimap){
        source_.clear_managers();return false;
    }
    return true;
}
bool Runtime::structural(u32 frame,u32 owner,u32 frame_base) noexcept {
    if(!started_ || !window_.safe())return false;
    (void)select_managers();
    // An absent/retiring owner is a normal empty session. The coordinator
    // drains an existing clone set, instead of permanently poisoning the loop.
    return coordinator_.run(frame,owner,frame_base);
}
}

extern "C" unsigned int rev_hud_heal_request(unsigned int actor) noexcept {
    return rev_runtime::HealSink && rev_runtime::HealSink->heal_event(actor) ? 1u:0u;
}
extern "C" unsigned int rev_genesis_camera(unsigned int widget,unsigned int manager,unsigned int* out) noexcept {
    if(!rev_runtime::HealSink)return 0;
    return static_cast<unsigned int>(rev_runtime::HealSink->genesis_camera(widget,manager,out));
}
