#include "game/scene.h"
#include "game/state.h"
#include "game/input.h"
#include "game/background.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/random.h"
#include "game/text.h"
#include "game/choice.h"
#include "game/world.h"
#include "game/book.h"
#include "game/motion.h"
#include "game/scene_motion.h"

static unsigned menu_kind(HNMemory *m)
{
    unsigned enabled=hn_read8(m,HN_SCENE_CONTROLS+0x1B);
    return enabled==3?1:enabled==5?2:enabled==1?3:0;
}
static bool menu_update(HNMemory *m,uint8_t selection)
{
    unsigned kind=menu_kind(m),language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);
    if(selection>=6||language>=5)return false;
    unsigned enabled=hn_read8(m,HN_SCENE_CONTROLS+0x1B),chosen=hn_read8(m,HN_SCENE_MENU_CODES+kind*6+selection);
    GameAddr table=hn_read32(m,HN_SCENE_MENU_BANKS+(language*4+kind)*4);
    for(unsigned option=1;option<=5;option++)if(option==5||((enabled>>(option-1))&1)){
        unsigned highlighted=option==chosen?5:0;
        if(!hn_background_configure(m,(uint8_t)(option-1),0x82,hn_read32(m,table+(option+highlighted)*4)))return false;
    }
    if(selection&&!hn_render_configure(m,27,1,0x70,56,56+(int)selection*16,0,
        hn_read32(m,HN_SCENE_MENU_CURSORS+selection*4)))return false;
    return !m->fault;
}
static bool flag(HNMemory *m,unsigned index)
{return ((hn_read8(m,0x03002180+(index>>3))>>(index&7))&1)!=0;}
bool hn_scene_indicator_complete(HNMemory *m,uint32_t index_word)
{
    /* Flag identifiers describe progress; original resources stay in the ROM. */
    static const uint8_t required[7][22]={
        {0x6A,0x9B,0x91,0x9A,0x99},
        {0x4B,0x8B,0x74,0x8A,0x82,0x54,0x89,0x73,0x4C,0x84,0x52,0x75,0x93,0x8F,0x81,0x7E,0x87,0x83,0x88,0x76,0x7F},
        {0x55,0x6B,0x61,0x62,0x60,0x59,0x51,0x5D,0x58,0x5F,0x5A,0x69,0x5E,0x7C},
        {0x98,0x8E,0x78,0x72,0x95,0x97,0x96,0x92,0x94,0x77,0x57,0x5B,0x5C,0x90},
        {0x56,0x7A,0x65,0x7B,0x64,0x7D,0x68,0x67,0x8D,0x66},
        {0x8C,0x71,0x70,0x63,0x79,0x6F,0x80,0x6C,0x6E,0x6D},{0x9C,0x86}};
    unsigned index=(uint8_t)index_word;if(!index||index>7)return false;
    for(unsigned i=0;i<22&&required[index-1][i];i++)if(!flag(m,required[index-1][i]))return false;
    return !m->fault;
}
bool hn_scene_indicator_visible(HNMemory *m,uint32_t index_word)
{
    unsigned mode=hn_read8(m,HN_SCENE_CONTROLS+0x16);if(!mode)return true;
    uint32_t shift=(uint32_t)(uint8_t)index_word-1;
    unsigned mask=hn_read8(m,HN_SCENE_INDICATOR_MASKS+mode-1);
    return shift<32&&((mask>>shift)&1)!=0;
}
bool hn_scene_indicator_configure(HNMemory *m,uint32_t index_word,uint32_t state_word)
{
    unsigned index=(uint8_t)index_word,state=(uint8_t)state_word;
    if(state>5)return true;
    unsigned slot=state==1||state==2?15:state==5?26:(uint8_t)(index+17);
    if(slot>=hn_read8(m,HN_RENDER_INFO+16)||!hn_read32(m,HN_RENDER_SLOTS))return false;
    if(!state){hn_write8(m,hn_render_slot(m,slot)+0x60,0);return !m->fault;}
    static const GameAddr animations[]={0,0x088F6FAC,0x088F6FB4,0x088F6FD6,0x088F6FBC,0x088F6FE8};
    GameAddr position=HN_SCENE_INDICATOR_POSITIONS+index*4;
    if(!hn_render_configure(m,(uint8_t)slot,1,0x74,(int16_t)hn_read16(m,position),(int16_t)hn_read16(m,position+2),0,animations[state]))return false;
    if(state==1||state==2||state==5)hn_render_flags(m,slot,12,4);
    return !m->fault;
}
bool hn_scene_indicator_update(HNMemory *m,uint32_t index_word)
{
    unsigned index=(uint8_t)index_word;
    if(flag(m,(uint8_t)(index+63))&&hn_scene_indicator_visible(m,index)){
        if(index==hn_read8(m,HN_SCENE_CONTROLS+0x6C)){
            unsigned language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);
            GameAddr background=hn_read32(m,HN_SCENE_INDICATOR_BACKGROUNDS+(language*8+index)*4);
            if(!hn_background_configure(m,4,0x81,background)||
               !hn_scene_indicator_configure(m,index,hn_read8(m,HN_SCENE_CONTROLS+0x13)?2:1))return false;
        }
        if(!hn_scene_indicator_configure(m,index,hn_scene_indicator_complete(m,index)?3:4))return false;
        if(index==hn_read8(m,HN_SCENE_CONTROLS+0xE5)&&!hn_scene_indicator_configure(m,index,5))return false;
    }
    return !m->fault;
}
static bool indicator_move(HNMemory *m)
{
    unsigned input=hn_input_query(m,0xF0,1);if(!input)return !m->fault;
    unsigned direction=0;while(direction<4&&!((input>>(direction+4))&1))direction++;
    unsigned previous=hn_read8(m,HN_SCENE_CONTROLS+0x6C),count=0;
    for(unsigned i=0;i<7;i++)if(flag(m,i+0x40))count++;
    unsigned mode=hn_read8(m,HN_SCENE_CONTROLS+0x16);
    GameAddr bank=mode?HN_SCENE_INDICATOR_MODE_NAVIGATION+(mode-1)*4:HN_SCENE_INDICATOR_NAVIGATION+(count-2)*4;
    unsigned selected=hn_read8(m,hn_read32(m,bank)+previous*4+direction);
    if(selected&&flag(m,(uint8_t)(selected+63))){
        hn_write8(m,HN_SCENE_INDICATOR_PREVIOUS,(uint8_t)previous);
        hn_write8(m,HN_SCENE_CONTROLS+0x6C,(uint8_t)selected);hn_write8(m,HN_SCENE_INDICATOR_CHANGED,1);
        if(!hn_audio_song(m,0x12E,HN_SONG_START))return false;
    }
    return !m->fault;
}
static void arithmetic_flag(HNMemory *m,uint8_t mask,bool enabled)
{
    uint8_t flags=hn_read8(m,0x03002182);
    hn_write8(m,0x03002182,(uint8_t)(enabled?flags|mask:flags&~mask));
}
static bool indirect_arithmetic(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    bool half=callback==0x0801C3B4||callback==0x0801C410,subtract=callback==0x0801C344||callback==0x0801C410;
    unsigned offset=half?2:3,bytes=8-offset;GameAddr step=0x03001138+offset;
    for(unsigned i=0;i<bytes;i++)hn_write8(m,step+i,hn_read8(m,arguments+i));
    GameAddr target=hn_read32(m,0x0300113C);
    if(half){
        if(subtract){
            arithmetic_flag(m,4,hn_read16(m,target)<hn_read16(m,step));
            hn_write16(m,target,(uint16_t)(hn_read16(m,target)-hn_read16(m,step)));
        }else{
            uint16_t result=(uint16_t)(hn_read16(m,target)+hn_read16(m,step));hn_write16(m,target,result);
            arithmetic_flag(m,4,result<hn_read16(m,step));
        }
        arithmetic_flag(m,2,hn_read16(m,target)==0);
    }else{
        uint8_t value=hn_read8(m,target),amount=hn_read8(m,step);
        hn_write8(m,target,(uint8_t)(subtract?value-amount:value+amount));
        /* Byte callbacks reload their global pointer after each write. */
        arithmetic_flag(m,2,hn_read8(m,hn_read32(m,0x0300113C))==0);
        value=hn_read8(m,hn_read32(m,0x0300113C));amount=hn_read8(m,step);
        arithmetic_flag(m,4,subtract?value>amount:value<amount);hn_write32(m,0x03001134,5);
    }
    *consumed=bytes;return !m->fault;
}
static bool panel_layout(HNMemory *m,unsigned variant,unsigned *row,unsigned *count)
{
    unsigned language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);if(variant>=4||language>=5)return false;
    *row=variant*23+language*92;int n=(int8_t)hn_read8(m,HN_PANEL_SHIFTS+*row);
    *count=n>0?(unsigned)n:0;return *count<=hn_read8(m,HN_RENDER_INFO+0x10)&&!m->fault;
}
static void panel_velocity(HNMemory *m,unsigned i,int32_t x,int32_t y,int32_t z,unsigned offset)
{
    GameAddr slot=hn_render_slot(m,i);hn_write32(m,slot+offset,(uint32_t)x);
    hn_write32(m,slot+offset+4,(uint32_t)y);hn_write32(m,slot+offset+8,(uint32_t)z);
}
static void panel_disable(HNMemory *m,unsigned i)
{
    if(i==hn_read8(m,0x03001F50))hn_write8(m,0x03002170,0);
    hn_write8(m,hn_render_slot(m,i)+0x60,0);hn_render_set_animation(m,i,0);
}
static bool panel_command(HNMemory *m,GameAddr callback,uint8_t variant)
{
    unsigned row,count;if(!panel_layout(m,variant,&row,&count))return false;
    unsigned language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);bool any=false;
    for(unsigned i=0;i<count;i++){
        int shift=(int8_t)hn_read8(m,HN_PANEL_SHIFTS+row+1+i);unsigned magnitude=(unsigned)(shift<0?-shift:shift);
        GameAddr countdown=HN_SCENE_CONTROLS+0x1A6+i;
        if(callback==0x080C0E3C){
            int x=shift*8+hn_read8(m,HN_PANEL_PIXELS+row+1+i);
            unsigned code=hn_read8(m,HN_PANEL_CODES+i+variant*22+language*88);if(code>=52)return false;
            if(!hn_render_configure(m,(uint8_t)i,1,0x7C,x,0,4,hn_read32(m,HN_PANEL_ANIMATIONS+code*4)))return false;
            panel_velocity(m,i,shift>0?-0x80000:0x80000,0,0,0x24);hn_write8(m,countdown,(uint8_t)magnitude);
        }else if(callback==0x080C0FD0){
            hn_write8(m,countdown,(uint8_t)(hn_read8(m,HN_PANEL_SHIFTS+row+1)-magnitude));
        }else if(hn_read8(m,countdown)){
            hn_write8(m,countdown,(uint8_t)(hn_read8(m,countdown)-1));any=true;
        }else if(callback==0x080C10B0){panel_velocity(m,i,0,0,0,0x24);}
        else if(!hn_read32(m,hn_render_slot(m,i)+0x24)){
            panel_velocity(m,i,shift>0?0x80000:-0x80000,0,0,0x24);
            /* The original zero test deliberately uses the language-zero row. */
            if((int8_t)hn_read8(m,HN_PANEL_SHIFTS+variant*23+1+i)==0){
                hn_write8(m,countdown,0);panel_disable(m,i);
            }else hn_write8(m,countdown,(uint8_t)(magnitude-1));
        }else panel_disable(m,i);
    }
    if((callback==0x080C10B0||callback==0x080C1150)&&!any)hn_write8(m,HN_SCENE_CONTROLS+0x1A6,255);
    return !m->fault;
}

