#include "game/presentation.h"
#include "game/state.h"
#include "game/render.h"
#include "game/video.h"
#include "game/random.h"
#include "game/task.h"
#include "game/transfer.h"

enum { CLASSES=0x0201DC40,INDICES=0x086E9696,TIMERS=0x030037D0,OFFSETS=0x03003DD0,ANIMATIONS=0x02002A40,TILES=0x02013F20 };
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static void velocity(HNMemory *m,uint32_t index,uint32_t x,uint32_t y,uint32_t z)
{GameAddr slot=hn_read32(m,HN_RENDER_SLOTS)+index*104;hn_write32(m,slot+0x24,x);hn_write32(m,slot+0x28,y);hn_write32(m,slot+0x2C,z);}
static bool free_slot(HNMemory *m,uint8_t *slot,unsigned *record)
{for(*record=0;*record<5;(*record)++){*slot=hn_read8(m,INDICES+*record*6);if(!hn_read8(m,hn_render_slot(m,*slot)+0x60))return true;}return false;}
bool hn_presentation_scroll_row(HNMemory *m,GameAddr descriptor,uint32_t bank_word,uint32_t position_word)
{
    uint8_t bank=(uint8_t)bank_word;uint16_t position=(uint16_t)position_word,tile=(uint16_t)((bank<<6)+2);
    unsigned column=(position>>3)&31,page=(position&256)>>8,first=32-column,second=column;
    for(unsigned i=0;i<first;i++){hn_write16(m,TILES+i*2,tile);hn_write16(m,TILES+(i+first)*2,(uint16_t)(tile+32));tile++;}
    if(second){
        tile=(uint16_t)((bank<<6)+2+first);
        for(unsigned i=0;i<second;i++){hn_write16(m,TILES+128+i*2,tile);hn_write16(m,TILES+128+(i+second)*2,(uint16_t)(tile+32));tile++;}
    }
    hn_transfer_tilemap_rows(m,TILES,0x0600C040+(page<<11)+(column<<1),first,2,0);
    if(second)hn_transfer_tilemap_rows(m,TILES+128,0x0600C040+((page^1)<<11),second,2,0);
    GameAddr task=hn_task_allocate(m);hn_write32(m,0x03003E40+bank*4,task);
    if(!task||!hn_task_graphics(m,task,32,2,0,1,2,3,0,0x06000040+((uint32_t)bank<<11),TILES-0x3800))return false;
    return hn_task_set_script(m,hn_read32(m,0x03003E40+bank*4),hn_read32(m,descriptor))&&!m->fault;
}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_presentation_background_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080CBE54:{
        uint8_t slot;unsigned record;if(!free_slot(m,&slot,&record))break;
        uint8_t direction=hn_read8(m,0x086E9700+control(m,0x69));
        REQUIRE(hn_render_configure_raw(m,slot,direction,114,104,73,-1,hn_read32(m,ANIMATIONS+record*4)));
        GameAddr vectors=0x086E96C0+((uint32_t)direction-1)*8;velocity(m,slot,hn_read32(m,vectors),hn_read32(m,vectors+4),0);
        hn_render_flags(m,slot,12,12);hn_write8(m,0x03003CC8,hn_read8(m,0x03003CC8)^1);break;
    }
    case 0x080CC0D0:case 0x080CC198:{
        GameAddr parent=hn_render_slot(m,19);uint8_t slot;unsigned record;if(!free_slot(m,&slot,&record))break;
        bool right=(callback&~1u)==0x080CC0D0;unsigned c=control(m,0x69);int16_t x=(int16_t)hn_read16(m,parent+0x1A);
        x=(int16_t)(x+(int)(c&1)*4+(right?20:-16));int16_t y=(int16_t)(hn_read16(m,parent+0x1E)+15);
        GameAddr image=hn_read32(m,ANIMATIONS+(right?9+(c%5)*2+1:19+c%5)*4);
        REQUIRE(hn_render_configure_raw(m,slot,1,114,x,y,-1,image));velocity(m,slot,right?0xB000:0u-0x14000,0x20000,0);break;
    }
    case 0x080CC328:{
        uint8_t c=control(m,0x69),index=(uint8_t)(c+26);int16_t x=(int16_t)hn_read16(m,0x086E9714+c*2);
        REQUIRE(hn_render_configure_raw(m,index,1,114,x,128,0,hn_read32(m,ANIMATIONS+(24+c%5)*4)));
        velocity(m,control(m,0x69)+26,0,0u-0x8000,0);break;
    }
    case 0x080CC39C:{GameAddr slot=hn_render_slot(m,hn_read8(m,arguments));uint32_t value=hn_read32(m,arguments+1);hn_write32(m,slot+0x34,value);*consumed=5;break;}
    case 0x080CC3C8:{
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS),slot=base+25*104;hn_write32(m,slot+0x34,0u-0x400);
        for(unsigned i=0;i<3;i++)hn_write32(m,slot+0x24+i*4,0);
        slot=base+10*104;hn_write32(m,slot+0x28,0xB000);hn_write32(m,slot+0x24,0u-0x6000);hn_write16(m,slot+0x44,2);hn_write16(m,slot+0x46,2);
        hn_write8(m,slot+0x64,hn_render_find_affine(m));hn_write8(m,HN_AFFINE_USAGE+hn_read8(m,slot+0x64),1);
        slot=hn_render_slot(m,13);hn_write32(m,slot+0x28,0xB000);hn_write32(m,slot+0x24,0x1800);hn_write16(m,slot+0x44,2);hn_write16(m,slot+0x46,2);
        hn_write8(m,slot+0x64,hn_render_find_affine(m));hn_write8(m,HN_AFFINE_USAGE+hn_read8(m,slot+0x64),1);break;
    }
    case 0x080CC484:{uint8_t index=hn_read8(m,0x03003580);hn_write8(m,0x03003580,(uint8_t)(index+1));hn_write16(m,0x03003E50+index*2,hn_read16(m,CLASSES+70));break;}
    case 0x080CC4AC:{unsigned index=0,value=hn_read16(m,CLASSES+70);if(value>=hn_read16(m,0x086E966C)){do{index++;if(index>18)break;}while(hn_read16(m,CLASSES+70)>=hn_read16(m,0x086E966C+index*2));}store(m,0x62,index);break;}
    case 0x080CBD90:{
        unsigned position=hn_read16(m,CLASSES+70),column=position>>3;
        if(column>31&&!(position&7)){uint16_t offset=(uint16_t)(column-32);hn_write16(m,TILES+256,1);hn_write16(m,TILES+258,1);
            hn_transfer_tilemap_rows(m,TILES+256,0x0600C040+(((offset&32)>>5)<<11)+((offset&31)<<1),1,2,0);}break;
    }
    case 0x080CBDF4:{
        GameAddr row=0x086E94CC+control(m,0x61)*8;
        if(hn_read16(m,CLASSES+70)>=hn_read16(m,row+4)){
            REQUIRE(hn_presentation_scroll_row(m,row,control(m,0x60),hn_read16(m,CLASSES+70)));
            uint8_t bank=(uint8_t)(control(m,0x60)+1);store(m,0x60,bank);if(bank>2)store(m,0x60,0);store(m,0x61,control(m,0x61)+1);
        }
        hn_write16(m,HN_VIDEO_STATE+0x3A,(uint16_t)(hn_read16(m,HN_VIDEO_STATE+0x3A)-1));hn_video_mark_backgrounds(m,1);break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_presentation_background_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080CBE54:case 0x080CC0D0:case 0x080CC198:case 0x080CC328:case 0x080CC3C8:case 0x080CC484:case 0x080CC4AC:case 0x080CBD90:case 0x080CBDF4:{uint32_t ignored;return hn_presentation_background_callback(m,callback,0,&ignored);}
    default:return HN_SCENE_UNKNOWN;
    }
}
HNSceneResult hn_presentation_background_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080CBF1C:case 0x080CC264:{
        uint8_t index=hn_read8(m,arguments);bool five=(callback&~1u)==0x080CBF1C;GameAddr timers=TIMERS+index*5,offsets=OFFSETS+index;
        for(unsigned i=0;i<(five?5u:4u);i++){hn_write8(m,timers+i,hn_read8(m,0x086E96B4+(five?0:5)+i));if(five)hn_write8(m,offsets+i,0);}break;
    }
    case 0x080CBF64:case 0x080CC29C:{
        uint8_t index=hn_read8(m,arguments);bool five=(callback&~1u)==0x080CBF64;GameAddr timers=TIMERS+index*5,offset=OFFSETS+index,children=INDICES+index*6+1;
        for(unsigned i=0;i<(five?5u:4u);i++,timers++,children++){
            uint8_t timer=hn_read8(m,timers);
            if(timer){hn_write8(m,timers,(uint8_t)(timer-1));continue;}
            hn_write8(m,timers,five?32:18);uint16_t x,y;GameAddr image;
            if(five){
                uint16_t random_x=(uint16_t)(hn_random_range(m,0,3)<<2),random_y=(uint16_t)(hn_random_range(m,0,3)<<2);
                x=hn_read16(m,slot+0x1A);y=hn_read16(m,slot+0x1E);uint8_t extra=hn_read8(m,offset);
                switch(hn_read8(m,slot+0x60)){
                case 1:x=(uint16_t)(x+random_x+extra);y=(uint16_t)(y+24);break;
                case 3:y=(uint16_t)(y+random_y+extra);break;
                case 5:x=(uint16_t)(x+random_x+extra);break;
                case 2:case 6:x=(uint16_t)(x+random_x+extra);y=(uint16_t)(y+random_x+extra);break;
                case 7:x=(uint16_t)(x+24);y=(uint16_t)(y+random_y+extra);break;
                case 4:case 8:x=(uint16_t)(x+random_x+extra);y=(uint16_t)((int16_t)y-((int)random_x+extra-20));break;
                }
                image=0x088FC348;
            }else{x=hn_read16(m,slot+0x1A);y=hn_read16(m,slot+0x1E);image=hn_read32(m,ANIMATIONS+(5+i)*4);}
            REQUIRE(hn_render_configure_raw(m,hn_read8(m,children),1,114,(int16_t)x,(int16_t)(y+1),-1,image));
            if(five){hn_render_flags(m,hn_read8(m,children),12,12);hn_write8(m,offset,hn_read8(m,offset)^12);}
        }break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    *consumed=1;*yield=0;return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
