#include "runtime.hpp"
namespace rev_runtime {
namespace {Runtime* HealSink=nullptr;}
Runtime::Runtime(Registry& registry,Host host,u32 count) noexcept
    : host_(host), registry_(registry), genesis_(genesis_host()),
      targets_({host.memory,host.thread}), target_views_(targets_,target_view_host()), count_(count),
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
        {this,[](void* p,u32 u,u32 a) noexcept {return static_cast<Runtime*>(p)->heal_feed(u,a);},
        [](void* p,u32 u) noexcept {return static_cast<Runtime*>(p)->scanner_begin(u);},
        [](void* p,u32 u) noexcept {return static_cast<Runtime*>(p)->scanner_end(u);}}),
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
rev_genesis::TargetViewHost Runtime::target_view_host() noexcept {
    return {this,[](void* p) noexcept {
        auto& r=*static_cast<Runtime*>(p);
        return r.started_ && r.host_.thread && r.thread_==r.host_.thread(r.host_.memory.context);
    },[](void* p,u32 widget,rev_genesis::ViewFrame* out) noexcept {
        auto& r=*static_cast<Runtime*>(p);auto h=r.genesis_host();rev_genesis::Owner owner{};
        const auto mode=h.resolve(h.context,widget,&owner);
        if(mode!=Mode::Local)return mode;
        PipelineStamp stamp{};
        if(!out || !r.clock_.capture(&stamp))return Mode::Hidden;
        *out={owner,stamp.frame};return Mode::Local;
    }};
}
bool Runtime::scanner_begin(u32 widget) noexcept {
    auto h=target_view_host();
    if(!h.on_thread(h.context) || !targets_.healthy() || scanner_scope_ || !gate().unit(this,WidgetKind::Scanner,widget))return false;
    rev_genesis::ViewFrame f{};
    if(h.resolve(h.context,widget,&f)!=Mode::Local)return false;
    scanner_scope_=widget;scanner_frame_=f;scanner_copy_failed_=false;return true;
}
bool Runtime::scanner_current() noexcept {
    auto h=target_view_host();rev_genesis::ViewFrame f{};
    if(!h.on_thread(h.context) || !scanner_scope_ ||
       (!detector_producing_ && !gate().unit(this,WidgetKind::Scanner,scanner_scope_)) ||
       h.resolve(h.context,scanner_scope_,&f)!=Mode::Local)return false;
    const auto equal=[](const Actor& a,const Actor& b) noexcept {
        return a.address==b.address && a.lifetime==b.lifetime && a.serial==b.serial && a.think_mode==b.think_mode;
    };
    const auto& a=f.owner;const auto& b=scanner_frame_.owner;
    return f.number==scanner_frame_.number && a.actor==b.actor && a.session.active==b.session.active &&
        a.session.epoch==b.session.epoch && equal(a.session.self,b.session.self) && equal(a.session.sub0,b.session.sub0);
}
bool Runtime::scanner_end(u32 widget) noexcept {
    auto h=target_view_host();
    if(!h.on_thread(h.context))return false;
    const bool ok=widget==scanner_scope_ && !scanner_copy_failed_ && scanner_current();
    scanner_scope_=0;scanner_frame_={};scanner_copy_failed_=false;return ok;
}
Mode Runtime::scanner_sample(u32 target,rev_genesis::ViewSample* out) noexcept {
    auto h=target_view_host();
    // Other threads and ordinary stock callbacks do not inspect scope state.
    if(!h.on_thread(h.context) || !scanner_scope_)return Mode::Stock;
    if(out)*out={};
    if(!out || scanner_copy_failed_ || !scanner_current())return Mode::Hidden;
    return target_views_.read(scanner_scope_,targets_.capture(target),out);
}
Mode Runtime::genesis_target_focus(u32 target,u32* out) noexcept {
    rev_genesis::ViewSample sample{};const auto mode=scanner_sample(target,&sample);
    if(mode!=Mode::Stock && out)*out=mode==Mode::Local?sample.focus:0;
    return mode;
}
Mode Runtime::genesis_target_position(u32 target,u32 destination) noexcept {
    rev_genesis::ViewSample sample{};const auto mode=scanner_sample(target,&sample);
    if(mode==Mode::Stock)return mode;
    u32 before[4]{};
    if(!destination || destination>Invalid-15 || !host_.write){scanner_copy_failed_=true;return Mode::Hidden;}
    for(u32 i=0;i<4;++i)if(!word(destination+i*4,before[i])){scanner_copy_failed_=true;return Mode::Hidden;}
    // January Vector3 copy sets w=0, including when x/y/z are hidden.
    const u32 values[]={sample.position[0],sample.position[1],sample.position[2],0};
    for(u32 i=0;i<4;++i)if(!host_.write(host_.memory.context,destination+i*4,values[i])){
        for(u32 j=0;j<i;++j)(void)host_.write(host_.memory.context,destination+j*4,before[j]);
        scanner_copy_failed_=true;return Mode::Hidden;
    }
    return mode;
}
Mode Runtime::detector_mode() noexcept {
    auto h=target_view_host();
    if(!h.on_thread(h.context) || !detector_producing_)return Mode::Stock;
    if(scanner_copy_failed_ || !scanner_current() || !targets_.healthy()){
        scanner_copy_failed_=true;return Mode::Hidden;
    }
    return Mode::Local;
}
void Runtime::genesis_detect(u32 manager) noexcept {
    if(!host_.native.method0)return;
    auto h=target_view_host();
    if(!h.on_thread(h.context)){
        host_.native.method0(host_.native.context,0x01BBECD1,manager);return;
    }
    const bool nested=detector_busy_;detector_busy_=true;
    const u32 saved_scope=scanner_scope_;const auto saved_frame=scanner_frame_;
    const bool saved_producing=detector_producing_,saved_failed=scanner_copy_failed_;
    // A reentrant ordinary engine call must not inherit the surrounding J2
    // target getter overrides. Preserve the outer scope after its stock pass.
    scanner_scope_=0;scanner_frame_={};detector_producing_=false;scanner_copy_failed_=false;
    host_.native.method0(host_.native.context,0x01BBECD1,manager);
    scanner_scope_=saved_scope;scanner_frame_=saved_frame;
    detector_producing_=saved_producing;scanner_copy_failed_=saved_failed;
    if(nested || saved_scope){detector_busy_=nested;return;}
    if(!targets_.healthy()){
        backend_.quarantine_scanner();lifecycle_.stop();detector_busy_=false;return;
    }
    const u32 widget=lifecycle_.unit(6);rev_genesis::ViewFrame frame{};
    u32 camera_manager=0,camera=0;
    if(!host_.image || !host_.image(host_.memory.context) || !targets_.healthy() ||
       driver_.in_event() || view_.active() || h.resolve(h.context,widget,&frame)!=Mode::Local ||
       !word(0x05799D3C,camera_manager) || genesis_camera(widget,camera_manager,&camera)!=Mode::Local ||
       !backend_.scanner_producer_ready(host_.memory,widget)){
        detector_busy_=false;return;
    }
    scanner_scope_=widget;scanner_frame_=frame;detector_producing_=true;scanner_copy_failed_=false;
    if(scanner_current())host_.native.method0(host_.native.context,0x01BBECD1,manager);
    const bool ok=!scanner_copy_failed_ && scanner_current() && targets_.healthy() &&
        backend_.scanner_producer_ready(host_.memory,widget);
    scanner_scope_=0;scanner_frame_={};detector_producing_=false;
    scanner_copy_failed_=false;detector_busy_=false;
    if(!ok)lifecycle_.stop();
}
Mode Runtime::genesis_detector_camera(u32 manager,u32* out) noexcept {
    const auto mode=detector_mode();
    if(mode==Mode::Stock)return mode;
    if(out)*out=0;
    const auto result=mode==Mode::Local?genesis_camera(scanner_scope_,manager,out):Mode::Hidden;
    if(result!=Mode::Local)scanner_copy_failed_=true;
    return result;
}
Mode Runtime::genesis_detector_actor(u32* out) noexcept {
    const auto mode=detector_mode();
    if(mode!=Mode::Stock && out)*out=mode==Mode::Local?scanner_frame_.owner.actor:0;
    return mode;
}
Mode Runtime::genesis_detector_widget(u32* out) noexcept {
    const auto mode=detector_mode();
    if(mode!=Mode::Stock && out)*out=mode==Mode::Local?scanner_scope_:0;
    return mode;
}
bool Runtime::genesis_candidate(u32 target) noexcept {
    const auto mode=detector_mode();
    if(mode==Mode::Stock)return true;
    rev_genesis::ViewSample sample{};
    return mode==Mode::Local && target_views_.read(scanner_scope_,targets_.capture(target),&sample)==Mode::Local;
}
Mode Runtime::genesis_set_focus(u32 target,u32 value) noexcept {
    const auto mode=detector_mode();
    if(mode!=Mode::Local)return mode;
    const auto key=targets_.capture(target);rev_genesis::ViewSample sample{};
    if(target_views_.read(scanner_scope_,key,&sample)!=Mode::Local)return Mode::Hidden;
    sample.focus=value;const auto result=target_views_.publish(scanner_scope_,key,sample);
    if(result!=Mode::Local)scanner_copy_failed_=true;
    return result;
}
Mode Runtime::genesis_set_position(u32 target,u32 source) noexcept {
    const auto mode=detector_mode();
    if(mode!=Mode::Local)return mode;
    const auto key=targets_.capture(target);
    if(!key.valid())return Mode::Hidden;
    rev_genesis::ViewSample sample{};
    // Every detector pass writes position before classifying focus. Start that
    // pass unfocused instead of carrying a previous view's classification.
    if(!source || source>Invalid-11){scanner_copy_failed_=true;return Mode::Hidden;}
    for(u32 i=0;i<3;++i)if(!word(source+i*4,sample.position[i])){
        scanner_copy_failed_=true;return Mode::Hidden;
    }
    const auto result=target_views_.publish(scanner_scope_,key,sample);
    if(result!=Mode::Local)scanner_copy_failed_=true;
    return result;
}
void Runtime::genesis_remove_target(u32 target) noexcept {
    auto h=target_view_host();
    // The earlier lifetime observer poisons admission for off-thread deaths.
    // Never access owner-thread clone state from that thread.
    if(!h.on_thread(h.context))return;
    if(!host_.image || !host_.image(host_.memory.context) ||
       !backend_.scanner_remove_target(host_.memory,target)){
        backend_.quarantine_scanner();lifecycle_.stop();scanner_copy_failed_=true;
    }
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

extern "C" unsigned int rev_genesis_target_focus(unsigned int target,unsigned int* out) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->genesis_target_focus(target,out)):0;
}
extern "C" unsigned int rev_genesis_target_position(unsigned int target,unsigned int destination) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->genesis_target_position(target,destination)):0;
}

