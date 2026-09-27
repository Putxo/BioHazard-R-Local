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
    u32 owner() const noexcept { return owner_; }
    Surface surface() const noexcept { return surface_; }
    void clear() noexcept { owner_=0; surface_=Surface::None; }
private:
    Access access_{};
    u32 owner_=0;
    Surface surface_=Surface::None;
    bool read(u32 address,u32& out) const noexcept;
    bool valid_sub(u32& out) const noexcept;
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
