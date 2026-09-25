#pragma once
// Source-only routing component. Never reads game memory or installs patches.
// The eventual engine adapter must call the lifecycle methods on the game
// thread and supply validated live actor snapshots. Addresses are PE32 keys.
namespace rev_hud {
using u32 = unsigned int;
using i32 = int;
static_assert(sizeof(u32) == 4 && sizeof(i32) == 4, "PE32 integer widths");
constexpr u32 Invalid = ~u32(0);
constexpr u32 ViewBits = 0x03FF0000u;
enum class ManagerKind : u32 { Cockpit, MiniMap };
enum class WidgetKind : u32 { Reticle, MainEquipment, MapHerb };
enum class Mode : u32 { Stock, Local, Hidden };
struct Token {
    u32 slot = Invalid, generation = 0;
    bool valid() const { return slot != Invalid && generation != 0; }
};
struct Actor {
    u32 address = 0, lifetime = 0;
    i32 serial = -1;
    u32 think_mode = 0;
};
struct Session {
    bool active = false;
    u32 epoch = 0;
    Actor self{}, sub0{};
};
struct Route {
    Mode mode = Mode::Stock;
    u32 actor = 0, member = 0, view = 0;
};
struct KindInfo {
    u32 vtable;
    ManagerKind manager;
    u32 actor_phase; // native virtual slot; not a phase to invoke twice
};
const KindInfo* kind_info(WidgetKind kind);
u32 manager_vtable(ManagerKind kind);
// Mirrors only the proven ten-bit mDrawView packing, not GUI coordinates.
u32 draw_view(u32 packed);
bool replace_draw_view(u32 packed, u32 mask, u32* result);
bool view_allowed(u32 packed, u32 view);

class Registry {
public:
    static constexpr u32 ManagerCapacity = 8, WidgetCapacity = 64;
    Token open_manager(u32 address, u32 observed_vtable, ManagerKind kind);
    // Called BEFORE freeing manager or widgets. Existing clone entries become
    // Hidden, not Stock. They remain tombstones until explicit retire_widget.
    bool invalidate_manager(Token manager);
    bool release_manager(Token manager); // requires all child entries retired
    // A lost/mismatched binding is revoked permanently. Re-enabling the same
    // actor address does not revive it: the adapter must retire and bind anew.
    void set_session(const Session& session);
    const Session& session() const { return session_; }
    Token bind_widget(Token manager, u32 address, u32 observed_vtable,
                      WidgetKind kind, u32 member, const Actor& actor, u32 view);
    bool retire_widget(Token widget); // caller owns actual widget destruction
    Route resolve(u32 address, WidgetKind kind) const;
    // Returns a scoped proposed flags value. Does NOT persistently overwrite
    // original flags or touch mDrawMode, links, resources or the render device.
    u32 flags_for(u32 address, WidgetKind kind, u32 original) const;
    u32 widget_count() const;
private:
    struct ManagerEntry {
        u32 address = 0, generation = 0;
        ManagerKind kind = ManagerKind::Cockpit;
        bool occupied = false, alive = false;
    };
    struct WidgetEntry {
        u32 address = 0, generation = 0, observed_vtable = 0;
        WidgetKind kind = WidgetKind::Reticle;
        Token manager{};
        u32 member = 0, view = 0, epoch = 0;
        Actor actor{};
        bool occupied = false, revoked = false;
    };
    Session session_{};
    ManagerEntry managers_[ManagerCapacity]{};
    WidgetEntry widgets_[WidgetCapacity]{};
    ManagerEntry* find_manager(Token token);
    const ManagerEntry* find_manager(Token token) const;
};
// Zero-initialized singleton used ONLY by the opt-in bridges. An engine adapter
// is deliberately not installed in this change. Registration is explicit.
Registry& registry();
}
extern "C" unsigned int rev_hud_resolve_actor(unsigned int widget,
        unsigned int kind, unsigned int* actor_out);
