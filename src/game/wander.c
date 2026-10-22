#include "game/wander.h"
#include "game/state.h"
#include "game/render.h"
#include "game/random.h"
#include "game/map_choice.h"
#include "game/video.h"
#include "gba/bios.h"

enum { MIN_X=0,MIN_Z=4,MAX_X=8,MAX_Z=12,RATE=16,BUDGET=20,PAUSE=24,
       MOVE_MIN=26,MOVE_MAX=27,PAUSE_MIN=28,PAUSE_MAX=29,INDEX=30,ANIMATIONS=31,STATE=32 };
static bool valid(HNMemory *m,unsigned index)
{return hn_read32(m,HN_RENDER_SLOTS)&&index<hn_read8(m,HN_RENDER_INFO+16);}
static bool animation(HNMemory *m,GameAddr context,GameAddr table,int32_t direction)
{
    unsigned family=hn_read8(m,context+ANIMATIONS);
    GameAddr resource=hn_read32(m,table+family*16+(uint32_t)direction*4);
    unsigned index=hn_read8(m,context+INDEX);
    /* SetRenderAnimation is a raw guest-array setter. A live aliased selector
       may have changed since the captured actor was validated. */
    hn_render_set_animation(m,index,resource);return !m->fault;
}
bool hn_wander_initialize(HNMemory *m,GameAddr arguments)
{
    uint8_t index=hn_read8(m,arguments+14);
    GameAddr context=HN_WANDER_CONTEXTS+(index==16?0:36);
    for(unsigned offset=0;offset<8;offset+=2)
        hn_write32(m,context+offset*2,((uint32_t)hn_read16(m,arguments+offset)<<16)|0x8000);
    hn_write32(m,context+RATE,(uint32_t)hn_read16(m,arguments+8)<<8);
    for(unsigned i=0;i<4;i++)hn_write8(m,context+MOVE_MIN+i,hn_read8(m,arguments+10+i));
    hn_write8(m,context+INDEX,index);hn_write8(m,context+ANIMATIONS,hn_read8(m,arguments+15));
    hn_write8(m,context+STATE,0);hn_write32(m,context+BUDGET,0);return !m->fault;
}
void hn_wander_velocity(HNMemory *m,GameAddr slot,int32_t rate,uint8_t direction)
{
    switch(direction){
    case 0:hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x2C,0u-(uint32_t)rate);break;
    case 1:hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x2C,(uint32_t)rate);break;
    case 2:hn_write32(m,slot+0x24,0u-(uint32_t)rate);hn_write32(m,slot+0x2C,0);break;
    case 3:hn_write32(m,slot+0x24,(uint32_t)rate);hn_write32(m,slot+0x2C,0);break;
    default:break;
    }
}
static int32_t magnitude(uint32_t value)
{return (int32_t)value<0?(int32_t)(0u-value):(int32_t)value;}
bool hn_wander_bounds(HNMemory *m,GameAddr context,GameAddr slot,uint8_t direction,uint32_t *blocked)
{
    int32_t x=(int32_t)(hn_read32(m,slot+0x18)+hn_read32(m,slot+0x24));
    int32_t z=(int32_t)(hn_read32(m,slot+0x20)+hn_read32(m,slot+0x2C));
    *blocked=1;
    if(((int32_t)hn_read32(m,context+MIN_X)>x&&(int32_t)hn_read32(m,slot+0x24)<0)||
       ((int32_t)hn_read32(m,context+MAX_X)<x&&(int32_t)hn_read32(m,slot+0x24)>0)){
        hn_write32(m,slot+0x24,0);return !m->fault;
    }
    if(((int32_t)hn_read32(m,context+MIN_Z)>z&&(int32_t)hn_read32(m,slot+0x2C)<0)||
       ((int32_t)hn_read32(m,context+MAX_Z)<z&&(int32_t)hn_read32(m,slot+0x2C)>0)){
        hn_write32(m,slot+0x2C,0);return !m->fault;
    }
    unsigned selector=hn_read8(m,HN_SCENE_CONTROLS+0xAD);
    if(selector){
        GameAddr callback=hn_read32(m,0x03002BB0+(selector-1)*4);uint32_t result;
        /* Reviewed u32(slot, direction) contract. This predicate ignores R1;
           void height helpers do not acquire a fabricated boolean return. */
        if((callback&~1u)!=0x08032C80||hn_map_choice_slot(m,callback,slot,&result)!=HN_SCENE_OK)return false;
        if(result)goto stop;
    }
    if(direction!=255){
        unsigned leader=hn_read8(m,HN_RENDER_CLASS_LEADER);if(!valid(m,leader))return false;
        GameAddr other=hn_render_slot(m,leader);
        int32_t dx=magnitude(hn_read32(m,slot+0x18)-hn_read32(m,other+0x18));
        int32_t dz=magnitude(hn_read32(m,slot+0x20)-hn_read32(m,other+0x20));
        if(dx<=0x27FFFF&&dz<=0x27FFFF){
            int towards=(int)(uint8_t)hn_render_direction_between(m,hn_read8(m,context+INDEX),(uint8_t)leader,false)-1;
            if((int)direction==towards)goto stop;
        }
    }
    *blocked=0;return !m->fault;
stop:
    hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x2C,0);return !m->fault;
}
bool hn_wander_update(HNMemory *m,GameAddr callback_slot,GameAddr arguments,uint32_t *yield)
{
    uint8_t index=hn_read8(m,arguments),tried[4]={0};unsigned record;
    GameAddr context=HN_WANDER_CONTEXTS;
    for(record=0;record<2;record++,context+=36)if(hn_read8(m,context+INDEX)==index)break;
    if(record==2)return !m->fault;
    index=hn_read8(m,context+INDEX);if(!valid(m,index))return false;
    GameAddr slot=hn_render_slot(m,index);unsigned flag=hn_read16(m,HN_WANDER_STOP_FLAGS+record*2);
    GameAddr flags=0x03002180+(flag>>3);unsigned mask=1u<<(flag&7);
    if(hn_read8(m,flags)&mask){
        hn_write8(m,flags,(uint8_t)(hn_read8(m,flags)&~mask));
        hn_write8(m,context+STATE,0);hn_write32(m,context+BUDGET,0);
        hn_write8(m,callback_slot+0x60,0);hn_write32(m,slot+0x2C,0);hn_write32(m,slot+0x24,0);
        if(!animation(m,context,HN_WANDER_IDLE,(int)hn_read8(m,slot+0x60)-1))return false;
        *yield=1;return !m->fault;
    }
    /* Valid source states transition at most twice per update. Invalid states
       spin forever on GBA; fail explicitly instead of hanging the host. */
    for(unsigned transitions=0;transitions<64;transitions++){
        switch(hn_read8(m,context+STATE)){
        case 0:{
            uint8_t direction=0;uint32_t next=0,blocked;
            for(unsigned trial=0;trial<4;trial++){
                direction=(uint8_t)hn_random_range(m,0,4);
                while(tried[direction])direction=(uint8_t)((direction+1)&3);
                tried[direction]=255;
                hn_wander_velocity(m,slot,(int32_t)hn_read32(m,context+RATE),direction);
                next=(uint32_t)direction+1;hn_write8(m,slot+0x60,(uint8_t)next);
                if(!hn_wander_bounds(m,context,slot,direction,&blocked))return false;
                if(!blocked)break;
            }
            if(!animation(m,context,HN_WANDER_MOVING,direction))return false;
            hn_write8(m,slot+0x60,(uint8_t)next);
            if(!hn_read32(m,context+BUDGET)){
                uint32_t budget=(uint32_t)hn_random_range(m,hn_read8(m,context+MOVE_MIN),
                                                         (uint16_t)(hn_read8(m,context+MOVE_MAX)+1))<<19;
                hn_write32(m,context+BUDGET,budget);
            }
            hn_write8(m,context+STATE,1);*yield=1;return !m->fault;
        }
        case 1:{
            hn_write32(m,context+BUDGET,hn_read32(m,context+BUDGET)-hn_read32(m,context+RATE));
            if((int32_t)hn_read32(m,context+BUDGET)<0){
                hn_write32(m,context+BUDGET,0);
                if(!hn_read8(m,context+PAUSE_MAX)){hn_write8(m,context+STATE,0);break;}
                hn_write32(m,slot+0x2C,0);hn_write32(m,slot+0x24,0);
                uint16_t frames=hn_random_range(m,hn_read8(m,context+PAUSE_MIN),
                                                (uint16_t)(hn_read8(m,context+PAUSE_MAX)+1));
                hn_write16(m,context+PAUSE,frames);
                if(!animation(m,context,HN_WANDER_IDLE,(int)hn_read8(m,slot+0x60)-1))return false;
                hn_write8(m,context+STATE,2);*yield=1;return !m->fault;
            }
            uint8_t direction=(hn_read8(m,0x03001FA8)&8)?(uint8_t)((hn_read8(m,slot+0x60)&15)-1):255;
            uint32_t blocked;if(!hn_wander_bounds(m,context,slot,direction,&blocked))return false;
            if(!blocked){*yield=1;return !m->fault;}
            if((int32_t)hn_read32(m,context+BUDGET)<=15)hn_write32(m,context+BUDGET,16);
            hn_write8(m,context+STATE,0);break;
        }
        case 2:{
            uint16_t frames=(uint16_t)(hn_read16(m,context+PAUSE)-1);hn_write16(m,context+PAUSE,frames);
            if(!frames){hn_write8(m,context+STATE,0);break;}
            *yield=1;return !m->fault;
        }
        default:return false;
        }
    }
    return false;
}
bool hn_wander_reset(HNMemory *m,GameAddr arguments)
{
    unsigned index=hn_read8(m,arguments);if(!valid(m,index))return false;
    GameAddr slot=hn_render_slot(m,index);uint8_t direction=(uint8_t)((hn_read8(m,slot+0x60)&15)-1);
    if(direction>3)direction=0;hn_write8(m,slot+0x60,(uint8_t)(direction+1));
    direction=hn_read8(m,HN_TURN_DIRECTIONS+direction);
    index=hn_read8(m,arguments);
    GameAddr resource=hn_read32(m,HN_WANDER_TURNS+hn_read8(m,arguments+1)*32+direction*4);
    hn_render_set_animation(m,index,resource);
    for(unsigned offset=0x24;offset<=0x38;offset+=4)hn_write32(m,slot+offset,0);
    return !m->fault;
}
bool hn_wander_turn_initialize(HNMemory *m,GameAddr arguments)
{
    GameAddr context=HN_ACTOR_TURN_CONTROL;hn_write8(m,context,0);
    for(unsigned i=0;i<3;i++)hn_write8(m,context+1+i,hn_read8(m,arguments+i));
    hn_write8(m,context+4,0);return !m->fault;
}
bool hn_wander_turn_update(HNMemory *m)
{
    GameAddr context=HN_ACTOR_TURN_CONTROL;unsigned index=hn_read8(m,context+1);
    if(!valid(m,index))return false;GameAddr slot=hn_render_slot(m,index);
    uint8_t direction=(uint8_t)hn_render_direction_between(m,(uint8_t)index,hn_read8(m,context+2),false);
    switch(hn_read8(m,context)){
    case 0:{
        uint8_t from=hn_read8(m,slot+0x60);
        if(from!=direction){hn_write8(m,context+4,4);hn_write8(m,context,1);}
        else hn_write8(m,context,2);
        uint32_t offset=hn_read8(m,slot+0x60)*4u-5u;
        hn_write8(m,slot+0x60,hn_read8(m,HN_TURN_NEXT+direction+offset));
        unsigned mapped=hn_read8(m,HN_TURN_DIRECTIONS+(uint32_t)((int)hn_read8(m,slot+0x60)-1));
        GameAddr resource=hn_read32(m,HN_WANDER_TURNS+hn_read8(m,context+3)*32+mapped*4);
        index=hn_read8(m,context+1);
        hn_render_set_animation(m,index,resource);break;
    }
    case 1:{
        uint8_t frames=(uint8_t)(hn_read8(m,context+4)-1);hn_write8(m,context+4,frames);
        if(!frames)hn_write8(m,context,0);break;
    }
    case 2:if(hn_read8(m,slot+0x60)!=direction)hn_write8(m,context,0);break;
    default:break;
    }
    return !m->fault;
}
HNSceneResult hn_wander_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,
                                  uint32_t *consumed,uint32_t *yield)
{
    bool ok;
    switch(callback&~1u){
    case 0x0801E45C:ok=hn_wander_initialize(m,arguments);if(ok){*consumed=16;*yield=0;}break;
    case 0x0801E4E4:ok=hn_wander_update(m,slot,arguments,yield);if(ok)*consumed=1;break;
    case 0x0801E718:ok=hn_wander_turn_initialize(m,arguments);if(ok){*consumed=3;*yield=0;}break;
    case 0x0801E740:ok=hn_wander_turn_update(m);if(ok){*consumed=0;*yield=1;}break;
    default:return HN_SCENE_UNKNOWN;
    }
    return ok?HN_SCENE_OK:HN_SCENE_RESOURCE;
}
HNSceneResult hn_wander_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    switch(callback&~1u){
    case 0x0801DA38:
        if(!hn_wander_reset(m,arguments))return HN_SCENE_RESOURCE;*consumed=2;break;
    case 0x0801C51C:{
        hn_write32(m,0x03001134,1);unsigned index=hn_read8(m,arguments);
        if(!valid(m,index))return HN_SCENE_RESOURCE;GameAddr slot=hn_render_slot(m,index);
        hn_write16(m,0x0201DC40,hn_read16(m,slot+0x1A));hn_write16(m,0x0201DC42,hn_read16(m,slot+0x22));*consumed=1;break;
    }
    case 0x0801C54C:{
        hn_write32(m,0x03001134,3);unsigned index=hn_read8(m,arguments),x=hn_read8(m,arguments+1),z=hn_read8(m,arguments+2);
        if(!valid(m,index))return HN_SCENE_RESOURCE;GameAddr slot=hn_render_slot(m,index);
        hn_write16(m,0x0201DC84,(uint16_t)(hn_read16(m,slot+0x1A)+x));
        hn_write16(m,0x0201DC86,(uint16_t)(hn_read16(m,slot+0x22)+z));*consumed=3;break;
    }
    case 0x0801C594:{
        unsigned index=hn_read8(m,arguments);if(!valid(m,index))return HN_SCENE_RESOURCE;
        hn_write16(m,0x0201DC40,hn_read16(m,hn_render_slot(m,index)+0x1E));*consumed=1;break;
    }
    case 0x0801C5B4:case 0x0801C5F4:case 0x0801C634:case 0x0801C650:{
        unsigned first,second;GameAddr target=callback&~1u;
        if(target==0x0801C634||target==0x0801C650){
            hn_write8(m,HN_SCENE_CONTROLS,hn_read8(m,arguments));
            hn_write8(m,HN_SCENE_CONTROLS+1,hn_read8(m,HN_SCENE_CONTROLS+(target==0x0801C634?0x12:0x17)));
        }
        hn_write32(m,0x03001134,target==0x0801C5B4?2:0);
        first=hn_read8(m,target==0x0801C5B4?arguments:HN_SCENE_CONTROLS);
        second=hn_read8(m,target==0x0801C5B4?arguments+1:HN_SCENE_CONTROLS+1);
        hn_write8(m,HN_SCENE_CONTROLS,(uint8_t)hn_render_direction_between(m,(uint8_t)first,(uint8_t)second,false));
        hn_write8(m,HN_SCENE_CONTROLS+1,(uint8_t)hn_render_direction_between(m,(uint8_t)second,(uint8_t)first,false));
        *consumed=target==0x0801C5B4||target==0x0801C5F4?hn_read32(m,0x03001134):1;break;
    }
    case 0x0801C66C:{
        unsigned target=hn_read8(m,HN_SCENE_CONTROLS+0x12);uint32_t ignored;
        hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)hn_render_direction_between(m,10,(uint8_t)target,true));
        hn_write8(m,HN_SCENE_CONTROLS+0x62,1);
        if(hn_scene_callback(m,0x0801C9B4,0x02000868,&ignored)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS+0x64,(uint8_t)hn_render_direction_between(m,13,(uint8_t)target,true));
        hn_write8(m,HN_SCENE_CONTROLS+0x66,1);
        if(hn_scene_callback(m,0x0801C9B4,0x0200086B,&ignored)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        *consumed=0;break;
    }
    case 0x0801C6BC:{
        uint8_t target=hn_read8(m,arguments);
        for(unsigned i=0;i<2;i++){
            uint32_t direction=hn_render_direction_between(m,(uint8_t)(i?13:10),target,true)-1u;
            GameAddr resource=hn_read32(m,HN_WANDER_TURNS+i*32+direction*4);
            unsigned index=hn_read8(m,i?HN_RENDER_CLASS_FOLLOWER:HN_RENDER_CLASS_LEADER);
            if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
            hn_render_set_animation(m,index,resource);
        }
        *consumed=1;break;
    }
    case 0x0801CA3C:{
        GameAddr control=HN_SCENE_CONTROLS+hn_read8(m,arguments);
        uint32_t direction=(uint32_t)hn_read8(m,control)-1;hn_write8(m,control,(uint8_t)direction);
        unsigned difference=(direction-hn_read8(m,control+1))&7;
        if(difference<=3)hn_write8(m,control+2,1);else if(difference>4)hn_write8(m,control+2,2);
        hn_write8(m,control+3,0);*consumed=1;break;
    }
    case 0x0801CA74:{
        GameAddr control=HN_SCENE_CONTROLS+hn_read8(m,arguments);unsigned actor=hn_read8(m,arguments+1);
        if(actor==0x81&&!hn_read8(m,HN_RENDER_CLASS_FOLLOWER)){hn_write8(m,control+2,0);*consumed=3;break;}
        if(actor==0x80)actor=hn_read8(m,HN_RENDER_CLASS_LEADER);
        else if(actor==0x81)actor=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
        if(!hn_read8(m,control+3)){
            if(hn_read8(m,control)!=hn_read8(m,control+1)){
                int step=hn_read8(m,control+2)==1?1:-1;
                hn_write8(m,control+1,(uint8_t)(hn_read8(m,control+1)+step));
                hn_write8(m,control+1,hn_read8(m,control+1)&7);
                GameAddr resource=hn_read32(m,HN_WANDER_TURNS+hn_read8(m,arguments+2)*32+hn_read8(m,control+1)*4);
                hn_render_set_animation(m,actor,resource);
            }else hn_write8(m,control+2,0);
            hn_write8(m,control+3,4);
            /* The source calls this setter again. The first call may change
               flags, control aliases or arguments; reload every live operand. */
            GameAddr resource=hn_read32(m,HN_WANDER_TURNS+hn_read8(m,arguments+2)*32+hn_read8(m,control+1)*4);
            hn_render_set_animation(m,actor,resource);
        }else hn_write8(m,control+3,(uint8_t)(hn_read8(m,control+3)-1));
        *consumed=3;break;
    }
    case 0x0801CB24:{
        unsigned index=hn_read8(m,arguments),value=hn_read8(m,arguments+1);
        GameAddr flags=hn_render_slot(m,index)+0x61;
        hn_write8(m,flags,hn_read8(m,flags)&127);flags=hn_render_slot(m,index)+0x61;
        hn_write8(m,flags,(uint8_t)(hn_read8(m,flags)|(value<<7)));*consumed=2;break;
    }
    case 0x0801CB38:hn_video_mark_backgrounds(m,hn_read8(m,arguments));*consumed=1;break;
    case 0x0801CB48:{
        int32_t source=(int8_t)hn_read8(m,arguments),destination=(int8_t)hn_read8(m,arguments+1);
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS),from=base+(uint32_t)source*104,to=base+(uint32_t)destination*104;
        for(unsigned offset=0x24;offset<=0x2C;offset+=4)hn_write32(m,to+offset,hn_read32(m,from+offset));
        *consumed=2;break;
    }
    case 0x0801CB7C:{
        GameAddr control=HN_SCENE_CONTROLS+0x60;uint32_t ignored;
        switch(hn_read8(m,control+8)){
        case 0:
            if(hn_scene_callback(m,0x0801C9B4,0x0200086E,&ignored)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
            hn_write8(m,control+9,(uint8_t)(hn_read8(m,control+9)-1));
            hn_write8(m,control+8,(uint8_t)(hn_read8(m,control+8)+1));break;
        case 1:
            if(hn_wander_callback(m,0x0801CA74,0x0200086E,&ignored)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
            hn_write8(m,control+9,(uint8_t)(hn_read8(m,control+9)-1));
            if(!hn_read8(m,control+6)){
                hn_write8(m,control+8,(uint8_t)(hn_read8(m,control+8)+1));
                hn_write8(m,control+5,hn_read8(m,control+4));hn_write8(m,control+4,5);
                if(hn_wander_callback(m,0x0801CA3C,0x0200086E,&ignored)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
                int difference=(int)hn_read8(m,control+4)-(int)hn_read8(m,control+5);if(difference<0)difference+=4;
                uint8_t delay=(uint8_t)((difference-1)*4+1);
                if(hn_read8(m,control+9)>delay)hn_write8(m,control+9,(uint8_t)(hn_read8(m,control+9)-delay));
                else hn_write8(m,control+8,(uint8_t)(hn_read8(m,control+8)+1));
            }break;
        case 2:
            hn_write8(m,control+9,(uint8_t)(hn_read8(m,control+9)-1));
            if(!hn_read8(m,control+9))hn_write8(m,control+8,(uint8_t)(hn_read8(m,control+8)+1));break;
        case 3:
            if(hn_wander_callback(m,0x0801CA74,0x0200086E,&ignored)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
            if(!hn_read8(m,control+6)){
                hn_write8(m,control+8,(uint8_t)(hn_read8(m,control+8)+1));
                GameAddr slot=hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER));
                hn_write8(m,slot+0x60,hn_read8(m,slot+0x60)&0xF0);
                slot=hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER));hn_write8(m,slot+0x60,hn_read8(m,slot+0x60)|2);
            }break;
        default:break;
        }
        *consumed=0;break;
    }
    case 0x0801CCE4:{
        unsigned index=hn_read8(m,HN_SCENE_CONTROLS+0x61);if(!valid(m,index))return HN_SCENE_RESOURCE;
        GameAddr slot=hn_render_slot(m,index);uint32_t x=(uint32_t)hn_read16(m,0x0201DC84)<<16,z=(uint32_t)hn_read16(m,0x0201DC86)<<16;
        int32_t frames=hn_read8(m,HN_SCENE_CONTROLS+0x62)+1;
        int32_t vx=hn_runtime_divide((int32_t)(x-hn_read32(m,slot+0x18)),frames);
        int32_t vz=hn_runtime_divide((int32_t)(z-(hn_read32(m,slot+0x1C)+hn_read32(m,slot+0x20))),frames);
        hn_write32(m,slot+0x24,(uint32_t)vx);hn_write32(m,slot+0x2C,(uint32_t)vz);*consumed=0;break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
