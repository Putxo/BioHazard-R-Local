#include "hunter_materials.hpp"
namespace rev_genesis {
namespace {
constexpr u32 Invalid=~u32(0),HunterVtable=0x04D09FBC;
constexpr u32 ColorHash=0x6C8011F4,AlphaHash=0xEFCA322B;
constexpr u32 Parts=0xF8,PartCount=0xFC,BaseAlpha=0x1A74,HunterManager=0x0556DD74;
constexpr u32 GroupCount=0x8C,GroupIndex=0x7C,EntryBase=0x30;
static float as_float(u32 v) noexcept {float f;__builtin_memcpy(&f,&v,4);return f;}
static u32 as_word(float f) noexcept {u32 v;__builtin_memcpy(&v,&f,4);return v;}
static float clamp(float v,float lo,float hi) noexcept {return v<lo?lo:(v>hi?hi:v);}
}
bool HunterMaterials::read(MaterialAccess a,u32 p,u32& out) noexcept {
    u32 v=0;
    if(!p||p>Invalid-3||!a.word||!a.word(a.context,p,&v))return false;
    out=v;
    return true;
}
bool HunterMaterials::add(u32 base,u32 off,u32& out) noexcept {
    if(base>Invalid-off)return false;
    out=base+off;
    return true;
}
bool HunterMaterials::parameter(MaterialAccess a,u32 part,u32 hash,u32& out) noexcept {
    out=0;u32 count=0,index=0,base=0,at=0;
    if(!part||!add(part,GroupCount,at)||!read(a,at,count)||!add(part,GroupIndex,at)||!read(a,at,index)||
       !add(part,EntryBase,at)||!read(a,at,base)||!count||count>4096||!index||!base)return false;
    const u32 target=hash&0xFFFu;u32 lo=0,hi=count-1;
    for(;;){
        const u32 mid=lo+((hi-lo)>>1),ioff=mid*4;u32 slot=0,ip=0;
        if(mid>Invalid/4||!add(index,ioff,ip)||!read(a,ip,slot)||slot>Invalid/12)return false;
        u32 entry=0;if(!add(base,slot*12,entry))return false;
        u32 key=0,raw=0;if(!read(a,entry,key))return false;
        key=(key>>20)&0xFFFu;
        if(key==target){
            u32 value=0;
            if(!add(entry,4,value)||!read(a,value,raw))return false;
            out=raw&~0xFu;
            return out!=0;
        }
        if(lo>=hi)return false;
        if(key<target)lo=mid+1;
        else {if(mid==0)return false;hi=mid-1;}
    }
}
void HunterMaterials::reset() noexcept {
    color_count_=alpha_count_=hunter_=0;active_=false;
}
bool HunterMaterials::collect(MaterialAccess a,u32 hunter) noexcept {
    u32 vt=0,count=0,array=0,at=0;
    if(!read(a,hunter,vt)||vt!=HunterVtable||!add(hunter,PartCount,at)||!read(a,at,count)||
       !add(hunter,Parts,at)||!read(a,at,array)||!count||count>MaxParts||!array)return false;
    for(u32 i=0;i<count;++i){
        u32 part=0,slot=0;
        if(i>Invalid/4||!add(array,i*4,slot)||!read(a,slot,part)||!part)return false;
        u32 color=0,alpha=0;
        if(!parameter(a,part,ColorHash,color)||!parameter(a,part,AlphaHash,alpha))return false;
        bool have=false;
        for(u32 j=0;j<color_count_;++j)if(colors_[j].address==color){have=true;break;}
        if(!have){
            if(color_count_>=MaxParts||color>Invalid-(46*4))return false;
            auto& r=colors_[color_count_++];r.address=color;
            for(u32 j=0;j<3;++j)if(!read(a,color+(44+j)*4,r.old[j]))return false;
            for(u32 j=0;j<3;++j)if(!read(a,color+j*4,r.old[3+j]))return false;
            for(u32 j=0;j<3;++j)if(!read(a,color+(40+j)*4,r.old[6+j]))return false;
        }
        have=false;
        for(u32 j=0;j<alpha_count_;++j)if(alphas_[j].address==alpha){have=true;break;}
        if(!have){
            if(alpha_count_>=MaxParts||alpha>Invalid-4)return false;
            auto& r=alphas_[alpha_count_++];r.address=alpha;
            if(!read(a,alpha+4,r.old))return false;
        }
    }
    return true;
}
bool HunterMaterials::restore(MaterialAccess a) noexcept {
    if(!a.write)return false;
    bool ok=true;
    for(u32 i=0;i<color_count_;++i){
        const auto& r=colors_[i];
        for(u32 j=0;j<3;++j)ok=a.write(a.context,r.address+(44+j)*4,r.old[j])&&ok;
        for(u32 j=0;j<3;++j)ok=a.write(a.context,r.address+j*4,r.old[3+j])&&ok;
        for(u32 j=0;j<3;++j)ok=a.write(a.context,r.address+(40+j)*4,r.old[6+j])&&ok;
    }
    for(u32 i=0;i<alpha_count_;++i)ok=a.write(a.context,alphas_[i].address+4,alphas_[i].old)&&ok;
    return ok;
}
MaterialResult HunterMaterials::begin(MaterialAccess a,u32 hunter,bool genesis) noexcept {
    if(active_||!a.word||!a.write||!hunter)return MaterialResult::Refused;
    reset();hunter_=hunter;
    u32 alpha_word=0,manager=0,alpha_at=0;
    if(!add(hunter,BaseAlpha,alpha_at)||!read(a,alpha_at,alpha_word)||!read(a,HunterManager,manager)||!collect(a,hunter)){
        reset();return MaterialResult::Fault;
    }
    float alpha=as_float(alpha_word);
    if(genesis&&manager&&alpha_word==0x3F800000u)alpha=0.99f;
    const float inv=1.0f-alpha;
    const u32 va=as_word(clamp(1.0f-inv*0.75f,0.25f,1.0f));
    const u32 vb=as_word(clamp(inv,0.0f,1.0f));
    const u32 vc=as_word(clamp(inv*0.8f,0.0f,0.8f));
    bool ok=true;
    for(u32 i=0;i<color_count_&&ok;++i){
        auto& r=colors_[i];
        for(u32 j=0;j<3;++j)ok=a.write(a.context,r.address+(44+j)*4,va)&&ok;
        for(u32 j=0;j<3&&ok;++j)ok=a.write(a.context,r.address+j*4,vb)&&ok;
        for(u32 j=0;j<3&&ok;++j)ok=a.write(a.context,r.address+(40+j)*4,vc)&&ok;
    }
    const u32 alpha_value=as_word(alpha);
    for(u32 i=0;i<alpha_count_&&ok;++i)ok=a.write(a.context,alphas_[i].address+4,alpha_value)&&ok;
    if(!ok){(void)restore(a);reset();return MaterialResult::Fault;}
    active_=true;
    return MaterialResult::Applied;
}
MaterialResult HunterMaterials::end(MaterialAccess a) noexcept {
    if(!active_)return MaterialResult::Refused;
    const bool ok=restore(a);reset();
    return ok?MaterialResult::Applied:MaterialResult::Fault;
}
}
