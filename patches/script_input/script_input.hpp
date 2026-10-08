#pragma once
namespace rev_script {
using u32=unsigned int;
static_assert(sizeof(u32)==4,"January PE32 word width");
struct Owner {u32 actor=0,serial=0,epoch=0;};
struct Host {
    void* context=nullptr;
    bool (*word)(void*,u32,u32*) noexcept=nullptr;
    bool (*on_thread)(void*) noexcept=nullptr;
    bool (*capture)(void*,Owner*) noexcept=nullptr;
};
// Synchronous resolution only; no ownership cached across script reassignment.
class InputRouter {
public:
    explicit InputRouter(Host h):host_(h){}
    u32 member(u32 input) noexcept;
private:
    Host host_;
    bool busy_=false;
    bool read(u32,u32&) const noexcept;
    u32 resolve(u32,const Owner&) const noexcept;
};
bool bind(InputRouter&) noexcept;
}
extern "C" unsigned int rev_script_input_member(unsigned int input) noexcept;
