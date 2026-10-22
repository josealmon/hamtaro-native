#include "game/scene_actor.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/wander.h"

static bool flag(HNMemory *m,unsigned id)
{return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static bool animation(HNMemory *m,uint8_t index,GameAddr resource)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,resource);return !m->fault;
}
/* Resource words name the player's ROM. Preserve ordered setup and live flags. */
static HNSceneResult simple_presets(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x0801F278:{ /* ScriptCallbackConfigureScreenSlot21 */
        if(!animation(m,10, 0x088A48E2))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,21, 1, 176, 0, 0, 0, 0x088E180F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,21, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1330))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A48E2);
        break;
    }
    case 0x0801F2DC:{ /* ScriptCallbackSetScreenPair4944 */
        if(!animation(m,10, 0x088A4944))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088A844A))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A4944);
        break;
    }
    case 0x0801F314:{ /* ScriptCallbackConfigureScreenSlots9And12 */
        if(!animation(m,10, 0x088A4C53))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088A88CE))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,9, 1, 176, 0, 0, 0, 0x088E18D8))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088E1904))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A4C53);
        break;
    }
    case 0x0801F398:{ /* ScriptCallbackConfigureScreenSlots11And14 */
        if(!animation(m,10, 0x088A4FE5))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11, 1, 176, 0, 0, 0, 0x088E19BC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,11, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088A8D07))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14, 1, 176, 0, 0, 0, 0x088E19BC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A4FE5);
        break;
    }
    case 0x0801F418:{ /* ScriptCallbackConfigureScreenSlots11And12 */
        if(!animation(m,10, 0x088A503E))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11, 1, 176, 0, 0, 0, 0x088E19EB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088A8D5D))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088E19EB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A503E);
        break;
    }
    case 0x0801F498:{ /* ScriptCallbackSetScreenPair5079 */
        if(!animation(m,10, 0x088A5079))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088A8D98))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5079);
        break;
    }
    case 0x0801F7D8:{ /* ScriptCallbackScreenPreset1F7D8 */
        if(!hn_graphics_configure(m,4,0x088E137B))return HN_SCENE_RESOURCE;
        if(!animation(m,10, 0x088A5AB7))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,9, 1, 176, 0, 0, 0, 0x088E1D87))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11, 1, 176, 0, 0, 0, 0x088E1DAD))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,11, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(flag(m,16)){
        if(!animation(m,13, 0x088A9BE9))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088E1D87))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14, 1, 176, 0, 0, 0, 0x088E1DAD))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14, 2, 3, 13))return HN_SCENE_RESOURCE;
        }
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5AB7);
        break;
    }
    case 0x0801F8B8:{ /* ScriptCallbackScreenPreset1F8B8 */
        if(!hn_graphics_configure(m,4,0x088E137B))return HN_SCENE_RESOURCE;
        if(!animation(m,10, 0x088A5AFB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,9, 1, 176, 0, 0, 0, 0x088E1D3B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11, 1, 176, 0, 0, 0, 0x088E1D61))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,11, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088A9C27))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088E1D3B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14, 1, 176, 0, 0, 0, 0x088E1D61))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5AFB);
        break;
    }
    case 0x0801F994:{ /* ScriptCallbackScreenPreset1F994 */
        if(!animation(m,10, 0x088A5B33))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088A9CAC))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5B33);
        break;
    }
    case 0x0801F9CC:{ /* ScriptCallbackScreenPreset1F9CC */
        if(!animation(m,10, 0x088A5BC9))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11, 1, 176, 0, 0, 0, 0x088E1DD3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,11, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(flag(m,16)){
        if(!animation(m,13, 0x088A82D4))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14, 1, 176, 0, 0, 0, 0x088E1DD3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14, 2, 3, 13))return HN_SCENE_RESOURCE;
        }
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5BC9);
        break;
    }
    case 0x0801FA54:{ /* ScriptCallbackScreenPreset1FA54 */
        if(!animation(m,10, 0x088A5DAB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,9, 1, 176, 0, 0, 0, 0x088E1E37))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11, 1, 176, 0, 0, 0, 0x088E1EFC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,11, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088A9F78))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088E1E37))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14, 1, 176, 0, 0, 0, 0x088E1EFC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5DAB);
        break;
    }
    case 0x0801FB20:{ /* ScriptCallbackScreenPreset1FB20 */
        if(!animation(m,10, 0x088A5ECC))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11, 1, 176, 0, 0, 0, 0x088E1FAD))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,11, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088AA090))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14, 1, 176, 0, 0, 0, 0x088E1FAD))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5ECC);
        break;
    }
    case 0x0801FBA0:{ /* ScriptCallbackScreenPreset1FBA0 */
        if(!animation(m,10, 0x088A5F6A))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,9, 1, 176, 0, 0, 0, 0x088E1FF0))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088AA128))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088E1FF0))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5F6A);
        break;
    }
    case 0x0801FC20:{ /* ScriptCallbackScreenPreset1FC20 */
        if(!animation(m,10, 0x088A5FF6))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088AA1E8))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A5FF6);
        break;
    }
    case 0x0801FC58:{ /* ScriptCallbackScreenPreset1FC58 */
        if(!animation(m,10, 0x088A6621))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088AB59D))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,9, 1, 176, 0, 0, 0, 0x088E308E))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088E308E))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A6621);
        break;
    }
    case 0x0801FCD8:{ /* ScriptCallbackScreenPreset1FCD8 */
        if(!hn_graphics_configure(m,4,0x088E13F3))return HN_SCENE_RESOURCE;
        if(!animation(m,10, 0x088A6659))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088AB5D2))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14, 1, 176, 0, 0, 0, 0x088E309C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14, 2, 3, 10))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A6659);
        break;
    }
    case 0x0801FD48:{ /* ScriptCallbackScreenPreset1FD48 */
        if(!animation(m,10, 0x088A66E2))return HN_SCENE_RESOURCE;
        if(!animation(m,13, 0x088AB655))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,9, 1, 176, 0, 0, 0, 0x088E30FE))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088E30FE))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088A66E2);
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}