extern "C" unsigned int rev_genesis_detect(unsigned int manager) noexcept {
    if(!rev_runtime::HealSink)return 0;
    rev_runtime::HealSink->genesis_detect(manager);return 1;
}
extern "C" unsigned int rev_genesis_detector_camera(unsigned int manager,unsigned int* out) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->genesis_detector_camera(manager,out)):0;
}
extern "C" unsigned int rev_genesis_detector_actor(unsigned int* out) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->genesis_detector_actor(out)):0;
}
extern "C" unsigned int rev_genesis_detector_widget(unsigned int* out) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->genesis_detector_widget(out)):0;
}
extern "C" unsigned int rev_genesis_candidate(unsigned int target) noexcept {
    return !rev_runtime::HealSink || rev_runtime::HealSink->genesis_candidate(target)?1:0;
}
extern "C" unsigned int rev_genesis_set_focus(unsigned int target,unsigned int value) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->genesis_set_focus(target,value)):0;
}
extern "C" unsigned int rev_genesis_set_position(unsigned int target,unsigned int source) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->genesis_set_position(target,source)):0;
}
extern "C" void rev_genesis_remove_target(unsigned int target) noexcept {
    if(rev_runtime::HealSink)rev_runtime::HealSink->genesis_remove_target(target);
}
