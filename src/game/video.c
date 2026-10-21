#include "game/video.h"
#include "game/state.h"
static void dirty(HNMemory *m,uint32_t mask)
{hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|mask);}
void hn_video_state_init(HNMemory *m)
{
    hn_write16(m,HN_DISPLAY_STATUS,(uint16_t)((hn_read16(m,HN_DISPLAY_STATUS)&7)|0x48));
    hn_write32(m,HN_VIDEO_STATE+4,0);hn_write32(m,HN_VIDEO_STATE+8,0);
    hn_write8(m,HN_VIDEO_STATE+12,0);hn_write8(m,HN_VIDEO_STATE+13,0);
    for(unsigned offset=0x1C;offset<=0x30;offset+=4)hn_write32(m,HN_VIDEO_STATE+offset,0);
    hn_write16(m,HN_DISPLAY_SHADOW,0x1180);
    for(unsigned layer=0;layer<4;layer++){
        GameAddr b=HN_VIDEO_STATE+0x38+layer*0x30;
        unsigned charbase=layer>=2?1:0;
        unsigned screenbase=layer==3?28:22+layer;
        unsigned size=layer>=2?3:0;
        hn_write16(m,b,(uint16_t)(1|(charbase<<2)|(screenbase<<8)|0x2000|(size<<14)));
        hn_write16(m,b+2,0);hn_write16(m,b+4,0);
    }
    hn_write32(m,HN_VIDEO_STATE,UINT32_MAX);
}
void hn_video_set_mode(HNMemory *m,uint8_t mode)
{
    uint8_t value=hn_read8(m,HN_DISPLAY_SHADOW);
    hn_write8(m,HN_DISPLAY_SHADOW,(uint8_t)((value&0xF0)|(mode&15)));dirty(m,1);
}
void hn_video_set_layers(HNMemory *m,uint8_t enabled,uint8_t mask)
{
    uint8_t value=hn_read8(m,HN_DISPLAY_SHADOW+1),layers=value&31;
    layers=(uint8_t)((enabled?(layers|mask):(layers&~mask))&31);
    hn_write8(m,HN_DISPLAY_SHADOW+1,(uint8_t)((value&0xE0)|layers));dirty(m,1);
}
void hn_video_forced_blank(HNMemory *m,int32_t blank)
{
    uint16_t value=hn_read16(m,HN_DISPLAY_SHADOW);
    if(blank){hn_write16(m,HN_DISPLAY_SHADOW,(uint16_t)(value|0x80));dirty(m,1);}
    else{value&=~0x80u;hn_write16(m,HN_DISPLAY_SHADOW,value);hn_write16(m,0x04000000,value);}
}
void hn_video_mark_backgrounds(HNMemory *m,uint8_t mask){dirty(m,(mask&15u)<<2);}
void hn_video_hblank(HNMemory *m,uint32_t selector)
{
    uint8_t enabled=(uint8_t)selector,status=hn_read8(m,HN_DISPLAY_STATUS);
    hn_write8(m,HN_HBLANK_SELECTOR,enabled);
    if(!enabled)hn_write8(m,HN_DISPLAY_STATUS,(uint8_t)((status&~0xD0u)|0x40));
    else if(!(status&16))hn_write8(m,HN_DISPLAY_STATUS,(uint8_t)((status&~0xC0u)|0x50));
}
void hn_video_blend_effect(HNMemory *m,uint32_t effect)
{
    hn_write8(m,HN_BLEND,(uint8_t)((hn_read8(m,HN_BLEND)&0x3F)|((effect&3)<<6)));dirty(m,2);
}
void hn_video_blend_targets(HNMemory *m,uint8_t first,uint8_t second)
{
    if(first!=255)hn_write8(m,HN_BLEND,(uint8_t)((hn_read8(m,HN_BLEND)&0xC0)|(first&63)));
    if(second!=255)hn_write8(m,HN_BLEND+1,(uint8_t)((hn_read8(m,HN_BLEND+1)&0xC0)|(second&63)));
    dirty(m,2);
}
void hn_video_blend_coefficients(HNMemory *m,uint8_t first,uint8_t second,uint8_t brightness)
{
    if(!(first&128))hn_write8(m,HN_BLEND+2,first&31);
    if(!(second&128))hn_write8(m,HN_BLEND+3,second&31);
    if(!(brightness&128))hn_write8(m,HN_BLEND+4,brightness&31);
    dirty(m,2);
}
void hn_video_blend_scale(HNMemory *m,uint8_t scale)
{
    hn_write8(m,HN_BLEND+5,scale);
    if(scale==16){
        hn_write8(m,HN_BLEND+2,0);hn_write8(m,HN_BLEND+3,0);hn_video_blend_effect(m,0);
    }else{
        hn_write8(m,HN_BLEND+2,scale&15);hn_write8(m,HN_BLEND+3,(uint8_t)(16-(scale&15)));hn_video_blend_effect(m,1);
    }
}
void hn_video_fade_phase(HNMemory *m,uint8_t phase)
{
    unsigned level=((unsigned)phase+4)>>3;
    hn_video_blend_effect(m,level<16?3:level>16?2:0);
    hn_write8(m,HN_BLEND+4,(uint8_t)(level<16?16-level:level>16?level-16:0));
    hn_write8(m,HN_BLEND,(uint8_t)(hn_read8(m,HN_BLEND)|63));
    hn_write8(m,HN_BLEND+6,phase);
}
void hn_mosaic_init(HNMemory *m)
{
    hn_write16(m,HN_MOSAIC,0);hn_write8(m,HN_MOSAIC+2,14);
    for(unsigned i=3;i<8;i++)hn_write8(m,HN_MOSAIC+i,0);
}
void hn_mosaic_update(HNMemory *m)
{
    unsigned flags=hn_read8(m,HN_MOSAIC+2),enabled=hn_read8(m,HN_MOSAIC+3);
    if(!enabled||(flags&1))return;
    unsigned bh=hn_read8(m,HN_MOSAIC+4)>>4,bv=hn_read8(m,HN_MOSAIC+5)>>4;
    unsigned oh=hn_read8(m,HN_MOSAIC+6)>>4,ov=hn_read8(m,HN_MOSAIC+7)>>4;
    unsigned value=hn_read16(m,HN_MOSAIC);
    if(enabled&15){
        value&=~0xFFu;value|=bh|((flags&2?bh:bv)<<4);
    }
    if(enabled&16){
        unsigned x=flags&8?bh:oh,y=(flags&2)?x:(flags&8?bv:ov);
        value&=~0xFF00u;value|=x<<8|y<<12;
    }
    hn_write16(m,HN_MOSAIC,(uint16_t)value);
}
void hn_mosaic_configure(HNMemory *m,uint8_t enabled,uint8_t flags)
{hn_write8(m,HN_MOSAIC+2,flags);hn_write8(m,HN_MOSAIC+3,enabled&31);}
void hn_mosaic_square(HNMemory *m,uint8_t size)
{
    hn_write8(m,HN_MOSAIC+4,size);
    hn_write8(m,HN_MOSAIC+2,(uint8_t)((hn_read8(m,HN_MOSAIC+2)&~1u)|10));
}
void hn_mosaic_shared(HNMemory *m,uint8_t horizontal,uint8_t vertical)
{
    hn_write8(m,HN_MOSAIC+4,horizontal);hn_write8(m,HN_MOSAIC+5,vertical);
    hn_write8(m,HN_MOSAIC+2,(uint8_t)((hn_read8(m,HN_MOSAIC+2)&~3u)|8));
}
void hn_video_commit(HNMemory *m)
{
    uint16_t status=hn_read16(m,HN_DISPLAY_STATUS);
    if(status&0xFFC0){
        if(((status>>6)&3)==1){
            status&=0xFF38;hn_write16(m,HN_DISPLAY_STATUS,status);hn_write16(m,0x04000004,status);
        }else{
            status=hn_read16(m,0x04000004);status&=~0xC0u;hn_write16(m,HN_DISPLAY_STATUS,status);
        }
    }
    uint32_t flags=hn_read32(m,HN_VIDEO_STATE);
    if(flags&1){hn_write16(m,0x04000000,hn_read16(m,HN_DISPLAY_SHADOW));flags&=~1u;}
    if(flags&2){
        hn_write16(m,0x04000050,hn_read16(m,HN_BLEND));
        hn_write16(m,0x04000052,(uint16_t)(hn_read8(m,HN_BLEND+2)|(hn_read8(m,HN_BLEND+3)<<8)));
        hn_write16(m,0x04000054,hn_read8(m,HN_BLEND+4));flags&=~2u;
    }
    uint32_t camera=hn_read16(m,HN_VIDEO_STATE+6)|((uint32_t)hn_read16(m,HN_VIDEO_STATE+10)<<16);
    unsigned mosaic_x=0,mosaic_y=0,enabled=hn_read8(m,HN_MOSAIC+3);
    unsigned mosaic=hn_read16(m,HN_MOSAIC);
    if(enabled){
        hn_write16(m,0x0400004C,(uint16_t)mosaic);
        if(hn_read8(m,HN_MOSAIC+2)&4){
            unsigned mx=mosaic&15,my=(mosaic>>4)&15;
            unsigned rx=240%(mx+1),ry=160%(my+1);
            mosaic_x=mx>>1;mosaic_y=my>>1;
            if(rx&&rx<=mosaic_x)mosaic_x=rx-1;
            if(ry&&ry<=mosaic_y)mosaic_y=ry-1;
        }
    }
    for(unsigned layer=0;layer<4;layer++){
        unsigned mask=4u<<layer;if(!(flags&mask))continue;
        GameAddr b=HN_VIDEO_STATE+0x38+layer*0x30;
        uint16_t control=hn_read16(m,b);
        unsigned dx=(enabled&(1u<<layer))?mosaic_x:0,dy=(enabled&(1u<<layer))?mosaic_y:0;
        hn_write16(m,0x04000008+2*layer,control);
        unsigned x=(camera-hn_read16(m,b+2)+dx)&((control&0x4000)?511u:255u);
        unsigned y=((uint32_t)((int32_t)camera>>16)-hn_read16(m,b+4)+dy)&((control&0x8000)?511u:255u);
        hn_write16(m,0x04000010+4*layer,(uint16_t)x);hn_write16(m,0x04000012+4*layer,(uint16_t)y);
        if(layer>=2){
            GameAddr r=0x04000020+(layer-2)*16;
            for(unsigned i=0;i<4;i++)hn_write16(m,r+i*2,hn_read16(m,b+6+i*2));
            uint32_t ox=hn_read32(m,b+0x10),oy=hn_read32(m,b+0x14);
            hn_write16(m,r+8,(uint16_t)ox);hn_write16(m,r+10,(uint16_t)((ox>>16)&0xFFF));
            hn_write16(m,r+12,(uint16_t)oy);hn_write16(m,r+14,(uint16_t)((oy>>16)&0xFFF));
        }
        flags&=~mask;
    }
    hn_write32(m,HN_VIDEO_STATE,flags);
}
