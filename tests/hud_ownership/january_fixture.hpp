#pragma once
#include "january_backend.hpp"
#include <map>
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
static unsigned checks=0,scenarios=0;
#define C(x) do {++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
struct Event { u32 target,self,arg,arity; };
struct Fake {
    std::map<u32,u32> m;std::vector<Event> events;std::vector<u32> deleted;
    std::vector<u32> allocations,writes,membership;
    u32 p[2]={0x200000,0x220000},original[3]={0x100000,0x120000,0x140000};
    Session session{true,1,{0x300000,1,0,1},{0x320000,1,1,1}};
    u32 next=0x600000,context=0x400000,ctor_count=0,init_count=0;
    u32 allocation_override=0,attached=0,reader_fail=0,write_fail=0;
    int no_alloc=-1,no_init=-1,bad_ctor=-1;
    bool admission=true,fresh=true,unit_available=true,draw_admission=true;
    bool deny_destroy=false,deny_init=false,stop_in_phase=false,stop_in_init=false;
    bool stop_in_construct=false,alias_tree=false,attach_in_phase=false;
    Lifecycle* life=nullptr;
    Fake(){
        for(u32 i=0;i<2;++i){m[p[i]]=manager_vtable(static_cast<ManagerKind>(i));m[p[i]+0x1C]=i?0x3F000000:0x3F800000;}
        for(u32 i=0;i<3;++i){
            const auto k=static_cast<WidgetKind>(i);plain(original[i],k);tree(original[i],k);
            const auto* t=january_type(k);const u32 vt=kind_info(k)->vtable;
            for(auto s: {0u,5u,8u,9u,11u})m[vt+s*4]=s==0?t->destroy:s==5?t->initialize:s==8?t->phase8:s==9?t->phase9:t->draw;
        }
        m[context+0x158]=0xA0000001;
    }
    void plain(u32 u,WidgetKind k){
        m[u]=kind_info(k)->vtable;m[u+0xC]=0x800343F9;m[u+0x14]=m[u+0x18]=0;
        m[u+0x1C]=0;m[u+0x290]=k==WidgetKind::MapHerb?0x99999999:0;
        m[u+0x294]=k==WidgetKind::MapHerb?13:0;
        m[u+0xF0]=m[u+0xF4]=m[u+0xF8]=0;
    }
    void tree(u32 u,WidgetKind k){
        u32 r=0x500000+static_cast<u32>(k)*0x10000;
        m[u+0xF0]=r;m[u+0xF4]=u+0x1000;m[u+0xF8]=u+0x2000;
        m[r+0x68]=r+0x100;m[r+0x144]=2;m[u+0x106C]=u;
        for(u32 i=0;i<2;++i){m[u+0x2000+i*4]=u+0x3000+i*0x100;m[u+0x306C+i*0x100]=u;}
    }
    static bool read(void* c,u32 a,u32* v) noexcept {
        auto& f=*static_cast<Fake*>(c);auto i=f.m.find(a);
        if(a==f.reader_fail || i==f.m.end())return false;
        *v=i->second;return true;
    }
    static bool write(void* c,u32 a,u32 v) noexcept {
        auto& f=*static_cast<Fake*>(c);if(a==f.write_fail || !f.m.count(a))return false;
        f.writes.push_back(a);f.m[a]=v;return true;
    }
    static bool permit(void* c,NativeOp op,WidgetKind,u32,u32 phase,u32 context) noexcept {
        auto& f=*static_cast<Fake*>(c);
        if(!f.admission || (op==NativeOp::Destroy && f.deny_destroy) || (op==NativeOp::Initialize && f.deny_init))return false;
        return op!=NativeOp::Phase || phase!=11 || (f.draw_admission && context==f.context);
    }
    static bool freshly(void* c,u32,u32) noexcept {return static_cast<Fake*>(c)->fresh;}
    static u32 alloc(void* c,u32 target,u32 size,u32 align) noexcept {
        auto& f=*static_cast<Fake*>(c);C(align==16);
        u32 index=3;for(u32 i=0;i<3;++i)if(january_type(static_cast<WidgetKind>(i))->allocator==target)index=i;
        C(index<3);C(size==january_type(static_cast<WidgetKind>(index))->size);
        if(static_cast<int>(index)==f.no_alloc)return 0;
        const u32 u=f.allocation_override?f.allocation_override:f.next;f.next+=0x10000;f.allocations.push_back(u);return u;
    }
    static u32 ctor(void* c,u32 target,u32 self) noexcept {
        auto& f=*static_cast<Fake*>(c);u32 i=3;
        for(u32 j=0;j<3;++j)if(january_type(static_cast<WidgetKind>(j))->constructor==target)i=j;
        C(i<3);++f.ctor_count;f.plain(self,static_cast<WidgetKind>(i));
        if(f.stop_in_construct && f.life)f.life->stop();
        if(static_cast<int>(i)==f.bad_ctor)f.m[self]=0;
        return self;
    }
    static void method0(void* c,u32 target,u32 self) noexcept {
        auto& f=*static_cast<Fake*>(c);f.events.push_back({target,self,0,0});
        u32 i=3;for(u32 j=0;j<3;++j)if(kind_info(static_cast<WidgetKind>(j))->vtable==f.m[self])i=j;
        C(i<3);const auto k=static_cast<WidgetKind>(i);const auto* t=january_type(k);
        if(target==t->initialize){
            ++f.init_count;if(static_cast<int>(i)!=f.no_init)f.tree(self,k);
            if(f.alias_tree)f.m[self+0xF8]=f.m[f.original[0]+0xF8];
            if(f.stop_in_init && f.life)f.life->stop();
        }else{
            C(target==t->phase8 || target==t->phase9);f.m[self+0xC]^=0x80000000;
            if(f.attach_in_phase)f.attached=self;
            if(f.stop_in_phase && f.life){f.life->stop();C(!f.life->collect());}
        }
    }
    static void method1(void* c,u32 target,u32 self,u32 arg) noexcept {
        auto& f=*static_cast<Fake*>(c);f.events.push_back({target,self,arg,1});
        u32 i=3;for(u32 j=0;j<3;++j)if(kind_info(static_cast<WidgetKind>(j))->vtable==f.m[self])i=j;
        C(i<3);const auto* t=january_type(static_cast<WidgetKind>(i));
        if(target==t->destroy){
            C(arg==1);for(u32 a:f.original)C(a!=self);for(u32 a:f.deleted)C(a!=self);
            if(f.life)C(!f.life->collect());
            f.deleted.push_back(self);
            for(auto it=f.m.begin();it!=f.m.end();)if(it->first>=self && it->first<self+0x10000)it=f.m.erase(it);else ++it;
        }else{C(target==t->draw);C(arg==f.context);f.m[self+0xC]^=0x80000000;}
    }
    static u32 singleton(void* c,u32 t) noexcept {C(t==0x01C8C27B);return static_cast<Fake*>(c)->unit_available?0x700000:0;}
    static u32 contains(void* c,u32 t,u32 s,u32 u) noexcept {
        auto& f=*static_cast<Fake*>(c);C(t==0x0326AA90);C(s==0x700000);f.membership.push_back(u);return f.attached==u;
    }
    JanuaryHost host(){return {{this,read},write,permit,freshly};}
    JanuaryCalls calls(){return {this,alloc,ctor,method0,method1,singleton,contains};}
};
