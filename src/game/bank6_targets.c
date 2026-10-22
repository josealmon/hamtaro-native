#include "game/bank6.h"
#include "game/state.h"
#include "game/render.h"
#include "game/input.h"
#include "game/random.h"
#include "gba/bios.h"

enum { CLASSES=0x0201DC40,TARGET_SETS=0x086BF94C };
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static void set_flag(HNMemory *m,unsigned id,bool value)
{GameAddr a=0x03002180+(id>>3);uint8_t bit=(uint8_t)(1u<<(id&7));hn_write8(m,a,(uint8_t)((hn_read8(m,a)&~bit)|(value?bit:0)));}
static bool animation(HNMemory *m,uint8_t index,GameAddr image)
{if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);hn_render_set_animation(m,index,image);return !m->fault;}
static int32_t magnitude(int32_t value){return value>0?value:(int32_t)(0u-(uint32_t)value);}
bool hn_bank6_target_motion(HNMemory *m,GameAddr coordinates,uint32_t moving_index,uint32_t target_index)
{
    GameAddr base=hn_read32(m,HN_RENDER_SLOTS),target=base+target_index*104,moving=base+moving_index*104;
    uint32_t x=((uint32_t)hn_read16(m,coordinates)<<16)+0x8000,z=((uint32_t)hn_read16(m,coordinates+2)<<16)+0x8000;
    if(hn_read32(m,target+4)==0x088E4C34){hn_write32(m,moving+0x18,hn_read32(m,target+0x18)+0x14000);hn_write32(m,moving+0x20,hn_read32(m,target+0x20)+0x4000);}
    else{hn_write32(m,moving+0x18,hn_read32(m,target+0x18)+0xC000);hn_write32(m,moving+0x20,hn_read32(m,target+0x20)+0xC000);}
    int32_t vx=hn_runtime_divide((int32_t)(x-hn_read32(m,moving+0x18)),63);hn_write32(m,moving+0x24,(uint32_t)vx);
    int32_t vz=hn_runtime_divide((int32_t)(z-hn_read32(m,moving+0x20)),63);hn_write32(m,moving+0x2C,(uint32_t)vz);
    hn_write32(m,moving+0x18,x-((uint32_t)vx*64-(uint32_t)vx));hn_write32(m,moving+0x20,z-((uint32_t)vz*64-(uint32_t)vz));return !m->fault;
}
bool hn_bank6_target_position(HNMemory *m,GameAddr coordinates,uint32_t index_word)
{return hn_render_configure_raw(m,(uint8_t)index_word,1,114,(int16_t)hn_read16(m,coordinates),0,(int16_t)hn_read16(m,coordinates+2),0x088E4466);}
bool hn_bank6_restore_targets(HNMemory *m,GameAddr arguments,GameAddr retained_r5)
{
    uint8_t selector=hn_read8(m,arguments);GameAddr set=retained_r5;
    for(unsigned i=0;i<7;i++)if(hn_read16(m,TARGET_SETS+i*16+8)==selector){set=TARGET_SETS+i*16;break;}
    unsigned choice=control(m,hn_read16(m,set+12));
    if(choice&&!hn_bank6_target_position(m,hn_read32(m,set)+choice*4,25))return false;
    choice=control(m,hn_read16(m,set+14));
    if(choice&&!hn_bank6_target_position(m,hn_read32(m,set)+choice*4,26))return false;return !m->fault;
}
static bool select_targets(HNMemory *m)
{
    unsigned selector=control(m,0x28),index=control(m,0x12);GameAddr slot=hn_render_slot(m,index),set=0,range=0;
    for(unsigned i=0;i<7;i++)if(hn_read16(m,TARGET_SETS+i*16+8)==selector){set=TARGET_SETS+i*16;break;}
    if(set){unsigned count=hn_read16(m,set+10);GameAddr base=hn_read32(m,set+4);
        for(unsigned i=0;i<count;i++)if(hn_read8(m,base+i*8+4)==index){range=base+i*8;break;}}
    if(!set||!range){hn_write8(m,hn_render_slot(m,25)+0x60,0);hn_write8(m,hn_render_slot(m,26)+0x60,0);set_flag(m,0x2A7,false);return !m->fault;}
    if(!animation(m,(uint8_t)index,hn_read32(m,slot+4)==0x088E4BFF?0x088E4C34:0x088E4C07))return false;
    set_flag(m,0x2A7,true);
    uint32_t choice=hn_random_range(m,hn_read16(m,range),(uint16_t)(hn_read16(m,range+2)+1));
    if(!hn_bank6_target_motion(m,hn_read32(m,set)+choice*4,25,index))return false;
    store(m,hn_read16(m,set+12),choice);
    int32_t count=(int32_t)hn_read16(m,range+2)-hn_read16(m,range)+1;
    uint16_t draw=hn_random_range(m,1,(uint16_t)count);int32_t first=hn_read16(m,range);
    choice=(uint32_t)hn_runtime_remainder((int32_t)(choice-(uint32_t)first+draw),count)+(uint32_t)first;
    if(!hn_bank6_target_motion(m,hn_read32(m,set)+choice*4,26,index))return false;
    store(m,hn_read16(m,set+14),choice);return !m->fault;
}
static bool cursor_motion(HNMemory *m)
{
    hn_input_script_mask(m,0);unsigned offset=(hn_read16(m,CLASSES)&0xF0)>>2;
    uint16_t x_step=hn_read16(m,0x086BF774+offset),z_step=hn_read16(m,0x086BF774+offset+2);GameAddr cursor=hn_render_slot(m,12);
    if(hn_read16(m,cursor+0x4E)<=8&&x_step>0xF000)x_step=0;
    if(hn_read16(m,cursor+0x4E)>207&&x_step<=0xEFFF)x_step=0;
    if(hn_read16(m,cursor+0x50)<=8&&z_step>0xF000)z_step=0;
    if(hn_read16(m,cursor+0x50)>127&&z_step<=0xEFFF)z_step=0;
    GameAddr base=hn_read32(m,HN_RENDER_SLOTS);cursor=base+12*104;
    hn_write32(m,cursor+0x18,hn_read32(m,cursor+0x18)+((uint32_t)x_step<<16));uint32_t x=hn_read32(m,cursor+0x18);
    hn_write32(m,cursor+0x20,hn_read32(m,cursor+0x20)+((uint32_t)z_step<<16));uint32_t z=hn_read32(m,cursor+0x20);
    uint32_t actor_x=hn_read32(m,base+10*104+0x18)+0x180000,actor_z=hn_read32(m,base+10*104+0x20)+0x180000;
    x+=0x100000;z+=0x100000;GameAddr image;unsigned facing;
    if(x>=actor_x){if(z>=actor_z){image=0x088AD7BE;facing=1;}else{image=0x088AD7D6;facing=3;}}
    else if(z>=actor_z){image=0x088AD7B2;facing=0;}else{image=0x088AD7CA;facing=2;}
    if(!animation(m,10,image))return false;store(m,0x1A0,facing);
    uint32_t ignored;if(hn_scene_callback(m,0x0801C7C8,0,&ignored)!=HN_SCENE_OK)return false;
    if(hn_read16(m,CLASSES)&1)set_flag(m,0x2A0,true);if(hn_read16(m,CLASSES)&2)set_flag(m,0x2B0,true);return !m->fault;
}
static bool launch(HNMemory *m,GameAddr arguments)
{
    set_flag(m,0x2A0,false);uint8_t index=hn_read8(m,arguments);
    if(!animation(m,12,0x0889F620)||!hn_render_configure_raw(m,11,1,114,0,0,0,index==10?0x088F3ADF:0x088F3AF5))return false;
    hn_render_flags(m,11,12,0);GameAddr actor=hn_render_slot(m,index),cursor=hn_render_slot(m,12);
    uint32_t actor_x=hn_read32(m,actor+0x18)+0x180000,actor_z=hn_read32(m,actor+0x20)+0x180000;
    uint32_t cursor_x=hn_read32(m,cursor+0x18)+0x100000,cursor_z=hn_read32(m,cursor+0x20)+0x110000;
    if(index==10){unsigned facing=control(m,0x1A0);GameAddr image=0,overlay=0;
        switch(facing){case 0:image=0x088AD82E;overlay=0x088AD863;break;case 1:image=0x088AD886;overlay=0x088AD8BB;break;case 2:image=0x088AD8DE;overlay=0x088AD913;break;case 3:image=0x088AD936;overlay=0x088AD96B;break;default:break;}
        if(image&&(!animation(m,10,image)||!hn_render_configure_raw(m,9,1,176,0,0,0,overlay)))return false;
        if(!hn_render_modes_raw(m,9,2,3,10))return false;
    }
    int32_t dx=(int32_t)(cursor_x-actor_x),dz=(int32_t)(cursor_z-actor_z),ax=magnitude(dx),az=magnitude(dz),duration=(ax>az?ax:az)>>18;
    store(m,0x1A1,(uint32_t)duration);GameAddr slot=hn_render_slot(m,11);
    hn_write32(m,slot+0x24,(uint32_t)hn_runtime_divide(dx,duration));hn_write32(m,slot+0x2C,(uint32_t)hn_runtime_divide(dz,duration));
    if(!control(m,0x1A1))store(m,0x1A1,1);return !m->fault;
}
static void rectangle(HNMemory *m,GameAddr arguments,bool cursor)
{
    store(m,0x60,0);uint8_t index=hn_read8(m,arguments);int min_x=hn_read8(m,arguments+1),min_z=hn_read8(m,arguments+2),max_x=hn_read8(m,arguments+3),max_z=hn_read8(m,arguments+4);
    GameAddr base=hn_read32(m,HN_RENDER_SLOTS),slot=base+index*104,target=base+(cursor?12:11)*104;
    int x=hn_read16(m,slot+0x1A),z=hn_read16(m,slot+0x22);
    int target_x=cursor?(uint16_t)((int16_t)hn_read16(m,target+0x1A)+16):hn_read16(m,target+0x1A);
    int target_z=cursor?(uint16_t)((int16_t)hn_read16(m,target+0x22)+16):hn_read16(m,target+0x22);
    if(hn_read8(m,slot+0x60)&&target_x>x+min_x&&target_x<x+max_x&&target_z>z+min_z&&target_z<z+max_z)store(m,0x60,1);
}
static bool task_pattern(HNMemory *m)
{
    unsigned match=0;
    for(unsigned kind=0;kind<3;kind++){
        GameAddr cursor=hn_read32(m,0x086BED68+kind*20+control(m,0xEB)*4);unsigned i=0;
        while(hn_read8(m,0x03002BA0+i)&&hn_read8(m,cursor)==hn_read8(m,0x03002BA0+i)){
            if(i++>=8192)return false;cursor++;
        }
        if(!hn_read8(m,cursor))match=kind+1;
    }
    uint8_t value=(uint8_t)hn_random_range(m,0,100);
    if(!match)value=(uint8_t)(value%3);
    else{value&=1;if(match==1)value++;else if(match==2)value&=16;}
    store(m,0x1B5,value);return !m->fault;
}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_bank6_targets_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080A21D4:REQUIRE(cursor_motion(m));break;
    case 0x080A2350:REQUIRE(launch(m,arguments));*consumed=1;break;
    case 0x080A2518:case 0x080A25B0:rectangle(m,arguments,(callback&~1u)==0x080A2518);*consumed=5;break;
    case 0x080A2744:REQUIRE(select_targets(m));break;
    case 0x080A28B4:
        /* MainScriptInvokeCallback loads R5 with its pointer scratch before BL. */
        REQUIRE(hn_bank6_restore_targets(m,arguments,0x030005E8));*consumed=1;break;
    case 0x080A291C:{
        uint8_t index=hn_read8(m,arguments),linked=control(m,0x12);GameAddr base=hn_read32(m,HN_RENDER_SLOTS),target=base+linked*104,aux=base+index*104;
        REQUIRE(hn_render_configure_raw(m,index,1,176,0,0,0,0x088F3D76));REQUIRE(hn_render_modes_raw(m,index,2,3,linked));
        if(hn_read32(m,target+4)==0x088E4BFF){hn_write32(m,aux+0x18,0x148000);hn_write32(m,aux+0x20,0x48000);}
        else{hn_write32(m,aux+0x18,0xC8000);hn_write32(m,aux+0x20,0xC8000);}
        hn_write32(m,aux+0x20,hn_read32(m,aux+0x20)+0x640000);hn_write32(m,aux+0x1C,hn_read32(m,aux+0x1C)+0xFF9C0000);hn_write16(m,CLASSES+110,64);*consumed=1;break;
    }
    case 0x080A2994:REQUIRE(task_pattern(m));break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank6_targets_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield)
{
    (void)arguments;*consumed=0;
    switch(callback&~1u){
    case 0x080A2628:hn_write32(m,slot+0x24,(uint32_t)hn_runtime_divide((int32_t)(0x1000000-hn_read32(m,slot+0x18)),40));*yield=0;break;
    case 0x080A2648:{
        int16_t x=(int16_t)(104-(int16_t)hn_read16(m,HN_VIDEO_STATE+0x6A)),z=(int16_t)(64-(int16_t)hn_read16(m,HN_VIDEO_STATE+0x6C));GameAddr cursor=hn_render_slot(m,12);
        hn_write32(m,cursor+0x18,((uint32_t)(uint16_t)x<<16)+0x8000);hn_write32(m,cursor+0x1C,0x8000);hn_write32(m,cursor+0x20,((uint32_t)(uint16_t)z<<16)+0x8000);*yield=1;break;
    }
    case 0x080A2AD4:{
        if(hn_read8(m,slot+0x64)==255){hn_write8(m,slot+0x64,hn_render_find_affine(m));hn_write8(m,HN_AFFINE_USAGE+hn_read8(m,slot+0x64),1);}
        uint32_t value=hn_read16(m,CLASSES+110),curve=(22*value*value)>>8;if(value)hn_write16(m,CLASSES+110,(uint16_t)(value-1));
        uint16_t scale=(uint16_t)(512-curve);hn_write16(m,slot+0x3C,scale);hn_write16(m,slot+0x3E,scale);*yield=0;break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
