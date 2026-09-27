#include "heal_queue.hpp"
#include <cstdio>
#include <cstdlib>
using namespace rev_hud;
unsigned checks=0;
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
Session local(){return {true,4,{0x1000,1,0,1},{0x2000,2,1,1}};}
int main(){
    for(u32 delay=0;delay<5;++delay){
        HealQueue q;auto s=local();C(q.request(s,12,s.sub0.address));
        C(q.request(s,12,s.sub0.address));C(!q.consume(s,11,s.sub0.address));
        C(!q.consume(s,12,s.self.address));C(q.consume(s,12+delay,s.sub0.address)==(delay<=1));
        C(!q.consume(s,12+delay,s.sub0.address));
    }
    for(u32 field=0;field<14;++field){
        HealQueue q;auto s=local();C(q.request(s,10,s.sub0.address));auto changed=s;
        if(field==0)changed.active=false;
        if(field==1)++changed.epoch;
        if(field==2)++changed.self.address;
        if(field==3)++changed.self.lifetime;
        if(field==4)++changed.self.serial;
        if(field==5)changed.self.think_mode=3;
        if(field==6)++changed.sub0.address;
        if(field==7)++changed.sub0.lifetime;
        if(field==8)++changed.sub0.serial;
        if(field==9)changed.sub0.think_mode=3;
        if(field==10)changed.sub0.address=changed.self.address;
        if(field==11)changed.sub0.lifetime=0;
        if(field==12)changed.sub0.serial=-1;
        if(field==13)changed.epoch=0;
        C(!q.consume(changed,10,changed.sub0.address));
        C(!q.consume(s,10,s.sub0.address));
        C(q.request(s,11,s.sub0.address));C(q.consume(s,11,s.sub0.address));
    }
    {HealQueue q;auto s=local();C(!q.request(s,0,s.sub0.address));C(!q.request(s,4,s.self.address));
     C(q.request(s,~0u,s.sub0.address));C(!q.consume(s,0,s.sub0.address));C(q.consume(s,~0u,s.sub0.address));}
    std::printf("{\"status\":\"PASS\",\"checks\":%u,\"gameplay_executed\":false}\n",checks);
}