static bool direction_preset(HNMemory *m,GameAddr callback)
{
    static const GameAddr resources[4][4]={
        {0x088AC0C6,0x088AC08F,0x088AC0FD,0x088AC134},
        {0x088B4055,0x088B4002,0x088B408C,0x088B40C3},
        {0x088D70D5,0x088D70A0,0x088D710A,0x088D713F},
        {0x088D74C4,0x088D74AC,0x088D74D9,0x088D74EE}
    };
    unsigned kind=(callback&~1u)==0x0801FE54?0:(callback&~1u)==0x0801FF1C?1:(callback&~1u)==0x0801FFE4?2:3;
    unsigned pair=kind?1:0;
    int16_t x=(int16_t)hn_read16(m,0x0201DC40+(46+pair)*2);
    int16_t y=(int16_t)hn_read16(m,0x0201DC40+(48+pair)*2);
    int16_t z=(int16_t)hn_read16(m,0x0201DC40+(50+pair)*2);
    uint8_t direction=hn_read8(m,HN_SCENE_CONTROLS+0x74+pair);
    if(direction>=1&&direction<=4&&!hn_render_configure(m,(uint8_t)(pair?13:10),direction,(uint8_t)(pair?0x72:0x33),x,y,z,resources[kind][direction-1]))return false;
    if(kind==3&&(!hn_render_configure(m,12,1,0xB0,0,0,0,0x0889F620)||!hn_render_modes(m,12,2,3,13)))return false;
    return !m->fault;
}
static void vector(HNMemory *m,unsigned index,uint32_t x,uint32_t y,uint32_t z)
{
    GameAddr slot=hn_render_slot(m,index);
    hn_write32(m,slot+0x24,x);hn_write32(m,slot+0x28,y);hn_write32(m,slot+0x2C,z);
}
static bool approach(HNMemory *m,GameAddr arguments)
{
    unsigned selector=hn_read8(m,arguments),target_x=hn_read8(m,arguments+1),target_z=hn_read8(m,arguments+2);
    unsigned bank=hn_read8(m,arguments+3),index=hn_read8(m,arguments+4);
    int8_t offset_x=(int8_t)hn_read8(m,arguments+5),offset_z=(int8_t)hn_read8(m,arguments+6);
    unsigned step=hn_read8(m,arguments+7),prefer_z=hn_read8(m,arguments+8);
    GameAddr state=HN_SCENE_CONTROLS+selector,moving=state+1;uint8_t phase=hn_read8(m,state);
    if(!phase){
        if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
        hn_write8(m,moving,0x80);hn_write8(m,state,1);
    }
    if(phase>1)return !m->fault;
    /* The source keeps this actor pointer while its callees reload the array. */
    GameAddr actor=hn_render_slot(m,index);
    int16_t x=(int16_t)((int16_t)hn_read16(m,actor+0x1A)+offset_x);
    int16_t z=(int16_t)((int16_t)hn_read16(m,actor+0x22)+offset_z);
    int16_t dx=(int16_t)((int16_t)hn_read16(m,0x0201DC40+target_x*2)-x);
    int16_t dz=(int16_t)((int16_t)hn_read16(m,0x0201DC40+target_z*2)-z);
    if(dx||dz){
        bool use_z=prefer_z?dz!=0:dx==0;
        int delta=use_z?dz:dx;unsigned direction=use_z?(delta<0?0:1):(delta<0?2:3);
        if(delta==1||delta==-1)step=1;
        if(hn_read8(m,moving)!=direction)
            hn_render_set_animation(m,index,hn_read32(m,HN_ACTOR_APPROACH_MOVING+bank*16+direction*4));
        uint32_t vx=hn_read32(m,HN_ACTOR_APPROACH_DIRECTIONS+direction*8)*step;
        uint32_t vz=hn_read32(m,HN_ACTOR_APPROACH_DIRECTIONS+direction*8+4)*step;
        vector(m,index,vx<<16,0,vz<<16);hn_write8(m,moving,(uint8_t)direction);
    }else{
        if(hn_read8(m,moving)!=0x80){
            hn_render_set_animation(m,index,hn_read32(m,HN_ACTOR_APPROACH_IDLE+bank*16+hn_read8(m,moving)*4));
            unsigned field=(hn_read8(m,actor+0x60)&0xF0)+1;
            hn_write8(m,hn_render_slot(m,index)+0x60,(uint8_t)(hn_read8(m,moving)+field));
        }
        vector(m,index,0,0,0);
        hn_write32(m,actor+0x18,(hn_read32(m,actor+0x18)&0xFFFF0000u)+0x8000);
        hn_write32(m,actor+0x20,(hn_read32(m,actor+0x20)&0xFFFF0000u)+0x8000);
        hn_write8(m,state,2);
    }
    return !m->fault;
}
HNSceneResult hn_scene_actor_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x0801CD4C:{
        GameAddr slot=hn_render_slot(m,hn_read8(m,HN_SCENE_CONTROLS+0x61));
        int32_t current=(int16_t)hn_read16(m,slot+0x1A);uint32_t target=hn_read16(m,0x0201DC88);
        int32_t difference=(int32_t)target-current;unsigned shift=hn_read8(m,HN_SCENE_CONTROLS+0x63);
        if(shift<=4)difference>>=shift;else difference=shift-4<32?(int32_t)((uint32_t)difference<<(shift-4)):0;
        hn_write16(m,0x0201DC84,(uint16_t)(target-(uint32_t)difference));uint32_t ignored;
        if(hn_wander_callback(m,0x0801CCE4,0,&ignored)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;break;
    }
    case 0x0801D120:if(!approach(m,arguments))return HN_SCENE_RESOURCE;*consumed=9;break;
    case 0x0801D73C:hn_write32(m,HN_RENDER_INFO,(uint32_t)hn_read16(m,arguments)<<16);*consumed=2;break;
    case 0x0801D798:{
        GameAddr indices=HN_SCENE_CONTROLS+0x14;
        hn_write8(m,indices,hn_read8(m,arguments));hn_write8(m,indices+1,hn_read8(m,arguments+1));
        for(unsigned i=0;i<2;i++){
            unsigned index=hn_read8(m,indices+i);
            if(index!=255&&!hn_memory_copy(m,HN_ACTOR_RENDER_SNAPSHOT+0x1A0+i*104,hn_render_slot(m,index),104))return HN_SCENE_RESOURCE;
        }
        *consumed=2;break;
    }
    case 0x0801D7E8:{
        GameAddr indices=HN_SCENE_CONTROLS+0x14;
        for(unsigned i=0;i<2;i++)if(hn_read8(m,indices+i)!=255){
            if(!hn_memory_copy(m,hn_render_slot(m,hn_read8(m,indices+i)),HN_ACTOR_RENDER_SNAPSHOT+(i+4)*104,104))return HN_SCENE_RESOURCE;
            hn_write8(m,hn_render_slot(m,hn_read8(m,indices+i))+0x63,0);
        }
        break;
    }
    case 0x0801D9C4:case 0x0801D9F8:case 0x0801DA18:{
        GameAddr slot=hn_render_slot(m,hn_read8(m,arguments));unsigned a=hn_read8(m,arguments+1),b=hn_read8(m,arguments+2);
        uint32_t value=(callback&~1u)==0x0801D9C4?(a<<8)|(b<<16):(a<<16)|(b<<8);
        unsigned offset=(callback&~1u)==0x0801D9C4?0x30:(callback&~1u)==0x0801D9F8?0x34:0x38;
        if(offset==0x30&&(value&0x800000))value|=0xFF000000;
        hn_write32(m,slot+offset,value);*consumed=3;break;
    }
    case 0x0801DAA0:case 0x0801DAD0:{
        GameAddr source=hn_render_slot(m,hn_read8(m,arguments)),destination=hn_render_slot(m,hn_read8(m,arguments+1));
        for(unsigned i=0;i<104;i++){
            uint8_t old=hn_read8(m,destination+i);hn_write8(m,destination+i,hn_read8(m,source+i));
            if((callback&~1u)==0x0801DAD0)hn_write8(m,source+i,old);
        }
        *consumed=2;break;
    }
    case 0x0801ECE8:
        hn_write16(m,0x0201DC84,152);hn_write16(m,0x0201DC86,48);
        hn_write8(m,HN_SCENE_CONTROLS+0x66,16);hn_write8(m,HN_SCENE_CONTROLS+0x67,19);hn_write8(m,HN_SCENE_CONTROLS+0x68,0);break;
    case 0x0801FDC8:{
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS);
        for(unsigned i=0;i<2;i++){
            GameAddr slot=base+(i?13:10)*104;
            hn_write8(m,HN_SCENE_CONTROLS+116+i,hn_read8(m,slot+0x60)&15);
            for(unsigned axis=0;axis<3;axis++)hn_write16(m,0x0201DC40+(46+i+axis*2)*2,hn_read16(m,slot+0x1A+axis*4));
        }
        break;
    }
    case 0x0801FE54:case 0x0801FF1C:case 0x0801FFE4:case 0x080200AC:
        if(!direction_preset(m,callback))return HN_SCENE_RESOURCE;break;
    case 0x08050C84:{
        unsigned mode=hn_read8(m,arguments),index=hn_read16(m,arguments+1),expected=hn_read16(m,arguments+3);
        GameAddr target=hn_read32(m,arguments+5);
        unsigned value=mode?hn_read16(m,0x0201DC40+index*2):hn_read8(m,HN_SCENE_CONTROLS+index);
        if(expected!=value){hn_write32(m,0x03002C60,target);*consumed=0xFFFFFFFCu;}else *consumed=9;break;
    }
    default:return simple_presets(m,callback);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_scene_actor_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x0801CD4C:case 0x0801CDA4:case 0x0801CE28:case 0x0801D7E8:case 0x0801ECE8:
    case 0x0801F278:case 0x0801F2DC:case 0x0801F314:case 0x0801F398:case 0x0801F418:case 0x0801F498:
    case 0x0801F7D8:case 0x0801F8B8:case 0x0801F994:case 0x0801F9CC:case 0x0801FA54:case 0x0801FB20:
    case 0x0801FBA0:case 0x0801FC20:case 0x0801FC58:case 0x0801FCD8:case 0x0801FD48:case 0x0801FDC8:
    case 0x0801FE54:case 0x0801FF1C:case 0x0801FFE4:case 0x080200AC:{
        uint32_t ignored;
        if((callback&~1u)==0x0801CDA4||(callback&~1u)==0x0801CE28)return hn_scene_callback(m,callback,0,&ignored);
        return hn_scene_actor_callback(m,callback,0,&ignored);
    }
    default:return HN_SCENE_UNKNOWN;
    }
}
