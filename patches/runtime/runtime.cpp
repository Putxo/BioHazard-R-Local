#include "runtime.hpp"
namespace rev_runtime {
namespace {
Runtime* HealSink=nullptr;
bool same_genesis_owner(const rev_genesis::Owner& a,const rev_genesis::Owner& b) noexcept {
    const auto actor=[](const Actor& x,const Actor& y) noexcept {
        return x.address==y.address && x.lifetime==y.lifetime && x.serial==y.serial && x.think_mode==y.think_mode;
    };
    return a.actor==b.actor && a.session.active==b.session.active && a.session.epoch==b.session.epoch &&
        actor(a.session.self,b.session.self) && actor(a.session.sub0,b.session.sub0);
}
bool same_effect_frame(const rev_genesis::ViewFrame& a,const rev_genesis::ViewFrame& b) noexcept {
    return a.number==b.number && same_genesis_owner(a.owner,b.owner);
}
}
Runtime::Runtime(Registry& registry,Host host,u32 count) noexcept
    : host_(host), registry_(registry), genesis_(genesis_host()),
      targets_({host.memory,host.thread}), effects_({host.memory,host.thread}),
      weapons_({host.memory,host.thread}),
      target_views_(targets_,target_view_host()), count_(count),
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
        [](void* p,u32 u) noexcept {return static_cast<Runtime*>(p)->scanner_end(u);},
        [](void* p,u32 u,u32 a) noexcept {return static_cast<Runtime*>(p)->scope_feed(u,a);},
        [](void* p,u32 u,u32 s,bool init) noexcept {return static_cast<Runtime*>(p)->scanner_effects(u,s,init);},
        [](void* p,u32 u,u32 s) noexcept {return static_cast<Runtime*>(p)->scanner_retire(u,s);}}),
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
bool Runtime::scanner_effects(u32 unit,u32 stock,bool prepare) noexcept {
    auto h=target_view_host();
    if(!h.on_thread(h.context) || !host_.image || !host_.image(host_.memory.context) ||
       unit!=backend_.retained(WidgetKind::Scanner) || !effects_.healthy())return false;
    if(prepare && (lifecycle_.state()!=LifeState::Preparing || !host_.write))return false;
    if(!prepare && (effects_owned_.scanner!=unit || effects_stock_.scanner!=stock))return false;
    // Native effect allocations are inspected only after their observed keys
    // have been admitted by capture_effects. No pointer-only liveness fallback.
    rev_genesis::Effects owned{},original{};
    if(!rev_genesis::capture_effects(host_.memory,effects_,unit,&owned) ||
       !rev_genesis::capture_effects(host_.memory,effects_,stock,&original) ||
       !rev_genesis::disjoint_effects(owned,original))return false;
    if(prepare){
        // The serialized structural window prevents scheduled draws here.
        // Change only the three exclusively owned roots, preserving all unit
        // state, activity and scheduling flags. Child render clipping is separate.
        for(const auto& key:owned.units){u32 flags=0;
            if(!effects_.live(key) || !word(key.address+12,flags) || !effects_.live(key) ||
               !host_.write(host_.memory.context,key.address+12,(flags&~0x03FF0000u)|0x20000u))return false;
        }
        effects_owned_=owned;effects_stock_=original;
        return scanner_effects(unit,stock,false);
    }
    return rev_genesis::same_effects(owned,effects_owned_) &&
        rev_genesis::same_effects(original,effects_stock_) && rev_genesis::effect_view(host_.memory,owned,1);
}
bool Runtime::scanner_retire(u32 unit,u32 stock) noexcept {
    // Called only by JanuaryBackend after its Destroy admission and graph
    // validation. No gameplay callback or render work may run in this window.
    return scanner_effects(unit,stock,false) &&
        rev_genesis::retire_effects(host_.memory,effects_,effects_owned_,host_.write);
}
u32 Runtime::unit_mask(u32 unit,u32 original) noexcept {
    auto h=target_view_host();
    // Local views use the native immediate draw path. Other threads must not
    // inspect these owner-thread certificates, including during replacement.
    if(!h.on_thread(h.context))return action_.mask(unit,original);
    rev_genesis::EffectKey key{};bool owned=false,found=false;
    for(const auto& k:effects_owned_.units)if(k.valid() && k.address==unit){key=k;owned=true;found=true;}
    if(!found)for(const auto& k:effects_stock_.units)if(k.valid() && k.address==unit){key=k;found=true;}
    if(!found)return action_.mask(unit,original);
    const u32 refused=owned?0:original;
    if(!effects_.healthy())return refused;
    const auto now=effects_.capture(unit,key.kind);
    // A new observed allocation at the same address is not our old effect.
    if(now.valid() && now.generation!=key.generation)return original;
    if(!effects_.live(key) || effect_mask_busy_)return refused;
    struct Busy {bool& value;explicit Busy(bool& v):value(v){value=true;}~Busy(){value=false;}} busy(effect_mask_busy_);
    rev_genesis::ViewFrame before{},after{};
    const u32 scanner=effects_owned_.scanner;
    if(h.resolve(h.context,scanner,&before)!=Mode::Local ||
       !scanner_effects(scanner,effects_stock_.scanner,false) ||
       h.resolve(h.context,scanner,&after)!=Mode::Local || !effects_.live(key))return refused;
    if(!same_effect_frame(before,after))return refused;
    // Never widen an inactive mask, and never rewrite J1's native flags.
    // Auxiliary views retain their stock bits for J1; J2 owns only View1.
    return owned?(original&2u):(original&~2u);
}
bool Runtime::effect_draw(u32 kind,u32 unit,u32 context) noexcept {
    auto h=target_view_host();
    if(!h.on_thread(h.context) || kind>=3)return false;
    const auto key=effects_owned_.units[kind];
    if(!key.valid() || key.address!=unit)return false;
    const auto now=effects_.capture(unit,key.kind);
    if(now.valid() && now.generation!=key.generation)return false;
    // A recognized local root is always handled, including refusal. Unrelated
    // units (also private FilterSet children) retain their native call path.
    if(effect_draw_busy_ || !effects_.live(key) || !host_.effect_draw)return true;
    struct Busy {bool& value;explicit Busy(bool& v):value(v){value=true;}~Busy(){value=false;}} busy(effect_draw_busy_);
    rev_genesis::ViewFrame before{},after{};const u32 scanner=effects_owned_.scanner;
    if(h.resolve(h.context,scanner,&before)!=Mode::Local || unit_mask(unit,2)!=2)return true;
    NativeViewScope draw({host_.memory,host_.thread});
    ManagerFrame frame{};frame.frame=before.number;frame.session=before.owner.session;
    if(!draw.enter({0,ManagerKind::Cockpit,11},frame,context))return true;
    if(!effects_.live(key) || h.resolve(h.context,scanner,&after)!=Mode::Local ||
       !same_effect_frame(before,after)){(void)draw.leave();return true;}
    host_.effect_draw(host_.memory.context,kind,unit,context);
    if(!draw.leave() || !effects_.live(key) || h.resolve(h.context,scanner,&after)!=Mode::Local ||
       !same_effect_frame(before,after) || unit_mask(unit,2)!=2){
        backend_.quarantine_scanner();lifecycle_.stop();
    }
    return true;
}
u32 Runtime::genesis_filter_loaded(u32 widget,u32 resource) noexcept {
    if(!widget || widget!=backend_.retained(WidgetKind::Scanner))return resource;
    auto h=target_view_host();
    if(!h.on_thread(h.context))return 0;
    const bool admitted=lifecycle_.state()==LifeState::Preparing && backend_.scanner_loading(widget) && host_.image &&
        host_.image(host_.memory.context) && host_.native.method0;
    const u32 copy=admitted && resource?rev_genesis::copy_filter_resource(host_.filter_copy,resource):0;
    // The loader has returned one cached reference. It must never reach
    // uFilterSet::setResource on J2, which initializes every mutable child.
    if(resource && host_.native.method0)host_.native.method0(host_.native.context,0x01C3C159,resource);
    if(!copy || lifecycle_.state()!=LifeState::Preparing || !backend_.scanner_loading(widget) ||
       !host_.image(host_.memory.context)) {
        backend_.quarantine_scanner();return 0;
    }
    return copy;
}
Mode Runtime::scope_actor(u32 widget,u32* out) noexcept {
    const auto route=registry_.resolve(widget,WidgetKind::Scope);
    if(route.mode==Mode::Stock)return Mode::Stock;
    if(out)*out=0;
    auto h=action_host();rev_action::Frame f{};
    if(!out || count_<8 || lifecycle_.state()!=LifeState::Live ||
       widget!=lifecycle_.unit(7) || route.mode!=Mode::Local || route.member!=1 || route.view!=1 ||
       !gate().unit(this,WidgetKind::Scope,widget) ||
       h.snapshot(h.memory.context,&f)!=rev_action::SnapshotMode::Local || route.actor!=f.session.sub0.address)return Mode::Hidden;
    *out=route.actor;return Mode::Local;
}
void Runtime::aim_visibility(u32 actor,u32 hide,u32 flag) noexcept {
    auto vh=target_view_host();
    if(!vh.on_thread(vh.context))return;
    if(aim_busy_ || aim_faulted_ || !host_.write || !host_.aim_weapon_hidden ||
       !host_.image || !host_.image(host_.memory.context) || count_<7 ||
       lifecycle_.state()!=LifeState::Live)return;
    const u32 widget=lifecycle_.unit(6);
    rev_genesis::ViewFrame before{},after{};
    u32 manager=0,camera=0,weapon=0;
    if(vh.resolve(vh.context,widget,&before)!=Mode::Local || before.owner.actor!=actor ||
       !actor || actor>Invalid-15 || !word(0x05799D3C,manager) ||
       genesis_camera(widget,manager,&camera)!=Mode::Local ||
       rev_genesis::equipment(genesis_host(),host_.memory,widget,&weapon)!=Mode::Local ||
       (weapon && (weapon>Invalid-15 || weapon==actor || weapon==before.owner.session.self.address)))return;
    struct Busy {bool& value;explicit Busy(bool& b):value(b){value=true;}~Busy(){value=false;}} busy(aim_busy_);
    const bool affect_weapon=weapon && host_.aim_weapon_hidden(host_.memory.context,actor);
    const auto same_actor=[](const Actor& a,const Actor& b) noexcept {
        return a.address==b.address && a.lifetime==b.lifetime &&
            a.serial==b.serial && a.think_mode==b.think_mode;
    };
    u32 now=0,camera_after=0;
    if(vh.resolve(vh.context,widget,&after)!=Mode::Local || after.number!=before.number ||
       after.owner.actor!=actor || after.owner.session.epoch!=before.owner.session.epoch ||
       !same_actor(before.owner.session.self,after.owner.session.self) ||
       !same_actor(before.owner.session.sub0,after.owner.session.sub0) ||
       genesis_camera(widget,manager,&camera_after)!=Mode::Local || camera_after!=camera ||
       rev_genesis::equipment(genesis_host(),host_.memory,widget,&now)!=Mode::Local || now!=weapon)return;
    u32 actor_flags=0,weapon_flags=0;
    if(!word(actor+12,actor_flags) || (affect_weapon && !word(weapon+12,weapon_flags)))return;
    // Native get/set pair modifies bit 0 of the ten-bit view mask. This
    // secondary-camera branch modifies only bit 1; every other flag survives.
    const auto changed=[&](u32 flags) noexcept {return (hide&255)==1?flags&~0x20000u:flags|0x20000u;};
    const u32 next_actor=changed(actor_flags),next_weapon=changed(weapon_flags);
    if(next_actor!=actor_flags && !host_.write(host_.memory.context,actor+12,next_actor))return;
    if(affect_weapon && next_weapon!=weapon_flags && !host_.write(host_.memory.context,weapon+12,next_weapon)){
        if(next_actor!=actor_flags && !host_.write(host_.memory.context,actor+12,actor_flags)){
            aim_faulted_=true;lifecycle_.stop();
        }
        return;
    }
    const u32 ticket=activator_.attached_ticket();
    if(count_>=8 && lifecycle_.live_ticket(ticket)) {
        if(aim_intent_.revision==Invalid){aim_faulted_=true;lifecycle_.stop();return;}
        aim_intent_={before.owner,ticket,aim_intent_.revision+1,hide&255,flag&255};
    }
}
bool Runtime::scope_feed(u32 unit,u32 actor) noexcept {
    if(!aim_intent_.ticket)return true; // Native constructor leaves Scope inactive.
    const auto intent=aim_intent_;
    const u32 ticket=activator_.attached_ticket();
    if(intent.ticket!=ticket) {aim_intent_.ticket=0;return true;}
    u32 resolved=0;
    if(aim_faulted_ || !host_.weapon_class || !host_.scope_activate ||
       !host_.image || !host_.image(host_.memory.context) || !lifecycle_.live_ticket(ticket) ||
       scope_actor(unit,&resolved)!=Mode::Local || resolved!=actor || actor!=intent.owner.actor)return false;
    const auto same_actor=[](const Actor& a,const Actor& b) noexcept {
        return a.address==b.address && a.lifetime==b.lifetime && a.serial==b.serial && a.think_mode==b.think_mode;
    };
    const auto same_owner=[&](const rev_genesis::Owner& a,const rev_genesis::Owner& b) noexcept {
        return a.actor==b.actor && a.session.active==b.session.active && a.session.epoch==b.session.epoch &&
            same_actor(a.session.self,b.session.self) && same_actor(a.session.sub0,b.session.sub0);
    };
    auto vh=target_view_host();rev_genesis::ViewFrame before{},after{};
    const u32 scanner=lifecycle_.unit(6);u32 weapon=0,kind=0,manager=0,camera=0;
    if(!vh.on_thread(vh.context) || vh.resolve(vh.context,scanner,&before)!=Mode::Local ||
       !same_owner(before.owner,intent.owner) || !word(0x05799D3C,manager) ||
       genesis_camera(scanner,manager,&camera)!=Mode::Local ||
       rev_genesis::equipment(genesis_host(),host_.memory,scanner,&weapon)!=Mode::Local)return false;
    const auto current=[&]() noexcept {
        u32 now=0,cam=0,owner=0;
        return lifecycle_.live_ticket(ticket) && activator_.attached_ticket()==ticket &&
            scope_actor(unit,&owner)==Mode::Local && owner==actor &&
            vh.resolve(vh.context,scanner,&after)==Mode::Local && after.number==before.number &&
            same_owner(after.owner,before.owner) && genesis_camera(scanner,manager,&cam)==Mode::Local && cam==camera &&
            rev_genesis::equipment(genesis_host(),host_.memory,scanner,&now)==Mode::Local && now==weapon;
    };
    // DTI class identity is distinct from the item ID used by Scope's panel.
    if(intent.hide && weapon && (!host_.weapon_class(host_.memory.context,weapon,&kind) || !current()))return false;
    if(!current())return false;
    if(aim_intent_.revision!=intent.revision)return true; // Newer event wins next phase.
    if(scope_ticket_==ticket && scope_applied_==intent.revision && scope_weapon_==weapon && scope_class_==kind)return true;
    const bool open=intent.hide && weapon && kind!=0x80051B30u;
    host_.scope_activate(host_.memory.context,unit,open?weapon:0,open?intent.flag:1);
    if(!current())return false;
    scope_ticket_=ticket;scope_applied_=intent.revision;scope_weapon_=weapon;scope_class_=kind;
    return true;
}
bool Runtime::genesis_weapon_retain(u32 widget,u32 weapon) noexcept {
    auto vh=target_view_host();
    if(!vh.on_thread(vh.context))return true;
    const auto route=registry_.resolve(widget,WidgetKind::Scanner);
    if(route.mode==Mode::Stock)return true;
    rev_genesis::ViewFrame before{},after{};u32 current=0;
    const u32 ticket=activator_.attached_ticket();
    const auto key=weapons_.capture(weapon);
    if(route.mode!=Mode::Local || !lifecycle_.live_ticket(ticket) || vh.resolve(vh.context,widget,&before)!=Mode::Local ||
       !scanner_effects(widget,effects_stock_.scanner,false) ||
       (weapon && !weapons_.live(key)) ||
       rev_genesis::equipment(genesis_host(),host_.memory,widget,&current)!=Mode::Local || current!=weapon ||
       vh.resolve(vh.context,widget,&after)!=Mode::Local || !same_effect_frame(before,after) ||
       (weapon && !weapons_.live(key)) || !lifecycle_.live_ticket(ticket) || activator_.attached_ticket()!=ticket)return false;
    scanner_weapon_key_=key;scanner_weapon_widget_=widget;scanner_weapon_owner_=before.owner;scanner_weapon_ticket_=ticket;
    return true;
}
bool Runtime::scanner_weapon_current(u32 widget) noexcept {
    u32 retained=0,current=0,again=0;
    if(!widget || widget>Invalid-0x2FF || !word(widget+0x2FC,retained))return false;
    // Native allows a scanner without a weapon. Never dereference the retained
    // pointer merely to decide whether the actor still owns it.
    if(!retained)return true;
    auto h=genesis_host();rev_genesis::Owner before{},after{};
    if(widget!=scanner_weapon_widget_ || !lifecycle_.live_ticket(scanner_weapon_ticket_) ||
       activator_.attached_ticket()!=scanner_weapon_ticket_ || retained!=scanner_weapon_key_.address || !weapons_.live(scanner_weapon_key_) ||
       h.resolve(h.context,widget,&before)!=Mode::Local || !same_genesis_owner(before,scanner_weapon_owner_))return false;
    return rev_genesis::equipment(genesis_host(),host_.memory,widget,&current)==Mode::Local &&
        current==retained && word(widget+0x2FC,again) && again==retained && weapons_.live(scanner_weapon_key_) &&
        h.resolve(h.context,widget,&after)==Mode::Local && same_genesis_owner(before,after);
}
bool Runtime::scanner_begin(u32 widget) noexcept {
    auto h=target_view_host();
    if(!h.on_thread(h.context) || !targets_.healthy() || scanner_scope_ || !gate().unit(this,WidgetKind::Scanner,widget))return false;
    rev_genesis::ViewFrame f{};
    if(h.resolve(h.context,widget,&f)!=Mode::Local || !scanner_weapon_current(widget))return false;
    scanner_scope_=widget;scanner_frame_=f;scanner_copy_failed_=false;scanner_abort_=0;return true;
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
    const bool ok=widget==scanner_scope_ && !scanner_copy_failed_ && scanner_current() && scanner_weapon_current(widget);
    scanner_scope_=0;scanner_frame_={};scanner_copy_failed_=false;scanner_abort_=0;return ok;
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
    if(!nested && !saved_scope)for(auto& n:notifications_)n={};
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
       !backend_.scanner_producer_ready(host_.memory,widget) || !scanner_weapon_current(widget)){
        detector_busy_=false;return;
    }
    scanner_scope_=widget;scanner_frame_=frame;detector_producing_=true;scanner_copy_failed_=false;
    if(scanner_current())host_.native.method0(host_.native.context,0x01BBECD1,manager);
    const bool ok=!scanner_copy_failed_ && scanner_current() && targets_.healthy() &&
        backend_.scanner_producer_ready(host_.memory,widget) && scanner_weapon_current(widget);
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
    if(completion_busy_)completion_removed_=true;
    if(!host_.image || !host_.image(host_.memory.context) ||
       !backend_.scanner_remove_target(host_.memory,target)){
        backend_.quarantine_scanner();lifecycle_.stop();scanner_copy_failed_=true;
    }
}
Mode Runtime::genesis_complete(u32 widget,u32 target) noexcept {
    auto vh=target_view_host();
    if(!vh.on_thread(vh.context))return Mode::Stock;
    const auto route=registry_.resolve(widget,WidgetKind::Scanner);
    if(route.mode==Mode::Stock)return Mode::Stock;
    const auto reject=[&]() noexcept {
        scanner_abort_=widget;
        backend_.quarantine_scanner();scanner_copy_failed_=true;lifecycle_.stop();return Mode::Hidden;
    };
    if(route.mode!=Mode::Local || completion_busy_ || detector_producing_ ||
       widget!=scanner_scope_ || !scanner_current() || !scanner_weapon_current(widget) ||
       !host_.native.method0)return reject();
    const auto key=targets_.capture(target);u32 vt=0,method=0,owner=0;
    if(target>Invalid-11 || !targets_.live(key) || !word(target,vt) || !word(target+8,owner) || !owner)return reject();
    constexpr u32 vtables[]={0x04DA8570,0x04DA8594,0x04DA85B8,0x04DA85DC,0x04DA8600};
    constexpr u32 methods[]={0x01C7F35A,0x01C359C1,0x01C392BF,0x01C19CA8,0x01B880C8};
    bool known=false;
    for(u32 i=0;i<5;++i)if(vt==vtables[i] && word(vt+0x14,method) && method==methods[i])known=true;
    rev_genesis::Collections collection{};bool listed=false;
    if(!known || !rev_genesis::capture_collections(host_.memory,widget,&collection))return reject();
    for(u32 value:collection.targets)if(value==target)listed=true;
    if(!listed || !backend_.scanner_completion_begin(widget))return reject();
    const auto frame=scanner_frame_;const bool failed=scanner_copy_failed_;
    completion_busy_=true;completion_removed_=false;
    // World completion must not inherit per-view target getter overrides.
    scanner_scope_=0;scanner_frame_={};scanner_copy_failed_=false;
    host_.native.method0(host_.native.context,method,target);
    const bool removed=completion_removed_;
    completion_busy_=false;completion_removed_=false;
    scanner_scope_=widget;scanner_frame_=frame;scanner_copy_failed_=scanner_copy_failed_ || failed;
    const bool ended=backend_.scanner_completion_end(widget);
    if(!ended || !targets_.healthy() || scanner_copy_failed_ || !scanner_current() || !scanner_weapon_current(widget) ||
       (!removed && !targets_.live(key)))return reject();
    if(targets_.live(key)) {
        u32 next_vt=0,next_owner=0;
        if(!word(target,next_vt) || next_vt!=vt || !word(target+8,next_owner) || next_owner!=owner)return reject();
    }
    // A removed list node may still be cached in the native caller's frame.
    // The gateway abandons that frame through its original epilogue.
    if(removed)scanner_abort_=widget;
    return removed?Mode::Hidden:Mode::Local;
}
bool Runtime::genesis_scan_aborted(u32 widget) noexcept {
    auto h=target_view_host();return h.on_thread(h.context) && widget && widget==scanner_abort_;
}
bool Runtime::genesis_notify(u32 target,u32 world_owner,u32 delegate) noexcept {
    auto h=target_view_host();
    if(!h.on_thread(h.context))return false;
    const auto mode=detector_mode();
    // Stock calls retain their exact count. During the paired stock/J2
    // traversal, remember which target callbacks stock already delivered.
    if(mode==Mode::Stock && !detector_busy_)return false;
    const auto key=targets_.capture(target);const u32 slot=targets_.slot(key);
    u32 invoke=0;
    const bool valid=slot<rev_genesis::TargetLifetime::Capacity && world_owner &&
        delegate && delegate<=Invalid-7 && word(delegate+4,invoke) && invoke;
    if(mode==Mode::Stock){
        if(valid)notifications_[slot]={key.generation,world_owner,invoke};
        return false;
    }
    if(mode!=Mode::Local || !valid || !host_.native.method0)return true;
    const auto& n=notifications_[slot];
    if(n.generation==key.generation && n.owner==world_owner && n.invoke==invoke)return true;
    notifications_[slot]={key.generation,world_owner,invoke};
    const u32 saved_scope=scanner_scope_;const auto saved_frame=scanner_frame_;
    const bool failed=scanner_copy_failed_;
    // A world/script callback must never inherit J2's global target getter or
    // setter overrides. Native callback storage stays on its caller's stack.
    scanner_scope_=0;scanner_frame_={};detector_producing_=false;scanner_copy_failed_=false;
    host_.native.method0(host_.native.context,0x01C315EC,delegate);
    scanner_scope_=saved_scope;scanner_frame_=saved_frame;detector_producing_=true;
    scanner_copy_failed_=scanner_copy_failed_ || failed || !scanner_current();
    return true;
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
    started_=thread && targets_.start(thread) && effects_.start(thread) && weapons_.start(thread) && source_.start(thread) && clock_.start(thread) &&
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
    if(!rev_genesis::bind_progress(genesis_) || !rev_genesis::bind_targets(targets_) ||
       !rev_genesis::bind_effects(effects_) || !rev_genesis::bind_weapons(weapons_))return false;
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
extern "C" unsigned int rev_genesis_complete(unsigned int widget,unsigned int target) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->genesis_complete(widget,target)):0;
}
extern "C" unsigned int rev_genesis_scan_aborted(unsigned int widget) noexcept {
    return rev_runtime::HealSink && rev_runtime::HealSink->genesis_scan_aborted(widget)?1:0;
}
extern "C" void rev_genesis_remove_target(unsigned int target) noexcept {
    if(rev_runtime::HealSink)rev_runtime::HealSink->genesis_remove_target(target);
}
extern "C" unsigned int rev_genesis_notify(unsigned int target,unsigned int owner,unsigned int delegate) noexcept {
    return rev_runtime::HealSink && rev_runtime::HealSink->genesis_notify(target,owner,delegate)?1:0;
}
extern "C" void rev_aim_visibility(unsigned int actor,unsigned int hide,unsigned int flag) noexcept {
    if(rev_runtime::HealSink)rev_runtime::HealSink->aim_visibility(actor,hide,flag);
}
extern "C" unsigned int rev_scope_actor(unsigned int widget,unsigned int* out) noexcept {
    return rev_runtime::HealSink?static_cast<unsigned int>(rev_runtime::HealSink->scope_actor(widget,out)):0;
}
extern "C" unsigned int rev_genesis_filter_loaded(unsigned int widget,unsigned int resource) noexcept {
    return rev_runtime::HealSink?rev_runtime::HealSink->genesis_filter_loaded(widget,resource):resource;
}
extern "C" unsigned int rev_runtime_unit_mask(unsigned int unit,unsigned int original) noexcept {
    return rev_runtime::HealSink?rev_runtime::HealSink->unit_mask(unit,original):original;
}
extern "C" unsigned int rev_genesis_effect_draw(unsigned int kind,unsigned int unit,unsigned int context) noexcept {
    return rev_runtime::HealSink && rev_runtime::HealSink->effect_draw(kind,unit,context)?1u:0u;
}
extern "C" unsigned int rev_genesis_weapon_retain(unsigned int widget,unsigned int weapon) noexcept {
    return !rev_runtime::HealSink || rev_runtime::HealSink->genesis_weapon_retain(widget,weapon)?1u:0u;
}
