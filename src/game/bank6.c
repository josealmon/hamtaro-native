#include "game/bank6.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "gba/bios.h"

enum { CLASSES=0x0201DC40 };
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static uint8_t selected(HNMemory *m,unsigned which)
{return hn_read8(m,which==0?HN_RENDER_CLASS_LEADER:which==1?HN_RENDER_CLASS_FOLLOWER:which==2?0x03002C04:0x03002C00);}
static bool flag(HNMemory *m,unsigned id){return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void set_flag(HNMemory *m,unsigned id,bool value)
{GameAddr a=0x03002180+(id>>3);uint8_t bit=(uint8_t)(1u<<(id&7));hn_write8(m,a,(uint8_t)((hn_read8(m,a)&~bit)|(value?bit:0)));}
static bool animation(HNMemory *m,uint8_t index,GameAddr image)
{if(index==selected(m,1))hn_write8(m,0x03002170,0);hn_render_set_animation(m,index,image);return !m->fault;}
static int32_t word(HNMemory *m,GameAddr a){return (int32_t)hn_read32(m,a);}
static int32_t magnitude(int32_t value){return value>0?value:(int32_t)(0u-(uint32_t)value);}
static uint8_t direction(HNMemory *m,uint32_t from,uint32_t to)
{return (uint8_t)hn_render_direction_raw(m,(uint8_t)from,(uint8_t)to,false);}
static void velocity(HNMemory *m,uint8_t index,int16_t x,int16_t y,int16_t z)
{GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x24,(uint32_t)(int32_t)x<<8);hn_write32(m,slot+0x28,(uint32_t)(int32_t)y<<8);hn_write32(m,slot+0x2C,(uint32_t)(int32_t)z<<8);}
static bool auxiliary(HNMemory *m,uint8_t index,uint8_t parent,GameAddr image)
{return hn_render_configure_raw(m,index,1,176,0,0,0,image)&&hn_render_modes_raw(m,index,2,3,parent);}
bool hn_bank6_orbit(HNMemory *m,uint32_t index_word,uint32_t class_word)
{
    uint8_t index=(uint8_t)index_word;GameAddr phase=CLASSES+(uint16_t)class_word*2u;
    uint32_t increment=hn_read16(m,0x086BF6DC+(control(m,0x1A2)*3+control(m,0x1A4))*2);
    uint32_t value=increment+hn_read16(m,phase);hn_write16(m,phase,(uint16_t)value);
    if((value<<16)>0xEFFF0000)hn_write16(m,phase,1023);
    if(hn_read16(m,phase)>1023)hn_write16(m,phase,0);
    unsigned angle=hn_read16(m,phase)>>2;
    int32_t cosine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+(angle+64)*2),sine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+angle*2);
    GameAddr center=hn_render_slot(m,25);int32_t x=(cosine+sine)/8+(int16_t)hn_read16(m,center+0x1A);
    int32_t z=(sine-cosine)/8+(int16_t)hn_read16(m,center+0x22);GameAddr slot=hn_render_slot(m,index);
    hn_write32(m,slot+0x18,(uint32_t)(uint16_t)x<<16);hn_write32(m,slot+0x1C,0);hn_write32(m,slot+0x20,(uint32_t)z<<16);return !m->fault;
}
void hn_bank6_planar_transform(HNMemory *m,GameAddr slot,uint32_t x,uint32_t y,uint32_t first,uint32_t second)
{hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+y*first+(0u-x)*second);hn_write32(m,slot+0x1C,hn_read32(m,slot+0x1C)+first*x+y*second);}
static bool oscillators(HNMemory *m)
{
    GameAddr slot=hn_render_slot(m,25);uint32_t step=(uint32_t)hn_read16(m,0x086BF6E8+control(m,0x1A3)*2)<<8;
    if(!control(m,0x1A2)){hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+step);if(word(m,slot+0x18)>0x97FFFF)store(m,0x1A2,1);}
    else{hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)-step);if(word(m,slot+0x18)<=0x280000)store(m,0x1A2,0);}
    slot=hn_render_slot(m,13);
    if(!control(m,0x1A5)){
        hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+0x10000);
        if(word(m,slot+0x18)>0x87FFFF){set_flag(m,23,true);store(m,0x1A5,1);if(!animation(m,13,control(m,0x13)!=3?0x088B42B0:0x088DB729))return false;}
    }else if(control(m,0x1A5)==1){
        hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)-0x10000);
        if(word(m,slot+0x18)<=0x380000){set_flag(m,23,true);store(m,0x1A5,0);if(!animation(m,13,control(m,0x13)!=3?0x088B434F:0x088DB768))return false;}
    }
    for(unsigned i=0;i<4;i++)if(!hn_bank6_orbit(m,i+26,i+224))return false;return !m->fault;
}
static bool align_follower(HNMemory *m,GameAddr arguments)
{
    GameAddr slot=hn_render_slot(m,13);bool fixed=hn_read8(m,arguments)!=0;
    int32_t target=fixed?0x600000:word(m,hn_render_slot(m,25)+0x18);bool after=word(m,slot+0x18)>target;
    set_flag(m,23,true);
    int32_t delta=(int32_t)(after?hn_read32(m,slot+0x18)-(uint32_t)target:(uint32_t)target-hn_read32(m,slot+0x18));
    hn_write16(m,CLASSES+86,(uint16_t)(delta>>18));
    if(hn_read16(m,CLASSES+86)<=1)hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+(after?0x80000u:0u-0x80000u));
    /* Reload both positions: the class store and first position write can alias. */
    uint32_t live_target=fixed?0x600000:hn_read32(m,hn_render_slot(m,25)+0x18);
    uint32_t correction=(after?hn_read32(m,slot+0x18)-live_target:live_target-hn_read32(m,slot+0x18))&0x70000;
    hn_write32(m,slot+0x18,after?hn_read32(m,slot+0x18)-correction:hn_read32(m,slot+0x18)+correction);
    GameAddr image;
    if(fixed)image=after?(control(m,0x13)!=3?0x088B900D:0x088DC809):(control(m,0x13)!=3?0x088B8FE5:0x088DC7E1);
    else image=after?(control(m,0x13)!=3?0x088B8FF9:0x088DC7F5):(control(m,0x13)!=3?0x088B8FD1:0x088DC7CD);
    if(!animation(m,13,image))return false;
    store(m,0x1A5,fixed?0:word(m,hn_render_slot(m,25)+0x18)>0x5FFFFF);
    hn_write16(m,CLASSES+86,hn_read16(m,CLASSES+86)&0xFFFE);return !m->fault;
}
static bool facing(HNMemory *m,GameAddr callback)
{
    if(callback==0x080A1BA0||callback==0x080A1BFC){
        unsigned target=callback==0x080A1BA0?2:3;uint32_t index=selected(m,0);
        hn_render_set_animation(m,index,hn_read32(m,0x02001898+((uint32_t)direction(m,index,selected(m,target))-1)*4));
        index=selected(m,1);hn_render_set_animation(m,index,hn_read32(m,0x020018A8+((uint32_t)direction(m,index,selected(m,target))-1)*4));return !m->fault;
    }
    if(callback==0x080A1C90){
        hn_render_set_animation(m,selected(m,0),0x088AD680);
        if(!auxiliary(m,(uint8_t)(selected(m,0)-1),selected(m,0),0x088AD695)||!auxiliary(m,(uint8_t)(selected(m,0)+1),selected(m,0),0x088AD6B4))return false;
        hn_render_set_animation(m,selected(m,1),0x088B59B5);
        return auxiliary(m,(uint8_t)(selected(m,1)-1),selected(m,1),0x088B59C1)&&auxiliary(m,(uint8_t)(selected(m,1)+1),selected(m,1),0x088B59D2);
    }
    if(callback==0x080A1984){
        uint8_t d=(uint8_t)(direction(m,selected(m,2),selected(m,0))-1);
        if(d<2)hn_render_set_animation(m,selected(m,2),0x088F2FC8);
        else if(d<=3){hn_render_set_animation(m,selected(m,2),d==2?0x088DEC87:0x088DECA0);
            if(!hn_render_configure_raw(m,(uint8_t)(selected(m,2)-1),1,176,0,-1,1,d==2?0x088F306E:0x088F3076)||
               !hn_render_modes_raw(m,(uint8_t)(selected(m,2)-1),2,3,selected(m,2)))return false;}
        return !m->fault;
    }
    bool next=callback==0x080A1850;
    if(!hn_graphics_initialize_raw(m,4,1,next?0x088E137B:0x088E135C))return false;
    uint32_t index=next&&!control(m,0x1A8)?42:selected(m,2);
    hn_render_set_animation(m,index,hn_read32(m,0x02001888+((uint32_t)direction(m,index,selected(m,0))-1)*4));
    uint32_t second_index=next?index:selected(m,2);
    GameAddr image=hn_read32(m,(next?0x020018B8:0x020018C8)+((uint32_t)direction(m,second_index,selected(m,0))-1)*4);
    uint8_t neighbor=next?(uint8_t)(index+1):(uint8_t)(selected(m,2)-1);
    if(!hn_render_configure_raw(m,neighbor,1,176,0,0,0,image))return false;
    return hn_render_modes_raw(m,next?neighbor:(uint8_t)(selected(m,2)-1),2,3,next?(uint8_t)index:selected(m,2));
}
static bool final_motion(HNMemory *m)
{
    bool reverse=flag(m,0x2B7);GameAddr moving=hn_render_slot(m,16),trigger=hn_render_slot(m,22);
    if(!reverse){
        if(word(m,trigger+0x18)<=0xC3FFFF&&hn_read32(m,moving+4)!=0x088DB768)if(!animation(m,22,0x088F40D7))return false;
        moving=hn_render_slot(m,16);trigger=hn_render_slot(m,22);
        if(word(m,moving+0x18)<=0xA7FFFF){if(hn_read32(m,moving+4)!=0x088DB768){set_flag(m,23,true);if(!animation(m,16,0x088DB768))return false;velocity(m,16,256,0,0);}}
        else if(word(m,trigger+0x18)<=0xC3FFFF){velocity(m,16,0,0,0);if(!animation(m,16,0x088DB5DC))return false;}
        else set_flag(m,0x2B7,true);
    }else{
        if(word(m,trigger+0x18)>0x3C0000&&hn_read32(m,moving+4)!=0x088DB729)if(!animation(m,22,0x088F40E9))return false;
        moving=hn_render_slot(m,16);trigger=hn_render_slot(m,22);
        if(word(m,moving+0x18)>0x580000){if(hn_read32(m,moving+4)!=0x088DB729){set_flag(m,23,true);if(!animation(m,16,0x088DB729))return false;velocity(m,16,-256,0,0);}}
        else if(word(m,trigger+0x18)>0x3C0000){velocity(m,16,0,0,0);if(!animation(m,16,0x088DB5C5))return false;}
        else set_flag(m,0x2B7,false);
    }
    return !m->fault;
}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_bank6_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080A0850:case 0x080A0890:{
        uint8_t id=hn_read8(m,arguments);unsigned i=0;for(;i<32;i++)if(hn_read8(m,0x0201EE40+i)==id)break;
        if((callback&~1u)==0x080A0850)store(m,0x60,i<32);
        else if(i<32){for(;i<31;i++)hn_write8(m,0x0201EE40+i,hn_read8(m,0x0201EE40+i+1));hn_write8(m,0x0201EE40+31,0);}
        *consumed=1;break;
    }
    case 0x080A09F0:REQUIRE(oscillators(m));break;
    case 0x080A0B3C:REQUIRE(align_follower(m,arguments));*consumed=1;break;
    case 0x080A1070:break;
    case 0x080A1E40:{
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS);hn_write32(m,base+selected(m,2)*104+0x24,0);hn_write32(m,base+selected(m,2)*104+0x2C,0);
        uint8_t d=direction(m,selected(m,2),selected(m,0));GameAddr image=d==1?0x088DB59A:d==2?0x088DB583:d==3?0x088DB5C5:0x088DB5DC;
        if(d>=1&&d<=4)hn_render_set_animation(m,selected(m,2),image);break;
    }
    case 0x080A1ED4:store(m,0x60,0);if(direction(m,selected(m,2),selected(m,0))==1)store(m,0x60,1);break;
    case 0x080A1F18:*consumed=word(m,hn_render_slot(m,selected(m,0))+0x20)>0xAFFFFF?5:0;break;
    case 0x080A1F48:{
        uint8_t first=hn_read8(m,arguments),second=hn_read8(m,arguments+1);
        if(word(m,hn_render_slot(m,second)+0x18)<word(m,hn_render_slot(m,first)+0x18)){hn_write32(m,0x03002C60,hn_read32(m,arguments+2));*consumed=UINT32_MAX-3;}
        else *consumed=6;break;
    }
    case 0x080A1FA0:{
        uint8_t first=hn_read8(m,arguments),target_index=hn_read8(m,arguments+1),moving=hn_read8(m,arguments+2);
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS),target=base+target_index*104,mover=base+moving*104,origin=base+first*104;
        int32_t dx=(int32_t)(hn_read32(m,target+0x18)-hn_read32(m,origin+0x18))/65536,dz=(int32_t)(hn_read32(m,target+0x20)-hn_read32(m,origin+0x20))/65536;
        uint8_t payload[8];for(unsigned i=0;i<8;i++)payload[i]=hn_read8(m,arguments+3+i);
        unsigned class_index=payload[0]|(payload[1]<<8);int32_t squared=(int32_t)((uint32_t)dx*(uint32_t)dx+(uint32_t)dz*(uint32_t)dz);
        uint32_t duration=hn_bios_sqrt((uint32_t)(squared/64));hn_write16(m,CLASSES+class_index*2,(uint16_t)duration);duration=(uint16_t)duration;
        int32_t vx=hn_runtime_divide((int32_t)((uint32_t)dx<<16),(int32_t)duration);hn_write32(m,mover+0x24,(uint32_t)vx);
        int32_t vz=hn_runtime_divide((int32_t)((uint32_t)dz<<16),(int32_t)duration);hn_write32(m,mover+0x2C,(uint32_t)vz);
        target=hn_render_slot(m,target_index);hn_write32(m,mover+0x18,hn_read32(m,target+0x18)-(uint32_t)vx*duration+0x140000);
        hn_write32(m,mover+0x20,hn_read32(m,target+0x20)-(uint32_t)vz*duration+0x140000);*consumed=5;break;
    }
    case 0x080A2C34:case 0x080A2C90:{
        bool task=(callback&~1u)==0x080A2C90;int16_t x=(int16_t)hn_read16(m,task?0x03002BB2:CLASSES+68),z=(int16_t)hn_read16(m,task?0x03002BBA:CLASSES+70);
        REQUIRE(hn_render_configure_raw(m,24,1,114,x,-96,z,task?0x088F3F78:0x088F3E0A));
        REQUIRE(hn_render_configure_raw(m,23,1,114,x,-32,z,task?0x088F3FC3:0x088F3E12));break;
    }
    case 0x080A2CE8:{
        uint32_t x1=((uint32_t)hn_read16(m,CLASSES+68)<<16)+0x8000,z1=((uint32_t)hn_read16(m,CLASSES+70)<<16)+0x8000;
        uint32_t x2=((uint32_t)hn_read16(m,CLASSES+72)<<16)+0x8000,z2=((uint32_t)hn_read16(m,CLASSES+74)<<16)+0x8000;
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS),slot=base+10*104;
        hn_write32(m,slot+0x24,(uint32_t)hn_runtime_divide((int32_t)(x1-hn_read32(m,slot+0x18)),18));hn_write32(m,slot+0x2C,(uint32_t)hn_runtime_divide((int32_t)(z1-hn_read32(m,slot+0x20)),18));
        slot=base+13*104;hn_write32(m,slot+0x24,(uint32_t)hn_runtime_divide((int32_t)(x2-hn_read32(m,slot+0x18)),23));hn_write32(m,slot+0x2C,(uint32_t)hn_runtime_divide((int32_t)(z2-hn_read32(m,slot+0x20)),23));break;
    }
    case 0x080A2D74:{GameAddr slot=hn_render_slot(m,21);hn_write32(m,slot+0x18,((uint32_t)hn_read16(m,CLASSES+68)<<16)+0x8000);hn_write32(m,slot+0x20,((uint32_t)hn_read16(m,CLASSES+70)<<16)+0x8000);break;}
    case 0x080A39CC:REQUIRE(final_motion(m));break;
    case 0x080A3B38:{
        GameAddr slot=hn_render_slot(m,19);hn_write32(m,slot+0x24,(0u-(uint32_t)(int32_t)(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+103*2))*96);
        hn_write32(m,slot+0x28,0);hn_write32(m,slot+0x2C,(0u-(uint32_t)(int32_t)(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+39*2))*96);break;
    }
    case 0x080A3B7C:*consumed=word(m,hn_render_slot(m,19)+0x20)<=0x3A7FFF?5:0;break;
    case 0x080A3BA4:*consumed=word(m,hn_render_slot(m,selected(m,0))+0x20)<word(m,hn_render_slot(m,selected(m,2))+0x20)?5:0;break;
    default:{HNSceneResult result=hn_bank6_targets_callback(m,callback,arguments,consumed);if(result!=HN_SCENE_UNKNOWN)return result;
        result=hn_bank6_chain_callback(m,callback,arguments,consumed);if(result!=HN_SCENE_UNKNOWN)return result;return hn_bank6_preset_callback(m,callback,consumed);}
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank6_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080A1328:{
        for(unsigned i=26;i<=36;i+=2)hn_render_set_animation(m,i,0x088F2EA7);
        GameAddr slot=hn_render_slot(m,26);for(unsigned i=0;i<6;i++,slot+=208){hn_write32(m,slot+0x1C,0x8000);hn_write32(m,slot+0x28,0);hn_write32(m,slot+0x34,0);}break;
    }
    case 0x080A1850:case 0x080A18EC:case 0x080A1984:case 0x080A1BA0:case 0x080A1BFC:case 0x080A1C90:REQUIRE(facing(m,callback&~1u));break;
    case 0x080A09F0:case 0x080A1070:case 0x080A1E40:case 0x080A1ED4:case 0x080A1F18:
    case 0x080A21D4:case 0x080A2744:case 0x080A2994:case 0x080A2C34:case 0x080A2C90:case 0x080A2CE8:case 0x080A2D74:
    case 0x080A39CC:case 0x080A3B38:case 0x080A3B7C:case 0x080A3BA4:{uint32_t ignored;return hn_bank6_callback(m,callback,0,&ignored);}
    default:{HNSceneResult result=hn_bank6_chain_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result;return hn_bank6_presets(m,callback);}
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank6_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080A0F44:case 0x080A0FE8:{
        bool payload=(callback&~1u)==0x080A0F44;uint32_t x,z;unsigned class_index;
        if(payload){x=hn_read16(m,arguments);z=hn_read16(m,arguments+2);class_index=hn_read8(m,arguments+4);
            uint32_t old_x=hn_read32(m,slot+0x18);hn_write16(m,CLASSES+76,(uint16_t)(old_x>>16));uint32_t old_z=hn_read32(m,slot+0x20);hn_write16(m,CLASSES+78,(uint16_t)(old_z>>16));
            int32_t dx=(int32_t)((x<<16)-old_x),dz=(int32_t)((z<<16)-old_z),ax=magnitude(dx),az=magnitude(dz),duration=(ax>az?ax:az)>>16;
            hn_write16(m,CLASSES+class_index*2,(uint16_t)(duration&-2));hn_write32(m,slot+0x24,(uint32_t)hn_runtime_divide(dx,duration));hn_write32(m,slot+0x2C,(uint32_t)hn_runtime_divide(dz,duration));*consumed=5;
        }else{
            int32_t dx=(int32_t)(((uint32_t)hn_read16(m,CLASSES+76)<<16)-hn_read32(m,slot+0x18));
            int32_t dz=(int32_t)(((uint32_t)hn_read16(m,CLASSES+78)<<16)-hn_read32(m,slot+0x20));int32_t ax=magnitude(dx),az=magnitude(dz),duration=(ax>az?ax:az)>>16;
            hn_write16(m,CLASSES+88,(uint16_t)(duration&-2));hn_write32(m,slot+0x24,(uint32_t)hn_runtime_divide(dx,duration));hn_write32(m,slot+0x2C,(uint32_t)hn_runtime_divide(dz,duration));
        }
        *yield=0;break;
    }
    case 0x080A2094:
        if(word(m,hn_render_slot(m,10)+0x20)<=0x3FFFFF){hn_write8(m,0x030021E0,hn_read8(m,0x030021E0)|4);hn_write16(m,0x03002A80,9);}*yield=1;break;
    default:{HNSceneResult result=hn_bank6_targets_animation(m,callback,slot,arguments,consumed,yield);if(result!=HN_SCENE_UNKNOWN)return result;
        return hn_bank6_chain_animation(m,callback,slot,arguments,consumed,yield);}
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
