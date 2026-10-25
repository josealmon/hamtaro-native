#include "game/save_group4.h"
#include "game/state.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/save.h"

enum { CONTEXT=0x03002BB0,RECEIVED=0x03003D30,RESOURCES=0x086E8CCC,PAYLOAD_FLAGS=0x086E906A };
static unsigned control(HNMemory *m,unsigned index){return hn_read8(m,HN_SCENE_CONTROLS+index);}
static void store(HNMemory *m,unsigned index,unsigned value){hn_write8(m,HN_SCENE_CONTROLS+index,(uint8_t)value);}
static GameAddr resource(unsigned row){return RESOURCES+row*28;}
static bool flag(HNMemory *m,unsigned index){return (hn_read8(m,0x03002180+(index>>3))>>(index&7))&1;}
static void visible(HNMemory *m,uint8_t index,uint8_t state){hn_write8(m,hn_render_slot(m,index)+0x60,state);}
static bool matches(HNMemory *m,GameAddr record,const uint8_t *pattern,unsigned length)
{
    for(unsigned i=0;i<length;i++)if(hn_read8(m,record+5+i)!=pattern[i])return false;
    return true;
}
static bool update_record(HNMemory *m)
{
    unsigned row=control(m,0x180);uint8_t pattern[9]={0};
    if(hn_read8(m,RECEIVED)==3){
        unsigned length=hn_read8(m,RECEIVED+2);if(length>sizeof(pattern))return false;
        for(unsigned i=0;i<length;i++)pattern[i]=hn_read8(m,RECEIVED+5+i);
    }else for(unsigned i=0;i<9;i++)pattern[i]=hn_read8(m,0x03002BA0+i);
    unsigned length=0;while(length<4&&pattern[length])length++;
    GameAddr records=hn_read32(m,CONTEXT)+row*1100;uint8_t number=1;unsigned depth=0;
    for(unsigned i=0;i<4;i++){
        GameAddr record=records+i*220;
        if(hn_read8(m,record)&&matches(m,record,pattern,length)){
            unsigned previous=hn_read8(m,record+16),n=hn_read8(m,record+1);
            if(previous>depth){depth=previous;number=(uint8_t)n;}
            else if(previous==depth){if(number<n)number=(uint8_t)n;}
            else if(number==n)number++;
        }
    }
    for(unsigned pass=0;pass<5;pass++)for(unsigned i=0;i<4;i++){
        GameAddr record=records+i*220;
        if(hn_read8(m,record)&&matches(m,record,pattern,length)&&number==hn_read8(m,record+1)){number++;break;}
    }
    if(number>50){
        number=1;depth++;
        for(unsigned pass=0;pass<5;pass++)for(unsigned i=0;i<4;i++){
            GameAddr record=records+i*220;
            if(hn_read8(m,record)&&matches(m,record,pattern,length)&&number==hn_read8(m,record+1))number++;
        }
        depth=(uint8_t)depth;
    }
    uint8_t i=(uint8_t)(hn_read8(m,RECEIVED+2)+2);
    if(number>1){
        if(number>9){hn_write8(m,RECEIVED+3+i,(uint8_t)(number/10+2));i++;}
        hn_write8(m,RECEIVED+3+i,(uint8_t)(number%10+2));i++;
    }else{hn_write8(m,RECEIVED+3+i,1);i++;hn_write8(m,RECEIVED+3+i,1);}
    while(i<=10){hn_write8(m,RECEIVED+3+i,1);i++;}
    hn_write8(m,RECEIVED+1,number);hn_write8(m,RECEIVED+3,(uint8_t)(control(m,0x181)+3));
    hn_write8(m,RECEIVED+4,1);hn_write8(m,RECEIVED+15,0);hn_write8(m,RECEIVED+16,(uint8_t)depth);
    records+=control(m,0x181)*220;
    if(!hn_memory_copy(m,records,RECEIVED,128)||!hn_save_write_record(m,4,control(m,0x180)*5+control(m,0x181)))return false;
    if(!hn_memory_copy(m,RECEIVED,hn_read32(m,CONTEXT)+0x2AF8,128)||!hn_save_write_record(m,4,50))return false;
    return hn_memory_copy(m,RECEIVED,records,128)&&!m->fault;
}
static void classify(HNMemory *m)
{
    GameAddr entry=resource(control(m,0x180)),data=hn_read32(m,entry+8);
    unsigned mode=control(m,0x60);store(m,0x61,0);
    if(mode==0){
        unsigned count=hn_read8(m,entry+0x16);
        for(unsigned i=0;i<count&&!m->fault;i++){
            unsigned value=hn_read8(m,RECEIVED+17+i),expected=hn_read8(m,data+i);
            if(value!=expected&&value>1){store(m,0x61,1);break;}
        }
    }else if(mode==1){
        unsigned count=hn_read8(m,entry+0x16);
        for(unsigned i=0;i<count&&!m->fault;i++)if(hn_read8(m,RECEIVED+17+i)==0x34){store(m,0x61,1);break;}
    }else if(mode==2){
        unsigned count=0,length=hn_read8(m,entry+0x16);
        for(unsigned row=0;row<10&&!m->fault;row++)for(unsigned i=0;i<length&&!m->fault;i++){
            int value=hn_read8(m,RECEIVED+17+i),expected=(int)hn_read8(m,PAYLOAD_FLAGS+row)-0x45;
            if(value==expected){count++;break;}
        }
        store(m,0x62,count<=4?1:count<=7?2:count<=9?3:0);
        if(count>9)store(m,0x61,1);
    }
}
HNSceneResult hn_group4_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080202BC:if(!hn_save_reset_group4(m))return HN_SCENE_RESOURCE;break;
    case 0x080202E0:hn_write32(m,0x03001F98,hn_read8(m,arguments));*consumed=1;break;
    case 0x080C846C:{
        GameAddr slot=hn_render_slot(m,26);
        if(hn_read8(m,slot+0x64)==255){hn_write8(m,slot+0x64,hn_render_find_affine(m));hn_write8(m,HN_AFFINE_USAGE+hn_read8(m,slot+0x64),1);}
        hn_write16(m,slot+0x3C,hn_read16(m,0x086E9004+(control(m,0x18C)-1)*2));
        hn_write16(m,hn_render_slot(m,26)+0x52,0xFFF0);break;
    }
    case 0x080C84D4:{
        GameAddr slot=hn_render_slot(m,26);unsigned group=control(m,0x18C)-1;
        unsigned value=hn_read8(m,0x086E9012+group*7+control(m,0x18B));
        hn_write32(m,slot+0x18,(value<<16)+0x009C8000);break;
    }
    case 0x080C8528:{
        unsigned hidden=flag(m,0x281)?(flag(m,0x1E9)?8:4):12,reset=flag(m,0x281)?2:6;
        for(unsigned i=0;i<hidden;i++){
            visible(m,hn_read8(m,0x086E83D8+i),0);visible(m,18,0);visible(m,20,0);visible(m,21,0);
        }
        for(unsigned i=0;i<reset;i++){
            GameAddr slot=hn_render_slot(m,hn_read8(m,0x086E83E4+i));
            for(unsigned offset=0x24;offset<=0x38;offset+=4)hn_write32(m,slot+offset,0);
            hn_write16(m,slot+0x3C,0x100);hn_write16(m,slot+0x3E,0x100);hn_write16(m,slot+0x40,0);
            hn_write16(m,slot+0x44,0);hn_write16(m,slot+0x46,0);hn_write16(m,slot+0x48,0);hn_write8(m,slot+0x64,255);
        }
        if(hn_group4_task(m,0x080C9000)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;break;
    }
    case 0x080C85F8:break;
    case 0x080C85FC:{
        GameAddr entry=resource(control(m,0x180)),words=hn_read32(m,entry);
        hn_write16(m,0x0201DC84,hn_read16(m,words+control(m,0x184)*2));break;
    }
    case 0x080C8638:{
        unsigned value=hn_read8(m,RECEIVED+17+control(m,0x184));store(m,0x186,value==1?0:value);break;
    }
    case 0x080C8668:hn_write16(m,0x0201DC84,hn_read16(m,resource(control(m,0x180))+0x12));break;
    case 0x080C8694:{
        GameAddr records=hn_read32(m,CONTEXT)+control(m,0x180)*1100;unsigned i=0;
        while(i<5&&hn_read8(m,records+i*220))i++;store(m,0x191,i);break;
    }
    case 0x080C86E8:{
        GameAddr record=hn_read32(m,CONTEXT)+control(m,0x180)*1100+control(m,0x181)*220;
        store(m,0x191,hn_read8(m,record)!=0);break;
    }
    case 0x080C87C8:if(!update_record(m))return HN_SCENE_RESOURCE;break;
    case 0x080C8AF0:{
        unsigned i=0;while(i<=6&&hn_read8(m,0x03002BA0+i))i++;
        hn_write8(m,RECEIVED+2,(uint8_t)i);hn_memory_copy(m,RECEIVED+5,0x03002BA0,7);hn_write8(m,RECEIVED,2);break;
    }
    case 0x080C8B28:
        if(!hn_audio_song(m,hn_read16(m,resource(control(m,0x180))+16),HN_SONG_START))return HN_SCENE_RESOURCE;break;
    case 0x080C8B54:case 0x080C8B94:{
        uint8_t index=(uint8_t)((flag(m,0x1E9)?6:0)+hn_read8(m,arguments));GameAddr animation=hn_read32(m,arguments+1);
        if((callback&~1u)==0x080C8B94){if(!hn_render_configure_raw(m,index,1,0xB0,0,0,0,animation))return HN_SCENE_RESOURCE;}
        else hn_render_set_animation(m,index,animation);
        *consumed=5;break;
    }
    case 0x080C8BE4:{
        unsigned offset=flag(m,0x1E9)?6:0;uint8_t first=(uint8_t)(offset+hn_read8(m,arguments)),second=(uint8_t)(offset+hn_read8(m,arguments+1));
        if(!hn_render_modes_raw(m,first,2,3,second))return HN_SCENE_RESOURCE;*consumed=2;break;
    }
    case 0x080C8C28:{
        unsigned x=hn_read16(m,arguments+1)*2,y=hn_read16(m,arguments+3)*2,index=hn_read8(m,arguments);
        uint32_t vx=(uint32_t)hn_read16(m,0x0201DC40+x)<<16,vy=(uint32_t)hn_read16(m,0x0201DC40+y)<<16;
        GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x18,vx);hn_write32(m,slot+0x1C,vy);hn_write32(m,slot+0x20,0);
        *consumed=5;break;
    }
    case 0x080C8C60:{
        GameAddr record=hn_read32(m,CONTEXT)+0x2AF8;unsigned index=control(m,0x191);uint8_t value;
        if(flag(m,hn_read16(m,0x086E83C4+index*2)))value=1;
        else{value=hn_read8(m,record+20+control(m,0x191));if(value)value=2;}
        store(m,0x63,value);break;
    }
    case 0x080C8CB0:{
        uint32_t y=((uint32_t)hn_read8(m,0x020046B3)<<20)-0x100000;
        for(unsigned i=0;i<7;i++)hn_write32(m,hn_render_slot(m,hn_read16(m,0x086E9044+i*2))+0x1C,y);
        y=((uint32_t)hn_read8(m,0x020046B3)<<20)+0x500000;
        for(unsigned i=7;i<10;i++)hn_write32(m,hn_render_slot(m,hn_read16(m,0x086E9044+i*2))+0x1C,y);
        GameAddr slot=hn_render_slot(m,30);hn_write32(m,slot+0x1C,hn_read32(m,slot+0x1C)-0x10000);
        slot=hn_render_slot(m,17);hn_write32(m,slot+0x1C,hn_read32(m,slot+0x1C)-0x10000);break;
    }
    case 0x080C8D44:
        hn_memory_copy(m,RECEIVED,hn_read32(m,CONTEXT)+0x2AF8,128);
        for(unsigned i=0;i<10;i++)if(flag(m,hn_read16(m,0x086E83C4+i*2)))hn_write8(m,RECEIVED+i,1);break;
    case 0x080C8D94:{
        GameAddr entry=resource(control(m,0x180));unsigned selected=control(m,0x187);
        visible(m,30,selected!=0);visible(m,31,(int)control(m,0x187)!=(int)hn_read8(m,entry+0x16)-1);break;
    }
    case 0x080C8DF8:
        visible(m,32,hn_read8(m,0x020046AB)!=0);visible(m,33,(int)hn_read8(m,0x020046AB)!=(int)control(m,0x18C)-1);break;
    case 0x080C8E48:case 0x080C8E70:{
        GameAddr context=hn_read32(m,CONTEXT);unsigned offset=hn_read16(m,arguments);
        if((callback&~1u)==0x080C8E70)store(m,offset,hn_read8(m,context+0x2B18));
        else hn_write8(m,context+0x2B18,(uint8_t)control(m,offset));
        *consumed=2;break;
    }
    case 0x080C8740:{
        unsigned row=control(m,0x180);GameAddr entry=resource(row),records=hn_read32(m,CONTEXT);
        unsigned column=control(m,0x181);
        for(unsigned i=0;i<110&&!m->fault;i++)hn_write8(m,RECEIVED+17+i,0);
        if(column<=3)hn_memory_copy(m,RECEIVED,records+row*1100+column*220,128);
        else if(column==5)hn_memory_copy(m,RECEIVED+17,hn_read32(m,entry+8),hn_read8(m,entry+0x16));
        break;
    }
    case 0x080C8E98:{
        GameAddr entry=resource(control(m,4));store(m,0x60,1);
        unsigned count=hn_read8(m,entry+0x16);
        for(unsigned i=0;i<count&&!m->fault;i++){
            unsigned value=hn_read8(m,RECEIVED+17+i);
            if(value>1){unsigned flag=value+0x45;
                if(!(hn_read8(m,0x03002180+(flag>>3))&(1u<<(flag&7)))){store(m,0x60,0);break;}}
        }
        break;
    }
    case 0x080C6EDC:classify(m);break;
    case 0x080C8EF0:{
        GameAddr slot=hn_render_slot(m,39),records=hn_read32(m,CONTEXT);
        for(unsigned i=0;i<4;i++)hn_write8(m,slot+i*104+0x60,hn_read8(m,records+(hn_read8(m,0x020046A0)*5+i)*220)==3);break;
    }
    case 0x080C8F48:{
        GameAddr slot=hn_render_slot(m,39),records=hn_read32(m,CONTEXT)+control(m,0x180)*5*220;unsigned column=control(m,0x181);
        records+=column*220;slot+=column*104;
        if(hn_read8(m,records)==3)hn_render_set_animation_pointer(m,slot,hn_read32(m,0x020021E8+column*4));break;
    }
    case 0x080C8FA8:{
        GameAddr slot=hn_render_slot(m,39);
        if(hn_read8(m,RECEIVED)==3){unsigned column=control(m,0x181);slot+=column*104;
            hn_render_set_animation_pointer(m,slot,hn_read32(m,0x020021F8+column*4));hn_write8(m,slot+0x60,1);}
        break;
    }
    default:return hn_group4_ui_callback(m,callback,arguments,consumed);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_group4_task(HNMemory *m,GameAddr callback)
{
    if((callback&~1u)!=0x080C9000)return HN_SCENE_UNKNOWN;
    unsigned index=0,end=2;GameAddr positions;
    switch(control(m,0x185)){
    case 0:positions=0x086E8402;if(flag(m,0x1E9)){index=2;end=4;}break;
    case 1:positions=0x086E8412;break;
    case 2:positions=0x086E841A;break;
    case 3:positions=0x086E83EA;end=6;break;
    case 4:positions=0x086E8422;break;
    default:positions=0x086E842A;break;
    }
    for(;index<end;index++){
        GameAddr slot=hn_render_slot(m,hn_read8(m,0x086E83E4+index));
        hn_write16(m,slot+0x3C,0x100);hn_write16(m,slot+0x3E,0x100);hn_write16(m,slot+0x40,0);
        hn_write32(m,slot+0x18,(uint32_t)hn_read16(m,positions)<<16);positions+=2;hn_write32(m,slot+0x1C,0x8000);
        hn_write32(m,slot+0x20,(uint32_t)hn_read16(m,positions)<<16);positions+=2;
        for(unsigned offset=0x24;offset<=0x38;offset+=4)hn_write32(m,slot+offset,0);
        hn_write16(m,slot+0x44,0);hn_write16(m,slot+0x46,0);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
