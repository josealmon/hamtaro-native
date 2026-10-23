#include "game/scene_selection.h"
#include "game/state.h"
#include "game/random.h"
#include "game/render.h"
#include "game/actor_presets.h"

enum { BANKS6=0x020018F0,BANKS8=0x02001910,FIXED8=0x086E6104,
       POSITIONS6=0x086E609C,INACTIVE6=0x086E60B4,
       POSITIONS8=0x086E610C,INACTIVE8=0x086E612C,
       SPECIAL=0x088E474F,NORMAL=0x088E4466,FIRST8=0x088E4679 };
static uint8_t control(HNMemory *m,unsigned index){return hn_read8(m,HN_SCENE_CONTROLS+index);}
static void store(HNMemory *m,unsigned index,uint8_t value){hn_write8(m,HN_SCENE_CONTROLS+index,value);}
static bool special(uint8_t value){return value==20||value==25||value==27||value==22;}
static void visible(HNMemory *m,uint8_t slot,uint8_t value){hn_write8(m,hn_render_slot(m,slot)+0x60,value);}
static bool slot_at(HNMemory *m,uint8_t slot,GameAddr positions,unsigned index,int y,int z_offset,GameAddr animation)
{
    int x=(int16_t)hn_read16(m,positions+index*4);
    int z=(int16_t)(hn_read16(m,positions+index*4+2)+z_offset);
    return hn_render_configure_raw(m,slot,1,0x72,x,y,z,animation);
}
static bool shuffle(HNMemory *m,bool eight)
{
    unsigned bank=control(m,0x7F),used=0;
    GameAddr source=hn_read32(m,(eight?BANKS8:BANKS6)+bank*4);
    if(eight)store(m,0x4D,hn_read8(m,FIXED8+bank));
    for(unsigned i=eight?1:0;i<(eight?8u:6u);i++){
        unsigned selected,attempts=4096;
        bool lower=i>(eight?4u:2u);
        do{selected=(uint8_t)hn_random_range(m,lower?0:4,lower?4:8);}
        while(((used>>selected)&1)&&--attempts&&!m->fault);
        if(!attempts||m->fault)return false;
        used=(used|(1u<<selected))&65535;
        store(m,(eight?0x4D:0x2E)+i,hn_read8(m,source+selected));
    }
    return !m->fault;
}
static bool configure6(HNMemory *m)
{
    for(unsigned i=0;i<6;i++){
        uint8_t slot=(uint8_t)(18+i);
        switch(control(m,0x3E+i)){
        case 0:
            if(i<=1){
                GameAddr animation=special(control(m,0x2E+i))?SPECIAL:NORMAL;
                if(!slot_at(m,(uint8_t)(slot-2),INACTIVE6,i,0,0,animation))return false;
                visible(m,slot,0);
            }break;
        case 1:{
            if(i<=1)visible(m,(uint8_t)(slot-2),0);
            GameAddr animation=special(control(m,0x2E+i))?SPECIAL:NORMAL;
            if(!slot_at(m,slot,POSITIONS6,i,0,0,animation))return false;
            if(i==1&&(hn_read8(m,0x03002180+(0x1EB>>3))&(1u<<(0x1EB&7)))){
                /* Position setters reload the slot base after each component. */
                hn_write32(m,hn_render_slot(m,slot)+0x18,((uint32_t)(uint16_t)(hn_read16(m,POSITIONS6+4)-48)<<16)+0x8000);
                hn_write32(m,hn_render_slot(m,slot)+0x1C,0x8000);
                hn_write32(m,hn_render_slot(m,slot)+0x20,((uint32_t)hn_read16(m,POSITIONS6+6)<<16)+0x8000);
            }break;
        }
        default:break;
        }
    }
    return !m->fault;
}
static bool configure8(HNMemory *m)
{
    for(unsigned i=0;i<8;i++){
        uint8_t slot=(uint8_t)(23+i);
        switch(control(m,0x45+i)){
        case 0:
            visible(m,slot,0);
            if(i!=5&&!slot_at(m,(uint8_t)(slot-8),INACTIVE8,i,-4,4,i?NORMAL:FIRST8))return false;
            break;
        case 1:
            visible(m,(uint8_t)(slot-8),0);
            if(!slot_at(m,slot,POSITIONS8,i,0,0,i?NORMAL:FIRST8))return false;
            break;
        case 2:visible(m,slot,0);break;
        default:break;
        }
    }
    return !m->fault;
}
HNSceneResult hn_scene_selection_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080BF770:if(!shuffle(m,false))return HN_SCENE_RESOURCE;break;
    case 0x080BF804:if(!configure6(m))return HN_SCENE_RESOURCE;break;
    case 0x080BF99C:store(m,0x60,(uint8_t)special(control(m,0x2E+hn_read8(m,arguments))));*consumed=1;break;
    case 0x080BF9D0:{
        unsigned selected=control(m,hn_read16(m,arguments));
        uint8_t result=control(m,0x2E+selected),side=control(m,0x60),random=(uint8_t)hn_random_range(m,0,100);
        if(random<=(selected>2?4:1))result=side?46:30;
        else if(random<=(selected>2?9:4))result=side?59:57;
        else if(random<=(selected>2?19:9))result=side?56:54;
        hn_write16(m,0x0201DC40+0x22*2,result);*consumed=2;break;
    }
    case 0x080BFA5C:break;
    case 0x080BFA60:if(!shuffle(m,true))return HN_SCENE_RESOURCE;break;
    case 0x080BFB04:if(!configure8(m))return HN_SCENE_RESOURCE;break;
    case 0x080BFC24:{
        unsigned selected=control(m,hn_read16(m,arguments));uint8_t result=control(m,0x4D+selected);
        if(selected){
            uint8_t random=(uint8_t)hn_random_range(m,0,100);
            if(random<=(selected>4?4:1))result=30;
            else if(random<=(selected>4?14:6))result=57;
            else if(random<=(selected>4?19:9))result=54;
        }
        hn_write16(m,0x0201DC40+0x22*2,result);*consumed=2;break;
    }
    case 0x080BFC8C:{
        unsigned key=hn_read8(m,arguments);uint8_t result=0;
        for(unsigned i=0;i<64;i++){
            GameAddr record=0x0201CB20+i*4;
            if((unsigned)(hn_read8(m,record)&127)==key&&((hn_read16(m,record)>>7)&31)){result=1;break;}
        }
        store(m,0x60,result);*consumed=1;break;
    }
    case 0x080BFCE4:{
        uint8_t value=hn_read8(m,0x0201CC20+hn_read8(m,arguments));
        store(m,0x60,value!=0&&value!=255);*consumed=1;break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_scene_selection_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080BFA58:case 0x080BFD14:break;
    case 0x080BFD18:
        if(hn_actor_presets_apply(m,0x0801596C)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));hn_write32(m,0x03001F7C,0x088CC40C);break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
