#pragma once
namespace rev_menu {
using u32 = unsigned int;
static_assert(sizeof(u32)==4,"January PE32 word width");
constexpr u32 Invalid = ~u32(0);
enum class Surface : u32 { None=0, Pause=5, SubMenu=8 };
struct OptionsAccess {
    void* context=nullptr;
    bool (*write)(void*,u32,u32) noexcept=nullptr;
    u32 (*thread)(void*) noexcept=nullptr;
    void (*read_native)(void*,u32) noexcept=nullptr;
    u32 (*apply_native)(void*,u32,u32,u32) noexcept=nullptr;
    void (*publish_native)(void*,u32,const u32*) noexcept=nullptr;
};
struct Access {
    void* context = nullptr;
    bool (*word)(void*,u32,u32*) noexcept = nullptr;
    u32 (*local_active)(void*) noexcept = nullptr;
    u32 (*sub0)(void*) noexcept = nullptr;
    OptionsAccess options{};
};
class MenuOwnerRouter {
public:
    explicit MenuOwnerRouter(Access access) : access_(access) {}
    MenuOwnerRouter(const MenuOwnerRouter&)=delete;
    MenuOwnerRouter& operator=(const MenuOwnerRouter&)=delete;
    u32 pause_open_word(u32 pad) noexcept;
    u32 pause_word_198(u32 pad) noexcept;
    u32 pause_word_1a0(u32 pad) noexcept;
    u32 pause_word_1ac(u32 pad) noexcept;
    u32 submenu_open_word(u32 pad) noexcept;
    u32 submenu_word_198(u32 pad) noexcept;
    u32 submenu_word_1a0(u32 pad) noexcept;
    u32 submenu_actor(u32 stock) noexcept;
    void state_transition(u32 next_state) noexcept;
    void options_read(u32 config,u32 reference) noexcept;
    u32 options_apply(u32 config,u32 save,u32 flag) noexcept;
    u32 options_index(u32 gamepad,u32 stock) const noexcept;
    u32 owner() const noexcept { return owner_; }
    Surface surface() const noexcept { return surface_; }
    void clear() noexcept {
        options_valid_=false;
        owner_=0; surface_=Surface::None;
        owner_actor_=0; owner_serial_=Invalid;
    }
private:
    Access access_{};
    u32 options_manager_=0,options_member_=Invalid,options_pad_=0,options_thread_=0;
    u32 options_actor_=0,options_serial_=Invalid;
    bool options_valid_=false,options_busy_=false,options_scope_=false,options_reference_=false;
    bool options_current() const noexcept;
    bool options_values(u32*) const noexcept;
    bool options_store(u32,u32) noexcept;
    u32 owner_=0;
    Surface surface_=Surface::None;
    u32 owner_actor_=0;
    u32 owner_serial_=Invalid;
    bool read(u32 address,u32& out) const noexcept;
    bool valid_sub(u32& out,u32* serial=nullptr) const noexcept;
    bool bound_sub_valid() const noexcept;
    bool pad_word(u32 pad,u32 member,u32 offset,u32& out) const noexcept;
    u32 stock_word(u32 pad,u32 offset) const noexcept;
    u32 open_word(u32 pad,u32 mask,Surface surface) noexcept;
    u32 owner_word(u32 pad,Surface surface,u32 offset) noexcept;
};
bool bind_menu_owner_sink(MenuOwnerRouter&) noexcept;
}
extern "C" unsigned int rev_menu_pause_open_word(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_pause_word_198(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_pause_word_1a0(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_pause_word_1ac(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_submenu_open_word(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_submenu_word_198(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_submenu_word_1a0(unsigned int pad) noexcept;
extern "C" unsigned int rev_menu_submenu_actor(unsigned int stock) noexcept;
extern "C" void rev_menu_state_transition(unsigned int next_state) noexcept;

extern "C" void rev_menu_options_read(unsigned int config,unsigned int reference) noexcept;
extern "C" unsigned int rev_menu_options_apply(unsigned int config,unsigned int save,unsigned int flag) noexcept;
extern "C" unsigned int rev_menu_options_index(unsigned int gamepad,unsigned int stock) noexcept;
