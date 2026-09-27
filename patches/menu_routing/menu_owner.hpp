#pragma once
namespace rev_menu {
using u32 = unsigned int;
static_assert(sizeof(u32)==4,"January PE32 word width");
constexpr u32 Invalid = ~u32(0);
enum class Surface : u32 { None=0, Pause=5, SubMenu=8 };
struct Access {
    void* context = nullptr;
    bool (*word)(void*,u32,u32*) noexcept = nullptr;
    u32 (*local_active)(void*) noexcept = nullptr;
    u32 (*sub0)(void*) noexcept = nullptr;
};
class MenuOwnerRouter {
public:
    explicit MenuOwnerRouter(Access access) : access_(access) {}
    MenuOwnerRouter(const MenuOwnerRouter&)=delete;
    MenuOwnerRouter& operator=(const MenuOwnerRouter&)=delete;
    u32 pause_open_word(u32 pad) noexcept;
    u32 pause_word_1a0(u32 pad) noexcept;
    u32 pause_word_1ac(u32 pad) noexcept;
    u32 submenu_open_word(u32 pad) noexcept;
    u32 submenu_word_198(u32 pad) noexcept;
    u32 submenu_word_1a0(u32 pad) noexcept;
    u32 submenu_actor(u32 stock) noexcept;
    // Notify after the common sIDCockpit state store. States 6/7 are not admitted.
    void observe_state(u32 cockpit,u32 state) noexcept;
    // Host must notify teardown, including same-address reuse in a new session.
    void reset_session() noexcept { clear(); cockpit_=0; }
    u32 owner() const noexcept { return owner_; }
    Surface surface() const noexcept { return surface_; }
    void clear() noexcept {
        owner_=0; surface_=Surface::None; actor_=0; serial_=Invalid;
    }
private:
    Access access_{};
    u32 owner_=0;
    Surface surface_=Surface::None;
    u32 actor_=0, serial_=Invalid, cockpit_=0;
    bool read(u32 address,u32& out) const noexcept;
    bool valid_sub(u32& out,u32& serial) const noexcept;
    bool validate_owner() noexcept;
    bool pad_word(u32 pad,u32 member,u32 offset,u32& out) const noexcept;
    u32 stock_word(u32 pad,u32 offset) const noexcept;
    u32 open_word(u32 pad,u32 mask,Surface surface) noexcept;
    u32 owner_word(u32 pad,Surface surface,u32 offset) noexcept;
};
bool bind_menu_owner_sink(MenuOwnerRouter&) noexcept;
}
extern "C" unsigned int rev_menu_pause_open_word(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_pause_word_1a0(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_pause_word_1ac(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_submenu_open_word(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_submenu_word_198(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_submenu_word_1a0(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_submenu_actor(unsigned int stock) noexcept;
extern "C" void rev_menu_observe_state(unsigned int cockpit,unsigned int state) noexcept;
extern "C" void rev_menu_reset_session() noexcept;
