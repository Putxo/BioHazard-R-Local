#pragma once
#include "lifecycle.hpp"
namespace rev_hud {
// Temporary masking of only the three stock P1 widgets that have an independent
// P2 clone. Shared cockpit/minimap widgets are never modified by this component.
struct P1MaskAccess {
    Reader memory{};
    bool (*write_word)(void*, u32 address, u32 value) noexcept = nullptr;
};
enum class P1MaskFault : u32 { None, Protocol, Memory, Layout, Restore };
struct P1MaskSite { u32 entry, exit; ManagerKind manager; };
const P1MaskSite* p1_mask_entry(u32 address);
const P1MaskSite* p1_mask_exit(u32 address);
class P1ViewMask {
public:
    explicit P1ViewMask(P1MaskAccess access) : access_(access) {}
    P1ViewMask(const P1ViewMask&) = delete;
    P1ViewMask& operator=(const P1ViewMask&) = delete;
    // Begin is called only after the P2 manual draw succeeded for view 1.
    // It removes bit 1 from mDrawView on the corresponding stock P1 widgets.
    bool begin(u32 entry_site, u32 manager, u32 context) noexcept;
    // End restores ONLY the ten mDrawView bits observed at begin, preserving
    // unrelated cUnit flag changes made by the stock loop.
    bool end(u32 exit_site, u32 manager) noexcept;
    bool active() const noexcept { return active_; }
    P1MaskFault fault() const noexcept { return fault_; }
private:
    P1MaskAccess access_{};
    u32 manager_ = 0, count_ = 0;
    ManagerKind kind_ = ManagerKind::Cockpit;
    u32 units_[2]{}, slots_[2]{}, masks_[2]{};
    bool active_ = false;
    P1MaskFault fault_ = P1MaskFault::None;
    bool word(u32 address, u32& value) const noexcept;
    bool write(u32 address, u32 value) const noexcept;
    bool collect(u32 manager, ManagerKind kind, u32 units[2], u32 slots[2],
                 u32* count) const noexcept;
    bool restore_one(u32 index) noexcept;
    bool fail(P1MaskFault fault) noexcept { fault_ = fault; return false; }
    void clear() noexcept;
};
bool bind_p1_mask_sink(P1ViewMask&) noexcept;
}
extern "C" unsigned int rev_hud_p1_mask_begin(unsigned int site,
    unsigned int manager, unsigned int context) noexcept;
extern "C" unsigned int rev_hud_p1_mask_end(unsigned int site,
    unsigned int manager) noexcept;
