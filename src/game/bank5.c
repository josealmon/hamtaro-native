#include "game/bank5.h"
#include "game/state.h"
#include "game/render.h"
#include "game/random.h"
#include "game/graphics.h"
#include "game/background.h"
#include "game/choice.h"
#include "gba/bios.h"

enum { CLASSES=0x0201DC40,GRID_POSITIONS=0x020017E0,GRID_IMAGES=0x020017A0,
       SELECTED_IMAGES=0x020017C0,SPAWN_COUNT=0x03001150,SPAWN_LIMIT=0x03001154,
       STATES=0x020046D7,COUNTERS=0x020046D3,RESULT=0x020046DE };
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static GameAddr leader(HNMemory *m){return hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER));}
static GameAddr follower(HNMemory *m){return hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER));}
static bool flag(HNMemory *m,unsigned id){return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void set_flag(HNMemory *m,unsigned id)
{GameAddr a=0x03002180+(id>>3);hn_write8(m,a,hn_read8(m,a)|(1u<<(id&7)));}
static bool animation(HNMemory *m,uint8_t index,GameAddr image)
{if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);hn_render_set_animation(m,index,image);return !m->fault;}
static void stop_slot(HNMemory *m,GameAddr slot)
{hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x28,0);hn_write32(m,slot+0x2C,0);}
static void request(HNMemory *m,uint16_t id)
{hn_choice_request(m,id);}
static void layer_scroll(HNMemory *m)
{int32_t offset=(int32_t)(0xF00000-hn_read32(m,hn_render_slot(m,10)+0x20));if(offset<0)offset=0;if(offset>0x300000)offset=0x300000;hn_write32(m,HN_LAYER_OFFSETS+0x40,(uint32_t)offset);}
static bool mechanism_selection(HNMemory *m)
{
    uint32_t state=control(m,0x1BD);
    if(state<=4){
        uint32_t consumed;
        if(hn_scene_callback(m,0x0801D120,0x08097062,&consumed)!=HN_SCENE_OK||
           hn_scene_callback(m,0x0801D120,0x0809706B,&consumed)!=HN_SCENE_OK)return false;
        if(control(m,0x62)==2&&control(m,0x60)==2){
            uint32_t selected=control(m,0x1BD),index=selected-1;store(m,0x1BD,selected+4);
            if(!animation(m,hn_read8(m,HN_RENDER_CLASS_LEADER),0x088A4C53)||
               !hn_render_configure_raw(m,(uint8_t)(hn_read8(m,HN_RENDER_CLASS_LEADER)+1),1,114,0,0,0,0x088E18D8)||
               !hn_render_modes_raw(m,(uint8_t)(hn_read8(m,HN_RENDER_CLASS_LEADER)+1),2,3,hn_read8(m,HN_RENDER_CLASS_LEADER))||
               !animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),0x088EFDC7)||
               !hn_render_configure_raw(m,(uint8_t)(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)+1),1,114,0,0,0,0x088E1904)||
               !hn_render_modes_raw(m,(uint8_t)(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)+1),2,3,hn_read8(m,HN_RENDER_CLASS_FOLLOWER)))return false;
            selected+=0x1B2;
            if(control(m,selected)==2)return hn_background_configure_raw(m,4,1,hn_read32(m,0x0200185C+index*8));
            if(control(m,selected)==1)return hn_background_configure_raw(m,4,1,hn_read32(m,0x0200185C+index*8+4));
        }
    }else{
        uint32_t index=state-5;
        if(hn_read32(m,hn_render_slot(m,13)+4)!=0x088EFDC7){
            unsigned offset=state+0x1AE;uint8_t count=control(m,offset);
            if(count){store(m,offset,count-1);
                if(!control(m,offset)&&!hn_render_configure_raw(m,15,1,114,(int16_t)((index*9+4)*8),0,240,0x088EFFCA))return false;}
            store(m,0x1BD,0);
        }
    }
    return !m->fault;
}
bool hn_bank5_primary_lane(HNMemory *m,uint32_t index)
{
    GameAddr state=STATES+index,partner_state=STATES+index+3;
    uint32_t primary_index=index+18,partner_index=index+21;
    GameAddr primary=hn_render_slot(m,primary_index),partner=hn_render_slot(m,partner_index);
    if(hn_read8(m,state)==0){
        if(hn_read8(m,partner_state)==1&&(int32_t)(hn_read32(m,partner+0x20)-hn_read32(m,primary+0x20))<=0xBFFFF){
            if(!animation(m,(uint8_t)primary_index,0x088EFF04))return false;
            stop_slot(m,hn_render_slot(m,(uint8_t)primary_index));hn_write8(m,state,1);
            if(!animation(m,(uint8_t)partner_index,0x088EFF5C))return false;
            hn_write8(m,partner_state,2);hn_write32(m,SPAWN_COUNT,hn_read32(m,SPAWN_COUNT)+1);
        }
    }else if(hn_read8(m,state)==1&&!hn_read8(m,primary+0x60)){
        int16_t x=(int16_t)(index*80+64);
        if((int32_t)hn_read32(m,SPAWN_COUNT)<(int32_t)hn_read32(m,SPAWN_LIMIT)){
            uint8_t choice=(uint8_t)hn_random_range(m,1,10);
            bool alternate=flag(m,0xB8);
            if(!hn_render_configure_raw(m,(uint8_t)primary_index,1,114,x,0,64,alternate?0x088EFE6E:0x088EFE3E))return false;
            if(alternate){
                if(index==2||(uint8_t)(choice-7)<=1)if(!animation(m,(uint8_t)primary_index,0x088EFE9E))return false;
                if(choice==9&&!animation(m,(uint8_t)primary_index,0x088EFECE))return false;
            }
        }else if(!hn_render_configure_raw(m,(uint8_t)primary_index,1,114,x,0,64,0x088EFECE))return false;
        hn_render_flags(m,primary_index,3,1);hn_write8(m,state,0);
    }
    return !m->fault;
}
bool hn_bank5_partner_lane(HNMemory *m,uint32_t index)
{
    GameAddr state=STATES+index+3;uint32_t slot_index=index+21;GameAddr slot=hn_render_slot(m,slot_index);
    switch(hn_read8(m,state)){
    case 1:if((int32_t)hn_read32(m,slot+0x20)<=0x2FFFFF){
        if(!animation(m,(uint8_t)slot_index,0x088EFF5C))return false;hn_write8(m,state,2);}break;
    case 2:if(!hn_read8(m,slot+0x60)){
        if(!hn_render_configure_raw(m,(uint8_t)slot_index,1,114,(int16_t)(index*80+64),0,176,0x088EFF90))return false;hn_write8(m,state,3);}break;
    case 3:if(hn_read32(m,slot+4)!=0x088EFF90)hn_write8(m,state,0);break;
    default:break;
    }
    return !m->fault;
}
uint32_t hn_bank5_outcome(HNMemory *m)
{
    hn_write8(m,RESULT,0);
    for(unsigned i=0;i<3;i++)if(!hn_read8(m,RESULT-7+i)&&(int32_t)hn_read32(m,hn_render_slot(m,18+i)+0x20)>0x9C0000){
        hn_write8(m,0x030021E0,hn_read8(m,0x030021E0)|4);if(flag(m,0x2C7))request(m,3);hn_write8(m,RESULT,1);return 1;
    }
    uint8_t sum=hn_read8(m,RESULT)+hn_read8(m,COUNTERS);
    for(unsigned i=1;i<4;i++)sum=(uint8_t)(sum+hn_read8(m,COUNTERS+i));
    if(!sum){
        for(unsigned i=0;i<3;i++){
            hn_write8(m,COUNTERS+4+i,2);if(!animation(m,(uint8_t)(18+i),0x088EFF04))return 0;stop_slot(m,hn_render_slot(m,18+i));
        }
        hn_write8(m,0x030021E0,hn_read8(m,0x030021E0)|4);if(flag(m,0x2C7))request(m,2);hn_write8(m,RESULT,2);return 1;
    }
    return 0;
}
static bool mechanism(HNMemory *m)
{
    layer_scroll(m);if(control(m,0x1BD)&&!mechanism_selection(m))return false;
    for(uint32_t i=0;i<3;i++)if(!hn_bank5_primary_lane(m,i)||!hn_bank5_partner_lane(m,i))return false;
    if(hn_bank5_outcome(m)){
        for(unsigned i=18;i<=23;i++){GameAddr s=hn_render_slot(m,i);hn_write32(m,s+0x24,0);hn_write32(m,s+0x2C,0);}
        GameAddr s=leader(m);hn_write32(m,s+0x24,0);hn_write32(m,s+0x2C,0);
        s=follower(m);hn_write32(m,s+0x24,0);hn_write32(m,s+0x2C,0);
    }
    if(hn_read32(m,0x03002BB0)<=0x587C6)hn_write32(m,0x03002BB0,hn_read32(m,0x03002BB0)+1);return !m->fault;
}
static void mirror(HNMemory *m)
{
    uint32_t baseline=(uint32_t)hn_read16(m,CLASSES+52*2)<<16;
    GameAddr source=leader(m),destination=hn_render_slot(m,control(m,0x23));
    hn_write32(m,destination+0x20,0xFFF00000-(hn_read32(m,source+0x20)-baseline)*2);
    uint16_t x=(uint16_t)(hn_read32(m,source+0x18)>>16),z=(uint16_t)(hn_read32(m,source+0x20)>>16);
    bool in_area=(uint16_t)(x-65)<=78&&z<=128;uint32_t i;
    for(i=0;i<=22;i++)if(hn_read32(m,source+4)==hn_read32(m,0x02002D60+i*4))break;
    if(i>22){
        if(in_area&&flag(m,0x15A))hn_write32(m,destination+0x20,0xA00000);
        else{hn_write32(m,destination+4,0x088FC9F9);hn_write8(m,destination+0x63,0);hn_write16(m,destination+0x4C,0);}
    }else if(!in_area){
        if(hn_read32(m,destination+4)!=hn_read32(m,0x02002E04+i*4)){
            hn_write32(m,destination+4,hn_read32(m,0x02002E04+i*4));hn_write8(m,source+0x63,0);hn_write8(m,destination+0x63,0);
            hn_write16(m,destination+0x4C,hn_read16(m,source+0x4C));}
    }else if(!flag(m,0x15A)){
        if(hn_read32(m,destination+4)!=hn_read32(m,0x020017FC+i*4)){
            hn_write32(m,destination+4,hn_read32(m,0x020017FC+i*4));hn_write8(m,source+0x63,0);hn_write8(m,destination+0x63,0);
            hn_write16(m,destination+0x4C,hn_read16(m,source+0x4C));}
        if(i-8<=3)set_flag(m,0x159);
    }else hn_write32(m,destination+0x20,0xA00000);
    if(flag(m,16)){
        source=follower(m);x=(uint16_t)(hn_read32(m,source+0x18)>>16);z=(uint16_t)(hn_read32(m,source+0x20)>>16);
        in_area=(uint16_t)(x-65)<=78&&z<=128;destination-=104;
        hn_write32(m,destination+0x20,in_area?0xA00000:0xFFF00000-(hn_read32(m,source+0x20)-baseline)*2);
        for(i=0;i<=17;i++)if(hn_read32(m,source+4)==hn_read32(m,0x02002DBC+i*4))break;
        if(i>17){hn_write32(m,destination+4,0x088FCA01);hn_write8(m,destination+0x63,0);hn_write16(m,destination+0x4C,0);}
        else if(hn_read32(m,destination+4)!=hn_read32(m,0x02002E60+i*4)){
            hn_write32(m,destination+4,hn_read32(m,0x02002E60+i*4));hn_write8(m,source+0x63,0);hn_write8(m,destination+0x63,0);
            hn_write16(m,destination+0x4C,hn_read16(m,source+0x4C));}
    }
}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_bank5_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x0808509C:if(hn_read16(m,CLASSES+12)>hn_read16(m,CLASSES+18))hn_write16(m,CLASSES+12,0);break;
    case 0x08085414:case 0x08085460:{
        bool replace=(callback&~1u)==0x08085460;if(replace)store(m,control(m,0x60)+0x1A4,7);
        for(unsigned i=0;i<3;i++){
            unsigned selected=hn_random_range(m,0,6)+0x1A4;
            if(!control(m,selected)){store(m,selected,control(m,0x61+i));if(!replace&&control(m,0x61+i)<=3)set_flag(m,0x2AC);}
        }break;
    }
    case 0x080854B0:
        for(unsigned cell=0;cell<6;cell++){
            REQUIRE(hn_render_configure_raw(m,(uint8_t)(cell+26),1,114,(int16_t)hn_read16(m,GRID_POSITIONS+cell*4),0,
                (int16_t)hn_read16(m,GRID_POSITIONS+cell*4+2),hn_read32(m,GRID_IMAGES+control(m,0x1A4+cell)*4)));
            if(!control(m,0x1A4+cell))hn_render_flags(m,cell+26,3,1);
        }break;
    case 0x08085558:{
        uint16_t choice=hn_random_range(m,0,0);unsigned cell=control(m,0x60),index=cell+26;
        REQUIRE(hn_render_configure_raw(m,(uint8_t)index,1,114,(int16_t)hn_read16(m,GRID_POSITIONS+cell*4),0,
            (int16_t)hn_read16(m,GRID_POSITIONS+cell*4+2),(int16_t)choice>=0?0x088EEE02:0x088EEE1A));
        hn_render_flags(m,index,3,1);break;
    }
    case 0x080855F4:case 0x080858EC:case 0x08086AB0:break;
    case 0x080858F0:{
        unsigned cell=control(m,0x60),kind=control(m,0x1A4+cell);
        REQUIRE(hn_render_configure_raw(m,19,1,114,(int16_t)hn_read16(m,GRID_POSITIONS+cell*4),0,
            (int16_t)hn_read16(m,GRID_POSITIONS+cell*4+2),hn_read32(m,SELECTED_IMAGES+kind*4)));break;
    }
    case 0x08085B80:{
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS),slot=base+hn_read8(m,HN_RENDER_CLASS_LEADER)*104;
        hn_write32(m,slot+0x20,hn_read32(m,slot+0x20)+0xFF700000);
        slot=base+hn_read8(m,HN_RENDER_CLASS_FOLLOWER)*104;hn_write32(m,slot+0x20,hn_read32(m,slot+0x20)+0xFF700000);break;
    }
    case 0x0808641C:{
        int32_t delta=(int32_t)(0x688000-hn_read32(m,leader(m)+0x20));hn_write16(m,CLASSES+42*2,(uint16_t)(delta>0?delta>>15:0));
        delta=(int32_t)(0x808000-hn_read32(m,follower(m)+0x20));hn_write16(m,CLASSES+43*2,(uint16_t)(delta>>15));
        if(delta<=0)hn_write16(m,CLASSES+43*2,0);break;
    }
    case 0x08086974:*consumed=hn_bank5_outcome(m);break;
    case 0x08086BE0:{
        uint16_t choice=hn_random_range(m,0,hn_read16(m,CLASSES+22)>49?500:1000);hn_write32(m,SPAWN_COUNT,0);uint32_t style,limit;
        if(!flag(m,0xB8)){style=0;limit=80;}
        else if(choice<=249){style=1;limit=60;hn_write16(m,CLASSES+12,10);}
        else if(choice<=399){style=2;limit=40;hn_write16(m,CLASSES+12,20);}
        else if(choice<=499){style=3;limit=20;hn_write16(m,CLASSES+12,30);}
        else if(choice<=699){style=4;limit=60;}else if(choice<=899){style=5;limit=40;}else{style=6;limit=20;}
        hn_write32(m,SPAWN_LIMIT,limit);hn_write8(m,0x020046DF,(uint8_t)style);break;
    }
    case 0x08086CC4:hn_write32(m,SPAWN_COUNT,0);break;
    case 0x08086CD0:{
        uint32_t ticks=hn_read32(m,0x03002BB0);
        if(ticks<=65535){unsigned rank=0;
            for(unsigned i=0;i<3;i++){uint16_t threshold=hn_read16(m,CLASSES+(63+i)*2);if(!threshold||ticks<=threshold){rank=i+1;break;}}
            store(m,0,rank);store(m,1,ticks/3600);store(m,2,(ticks%3600)/60);store(m,3,((ticks%60)*167)/100);
            if(rank){for(unsigned i=65;i>62+rank;i--)hn_write16(m,CLASSES+i*2,hn_read16(m,CLASSES+(i-1)*2));hn_write16(m,CLASSES+(62+rank)*2,(uint16_t)ticks);}
        }break;
    }
    case 0x08087048:{
        uint32_t index=hn_read8(m,arguments);int32_t x=(int8_t)hn_read8(m,arguments+1),y=(int8_t)hn_read8(m,arguments+2),z=(int8_t)hn_read8(m,arguments+3);
        GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+((uint32_t)x<<16));
        hn_write32(m,slot+0x1C,hn_read32(m,slot+0x1C)+((uint32_t)y<<16));hn_write32(m,slot+0x20,hn_read32(m,slot+0x20)+((uint32_t)z<<16));*consumed=4;break;
    }
    case 0x08087B04:{
        GameAddr slot=hn_render_slot(m,13);uint16_t z=(uint16_t)(hn_read32(m,slot+0x20)>>16);
        if((int16_t)z<=71)z=72;if((((uint32_t)z<<16)+0xFFA80000)>>16<=39)z=88;
        if((((uint32_t)z<<16)+0xFF800000)>>16<=95)z=224;if((int16_t)z>240)z=240;
        hn_write32(m,slot+0x20,((uint32_t)z<<16)+0x8000);break;
    }
    default:return hn_bank5_preset_callback(m,callback,consumed);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank5_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080864B4:return mechanism(m)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x08086550:layer_scroll(m);return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
    case 0x08086584:return mechanism_selection(m)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x0808509C:case 0x08085414:case 0x08085460:case 0x080854B0:case 0x08085558:
    case 0x080855F4:case 0x080858EC:case 0x080858F0:case 0x08085B80:case 0x0808641C:
    case 0x08086974:case 0x08086AB0:case 0x08086BE0:case 0x08086CC4:case 0x08086CD0:case 0x08087B04:{
        uint32_t ignored;return hn_bank5_callback(m,callback,0,&ignored);
    }
    default:return hn_bank5_presets(m,callback);
    }
}
HNSceneResult hn_bank5_irq(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080864B4:return mechanism(m)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    default:return HN_SCENE_UNKNOWN;
    }
}
HNSceneResult hn_bank5_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x08085948:case 0x08086160:case 0x08087000:case 0x0808781C:{
        GameAddr phase=(callback&~1u)==0x08085948?0x020017F8:(callback&~1u)==0x08086160?0x02001858:(callback&~1u)==0x08087000?0x0200187C:0x02001884;
        hn_write32(m,slot+0x1C,(uint32_t)(int32_t)(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+(hn_read8(m,phase)+64)*2)<<10);
        unsigned step=(callback&~1u)==0x08086160?(flag(m,0x2A9)&&flag(m,0x2AA)?1:2):(callback&~1u)==0x08087000?(control(m,0xA1)<=4?2:1):2;
        hn_write8(m,phase,(uint8_t)(hn_read8(m,phase)+step));*yield=1;break;
    }
    case 0x08085978:*yield=1;mirror(m);break;
    case 0x080861B8:
        hn_write32(m,slot+0x24,(uint32_t)hn_runtime_divide((int32_t)(((uint32_t)hn_read16(m,CLASSES+38*2)<<16)-hn_read32(m,slot+0x18)),hn_read8(m,arguments)));
        hn_write32(m,slot+0x2C,(uint32_t)hn_runtime_divide((int32_t)(((uint32_t)hn_read16(m,CLASSES+39*2)<<16)-hn_read32(m,slot+0x20)),hn_read8(m,arguments)));
        *yield=0;*consumed=1;break;
    case 0x080875A4:case 0x08087600:case 0x0808767C:{
        unsigned kind=(callback&~1u)==0x080875A4?0:(callback&~1u)==0x08087600?1:2;GameAddr counter=0x02001880+kind;uint8_t count=hn_read8(m,counter);
        if(kind==0&&(count==192||count==96)){
            hn_write32(m,slot+0x1C,0xFFDD8000);hn_write32(m,slot+0x34,count==192?0x100:0xFFFFFF00);hn_write32(m,slot+0x28,count==192?0xFFFFD000:0x3000);
        }
        if(kind==1&&(count==32||count==16)){
            hn_write32(m,slot+0x18,0xC08000);hn_write32(m,slot+0x20,0x8E8000);hn_write32(m,slot+0x1C,0xFFDD8000);hn_write32(m,slot+0x30,0);
            hn_write32(m,slot+0x34,count==32?0xFFFFFC00:0x400);hn_write32(m,slot+0x28,count==32?0x2000:0xFFFFE000);
        }
        if(kind==2&&count==40){
            hn_write32(m,slot+0x18,0xB68000);hn_write32(m,slot+0x20,0x8E8000);hn_write32(m,slot+0x1C,0xFFDD8000);hn_write32(m,slot+0x30,0xFFFFFE00);hn_write32(m,slot+0x24,0x5000);
        }
        hn_write8(m,counter,(uint8_t)(hn_read8(m,counter)-1));if(!hn_read8(m,counter))hn_write8(m,counter,kind==0?192:kind==1?32:40);*yield=1;break;
    }
    case 0x080879FC:{
        uint8_t selected=(uint8_t)hn_random_range(m,1,6);GameAddr position=0x08695D88+selected*4;
        hn_write32(m,slot+0x18,((uint32_t)(int32_t)(int16_t)hn_read16(m,position)<<16)+0x8000);
        hn_write32(m,slot+0x20,((uint32_t)(int32_t)(int16_t)hn_read16(m,position+2)<<16)+0x8000);store(m,0x1AE,selected);store(m,0x1B1,0);*yield=0;break;
    }
    case 0x08087A58:
        if(control(m,0x1B0)&&control(m,0x1AD)==control(m,0x1AE)){store(m,0x1B2,control(m,0x1B2)+1);store(m,0x1B1,control(m,0x1B2)>2?2:1);}*yield=0;break;
    case 0x08087AB8:for(unsigned i=0;i<4;i++){hn_write32(m,slot+8+i*4,0);hn_write16(m,slot+0x56+i*2,0);}*yield=0;break;
    case 0x08087AE8:REQUIRE(hn_graphics_initialize_raw(m,4,1,0x088F1F92));*yield=0;break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
