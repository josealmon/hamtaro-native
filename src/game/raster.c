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
void hn_raster_scanline_phase_reset(HNMemory *m){hn_write8(m,HN_SCANLINE_PHASE,0);}
bool hn_raster_scanline_profile(HNMemory *m,uint8_t retained_first)
{
    uint16_t phase=(uint16_t)(hn_read16(m,HN_SCANLINE_WINDOWS+2)+hn_read16(m,HN_SCANLINE_RADIUS));
    hn_write16(m,HN_SCANLINE_RADIUS,phase);
    unsigned increment=hn_read8(m,HN_SCANLINE_TRANSITION+3),old=hn_read8(m,HN_SCANLINE_TRANSITION+2),fraction=phase>>8;
    unsigned radius=(uint8_t)(old+increment+fraction);
    if(increment&128){if(old<256u-((increment+fraction)&255))radius=0;}
    else if(radius>240)radius=240;
    if(radius==255)return false; /* The original byte copy would never terminate. */
    /* Contain the source scratch overrun at radius239/240 in host storage;
       preserve every original guest destination write, including its last byte. */
    uint8_t profile[256]={0};profile[0]=radius?retained_first:0;
    if(radius){
        unsigned x=0,y=radius;uint16_t decision=(uint16_t)(3-radius*2);
        do{
            profile[radius-y+1]=(uint8_t)x;profile[radius-x+1]=(uint8_t)y;
            int d=(int16_t)decision;
            if(d<0)decision=(uint16_t)(d+(int)x*4+6);
            else{decision=(uint16_t)(d+((int)x-(int)y)*4+10);y=(uint16_t)(y-1);if(!y)break;}
            x=(uint16_t)(x+1);
        }while(x<=y);
    }
    GameAddr destination=hn_read32(m,HN_SCANLINE_PROFILE);
    for(unsigned i=0;i<=radius;i++){
        /* Configure's first pre-allocation update can target BIOS address zero.
           CPU byte stores there are discarded; this is local to this copy. */
        if(destination+i>=0x4000)hn_write8(m,destination+i,profile[i]);
        if(m->fault)return false;
    }
    hn_write8(m,HN_SCANLINE_TRANSITION+2,(uint8_t)radius);return !m->fault;
}
static bool scanline_window(HNMemory *m,unsigned mode)
{
    unsigned line=hn_read16(m,0x04000006);bool scrolled=(mode&1)!=0;
    if(line<160||line==227){
        if(!hn_read8(m,HN_SCANLINE_PHASE)){
            unsigned edge=240-hn_read8(m,HN_SCANLINE_TRANSITION);
            if(scrolled&&mode<2)hn_write16(m,0x0400001C,(uint16_t)edge);
            hn_write16(m,HN_SCANLINE_WINDOWS,(uint16_t)(edge&255));
        }
        unsigned phase=hn_read8(m,HN_SCANLINE_PHASE);
        if(mode<2){
            if(phase==1){
                if(scrolled)hn_write16(m,0x0400001C,(uint16_t)(0xFF10+hn_read8(m,HN_SCANLINE_TRANSITION)));
                hn_write16(m,HN_SCANLINE_WINDOWS,(uint16_t)((hn_read8(m,HN_SCANLINE_TRANSITION)<<8)+240));
            }
            hn_write8(m,HN_SCANLINE_PHASE,hn_read8(m,HN_SCANLINE_PHASE)^1);
        }else{
            if(scrolled&&phase==1)hn_write16(m,0x0400001C,(uint16_t)(0xFF10+hn_read8(m,HN_SCANLINE_TRANSITION)));
            unsigned middle=hn_read8(m,HN_SCANLINE_PHASE+1)>>1;
            if(phase==middle)hn_write16(m,HN_SCANLINE_WINDOWS,(uint16_t)((hn_read8(m,HN_SCANLINE_TRANSITION)<<8)+240));
            if(scrolled&&hn_read8(m,HN_SCANLINE_PHASE)==middle+1)hn_write16(m,0x0400001C,(uint16_t)(240-hn_read8(m,HN_SCANLINE_TRANSITION)));
            uint8_t next=(uint8_t)(phase+1);hn_write8(m,HN_SCANLINE_PHASE,next);
            if(next>=hn_read8(m,HN_SCANLINE_PHASE+1))hn_write8(m,HN_SCANLINE_PHASE,0);
        }
    }
    if(hn_read16(m,0x04000006)==160){
        uint8_t next=(uint8_t)(hn_read8(m,HN_SCANLINE_TRANSITION+1)+hn_read8(m,HN_SCANLINE_TRANSITION));
        hn_write8(m,HN_SCANLINE_TRANSITION,next);hn_write8(m,HN_SCANLINE_PHASE,0);
        if(next>240){hn_write8(m,HN_SCANLINE_TRANSITION,240);hn_write8(m,HN_SCANLINE_TRANSITION+1,0);}
    }
    return !m->fault;
}
static uint16_t bounded_edge(uint32_t value)
{uint16_t edge=(uint16_t)value;if(edge&0x8000)return 0;return edge>240?240:edge;}
static bool scanline_steps(HNMemory *m,bool scrolled)
{
    unsigned line=hn_read16(m,0x04000006);line=line==227?0:(uint16_t)(line+1);
    unsigned period=hn_read8(m,HN_SCANLINE_PHASE+1);
    /* The original libgcc division and remainder return zero for divisor zero. */
    if(line<=160&&(period?line%period:0)==0){
        uint16_t edge=bounded_edge(hn_read16(m,HN_SCANLINE_WINDOWS+2)-hn_read8(m,HN_SCANLINE_PHASE));
        hn_write16(m,HN_SCANLINE_WINDOWS,edge);
        if(scrolled)hn_write16(m,0x0400001C,(uint16_t)(240-edge));
        hn_write16(m,HN_SCANLINE_WINDOWS,(uint16_t)((hn_read16(m,HN_SCANLINE_WINDOWS)<<8)+240));
        hn_write8(m,HN_SCANLINE_PHASE,(uint8_t)(hn_read8(m,HN_SCANLINE_PHASE)+hn_read8(m,HN_SCANLINE_PHASE+2)));
    }
    if(hn_read16(m,0x04000006)==160){
        hn_write16(m,HN_SCANLINE_WINDOWS+2,(uint16_t)(hn_read16(m,HN_SCANLINE_WINDOWS+2)+hn_read8(m,HN_SCANLINE_TRANSITION+1)));
        hn_write8(m,HN_SCANLINE_PHASE,0);period=hn_read8(m,HN_SCANLINE_PHASE+1);
        unsigned limit=hn_read8(m,HN_SCANLINE_PHASE+2)*(period?160/period:0)+240;
        if(hn_read16(m,HN_SCANLINE_WINDOWS+2)>limit){hn_write16(m,HN_SCANLINE_WINDOWS,(uint16_t)limit);hn_write8(m,HN_SCANLINE_TRANSITION+1,0);}
    }
    return !m->fault;
}
static bool scanline_circle(HNMemory *m)
{
    unsigned line=hn_read16(m,0x04000006);
    if(line<160||line==227){
        unsigned center=hn_read8(m,HN_SCANLINE_TRANSITION+1),radius=hn_read8(m,HN_SCANLINE_TRANSITION+2),phase=hn_read8(m,HN_SCANLINE_PHASE);
        uint16_t packed=0;
        if((int)center-(int)radius<(int)phase&&center+radius>phase){
            unsigned distance=(center<phase?phase-center:center-phase)&255;
            unsigned width=hn_read8(m,hn_read32(m,HN_SCANLINE_PROFILE)+radius-distance),x=hn_read8(m,HN_SCANLINE_TRANSITION);
            unsigned left=x<width?0:(x-width)&255,right=hn_read8(m,HN_SCANLINE_TRANSITION);
            right=(int)right>=240-(int)width?240:(width+right)&255;packed=(uint16_t)((left<<8)+right);
        }
        hn_write16(m,HN_SCANLINE_WINDOWS,packed);hn_write8(m,HN_SCANLINE_PHASE,(uint8_t)(hn_read8(m,HN_SCANLINE_PHASE)+1));
    }
    if(hn_read16(m,0x04000006)==160){
        hn_write8(m,HN_SCANLINE_PHASE,0);
        if((uint8_t)(hn_read8(m,HN_SCANLINE_TRANSITION+2)-1)>198)hn_write8(m,HN_SCENE_CONTROLS+0x180,255);
        else return hn_raster_scanline_profile(m,0);
    }
    return !m->fault;
}
static bool configure_scanline(HNMemory *m,GameAddr arguments)
{
    hn_video_hblank(m,0);unsigned mode=hn_read8(m,HN_SCENE_CONTROLS+0x61),selector;
    if(mode<=3){
        hn_write8(m,HN_SCANLINE_PHASE+1,mode<2?1:hn_read8(m,HN_SCENE_CONTROLS+0x64));
        hn_write8(m,HN_SCANLINE_TRANSITION+1,hn_read8(m,HN_SCENE_CONTROLS+0x63));
        hn_write8(m,HN_SCANLINE_PHASE,0);hn_write8(m,HN_SCANLINE_TRANSITION,0);hn_write16(m,HN_SCANLINE_WINDOWS,0);
        selector=(uint8_t)(hn_read8(m,arguments)+mode);
    }else if(mode<=5){
        hn_write8(m,HN_SCANLINE_PHASE+1,hn_read8(m,HN_SCENE_CONTROLS+0x65));hn_write8(m,HN_SCANLINE_PHASE+2,hn_read8(m,HN_SCENE_CONTROLS+0x64));
        hn_write8(m,HN_SCANLINE_TRANSITION+1,hn_read8(m,HN_SCENE_CONTROLS+0x63));hn_write8(m,HN_SCANLINE_PHASE,0);
        hn_write16(m,HN_SCANLINE_WINDOWS,0);hn_write16(m,HN_SCANLINE_WINDOWS+2,0);selector=(uint8_t)(hn_read8(m,arguments)+mode);
    }else if(mode<=8){
        hn_write8(m,HN_SCANLINE_TRANSITION,120);hn_write8(m,HN_SCANLINE_TRANSITION+1,mode==6?80:160);
        hn_write8(m,HN_SCANLINE_TRANSITION+2,mode==6?120:mode==7?0:24);hn_write8(m,HN_SCANLINE_TRANSITION+3,mode==6?255:mode==7?2:0);
        hn_write16(m,HN_SCANLINE_WINDOWS+2,0);hn_write16(m,HN_SCANLINE_WINDOWS,0);hn_write8(m,HN_SCANLINE_PHASE,0);
        hn_write16(m,HN_SCANLINE_RADIUS,0);hn_write8(m,HN_SCENE_CONTROLS+0x180,0);
        if(!hn_raster_scanline_profile(m,0))return false;
        selector=(uint8_t)(hn_read8(m,arguments)+6);
        GameAddr profile=hn_heap_allocate(m,240,true);hn_write32(m,HN_SCANLINE_PROFILE,profile);if(!profile)return false;
    }else return false; /* The source selector word is unspecified above mode8. */
    hn_write32(m,0x040000B0,HN_SCANLINE_WINDOWS);hn_write32(m,0x040000B4,0x04000040);
    hn_write16(m,0x040000B8,1);hn_write16(m,0x040000BA,0xA340);
    hn_write16(m,0x04000000,hn_read16(m,0x04000000)|0x2000);hn_write16(m,0x04000040,0);
    hn_write16(m,0x04000044,160);hn_write16(m,0x04000048,31);hn_write16(m,0x0400004A,16);hn_video_hblank(m,selector);return !m->fault;
}
bool hn_raster_wave(HNMemory *m)
{
    unsigned line=hn_read16(m,0x04000006),phase=hn_read8(m,HN_SCANLINE_PHASE+1);
    int32_t sine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+((line+phase)&255)*2);
    hn_write16(m,0x04000018,(uint16_t)(sine>>4));
    int position=(int16_t)(hn_read16(m,0x04000006)+hn_read16(m,HN_SCANLINE_RADIUS));uint16_t alpha;
    if(position>112)alpha=0x1000;
    else if(position<=55)alpha=0x0808;
    else if(position<=64)alpha=0x0907;
    else if(position<=72)alpha=0x0A06;
    else if(position<=80)alpha=0x0B05;
    else if(position<=88)alpha=0x0C04;
    else if(position<=96)alpha=0x0D03;
    else if(position<=104)alpha=0x0E02;
    else alpha=0x0F01;
    hn_write16(m,0x04000052,alpha);
    if(hn_read16(m,0x04000006)==227){
        hn_write8(m,HN_SCANLINE_PHASE,(uint8_t)(hn_read8(m,HN_SCANLINE_PHASE)+1));
        if(!(hn_read8(m,HN_SCANLINE_PHASE)&1))hn_write8(m,HN_SCANLINE_PHASE+1,(uint8_t)(hn_read8(m,HN_SCANLINE_PHASE+1)+1));
        unsigned y=hn_read16(m,HN_VIDEO_STATE+0x6C);
        int32_t wave=(int32_t)((uint32_t)hn_read16(m,HN_RENDER_SINE_TABLE+hn_read8(m,HN_SCANLINE_PHASE+1)*2)<<16)>>19;
        hn_write16(m,HN_SCANLINE_RADIUS,(uint16_t)((uint32_t)wave-y));
    }
    return !m->fault;
}
HNSceneResult hn_raster_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080CE678:{
        unsigned operation=hn_read8(m,arguments);
        if(operation<=5){
            unsigned field=operation<2?1:operation<4?0:2,step=operation<4?4:2,maximum=operation<4?239:199;
            int value=(int)hn_read8(m,HN_SCANLINE_TRANSITION+field)+((operation&1)?(int)step:-(int)step);
            if((operation&1)?value<=(int)maximum:value>0)hn_write8(m,HN_SCANLINE_TRANSITION+field,(uint8_t)value);
        }
        *consumed=1;break;
    }
    case 0x080CE72C:if(!configure_scanline(m,arguments))return HN_SCENE_RESOURCE;*consumed=2;break;
    case 0x080CE9F4:
        hn_write16(m,0x04000000,hn_read16(m,0x04000000)&0x1FFF);hn_video_hblank(m,0);
        hn_write32(m,0x040000B0,0);hn_write32(m,0x040000B4,0);hn_write32(m,0x040000B8,0);
        hn_heap_free(m,hn_read32(m,HN_SCANLINE_PROFILE));break; /* Keep the source's retained pointer. */
    case 0x080CEA34:{
        unsigned parameter=hn_read8(m,HN_SCENE_CONTROLS+0x60);hn_video_hblank(m,4);
        hn_write16(m,0x04000000,hn_read16(m,0x04000000)|0xB000);hn_write16(m,0x04000048,0x3F);
        hn_write16(m,0x0400004A,0x3D3B);hn_write16(m,0x04000040,0xB9CF);
        hn_write16(m,0x04000044,(uint16_t)(((16u-(parameter<<1))<<11)+138));break;
    }
    case 0x0802E22C:
        hn_video_hblank(m,1);hn_write16(m,HN_SCANLINE_RADIUS,hn_read16(m,HN_VIDEO_STATE+0x6C));
        hn_write8(m,HN_SCANLINE_PHASE,0);hn_write8(m,HN_SCANLINE_PHASE+1,45);break;
    case 0x0802E258:hn_video_hblank(m,0);break;
    case 0x080CD33C:
        /* These original field stores leave the existing dirty mask intact. */
        hn_write8(m,HN_BLEND,0x42);hn_write8(m,HN_BLEND+1,(uint8_t)((hn_read8(m,HN_BLEND+1)&0xC0)|19));
        hn_write8(m,HN_BLEND+2,3);hn_write8(m,HN_BLEND+3,14);hn_video_vcount(m,1,104);break;
    case 0x080CF218:
        hn_write16(m,0x04000200,hn_read16(m,0x04000200)|4);hn_video_vcount(m,2,1);break;
    case 0x080CD580:hn_write16(m,0x04000200,hn_read16(m,0x04000200)|4);break;
    case 0x080CD554:case 0x080CF248:{
        uint16_t master=hn_read16(m,0x04000208);hn_write16(m,0x04000208,0);
        hn_write16(m,0x04000200,hn_read16(m,0x04000200)&0xFFFB);hn_write16(m,0x04000208,master);break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
static void complete_raster(HNMemory *m,GameAddr effect)
{
    restore_layers(m,effect);hn_video_hblank(m,0);dma_stop(m);free_effect(m);hn_write8(m,HN_SCENE_CONTROLS+0x60,1);
}
static bool raster_bands(HNMemory *m,unsigned mode)
{
    GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);if(!effect)return false;
    bool scrolled=mode==13,alternating=mode==17;unsigned line=hn_read16(m,0x04000006);
    if(line<160||line==227){
        if(!hn_read8(m,effect+5)){
            uint32_t edge=240u-hn_read8(m,effect+6);
            hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)(alternating?(edge<<8)+240:edge));
        }
        effect=hn_read32(m,HN_RASTER_EFFECT);
        if(scrolled&&hn_read8(m,effect+5)==1)hn_write16(m,0x0400001C,(uint16_t)(0xFF10+hn_read8(m,effect+6)));
        if(hn_read8(m,effect+5)==(hn_read8(m,effect+4)>>1)){
            unsigned edge=hn_read8(m,effect+6);hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)(alternating?edge:(edge<<8)+240));
        }
        if(scrolled&&hn_read8(m,effect+5)==(unsigned)(hn_read8(m,effect+4)>>1)+1)
            hn_write16(m,0x0400001C,(uint16_t)(240-hn_read8(m,effect+6)));
        hn_write8(m,effect+5,(uint8_t)(hn_read8(m,effect+5)+1));
        GameAddr current=hn_read32(m,HN_RASTER_EFFECT);
        if(hn_read8(m,current+5)>=hn_read8(m,current+4))hn_write8(m,effect+5,0);
    }
    if(hn_read16(m,0x04000006)==160){
        effect=hn_read32(m,HN_RASTER_EFFECT);hn_write8(m,effect+6,(uint8_t)(hn_read8(m,effect+3)+hn_read8(m,effect+6)));
        effect=hn_read32(m,HN_RASTER_EFFECT);hn_write8(m,effect+5,0);
        GameAddr sample=hn_read32(m,HN_RASTER_EFFECT);uint32_t edge=240u-hn_read8(m,sample+6);
        uint16_t packed=(uint16_t)(alternating?(edge<<8)+240:edge);hn_write16(m,HN_RASTER_SCANLINE,packed);
        hn_write16(m,hn_read8(m,sample)?0x04000042:0x04000040,packed);
        GameAddr current=hn_read32(m,HN_RASTER_EFFECT);
        if(scrolled)hn_write16(m,0x0400001C,(uint16_t)(0xFF10+hn_read8(m,current+6)));
        unsigned value=hn_read8(m,current+6);
        if((mode==12&&value>=240)||(mode!=12&&value>240))complete_raster(m,current);
    }
    return !m->fault;
}
static bool raster_steps(HNMemory *m,unsigned mode)
{
    unsigned line=hn_read16(m,0x04000006);line=line==227?0:(uint16_t)(line+1);
    if(line<160){
        GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);if(!effect)return false;
        int period=hn_read8(m,effect+4),remainder=period?(int)line%period:0;
        if(remainder==period-1){
            hn_write8(m,effect+6,(uint8_t)(hn_read8(m,effect+5)+hn_read8(m,effect+6)));
            GameAddr sample=hn_read32(m,HN_RASTER_EFFECT);unsigned baseline=(hn_read8(m,sample+8)<<8)|hn_read8(m,sample+7);
            hn_write16(m,HN_RASTER_SCANLINE,bounded_edge(baseline-hn_read8(m,sample+6)));
            if(mode==15)hn_write16(m,0x0400001C,(uint16_t)(240-hn_read16(m,HN_RASTER_SCANLINE)));
            if(mode!=18)hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)((hn_read16(m,HN_RASTER_SCANLINE)<<8)+240));
        }
    }
    if(hn_read16(m,0x04000006)==160){
        GameAddr effect=hn_read32(m,HN_RASTER_EFFECT);if(!effect)return false;
        unsigned baseline=(hn_read8(m,effect+8)<<8)|hn_read8(m,effect+7);
        baseline=(uint16_t)(baseline+hn_read8(m,effect+3));hn_write8(m,effect+8,(uint8_t)(baseline>>8));hn_write8(m,effect+7,(uint8_t)baseline);
        effect=hn_read32(m,HN_RASTER_EFFECT);hn_write8(m,effect+6,0);
        GameAddr sample=hn_read32(m,HN_RASTER_EFFECT);hn_write16(m,HN_RASTER_SCANLINE,bounded_edge(baseline-hn_read8(m,sample+6)));
        unsigned edge=hn_read16(m,HN_RASTER_SCANLINE);uint16_t packed=(uint16_t)(mode==18?edge:(edge<<8)+240);
        hn_write16(m,hn_read8(m,sample)?0x04000042:0x04000040,packed);
        if(mode!=18)hn_write16(m,HN_RASTER_SCANLINE,(uint16_t)((hn_read16(m,HN_RASTER_SCANLINE)<<8)+240));
        GameAddr current=hn_read32(m,HN_RASTER_EFFECT);unsigned period=hn_read8(m,current+4),step=hn_read8(m,current+5);
        unsigned limit=step*(period?160/period:0)+240;if(baseline>limit)complete_raster(m,current);
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
    if(callback==0x0802E268)return hn_raster_wave(m);
    switch(callback){
    case 0x080CEA90:hn_write16(m,0x04000018,hn_read16(m,HN_RENDER_SINE_TABLE+(hn_read16(m,0x04000006)>>1)*2));return !m->fault;
    case 0x080CE0E0:return scanline_window(m,0);
    case 0x080CE15C:return scanline_window(m,1);
    case 0x080CE1F4:return scanline_window(m,2);
    case 0x080CE280:return scanline_window(m,3);
    case 0x080CE33C:return scanline_steps(m,false);
    case 0x080CE3EC:return scanline_steps(m,true);
    case 0x080CE4AC:return scanline_circle(m);
    case 0x0800EF60:return raster_bands(m,12);
    case 0x0800F084:return raster_bands(m,13);
    case 0x0800F6AC:return raster_bands(m,17);
    case 0x0800F1EC:return raster_steps(m,14);
    case 0x0800F36C:return raster_steps(m,15);
    case 0x0800F7D8:return raster_steps(m,18);
    }
    return false;
}
bool hn_raster_vcount_blend(HNMemory *m)
{
    bool second=hn_read8(m,HN_VCOUNT_PHASE)!=0;
    hn_write16(m,0x04000052,second?0x040C:0x0A06);
    hn_write16(m,0x04000004,hn_read16(m,0x04000004)&0x003F);
    if(!second)hn_write16(m,0x04000004,hn_read16(m,0x04000004)|0x6800);
    hn_write8(m,HN_VCOUNT_PHASE,second?0:1);hn_write16(m,0x04000050,0x1D42);return !m->fault;
}
bool hn_raster_vcount_increment(HNMemory *m)
{
    GameAddr value=0x0201DC40+0x29*2;hn_write16(m,value,(uint16_t)(hn_read16(m,value)+1));return !m->fault;
}
bool hn_raster_vcount(HNMemory *m)
{
    unsigned selector=hn_read8(m,HN_VCOUNT_SELECTOR);
    if(!selector){hn_write16(m,0x04000004,hn_read16(m,0x04000004)&0xFFDF);return !m->fault;}
    if(selector>=3)return false;
    switch(hn_read32(m,HN_VCOUNT_HANDLERS+selector*4)&~1u){
    case 0x080005B8:return !m->fault;
    case 0x080CD4E0:return hn_raster_vcount_blend(m);
    case 0x080CF238:return hn_raster_vcount_increment(m);
    default:return false;
    }
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