static bool record_menu(HNMemory *m)
{
    unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x1E);
    if(selected>=5||hn_read8(m,HN_BACKGROUND_COUNT)<7)return false;
    for(unsigned i=0;i<5;i++)if(!hn_background_configure(m,(uint8_t)i,0x82,
        hn_read32(m,HN_SCENE_RECORD_MENU+(i+(i==selected?5:0))*4)))return false;
    if(!hn_background_configure(m,5,0x82,hn_read32(m,HN_SCENE_RECORD_LABELS+selected*4)))return false;
    unsigned mode=hn_read8(m,HN_SCENE_CONTROLS+0xEC)?0:5;
    if(!hn_background_configure(m,6,0x82,hn_read32(m,HN_SCENE_RECORD_MODES+(selected+mode)*4)))return false;
    if(!hn_render_configure(m,27,1,0x70,88,56+(int)selected*16,0,HN_SCENE_RECORD_CURSOR))return false;
    hn_render_flags(m,27,12,0);return !m->fault;
}
static const GameAddr actor_classes[]={HN_RENDER_CLASS_LEADER,0x03002274,HN_RENDER_CLASS_FOLLOWER,0x03002270};
static bool save_actor_slots(HNMemory *m)
{
    for(unsigned i=0;i<4;i++){
        unsigned index=hn_read8(m,actor_classes[i]);if(index>=hn_read8(m,HN_RENDER_INFO+16))return false;
        if(!hn_memory_copy(m,HN_ACTOR_RENDER_SNAPSHOT+i*104,hn_render_slot(m,index),104))return false;
    }
    return true;
}
static bool restore_actor_slots(HNMemory *m,bool idle)
{
    unsigned variant=0;
    for(unsigned i=0;i<4;i++){
        GameAddr saved=HN_ACTOR_RENDER_SNAPSHOT+i*104;unsigned index=hn_read8(m,actor_classes[i]);
        if(i&&(!hn_read8(m,saved+96)||(idle&&!index)))continue;
        if(index>=hn_read8(m,HN_RENDER_INFO+16))return false;
        GameAddr animation=0;
        if(idle){
            unsigned primary=hn_read8(m,0x03001F60),secondary=hn_read8(m,0x03001F90),family;
            if(i==0||i==2)variant=(uint8_t)((hn_read8(m,saved+96)&15)-1);
            if(variant>=4)return false;
            if(!(i&1)){
                family=(i==2?(primary^1):primary)*2+1;
                animation=hn_read32(m,hn_read32(m,HN_PRIMARY_ANIMATION_FAMILIES+family*4)+variant*4);
            }else{
                family=secondary*2+(i==3);
                animation=hn_read32(m,hn_read32(m,HN_SECONDARY_ANIMATION_FAMILIES+family*4)+variant*8);
            }
        }else hn_write8(m,saved+99,0);
        if(!hn_memory_copy(m,hn_render_slot(m,index),saved,104))return false;
        if(idle)hn_render_set_animation(m,index,animation);
    }
    return !m->fault;
}
static bool class_screen_side(HNMemory *m,bool store_result)
{
    unsigned index=hn_read8(m,HN_SCENE_CONTROLS+0x12),count=hn_read8(m,HN_RENDER_INFO+16);
    if(store_result){
        if(index>=count)return false;
        if(index<=7&&!class_screen_side(m,false))return false;
        int difference=(int)hn_read16(m,hn_render_slot(m,index)+0x50)-0x47;
        if(!difference){
            if(!class_screen_side(m,false))return false;
            hn_write16(m,0x03001FBC,hn_read8(m,HN_SCENE_CONTROLS+0x60));
        }else hn_write16(m,0x03001FBC,(uint16_t)(difference>0?1:2));
    }else{
        unsigned primary=hn_read8(m,HN_RENDER_CLASS_LEADER),secondary=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
        if(primary>=count)return false;
        int difference=(int)hn_read16(m,hn_render_slot(m,primary)+0x50)-0x47;
        if(index<=7){
            if(secondary){if(secondary>=count)return false;difference+=(int)hn_read16(m,hn_render_slot(m,secondary)+0x50)-0x47;}
        }else if(difference>-8&&difference<0){
            if(index>=count)return false;difference=(int)hn_read16(m,hn_render_slot(m,index)+0x50)-0x47;
        }
        hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)(difference>0?1:2));
    }
    return !m->fault;
}
HNSceneResult hn_scene_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x08011C8C:if(!hn_motion_approach_clamped(m,arguments))return HN_SCENE_RESOURCE;*consumed=2;break;
    case 0x08011DC0:{
        unsigned index,count=hn_read8(m,HN_RENDER_INFO+16);if(!hn_read32(m,HN_RENDER_SLOTS))return HN_SCENE_RESOURCE;
        for(index=0;index<count;index++)if(hn_read32(m,hn_render_slot(m,index)+4)==HN_APPROACH_ANIMATION)break;
        if(index==count||!hn_render_modes(m,(uint8_t)index,1,3,hn_read8(m,HN_SCENE_CONTROLS+0xE7))||
           !hn_render_modes(m,hn_read8(m,HN_SCENE_CONTROLS+0xE4),0,3,hn_read8(m,HN_SCENE_CONTROLS+0xE8)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08011E3C:
        if(!hn_render_configure(m,9,1,0xB0,20,-8,0,hn_read32(m,0x03002BB0))||
           !hn_render_modes(m,9,2,3,hn_read8(m,HN_RENDER_CLASS_LEADER)))return HN_SCENE_RESOURCE;break;
    case 0x08011E7C:
        if(hn_read8(m,HN_RENDER_INFO+16)<=9||!hn_read32(m,HN_RENDER_SLOTS))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)==9)hn_write8(m,0x03002170,0);
        hn_render_set_animation(m,9,hn_read32(m,0x03002BB4));
        if(!hn_render_modes(m,9,2,3,hn_read8(m,HN_RENDER_CLASS_LEADER)))return HN_SCENE_RESOURCE;break;
    case 0x08011EA4:case 0x08011EEC:case 0x08011EF0:case 0x08011F08:break;
    case 0x08011EA8:{
        unsigned first=hn_read8(m,arguments);GameAddr result=HN_SCENE_CONTROLS+hn_read8(m,arguments+1);unsigned second=hn_read8(m,result);
        if(first>=hn_read8(m,HN_RENDER_INFO+16)||second>=hn_read8(m,HN_RENDER_INFO+16)||!hn_read32(m,HN_RENDER_SLOTS))return HN_SCENE_RESOURCE;
        int32_t left=(int32_t)hn_read32(m,hn_render_slot(m,first)+0x20),right=(int32_t)hn_read32(m,hn_render_slot(m,second)+0x20);
        hn_write8(m,result,(uint8_t)(left==right?0:left>right?1:2));*consumed=2;break;
    }
    case 0x08011EF4:hn_write8(m,HN_SAVED_CALL_DEPTH,hn_read8(m,0x03002BF0));break;
    case 0x0802D190:if(!hn_motion_scene_actor(m,arguments))return HN_SCENE_RESOURCE;*consumed=1;break;
    case 0x0802D2EC:if(!hn_motion_restore_actors(m))return HN_SCENE_RESOURCE;break;
    case 0x0801D414:{
        unsigned index=hn_read8(m,HN_RENDER_CLASS_LEADER);
        if(index>=hn_read8(m,HN_RENDER_INFO+16)||!hn_read32(m,HN_RENDER_SLOTS))return HN_SCENE_RESOURCE;
        GameAddr slot=hn_render_slot(m,index);
        hn_write8(m,HN_SCENE_CONTROLS+0x11,hn_read8(m,slot+0x60)&15);
        hn_write16(m,0x0201DC60,hn_read16(m,slot+0x1A));hn_write16(m,0x0201DC62,hn_read16(m,slot+0x22));break;
    }
    case 0x0801D454:
        if(!hn_motion_transition(m,false,hn_read8(m,arguments)!=0))return HN_SCENE_RESOURCE;*consumed=1;break;
    case 0x0801D5E4:if(!hn_motion_transition(m,true,false))return HN_SCENE_RESOURCE;break;
    case 0x0801D580:
        if(!hn_motion_interpolate(m,hn_read8(m,arguments)))return HN_SCENE_RESOURCE;*consumed=1;break;
    case 0x080C1458:
        for(unsigned i=1;i<=7;i++)if(!hn_scene_indicator_update(m,i))return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_INDICATOR_PREVIOUS,hn_read8(m,HN_SCENE_CONTROLS+0x6C));hn_write8(m,HN_SCENE_INDICATOR_CHANGED,0);break;
    case 0x080C1490:
        if(hn_read8(m,HN_SCENE_INDICATOR_CHANGED)){
            if(!hn_scene_indicator_update(m,hn_read8(m,HN_SCENE_CONTROLS+0x6C))||
               !hn_scene_indicator_update(m,hn_read8(m,HN_SCENE_INDICATOR_PREVIOUS)))return HN_SCENE_RESOURCE;
            hn_write8(m,HN_SCENE_INDICATOR_CHANGED,0);
        }break;
    case 0x080C19FC:if(!indicator_move(m))return HN_SCENE_RESOURCE;break;
    case 0x080C1AC4:
        for(unsigned i=0;i<8;i++)if(flag(m,i+0x180)){
            GameAddr position=HN_SCENE_INDICATOR_POSITIONS+4+i*4;
            if(!hn_render_configure(m,(uint8_t)(i+18),1,0x74,(int16_t)hn_read16(m,position),(int16_t)hn_read16(m,position+2),0,0x088F6FC4))return HN_SCENE_RESOURCE;
            GameAddr clear=0x03002180+(i+0x180)/8,set=0x03002180+(i+0x40)/8;
            hn_write8(m,clear,(uint8_t)(hn_read8(m,clear)&~(1u<<(i&7))));hn_write8(m,set,(uint8_t)(hn_read8(m,set)|(1u<<(i&7))));break;
        }break;
    case 0x080C1B34:{
        unsigned index=hn_read8(m,HN_SCENE_CONTROLS+0xE6);hn_write8(m,HN_SCENE_CONTROLS+0xE5,(uint8_t)index);
        GameAddr position=HN_SCENE_INDICATOR_POSITIONS+index*4;
        if(!hn_render_configure(m,26,1,0x74,(int16_t)hn_read16(m,position),(int16_t)hn_read16(m,position+2),0,0x088F6FF0))return HN_SCENE_RESOURCE;
        hn_render_flags(m,26,12,4);break;
    }
    case 0x0801DC9C:case 0x0801DD5C:case 0x0801E1D0:case 0x0801E288:{
        GameAddr address=callback&~1u;
        if(!hn_text_counter_change(m,address>=0x0801E1D0?2:0,address==0x0801DD5C||address==0x0801E1D0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801C974:{
        if(hn_read8(m,HN_RENDER_INFO+16)<=10||!hn_read32(m,HN_RENDER_SLOTS))return HN_SCENE_RESOURCE;
        GameAddr destination=HN_SCENE_CONTROLS+hn_read8(m,arguments+1);
        int z=(int16_t)hn_read16(m,hn_render_slot(m,10)+0x22),half=(int16_t)hn_read16(m,HN_BACKGROUND_BOUNDS+6)>>1;
        hn_write8(m,destination,(uint8_t)(z<half?2:1));*consumed=2;break;
    }
    case 0x0801C9B4:{
        GameAddr control=HN_SCENE_CONTROLS+hn_read8(m,arguments);unsigned actor=hn_read8(m,arguments+1);
        if(actor==0x81&&!hn_read8(m,HN_RENDER_CLASS_FOLLOWER)){hn_write8(m,control+2,0);*consumed=2;break;}
        if(actor==0x80)actor=hn_read8(m,HN_RENDER_CLASS_LEADER);
        else if(actor==0x81)actor=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
        if(actor>=hn_read8(m,HN_RENDER_INFO+16)||!hn_read32(m,HN_RENDER_SLOTS))return HN_SCENE_RESOURCE;
        int direction=(int)hn_read8(m,control)-1;hn_write8(m,control,(uint8_t)direction);
        int offset=(int)(hn_read8(m,hn_render_slot(m,actor)+0x60)&7)-1;
        hn_write8(m,control+1,hn_read8(m,HN_TURN_DIRECTIONS+(uint32_t)offset));
        unsigned difference=((uint32_t)direction-hn_read8(m,control+1))&7;
        if(difference<=3)hn_write8(m,control+2,1);else if(difference>4)hn_write8(m,control+2,2);
        hn_write8(m,control+3,0);*consumed=2;break;
    }
    case 0x0801CDA4:case 0x0801CE28:if(!class_screen_side(m,(callback&~1u)==0x0801CE28))return HN_SCENE_RESOURCE;break;
    case 0x080C1C40:if(!save_actor_slots(m))return HN_SCENE_RESOURCE;break;
    case 0x080C1CBC:if(!restore_actor_slots(m,false))return HN_SCENE_RESOURCE;break;
    case 0x080C2A04:case 0x080C3A88:case 0x080CB3E4:case 0x080CB99C:case 0x080CC644:case 0x080CEE10:{
        unsigned map=(callback&~1u)==0x080C2A04?4:(callback&~1u)==0x080C3A88?5:(callback&~1u)==0x080CB3E4?10:
                     (callback&~1u)==0x080CB99C?11:(callback&~1u)==0x080CC644?12:17;
        if(!save_actor_slots(m))return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS+0x27,hn_read8(m,HN_SCENE_CONTROLS+0x0D));
        hn_write8(m,HN_SCENE_CONTROLS+0x28,hn_read8(m,HN_SCENE_CONTROLS+0x0E));
        if(!hn_map_initialize(m,10,(uint8_t)map))return HN_SCENE_RESOURCE;*consumed=UINT32_MAX-3;break;
    }
    case 0x080C2A9C:case 0x080C3B20:case 0x080CB47C:case 0x080CBA34:case 0x080CC6DC:case 0x080CEEA8:{
        GameAddr cursor=hn_read32(m,0x03002C60);
        if(!hn_map_initialize(m,hn_read8(m,HN_SCENE_CONTROLS+0x27),hn_read8(m,HN_SCENE_CONTROLS+0x28))||!restore_actor_slots(m,true))return HN_SCENE_RESOURCE;
        hn_write32(m,0x03002C60,cursor);break;
    }
    case 0x08011AAC:{
        unsigned layer=hn_read8(m,arguments),enabled=hn_read8(m,HN_DISPLAY_SHADOW+1)&31;
        uint8_t flags=hn_read8(m,0x03002182);
        hn_write8(m,0x03002182,(uint8_t)((layer<5&&((enabled>>layer)&1))?flags|8:flags&~8));
        *consumed=1;break;
    }
    case 0x0801D708:{
        unsigned kept=hn_read8(m,arguments);GameAddr state=hn_read32(m,HN_GRAPHICS_SCRIPTS);
        for(unsigned i=0;i<hn_read8(m,HN_GRAPHICS_COUNT)&&!m->fault;i++,state+=44)
            if(i!=kept)hn_write8(m,state,0);
        *consumed=1;break;
    }
    case 0x08009318:if(!hn_choice_reset(m))return HN_SCENE_RESOURCE;break;
    case 0x0801C2D4:case 0x0801C344:case 0x0801C3B4:case 0x0801C410:
        if(!indirect_arithmetic(m,callback&~1u,arguments,consumed))return HN_SCENE_RESOURCE;break;
    case 0x0801C474:{
        unsigned index=hn_read8(m,arguments);if(index!=255)hn_write8(m,HN_RENDER_CLASS_LEADER,(uint8_t)index);
        index=hn_read8(m,arguments+1);if(index!=255)hn_write8(m,HN_RENDER_CLASS_FOLLOWER,(uint8_t)index);
        hn_write32(m,0x03001134,2);*consumed=2;break;
    }
    case 0x0801C4A4:{
        hn_write32(m,0x03001134,0);unsigned a=hn_read8(m,HN_RENDER_CLASS_LEADER),b=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
        if(a>=hn_read8(m,HN_RENDER_INFO+0x10)||b>=hn_read8(m,HN_RENDER_INFO+0x10))return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS,hn_read8(m,hn_render_slot(m,a)+0x60)&15);
        hn_write8(m,HN_SCENE_CONTROLS+1,hn_read8(m,hn_render_slot(m,b)+0x60)&15);break;
    }
    case 0x0801C4F8:{
        unsigned index=hn_read8(m,arguments);if(index>=hn_read8(m,HN_RENDER_INFO+0x10))return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS,hn_read8(m,hn_render_slot(m,index)+0x60)&15);*consumed=1;break;
    }
    case 0x080CB150:
        hn_write16(m,HN_BACKGROUND_BOUNDS+2*12+2,1024);
        hn_write16(m,HN_BACKGROUND_BOUNDS+2*12+6,1024);break;
    case 0x0801C864:case 0x0801C888:case 0x0801C8AC:case 0x0801C8D0:{
        uint16_t mask=hn_read16(m,arguments),keys;
        unsigned mode=((callback&~1u)-0x0801C864)/36;
        if(mode==3)keys=hn_input_query(m,mask&0xF0,0)|hn_input_query(m,mask&0x30F,1);
        else keys=hn_input_query(m,mask,(uint8_t)mode);
        uint16_t result=0,ordinal=1;
        for(unsigned bit=0x200;bit;bit>>=1){
            if(keys&mask&bit){result=ordinal;break;}
            if(mask&bit)ordinal++;
        }
        hn_write16(m,0x03001FBC,result);*consumed=2;break;
    }
    case 0x080C0E3C:case 0x080C0FD0:case 0x080C10B0:case 0x080C1150:
        if(!panel_command(m,callback&~1u,hn_read8(m,arguments)))return HN_SCENE_RESOURCE;*consumed=1;break;
    case 0x080BFD34:{
        unsigned kind=menu_kind(m),selected=hn_read8(m,HN_SCENE_CONTROLS+0x19),next=selected;
        uint16_t keys=hn_input_query(m,0xC0,1);
        if(keys&0x40){
            if(kind==3){if(selected==5)next=3;}else if(selected>(kind?3u:1u))next=selected-1;
        }else if(keys){if(kind==3){if(selected==3)next=5;}else if(selected<=4)next=selected+1;}
        if(next!=selected){
            if(!hn_audio_ready(m))return HN_SCENE_RESOURCE;
            hn_write8(m,HN_SCENE_CONTROLS+0x19,(uint8_t)next);
            if(!hn_audio_song(m,302,HN_SONG_START)||!menu_update(m,(uint8_t)next))return HN_SCENE_RESOURCE;
        }
        break;
    }
    case 0x080BFE10:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x19),enabled,initial;
        if(flag(m,0x47)){bool complete=flag(m,0x10B);enabled=complete?15:3;initial=complete?2:4;}
        else{enabled=1;initial=3;for(unsigned i=0x140;i<=0x149;i++)if(flag(m,i)){enabled=5;break;}}
        hn_write8(m,HN_SCENE_CONTROLS+0x1B,(uint8_t)enabled);
        if(!selected)hn_write8(m,HN_SCENE_CONTROLS+0x19,(uint8_t)initial);
        if(!menu_update(m,0))return HN_SCENE_RESOURCE;break;
    }
    case 0x080BFF00:if(!menu_update(m,hn_read8(m,HN_SCENE_CONTROLS+0x19)))return HN_SCENE_RESOURCE;break;
    case 0x080C000C:{
        unsigned kind=menu_kind(m),selected=hn_read8(m,HN_SCENE_CONTROLS+0x19),language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);
        if(selected>=6||language>=5)return HN_SCENE_RESOURCE;
        unsigned chosen=hn_read8(m,HN_SCENE_MENU_CODES+kind*6+selected);
        GameAddr table=hn_read32(m,HN_SCENE_MENU_BANKS+(language*4+kind)*4);
        if(!hn_background_configure(m,0,0x82,hn_read32(m,table+(chosen+10)*4)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080C005C:{
        unsigned i=0;while(i<6&&!hn_read8(m,0x03003730+i*16))i++;
        if(i==6)hn_write8(m,HN_SCENE_CONTROLS+0x60,0);
        else{
            unsigned attempts=4096;
            do{i=hn_random_range(m,0,6);}while(!hn_read8(m,0x03003730+i*16)&&--attempts);
            if(!attempts)return HN_SCENE_RESOURCE;
            hn_write8(m,HN_SCENE_CONTROLS+0x67,(uint8_t)i);
            hn_write8(m,HN_SCENE_CONTROLS+0x60,hn_read8(m,0x03003730+i*16));
        }
        break;
    }
    case 0x080C00CC:
        if(hn_read8(m,HN_SCENE_CONTROLS+0x1F)!=255&&(int32_t)hn_read32(m,0x03004ED4)<0)
            hn_write8(m,HN_SCENE_CONTROLS+0x1F,1);break;
    case 0x080C00F0:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x1D),language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);
        if(language>4)language=0;if(selected>=5)return HN_SCENE_RESOURCE;
        if(selected&&!hn_background_configure(m,8,0x81,hn_read32(m,HN_SCENE_MENU_STATUS+(language*5+selected)*4)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080C0128:
        hn_write8(m,HN_SCENE_CONTROLS+8,hn_read8(m,0x03002168+hn_read8(m,arguments)));*consumed=1;break;
    case 0x080C0168:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x19);if(selected>=6)return HN_SCENE_RESOURCE;
        hn_write16(m,0x03001FBC,hn_read8(m,HN_SCENE_MENU_CODES+menu_kind(m)*6+selected));break;
    }
    case 0x080C019C:if(!record_menu(m))return HN_SCENE_RESOURCE;break;
    case 0x080C0260:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x1E),next=selected;
        uint16_t keys=hn_input_query(m,0xC0,1);
        if(keys&0x40){if(selected)next=selected-1;}else if(keys&&selected<=3)next=selected+1;
        if(next!=selected){
            if(!hn_audio_ready(m))return HN_SCENE_RESOURCE;
            hn_write8(m,HN_SCENE_CONTROLS+0x1E,(uint8_t)next);
            if(!hn_audio_song(m,302,HN_SONG_START)||!record_menu(m))return HN_SCENE_RESOURCE;
        }
        break;
    }
    case 0x080C02BC:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x1E);if(selected>=5)return HN_SCENE_RESOURCE;
        if(!hn_background_configure(m,0,0x82,hn_read32(m,HN_SCENE_RECORD_MENU+(selected+10)*4)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080C02E4:case 0x080C0304:
        for(unsigned i=0;i<2;i++){
            GameAddr control=HN_SCENE_CONTROLS+0xEB+i,snapshot=HN_SCENE_RECORD_LANGUAGE+i;
            if((callback&~1u)==0x080C02E4)hn_write8(m,snapshot,hn_read8(m,control));
            else hn_write8(m,control,hn_read8(m,snapshot));
        }
        break;
    case 0x080C0324:hn_input_repeat(m,16,8);break;
    case 0x080C0334:{
        hn_write16(m,0x03001FBC,0);uint16_t keys=hn_input_script_mask(m,1);
        GameAddr selected=HN_SCENE_CONTROLS+0x1A1;
        if(keys&1)hn_write16(m,0x03001FBC,(uint16_t)(hn_read8(m,selected)+2));
        else if(keys&2){hn_write8(m,selected,3);hn_write16(m,0x03001FBC,5);}
        else{
            keys=hn_input_script_mask(m,2);unsigned current=hn_read8(m,selected);
            if(keys&0x60){hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)current);
                hn_write8(m,selected,(uint8_t)(current?current-1:2));hn_write16(m,0x03001FBC,1);}
            else if(keys&0x90){hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)current);
                hn_write8(m,selected,(uint8_t)(current<=1?current+1:0));hn_write16(m,0x03001FBC,1);}
        }
        break;
    }
    case 0x0801C760:hn_input_script_mask(m,1);break;
    case 0x0801C794:hn_input_script_mask(m,2);break;
    case 0x0801C7C8:{
        uint16_t mask=hn_read16(m,0x03001F64);
        uint16_t keys=hn_input_query(m,mask&0xF0,0)|hn_input_query(m,mask&0x30F,1);
        hn_write16(m,0x0201DC40,keys);hn_write16(m,0x03002180,(uint16_t)((keys<<6)|(hn_read16(m,0x03002180)&63)));break;
    }
    default:{
        HNSceneResult result=hn_scene_motion_callback(m,callback,arguments,consumed);
        if(result!=HN_SCENE_UNKNOWN)return result;
        result=hn_scene_text_callback(m,callback,arguments,consumed);
        return result==HN_SCENE_UNKNOWN?hn_book_callback(m,callback,arguments,consumed):result;
    }
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_scene_animation_callback(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,
                                         uint32_t *consumed,uint32_t *yield)
{
    (void)slot;(void)arguments;
    HNSceneResult motion=hn_scene_motion_animation(m,callback,consumed,yield);if(motion!=HN_SCENE_UNKNOWN)return motion;
    switch(callback&~1u){
    /* These script callbacks ignore their argument register. Animation streams
       also invoke them; their zero return consumes no bytes and preserves yield. */
    case 0x08011DC0:case 0x08011E3C:case 0x08011E7C:case 0x08011EA4:
    case 0x08011EEC:case 0x08011EF0:case 0x08011EF4:case 0x08011F08:
        return hn_scene_callback(m,callback,0,consumed);
    default:break;
    }
    if((callback&~1u)==0x0802FFF4){
        hn_write32(m,slot+0x24,(uint32_t)((int32_t)(0x00E08000u-hn_read32(m,slot+0x18))/48));
        hn_write32(m,slot+0x2C,(uint32_t)((int32_t)(0x00B88000u-hn_read32(m,slot+0x20))/48));
        *consumed=0;*yield=0;return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
    }
    if((callback&~1u)==0x0803002C){
        unsigned leader=hn_read8(m,HN_RENDER_CLASS_LEADER),count=hn_read8(m,HN_RENDER_INFO+16);
        if(leader>=count||!hn_read32(m,HN_RENDER_SLOTS)||!hn_motion_map_height(m,hn_render_slot(m,leader)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,0x03002182)&1){
            unsigned follower=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
            if(follower>=count||!hn_motion_map_height(m,hn_render_slot(m,follower)))return HN_SCENE_RESOURCE;
        }
        *consumed=0;*yield=1;return HN_SCENE_OK;
    }
    if((callback&~1u)==0x0801E800){
        if(!hn_audio_region(m,arguments,yield))return HN_SCENE_RESOURCE;
        *consumed=13;return HN_SCENE_OK;
    }
    if((callback&~1u)==0x0801E348){
        if(!hn_text_counter_update(m,yield))return HN_SCENE_RESOURCE;
        *consumed=0;return HN_SCENE_OK;
    }
    if((callback&~1u)==0x0801E3E8){
        if(!hn_text_counter(m,hn_read16(m,0x0201DC52),0))return HN_SCENE_RESOURCE;
        *consumed=0;*yield=(*yield&0xFFFFFF00u)|1;return HN_SCENE_OK;
    }
    if((callback&~1u)!=0x080C12E4)return HN_SCENE_UNKNOWN;
    unsigned row,count;if(!panel_layout(m,hn_read8(m,HN_SCENE_CONTROLS+0x1B9),&row,&count))return HN_SCENE_RESOURCE;
    uint8_t remaining=(uint8_t)(hn_read8(m,HN_SCENE_CONTROLS+0x1B7)-1);hn_write8(m,HN_SCENE_CONTROLS+0x1B7,remaining);
    if(!remaining){
        hn_write8(m,HN_SCENE_CONTROLS+0x1B7,4);unsigned index=hn_read8(m,HN_SCENE_CONTROLS+0x1B8);
        unsigned target=hn_read8(m,HN_PANEL_ORDER+row+1+index);
        if(target>=hn_read8(m,HN_RENDER_INFO+0x10))return HN_SCENE_RESOURCE;
        hn_write32(m,hn_render_slot(m,target)+0x20,0x40000);
        panel_velocity(m,target,0,0,-0x8000,0x24);panel_velocity(m,target,0,0,0x1000,0x30);
        index=(index+1)&255;hn_write8(m,HN_SCENE_CONTROLS+0x1B8,(uint8_t)index);
        if(index==hn_read8(m,HN_PANEL_ORDER+row)){
            hn_write8(m,HN_SCENE_CONTROLS+0x1B8,0);hn_write8(m,HN_SCENE_CONTROLS+0x1B7,127);
        }
    }
    for(unsigned i=0;i<count;i++)if((int32_t)hn_read32(m,hn_render_slot(m,i)+0x20)>0x47FFF){
        hn_write32(m,hn_render_slot(m,i)+0x20,0x48000);panel_velocity(m,i,0,0,0,0x24);panel_velocity(m,i,0,0,0,0x30);
    }
    *consumed=0;*yield=1;return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
