#include "game/presentation.h"
#include "game/state.h"
#include "game/render.h"
#include "game/task.h"

enum { STYLES=0x086E985C,STAGES=0x086E987C,SCRIPTS=0x086E9910,SEQUENCES=0x086E9B24,HANDLES=0x03003D10,TILES=0x02010740 };
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static bool configure(HNMemory *m,uint8_t index,int32_t x,int32_t z,GameAddr image)
{return hn_render_configure_raw(m,index,1,114,x,0,z,image);}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_presentation_window_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080CEAB0:{
        uint32_t selection=control(m,0x60),remaining=15-selection*2;
        hn_write16(m,0x04000044,(uint16_t)(((16-selection*2)<<11)+138));int32_t baseline=(int32_t)((remaining<<19)+0xFFFE0000)>>16;
        REQUIRE(configure(m,16,168,baseline,0x088FCDAB));REQUIRE(configure(m,17,208,baseline,0x088FCE1B));REQUIRE(configure(m,18,184,baseline,0x088FCCDE));
        REQUIRE(configure(m,19,184,(int32_t)(remaining*8),0x088FCCD4));REQUIRE(configure(m,20,184,128,0x088FCCCA));
        const GameAddr images[]={0x088FCCBB,0x088FCCB6,0x088FCCB1,0x088FCCAC,0x088FCCA7,0x088FCC9D,0x088FCC98,0x088FCC93,0x088FCC8E,0x088FCC89};
        for(unsigned i=0;i<10;i++)REQUIRE(configure(m,(uint8_t)(22+i),192,132,images[i]));
        for(unsigned i=22;i<=31;i++)hn_render_flags(m,i,12,0);break;
    }
    case 0x080CEC70:break;
    case 0x080CEC74:{
        GameAddr styles[8];for(unsigned i=0;i<8;i++)styles[i]=hn_read32(m,STYLES+i*4);
        uint8_t first=hn_read8(m,arguments),style=hn_read8(m,arguments+1),mask=hn_read8(m,arguments+2);
        /* Only the eight pointers copied by the original stack frame are defined. */
        if(style>=8)return HN_SCENE_RESOURCE;
        hn_write8(m,HN_DISPLAY_SHADOW+1,hn_read8(m,HN_DISPLAY_SHADOW+1)|128);hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|1);
        hn_write16(m,0x04000000,hn_read16(m,0x04000000)|0x8000);hn_write16(m,0x0400004A,(uint16_t)mask<<8);
        for(unsigned row=0;row<5;row++)for(unsigned column=0;column<8;column++){
            uint8_t index=(uint8_t)(column+row*8+first);REQUIRE(hn_render_configure_raw(m,index,1,114,(int32_t)column*32-8,(int32_t)row*32,0,styles[style]));hn_render_flags(m,index,32,0);
        }
        *consumed=3;break;
    }
    case 0x080CED40:{
        const GameAddr images[]={0x088FD138,0x088FD367,0x088FD375,0x088FD383,0x088FD391,0x088FD39F,0x088FD3AD};
        for(unsigned i=0;i<7;i++)REQUIRE(configure(m,(uint8_t)(17+i),16+(int32_t)i*32,208,images[i]));
        for(unsigned i=17;i<=23;i++)hn_render_flags(m,i,32,0);break;
    }
    case 0x080CF044:{
        GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);unsigned value=hn_read8(m,effect+5)|(hn_read8(m,effect+6)<<8);
        if(value<=40){
            hn_write8(m,effect+5,40);hn_write8(m,effect+6,0);hn_write8(m,hn_read32(m,HN_RASTER_EFFECT)+9,0);
            effect=hn_read32(m,HN_RASTER_EFFECT);hn_write8(m,effect+10,0);hn_write8(m,effect+11,0);
            effect=hn_read32(m,HN_RASTER_EFFECT);hn_write8(m,effect+13,0);hn_write8(m,effect+14,0);
        }break;
    }
    case 0x080CF078:{
        GameAddr stage=STAGES+control(m,255)*4;
        if(hn_read16(m,0x0201DC40+82)>=hn_read16(m,stage)){
            unsigned command=hn_read16(m,stage+2);
            if(command==1){store(m,0x68,command);store(m,0x69,2);}
            else if(command==3){store(m,0x68,11);store(m,0x67,control(m,0x67)+1);}
            else if(command==6)store(m,0x68,12);
            store(m,255,control(m,255)+1);
        }break;
    }
    case 0x080CF0F4:{
        uint8_t delay=(uint8_t)(control(m,0x69)-1);store(m,0x69,delay);if(delay)break;
        uint8_t step=control(m,0x68),row=(uint8_t)(step-1);
        unsigned style=hn_read8(m,SEQUENCES+(uint32_t)control(m,0x67)*7-1+step);
        if(!style){store(m,0x68,7);break;}
        GameAddr task=hn_task_allocate(m),handle=HANDLES+row*4;hn_write32(m,handle,task);
        uint32_t column=(uint32_t)control(m,0x68)-1;
        if(hn_read32(m,SCRIPTS+(column+control(m,0x67)*7)*4)){
            REQUIRE(task&&hn_task_graphics(m,task,14,2,255,1,delay,3,8,0x06000040+row*7*128,TILES+style*64));
            column=(uint32_t)control(m,0x68)-1;REQUIRE(hn_task_set_script(m,hn_read32(m,handle),hn_read32(m,SCRIPTS+(column+control(m,0x67)*7)*4)));
        }
        store(m,0x68,control(m,0x68)+1);break;
    }
    case 0x080CF1E8:{uint8_t index=(uint8_t)(control(m,0x68)-2);REQUIRE(hn_task_remove(m,hn_read32(m,HANDLES+index*4)));store(m,0x69,2);break;}
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_presentation_window_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080CEAB0:case 0x080CEC70:case 0x080CED40:case 0x080CF044:case 0x080CF078:case 0x080CF0F4:case 0x080CF1E8:{uint32_t ignored;return hn_presentation_window_callback(m,callback,0,&ignored);}
    default:return HN_SCENE_UNKNOWN;
    }
}
