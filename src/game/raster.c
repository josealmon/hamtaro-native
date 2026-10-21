#include "game/raster.h"
#include "game/state.h"
#include "game/heap.h"
#include "game/input.h"
#include "game/render.h"
#include "game/video.h"

static void dma_stop(HNMemory *m)
{
    hn_write32(m,0x040000B8,0x8000);hn_write32(m,0x040000B0,0);
    hn_write32(m,0x040000B4,0);hn_write32(m,0x040000B8,0);
}
static void free_effect(HNMemory *m)
{
    GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);
    if(effect){hn_heap_free(m,effect);hn_write32(m,HN_RASTER_EFFECT,0);}
}
static void display_dirty(HNMemory *m)
{hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|1);}
static void restore_layers(HNMemory *m,GameAddr effect)
{
    unsigned display=hn_read8(m,HN_DISPLAY_SHADOW+1),window=hn_read8(m,effect)?64:32;
    display&=~window;
    hn_write8(m,HN_DISPLAY_SHADOW+1,(uint8_t)((display&0xE0)|(hn_read8(m,effect+2)&31)));display_dirty(m);
}
bool hn_raster_profile(HNMemory *m)
{
    GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);if(!effect)return false;
    uint16_t phase=(uint16_t)(hn_read16(m,effect+13)+hn_read16(m,effect+10));
    hn_write16(m,effect+13,phase);
    unsigned increment=hn_read8(m,effect+9);if(increment&128)increment|=0xFF00;
    unsigned fraction=phase>>8,radius=(uint16_t)(hn_read16(m,effect+5)+increment+fraction);
    if(increment&0xFF00){if((int32_t)radius<(int32_t)(0xFFFFu-(increment+fraction)))radius=0;}
    else if(radius>240)radius=240;
    /* Preserve the negative route's wrapping radius using guest bus writes. */
    if(radius){
        unsigned x=0,y=radius;uint16_t decision=(uint16_t)(3-radius*2);
        do{
            hn_write8(m,effect+17+radius-y,(uint8_t)x);
            hn_write8(m,effect+17+radius-x,(uint8_t)y);
            int d=(int16_t)decision;
            if(d<0)decision=(uint16_t)(d+(int)(x*4)+6);
            else{
                decision=(uint16_t)(d+((int)x-(int)y)*4+10);
                y=(uint16_t)(y-1);if(!y)break;
            }
            x=(uint16_t)(x+1);
        }while(x<=y);
    }
    hn_write16(m,effect+5,(uint16_t)radius);return !m->fault;
}
bool hn_raster_position(HNMemory *m,uint8_t source)
{
    GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);if(!effect)return false;
    bool alternate=(hn_read8(m,0x030021AB)&8)!=0;unsigned slot=alternate?13:10;
    uint16_t x,y;
    if(source==0xFE){x=hn_read16(m,0x0201DC84);y=hn_read16(m,0x0201DC86);}
    else if(source==0xFD||source==0xFF){
        if(slot>=hn_read8(m,HN_RENDER_INFO+0x10))return false;
        GameAddr s=hn_render_slot(m,slot);x=hn_read16(m,s+0x1A);y=hn_read16(m,s+0x22);
    }else return false;
    x=(uint16_t)(x+hn_read16(m,HN_VIDEO_STATE+0x6A)+20);
    y=(uint16_t)(y+hn_read16(m,HN_VIDEO_STATE+0x6C)+20);
    if(source==0xFD){
        uint16_t distance=hn_read16(m,alternate?0x0201DC96:0x0201DC94);
        if(hn_input_query(m,2,0))distance=(uint16_t)(distance*2);
        unsigned direction=hn_read8(m,hn_render_slot(m,slot)+0x60);
        if(direction==1)y=(uint16_t)(y+distance);else if(direction==2)y=(uint16_t)(y-distance);
        else if(direction==3)x=(uint16_t)(x-distance);else if(direction==4)x=(uint16_t)(x+distance);
    }
    hn_write8(m,effect+3,(uint8_t)x);hn_write8(m,effect+4,(uint8_t)y);return !m->fault;
}
static bool circle_window(HNMemory *m)
{
    GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);if(!effect)return false;
    if(hn_read16(m,0x04000006)<160){
        if(hn_read16(m,0x04000006)==158)hn_write8(m,effect+12,0);
        unsigned center=hn_read8(m,effect+4),radius=hn_read8(m,effect+5),current=hn_read8(m,effect+12);
        if((int)center-(int)radius<(int)current&&center+radius>current){
            unsigned distance=(center<current?current-center:center-current)&255;
            unsigned extent=hn_read8(m,effect+hn_read16(m,effect+5)-distance+16);
            unsigned x=hn_read8(m,effect+3),left=x<extent?0:x-extent,right=x+extent;
            if(right>239)right=240;
            hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)(((left&255)<<8)+(right&255)));
        }else hn_write16(m,HN_RASTER_SCANLINE,0);
        hn_write8(m,effect+12,(uint8_t)(current+1));
    }
    if(hn_read16(m,0x04000006)==160){
        unsigned limit=hn_read8(m,effect+7),increment=hn_read8(m,effect+9);
        if(limit){
            unsigned radius=hn_read8(m,effect+5);
            if(((increment&128)&&radius<=limit)||(((increment-1)&255)<=126&&radius>=limit)){
                if(!(increment&128))hn_write16(m,hn_read8(m,effect)?0x04000042:0x04000040,240);
                restore_layers(m,effect);hn_video_hblank(m,0);dma_stop(m);free_effect(m);
                hn_write8(m,HN_SCENE_CONTROLS+0x60,1);
            }else return hn_raster_profile(m);
        }
    }
    return !m->fault;
}
bool hn_raster_hblank(HNMemory *m)
{
    unsigned selector=hn_read8(m,HN_HBLANK_SELECTOR);
    if(!selector){hn_write16(m,0x04000004,hn_read16(m,0x04000004)&0xFFEF);return !m->fault;}
    if(selector>=19)return false;
    GameAddr callback=hn_read32(m,0x02000000+selector*4)&~1u;
    if(callback==0x08000578||callback==0x080CEA30)return !m->fault;
    if(callback==0x0800F4FC)return circle_window(m);
    return false;
}
static bool begin(HNMemory *m,GameAddr state,GameAddr effect)
{
    unsigned kind=hn_read8(m,HN_RASTER_KIND),routine=kind+11,channel=0;
    if(kind==1||kind==2||kind==6){hn_write8(m,effect+5,0);hn_write8(m,effect+6,0);}
    else if(kind==3||kind==4||kind==7){for(unsigned i=6;i<=8;i++)hn_write8(m,effect+i,0);}
    else if(kind==5){
        for(unsigned i=12;i<=14;i++)hn_write8(m,effect+i,0);if(!hn_raster_profile(m))return false;
    }else if(kind==8){
        hn_write8(m,state,0xAE);unsigned axis=hn_read8(m,effect+4);
        hn_write8(m,effect+5,(hn_read8(m,effect+3)&128)?(axis?80:120):0);
        hn_write16(m,axis?0x04000040:0x04000044,(uint16_t)(axis?240:160));
        routine=0;channel=axis?2:0;
    }else return false;
    if(hn_read8(m,effect))channel++;
    hn_write32(m,0x040000B0,HN_RASTER_SCANLINE);hn_write32(m,0x040000B4,0x04000040+channel*2);
    hn_write16(m,0x040000B8,1);hn_write16(m,0x040000BA,(uint16_t)(kind==8?0x9340:0xA340));
    uint8_t display=hn_read8(m,HN_DISPLAY_SHADOW+1),window=hn_read8(m,effect)?64:32;
    unsigned layers=(display&31)|hn_read8(m,effect+1);
    hn_write8(m,HN_DISPLAY_SHADOW+1,(uint8_t)((display&0xE0)|window|(layers&31)));display_dirty(m);
    hn_video_hblank(m,routine);hn_write8(m,HN_SCENE_CONTROLS+0x60,0);return !m->fault;
}
static void sweep(HNMemory *m,GameAddr state,GameAddr effect)
{
    uint8_t phase=(uint8_t)(hn_read8(m,effect+5)+hn_read8(m,effect+3));hn_write8(m,effect+5,phase);
    unsigned center=hn_read8(m,effect+4)?80:120;
    uint8_t left=(uint8_t)((int)center-(int8_t)phase),right=(uint8_t)((int)center+(int8_t)phase);
    if((int8_t)left<0)left=0;if(right>center*2)right=(uint8_t)(center*2);
    if((int8_t)left>(int)center-1){hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)(center*257));left=0;}
    else hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)((left<<8)+right));
    if(left)return;
    restore_layers(m,effect);
    free_effect(m);dma_stop(m);hn_write8(m,HN_SCENE_CONTROLS+0x60,1);hn_write8(m,state,1);
}
bool hn_raster_command(HNMemory *m,GameAddr state,unsigned op,GameAddr data,uint32_t *running)
{
    GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);unsigned bytes=0;
    if(op==0xA7){
        unsigned kind=hn_read8(m,data),size;
        hn_write8(m,HN_RASTER_KIND,(uint8_t)kind);free_effect(m);
        if(kind==1||kind==2||kind==6)size=7;else if(kind==3||kind==4||kind==7)size=9;
        else if(kind==5)size=0x102;else if(kind==8)size=6;else return false;
        effect=hn_heap_allocate(m,size,true);if(!effect)return false;hn_write32(m,HN_RASTER_EFFECT,effect);bytes=1;
    }else{
        if(!effect)return false;
        switch(op){
        case 0xA1:{
            hn_write8(m,effect,hn_read8(m,data));unsigned inside=hn_read16(m,data+1),outside=hn_read16(m,data+3);
            hn_write16(m,0x04000048,(uint16_t)inside);hn_write16(m,0x0400004A,(uint16_t)outside);
            hn_write8(m,effect+2,hn_read8(m,data+5));hn_write8(m,effect+1,(uint8_t)(inside|(inside>>8)|outside|(outside>>8)));
            unsigned edge=hn_read8(m,data+6)==1?240:0,window=hn_read8(m,effect)?2:0;
            hn_write16(m,0x04000040+window,(uint16_t)edge);hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)edge);
            hn_write16(m,0x04000044+window,160);bytes=7;break;
        }
        case 0xA2:if(!begin(m,state,effect))return false;break;
        case 0xA3:case 0xA8:case 0xAD:
            if(op==0xA3&&hn_read8(m,data)>=0xFD){if(!hn_raster_position(m,hn_read8(m,data)))return false;}
            else{hn_write8(m,effect+3,hn_read8(m,data));hn_write8(m,effect+4,hn_read8(m,data+1));}
            bytes=2;break;
        case 0xA4:for(unsigned i=0;i<4;i++)hn_write8(m,effect+5+i,hn_read8(m,data+i));bytes=4;break;
        case 0xA5:
            for(unsigned i=0;i<3;i++)hn_write8(m,effect+9+i,hn_read8(m,data+i));
            hn_write8(m,effect+13,0);hn_write8(m,effect+14,0);bytes=3;break;
        case 0xA6:{
            unsigned window=hn_read8(m,effect)?64:32;
            hn_write8(m,HN_DISPLAY_SHADOW+1,(uint8_t)(hn_read8(m,HN_DISPLAY_SHADOW+1)&~window));
            display_dirty(m);hn_video_hblank(m,0);dma_stop(m);free_effect(m);break;
        }
        case 0xA9:
            for(unsigned i=0;i<3;i++)hn_write8(m,effect+3+i,hn_read8(m,data+i));bytes=3;break;
        case 0xAE:sweep(m,state,effect);*running=0;return !m->fault;
        default:return false;
        }
    }
    hn_write32(m,state+0x18,data+bytes);return !m->fault;
}
