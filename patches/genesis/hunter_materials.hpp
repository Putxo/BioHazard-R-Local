#pragma once
namespace rev_genesis {
using u32=unsigned int;
static_assert(sizeof(u32)==4,"January word width");
struct MaterialAccess {
    void* context=nullptr;
    bool (*word)(void*,u32,u32*) noexcept=nullptr;
    bool (*write)(void*,u32,u32) noexcept=nullptr;
};
enum class MaterialResult:u32 { Refused=0, Applied=1, Fault=2 };
class HunterMaterials {
public:
    static constexpr u32 MaxParts=256;
    MaterialResult begin(MaterialAccess,u32 hunter,bool genesis) noexcept;
    MaterialResult end(MaterialAccess) noexcept;
    bool active() const noexcept {return active_;}
    u32 hunter() const noexcept {return hunter_;}
    u32 color_count() const noexcept {return color_count_;}
    u32 alpha_count() const noexcept {return alpha_count_;}
private:
    struct Color {u32 address=0;u32 old[9]{};};
    struct Alpha {u32 address=0;u32 old=0;};
    Color colors_[MaxParts]{};
    Alpha alphas_[MaxParts]{};
    u32 color_count_=0,alpha_count_=0,hunter_=0;
    bool active_=false;
    static bool read(MaterialAccess,u32,u32&) noexcept;
    static bool add(u32,u32,u32&) noexcept;
    static bool parameter(MaterialAccess,u32,u32,u32&) noexcept;
    bool collect(MaterialAccess,u32) noexcept;
    bool restore(MaterialAccess) noexcept;
    void reset() noexcept;
};
}
