#include "p1_view_mask.hpp"
namespace rev_hud {
namespace {
constexpr P1MaskSite Sites[] = {
    {0x02B49DE3, 0x02B49E5C, ManagerKind::Cockpit},
    {0x02B68724, 0x02B687F0, ManagerKind::MiniMap},
};
P1ViewMask* Sink = nullptr;
}
const P1MaskSite* p1_mask_entry(u32 address) {
    for (const auto& s : Sites) if (s.entry == address) return &s;
    return nullptr;
}
const P1MaskSite* p1_mask_exit(u32 address) {
    for (const auto& s : Sites) if (s.exit == address) return &s;
    return nullptr;
}
bool P1ViewMask::word(u32 address, u32& value) const noexcept {
    return address && address <= Invalid-3 && access_.memory.word &&
        access_.memory.word(access_.memory.context, address, &value);
}
bool P1ViewMask::write(u32 address, u32 value) const noexcept {
    return address && address <= Invalid-3 && access_.write_word &&
        access_.write_word(access_.memory.context, address, value);
}
void P1ViewMask::clear() noexcept {
    manager_ = count_ = 0; kind_ = ManagerKind::Cockpit; active_ = false;
    for (u32 i=0;i<2;++i) units_[i]=slots_[i]=masks_[i]=0;
}
bool P1ViewMask::collect(u32 manager, ManagerKind kind, u32 units[2],
                         u32 slots[2], u32* count) const noexcept {
    if (!manager || !count) return false;
    u32 vt=0;
    if (!word(manager,vt) || vt != manager_vtable(kind)) return false;
    if (kind == ManagerKind::Cockpit) {
        const u32 offsets[2] = {0x5C,0x90};
        const WidgetKind kinds[2] = {WidgetKind::MainEquipment,WidgetKind::Reticle};
        for (u32 i=0;i<2;++i) {
            if (!word(manager+offsets[i],units[i]) || !units[i] ||
                !word(units[i],vt) || vt != kind_info(kinds[i])->vtable) return false;
            slots[i]=offsets[i];
        }
        if (units[0] == units[1]) return false;
        *count=2; return true;
    }
    u32 alias=0;
    if (!word(manager+0x30,units[0]) || !units[0] ||
        !word(manager+0x40,alias) || alias != units[0] ||
        !word(units[0],vt) || vt != kind_info(WidgetKind::MapHerb)->vtable) return false;
    slots[0]=0x30; *count=1; return true;
}
bool P1ViewMask::restore_one(u32 i) noexcept {
    if (i >= count_ || !units_[i]) return false;
    u32 current=0, in_slot=0, vt=0, restored=0;
    const WidgetKind k = kind_==ManagerKind::MiniMap ? WidgetKind::MapHerb :
        (slots_[i]==0x5C ? WidgetKind::MainEquipment : WidgetKind::Reticle);
    if (!word(manager_+slots_[i],in_slot) || in_slot != units_[i] ||
        !word(units_[i],vt) || vt != kind_info(k)->vtable ||
        !word(units_[i]+0x0C,current) ||
        !replace_draw_view(current,masks_[i],&restored) ||
        !write(units_[i]+0x0C,restored)) return false;
    return true;
}
bool P1ViewMask::begin(u32 entry_site, u32 manager, u32 context) noexcept {
    if (fault_ != P1MaskFault::None || active_) return fail(P1MaskFault::Protocol);
    const P1MaskSite* site=p1_mask_entry(entry_site);
    if (!site || !context || context > Invalid-0x15B) return fail(P1MaskFault::Protocol);
    u32 view=0;
    if (!word(context+0x158,view) || (view&0xFF)!=1) return fail(P1MaskFault::Layout);
    u32 units[2]{},slots[2]{},count=0;
    if (!collect(manager,site->manager,units,slots,&count)) return fail(P1MaskFault::Layout);
    manager_=manager;kind_=site->manager;count_=count;
    for(u32 i=0;i<count_;++i){units_[i]=units[i];slots_[i]=slots[i];}
    u32 written=0;
    for(u32 i=0;i<count_;++i){
        u32 flags=0,masked=0;
        if (!word(units_[i]+0x0C,flags) ||
            !replace_draw_view(flags,draw_view(flags)&~2u,&masked)) {
            clear(); return fail(P1MaskFault::Memory);
        }
        masks_[i]=draw_view(flags);
        if (masked!=flags && !write(units_[i]+0x0C,masked)) {
            bool rollback=true;
            for(u32 j=written;j-- >0;) rollback=restore_one(j)&&rollback;
            clear(); return fail(rollback?P1MaskFault::Memory:P1MaskFault::Restore);
        }
        ++written;
    }
    active_=true; return true;
}
bool P1ViewMask::end(u32 exit_site, u32 manager) noexcept {
    const P1MaskSite* site=p1_mask_exit(exit_site);
    if (!site) return fail(P1MaskFault::Protocol);
    if (!active_) return fault_==P1MaskFault::None;
    if (manager!=manager_ || site->manager!=kind_) {
        clear(); return fail(P1MaskFault::Protocol);
    }
    bool ok=true;
    for(u32 i=count_;i-- >0;) ok=restore_one(i)&&ok;
    clear();
    if (!ok) return fail(P1MaskFault::Restore);
    return true;
}
bool bind_p1_mask_sink(P1ViewMask& mask) noexcept {
    if (Sink) return false;
    Sink=&mask; return true;
}
}
extern "C" unsigned int rev_hud_p1_mask_begin(unsigned int site,
        unsigned int manager, unsigned int context) noexcept {
    return rev_hud::Sink && rev_hud::Sink->begin(site,manager,context) ? 1u : 0u;
}
extern "C" unsigned int rev_hud_p1_mask_end(unsigned int site,
        unsigned int manager) noexcept {
    // No active scope is a valid path when P2 dispatch was skipped or MiniMap
    // took its hidden branch; end() itself distinguishes a latched fault.
    return rev_hud::Sink && rev_hud::Sink->end(site,manager) ? 1u : 0u;
}
