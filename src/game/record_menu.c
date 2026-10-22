#include "game/record_menu.h"
#include "game/state.h"
#include "game/render.h"
#include "game/transfer.h"
#include "game/task.h"
#include "game/background.h"
#include "gba/bios.h"

enum { DESCRIPTORS=0x0858D0D8,COUNTERS=0x0201CC20,LIST=0x0201EE40,
       TEXT_TILES=0x02004C36,CONTEXTS=0x030037A0,DIGITS=0x086E7F54,
       MARKER_COORDS=0x086E7F94,MARKER_ANIMATIONS=0x086E7F7C,TEXT_BUNDLES=0x086E7FAC,
       DETAIL_BUFFER=0x02005720,EMPTY_ANIMATION=0x0889F620 };
static uint8_t control(HNMemory *m,unsigned index){return hn_read8(m,HN_SCENE_CONTROLS+index);}
static void store(HNMemory *m,unsigned index,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+index,(uint8_t)value);}
static GameAddr resource(HNMemory *m,GameAddr table,uint32_t index){return hn_read32(m,table+index*4);}
static void priority(HNMemory *m,uint32_t index,uint8_t value){hn_render_flags(m,index,12,(uint8_t)(value<<2));}
static void animation(HNMemory *m,uint8_t index,GameAddr value)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,value);
}
static void hide(HNMemory *m,uint8_t index){hn_write8(m,hn_render_slot(m,index)+0x60,0);}
static void clear_marker(HNMemory *m,uint8_t index)
{if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);hide(m,index);hn_render_set_animation(m,index,0);priority(m,index,0);}
static void flag(HNMemory *m,unsigned id,bool enabled)
{
    GameAddr address=0x03002180+(id>>3);uint8_t mask=(uint8_t)(1u<<(id&7));
    hn_write8(m,address,(uint8_t)((hn_read8(m,address)&~mask)|(enabled?mask:0)));
}
static bool slot(HNMemory *m,uint8_t index,uint8_t mode,int32_t x,int32_t y,int32_t z,GameAddr image)
{return hn_render_configure_raw(m,index,1,mode,x,y,z,image);}
static bool rows(HNMemory *m,unsigned kind)
{
    uint8_t record=(uint8_t)(control(m,0x180)-control(m,0x181)-(kind==2));
    if(!kind&&(record&1)&&!hn_background_configure_raw(m,5,0x82,0x088FA302))return false;
    GameAddr table=kind==0?0x086E7DEC:kind==1?0x086E7E64:0x086E7EDC;
    GameAddr empty=kind==0?0x088F9C60:kind==1?0x088F9C68:0x088F9C7C;
    for(unsigned i=0;i<(kind?7u:6u);i++){
        if(record>79)record=(uint8_t)(record+30);if(record>29)record=(uint8_t)(record-30);
        uint32_t next=(uint32_t)record+1;uint8_t value=hn_read8(m,COUNTERS+next);
        GameAddr image=value!=255?resource(m,table,record):empty+((record&1)?48:0);
        if(!slot(m,(uint8_t)(20+i),0x71,116,0,(int16_t)(i*24+(kind==2?-12:12)),image))return false;
        record=(uint8_t)next;
    }
    return !m->fault;
}
static bool load_detail(HNMemory *m,GameAddr descriptor)
{
    GameAddr source=hn_read32(m,descriptor+12);
    return hn_transfer_decompress_rows(m,source,DETAIL_BUFFER,256,8,1024);
}
bool hn_record_menu_load_list(HNMemory *m)
{
    uint32_t index=(control(m,0x180)&128)?255:control(m,0x180);uint8_t entry=hn_read8(m,LIST+index);
    if(entry&&index!=255){
        if(!hn_transfer_decompress_rows(m,hn_read32(m,0x0858D344+entry*24+16),DETAIL_BUFFER,256,8,1024))return false;
        entry=hn_read8(m,LIST+index);
        if(!hn_bios_lz77(m,hn_read32(m,0x0858D344+entry*24+20),DETAIL_BUFFER-4096,0x40000)||
           !slot(m,12,0x72,12,0,8,0x088FA387))return false;
        priority(m,12,2);
    }else{if(!slot(m,12,0x72,12,0,8,EMPTY_ANIMATION))return false;priority(m,12,3);}
    return !m->fault;
}
static bool navigate(HNMemory *m,uint8_t direction)
{
    hn_write16(m,0x03001FBC,0);flag(m,255,hn_read8(m,COUNTERS+(uint32_t)control(m,0x180)+1)!=255);
    if(!direction){
        if(!control(m,0x180))store(m,0x180,30);
        if(!control(m,0x181)){
            if(!hn_background_configure_raw(m,5,0x82,(control(m,0x180)&1)?0x088FA2F3:0x088FA2E4)||!rows(m,2))return false;
            animation(m,12,0x088F994A);
        }else{animation(m,0,0x088F98DD);store(m,0x181,control(m,0x181)-1);}
        if(control(m,0x180)==30){if(!slot(m,10,0x71,84,96,24,0x088F9905))return false;priority(m,10,0);}
        else animation(m,10,0x088F990D);
        store(m,0x180,control(m,0x180)-1);hn_write16(m,0x03001FBC,1);
    }else if(direction==1){
        uint32_t current=control(m,0x180);if(current==29){current=255;store(m,0x180,current);}
        if(current!=29){
            if(control(m,0x181)==5){
                if(!hn_background_configure_raw(m,5,0x82,(control(m,0x180)&1)?0x088FA2C6:0x088FA2D5)||!rows(m,1))return false;
                animation(m,11,0x088F993D);
            }else{animation(m,0,0x088F98F1);store(m,0x181,control(m,0x181)+1);}
            if(control(m,0x180)==255){if(!slot(m,10,0x71,84,0,24,0x088F9905))return false;priority(m,10,0);}
            else animation(m,10,0x088F9921);
            store(m,0x180,control(m,0x180)+1);hn_write16(m,0x03001FBC,1);
        }
    }
    if(hn_read16(m,0x03001FBC)==1)flag(m,256,hn_read8(m,COUNTERS+(uint32_t)control(m,0x180)+1)!=255);
    return !m->fault;
}
#define REQUIRE(operation) do {if(!(operation))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_record_menu_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080C4634:{
        uint32_t initial=control(m,0x8A);
        if(initial==255){store(m,0x180,0);store(m,0x181,0);store(m,0x60,0);}
        else{store(m,0x180,initial-1);store(m,0x181,initial<=25?0:initial-25);store(m,0x60,1);}break;
    }
    case 0x080C46A4:{
        hn_write16(m,0x03001FBC,(uint16_t)(control(m,0x180)+1));
        if(hn_read8(m,COUNTERS+(uint32_t)control(m,0x180)+1)!=255){
            REQUIRE(load_detail(m,DESCRIPTORS+((uint32_t)control(m,0x180)+1)*20));
            REQUIRE(hn_bios_lz77(m,hn_read32(m,DESCRIPTORS+((uint32_t)control(m,0x180)+1)*20+16),DETAIL_BUFFER-4096,0x40000));
            REQUIRE(slot(m,9,0x71,10,0,8,0x088F9994));priority(m,9,0);
        }else{REQUIRE(slot(m,9,0x72,10,0,8,EMPTY_ANIMATION));priority(m,9,3);}break;
    }
    case 0x080C4770:
        hn_write16(m,0x03001FBC,0);
        if(hn_read8(m,COUNTERS+(uint32_t)control(m,0x180)+1)!=255){
            hn_write16(m,0x03001FBC,1);
            for(unsigned i=0;i<5;i++)for(unsigned j=0;j<16;j++){
                hn_write16(m,TEXT_TILES+i*128+j*2,0x2C5);hn_write16(m,TEXT_TILES+i*128+j*2+64,0x2C5);
            }
            for(unsigned i=0;i<5;i++){
                GameAddr task=hn_task_allocate(m);hn_write32(m,CONTEXTS+i*4,task);REQUIRE(task);
                REQUIRE(hn_task_graphics(m,task,15,2,0,1,0,3,24,0x06005800+i*1024,TEXT_TILES+2+i*128));
                task=hn_read32(m,CONTEXTS+i*4);GameAddr text=resource(m,TEXT_BUNDLES,control(m,0x180)*5+i);
                if(text!=HN_TASK_DEFAULT_SCRIPT)text=resource(m,text,control(m,0xEB));REQUIRE(hn_task_set_script(m,task,text));
            }
        }break;
    case 0x080C48A8:
        if(hn_read8(m,COUNTERS+(uint32_t)control(m,0x180)+1)!=255)
            for(unsigned count=5;count;count--)REQUIRE(hn_task_remove(m,hn_read32(m,CONTEXTS+(count-1)*4)));
        break;
    case 0x080C48EC:{
        hn_write8(m,HN_DISPLAY_SHADOW+1,hn_read8(m,HN_DISPLAY_SHADOW+1)|0x7F);
        hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|1);
        hn_write16(m,0x04000048,hn_read8(m,arguments)?0x1B0C:0x0C1B);hn_write16(m,0x0400004A,31);
        hn_write16(m,0x04000040,0x50E0);hn_write16(m,0x04000044,160);hn_write16(m,0x04000042,0x50E0);hn_write16(m,0x04000046,160);
        hn_write16(m,HN_RASTER_SCANLINE,0x50E0);hn_write32(m,0x040000B0,HN_RASTER_SCANLINE);hn_write32(m,0x040000B4,0x04000040);
        hn_write16(m,0x040000B8,1);hn_write16(m,0x040000BA,0x9340);*consumed=1;break;
    }
    case 0x080C4990:{
        uint32_t value=hn_read16(m,HN_RASTER_SCANLINE)+0x400;hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)value);store(m,0x60,0);
        if(((uint16_t)value>>8)>223){hn_write32(m,0x040000B0,0);hn_write32(m,0x040000B4,0);hn_write32(m,0x040000B8,0);store(m,0x60,1);}break;
    }
    case 0x080C49D4:{
        const uint8_t hidden[]={0,10,11,12,13,14,20,21,22,23,24,25,26};
        for(unsigned i=0;i<sizeof(hidden);i++)hide(m,hidden[i]);
        uint32_t mask=hn_read8(m,DESCRIPTORS+((uint32_t)control(m,0x180)+1)*20+6);
        for(unsigned i=0;i<6;i++,mask>>=1)if(mask&1){
            REQUIRE(slot(m,(uint8_t)(i+10),0x71,hn_read8(m,MARKER_COORDS+i*4),0,hn_read8(m,MARKER_COORDS+i*4+1),0x088F9BC3));
            REQUIRE(slot(m,(uint8_t)(i+16),0x71,hn_read8(m,MARKER_COORDS+i*4+2),0,hn_read8(m,MARKER_COORDS+i*4+3),resource(m,MARKER_ANIMATIONS,i)));
        }break;
    }
    case 0x080C4AF0:
        for(unsigned i=0;i<6;i++)animation(m,(uint8_t)(i+10),0x088F9BEA);
        animation(m,16,0x088F9A9D);animation(m,17,0x088F9B37);animation(m,18,0x088F9A50);
        animation(m,19,0x088F9AEA);animation(m,20,0x088F9A03);animation(m,21,0x088F9B84);break;
    case 0x080C4B60:
        REQUIRE(slot(m,0,0x71,107,control(m,0x181)*24,9,0x088F98D0));priority(m,0,0);
        REQUIRE(slot(m,10,0x71,84,((uint32_t)control(m,0x180)*0x34F70)>>16,24,0x088F9905));priority(m,10,0);
        REQUIRE(slot(m,11,0x71,148,0,0,0x088F993D));priority(m,11,0);
        REQUIRE(slot(m,12,0x71,148,0,152,0x088F994A));priority(m,12,0);
        REQUIRE(slot(m,13,0x71,108,0,0,0x088F9C08));priority(m,13,1);
        REQUIRE(slot(m,14,0x71,108,0,152,0x088F9C08));priority(m,14,1);break;
    case 0x080C4CBC:REQUIRE(navigate(m,hn_read8(m,arguments)));*consumed=1;break;
    case 0x080C4EF4:{
        uint8_t value=hn_read8(m,COUNTERS+(uint32_t)control(m,0x180)+1);store(m,0x61,value);unsigned i=0;
        GameAddr tens=resource(m,DIGITS,value==255?0:value/10);
        REQUIRE(slot(m,2,0x71,8,0,80,tens));priority(m,2,0);
        REQUIRE(slot(m,3,0x71,16,0,80,value==255?tens:resource(m,DIGITS,value%10)));priority(m,3,0);
        if(value!=255){
            while(i<hn_read8(m,DESCRIPTORS+((uint32_t)control(m,0x180)+1)*20+4)){
                REQUIRE(slot(m,(uint8_t)(i+4),0x71,(int16_t)(i*12+9),0,107,0x088F9984));priority(m,(uint8_t)(i+4),0);
                i=(uint8_t)(i+1);if(!i)return HN_SCENE_RESOURCE;
            }
        }
        for(;i<=4;i++)clear_marker(m,(uint8_t)(i+4));
        hn_write16(m,0x03001FBC,value==255?0:(uint16_t)(control(m,0x180)+1));break;
    }
    case 0x080C50A8:REQUIRE(rows(m,0));break;
    case 0x080C51B4:REQUIRE(rows(m,1));break;
    case 0x080C52A8:REQUIRE(rows(m,2));break;
    case 0x080C53A0:{
        REQUIRE(slot(m,9,0x72,10,0,8,EMPTY_ANIMATION));priority(m,9,3);
        for(unsigned i=0;i<5;i++)clear_marker(m,(uint8_t)(i+4));
        uint32_t parity=control(m,0x180)&1;
        REQUIRE(slot(m,(uint8_t)(control(m,0x181)+20),0x71,116,0,(int16_t)(control(m,0x181)*24+12),parity?0x088F9C90:0x088F9C60));break;
    }
    case 0x080C5480:{
        unsigned i=0;while(i<hn_read8(m,DESCRIPTORS+((uint32_t)control(m,0x180)+1)*20+4)){
            REQUIRE(slot(m,(uint8_t)(i+4),0x71,(int16_t)(i*12+9),0,107,resource(m,0x086E8204,i)));priority(m,(uint8_t)(i+4),0);
            if(i>3)break;i++;
        }break;
    }
    case 0x080C5510:
        if(control(m,0x8A)==255){store(m,0x180,0);REQUIRE(slot(m,21,0x72,5,0,79,0x088FA30C));store(m,0x60,0);}
        else{
            unsigned i=0;if(hn_read8(m,LIST))do{i++;if(i>29)break;}while(hn_read8(m,LIST+i));
            store(m,0x180,i-1);uint32_t current=control(m,0x180);
            REQUIRE(slot(m,21,0x72,(current%10)*22+5,0,(int32_t)(current/10)*20+79,0x088FA30C));store(m,0x60,1);
        }break;
    case 0x080C55E8:REQUIRE(hn_record_menu_load_list(m));break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_record_menu_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080C4634:case 0x080C46A4:case 0x080C4770:case 0x080C48A8:case 0x080C4990:
    case 0x080C49D4:case 0x080C4AF0:case 0x080C4B60:case 0x080C4EF4:case 0x080C50A8:
    case 0x080C51B4:case 0x080C52A8:case 0x080C53A0:case 0x080C5480:case 0x080C5510:case 0x080C55E8:{
        uint32_t ignored;return hn_record_menu_callback(m,callback,0,&ignored);
    }
    default:return HN_SCENE_UNKNOWN;
    }
}
