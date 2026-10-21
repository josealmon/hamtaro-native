#include "gba/video.h"
#include <limits.h>

typedef struct { uint16_t color; uint8_t layer, priority; bool opaque, semi; } Pixel;
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0]|p[1]<<8); }
static uint16_t reg16(const HNMemory *m,unsigned o) { return u16(m->io+o); }
static int32_t reference(const HNMemory *m,unsigned o)
{
    uint32_t v=(uint32_t)reg16(m,o)|((uint32_t)reg16(m,o+2)<<16);
    v&=0x0FFFFFFF;
    return v&0x08000000?(int32_t)(v|0xF0000000u):(int32_t)v;
}
static uint8_t vram(const HNMemory *m,unsigned o)
{
    o&=0x1FFFF;
    if(o>=0x18000) o-=0x8000;
    return m->vram[o];
}
static uint16_t vram16(const HNMemory *m,unsigned o)
{ return (uint16_t)(vram(m,o)|vram(m,o+1)<<8); }
static uint16_t palette(const HNMemory *m,unsigned index)
{ return u16(m->palette+(index&511)*2); }
static uint32_t rgba(uint16_t color)
{
    uint32_t r=color&31,g=(color>>5)&31,b=(color>>10)&31;
    r=(r<<3)|(r>>2); g=(g<<3)|(g>>2); b=(b<<3)|(b>>2);
    return 0xFF000000u|(b<<16)|(g<<8)|r;
}
static Pixel empty(void)
{ Pixel p={0,0,0,false,false}; return p; }
static Pixel background(const HNMemory *m,unsigned bg,int x,int y,unsigned mode)
{
    Pixel p=empty();
    unsigned cnt=reg16(m,8+bg*2), mosaic=reg16(m,0x4C);
    p.layer=(uint8_t)bg; p.priority=(uint8_t)(cnt&3);
    if(cnt&0x40) { x-=x%(int)((mosaic&15)+1); y-=y%(int)(((mosaic>>4)&15)+1); }
    bool affine=(mode==1 && bg==2) || (mode==2 && bg>=2);
    unsigned charbase=((cnt>>2)&3)*16384, mapbase=((cnt>>8)&31)*2048;
    unsigned index;
    if(affine) {
        unsigned r=0x20+(bg-2)*16;
        int sx=(reference(m,r+8)+(int16_t)reg16(m,r)*x+(int16_t)reg16(m,r+2)*y)>>8;
        int sy=(reference(m,r+12)+(int16_t)reg16(m,r+4)*x+(int16_t)reg16(m,r+6)*y)>>8;
        int size=128<<((cnt>>14)&3);
        if(cnt&0x2000) { sx&=size-1; sy&=size-1; }
        else if(sx<0 || sy<0 || sx>=size || sy>=size) return p;
        unsigned tile=vram(m,mapbase+(unsigned)(sy/8)*(unsigned)(size/8)+(unsigned)sx/8);
        index=vram(m,charbase+tile*64+(sy&7)*8+(sx&7));
    } else {
        unsigned size=cnt>>14, wide=(size&1)?512:256, high=(size&2)?512:256;
        unsigned sx=((unsigned)x+reg16(m,0x10+bg*4))&(wide-1);
        unsigned sy=((unsigned)y+reg16(m,0x12+bg*4))&(high-1);
        unsigned tx=sx/8,ty=sy/8;
        unsigned block=(ty/32)*(wide/256)+tx/32;
        uint16_t entry=vram16(m,mapbase+block*2048+((ty&31)*32+(tx&31))*2);
        unsigned px=sx&7,py=sy&7;
        if(entry&0x400) px=7-px;
        if(entry&0x800) py=7-py;
        unsigned tile=entry&1023;
        if(cnt&0x80) index=vram(m,charbase+tile*64+py*8+px);
        else {
            unsigned packed=vram(m,charbase+tile*32+py*4+px/2);
            index=(packed>>((px&1)*4))&15;
            if(index) index+=((entry>>12)&15)*16;
        }
    }
    if(index) { p.color=palette(m,index); p.opaque=true; }
    return p;
}
static Pixel bitmap(const HNMemory *m,int x,int y,unsigned mode,unsigned display)
{
    Pixel p=empty();
    unsigned cnt=reg16(m,0xC), mosaic=reg16(m,0x4C);
    if(cnt&0x40) { x-=x%(int)((mosaic&15)+1); y-=y%(int)(((mosaic>>4)&15)+1); }
    int sx=(reference(m,0x28)+(int16_t)reg16(m,0x20)*x+(int16_t)reg16(m,0x22)*y)>>8;
    int sy=(reference(m,0x2C)+(int16_t)reg16(m,0x24)*x+(int16_t)reg16(m,0x26)*y)>>8;
    int w=mode==5?160:240,h=mode==5?128:160;
    if(sx<0||sy<0||sx>=w||sy>=h) return p;
    unsigned page=(mode!=3 && (display&16))?0xA000:0;
    unsigned offset=(unsigned)(sy*w+sx),index=mode==4?vram(m,page+offset):0;
    if(mode==4&&!index)return p;
    p.layer=2; p.priority=(uint8_t)(cnt&3); p.opaque=true;
    p.color=mode==4?palette(m,index):vram16(m,page+offset*2);
    return p;
}
static bool in_range(int value,unsigned interval)
{
    int start=(interval>>8)&255,end=interval&255;
    return start<=end?(value>=start && value<end):(value>=start || value<end);
}
static Pixel object(const HNMemory *m,unsigned n,int x,int y,unsigned display,unsigned mode)
{
    Pixel p=empty();
    const uint8_t *o=m->oam+n*8;
    unsigned a=u16(o),b=u16(o+2),c=u16(o+4),shape=a>>14,size=b>>14;
    bool affine=(a&0x100)!=0;
    if(shape==3 || (!affine && (a&0x200)) || ((a>>10)&3)==3) return p;
    static const uint8_t widths[3][4]={{8,16,32,64},{16,32,32,64},{8,8,16,32}};
    static const uint8_t heights[3][4]={{8,16,32,64},{8,8,16,32},{16,32,32,64}};
    int w=widths[shape][size],h=heights[shape][size],bw=w,bh=h;
    if(affine && (a&0x200)) { bw*=2; bh*=2; }
    int ox=b&511,oy=a&255;
    if(ox>=256) ox-=512;
    if(oy>=160) oy-=256;
    int dx=x-ox,dy=y-oy;
    unsigned mosaic=reg16(m,0x4C),mh=((mosaic>>8)&15)+1,mv=((mosaic>>12)&15)+1;
    int right=ox+bw;
    if((a&0x1000)&&right%(int)mh)right+=(int)mh-right%(int)mh;
    if(dx<0||dy<0||x>=right||dy>=bh) return p;
    if(a&0x1000) {
        dy=y-y%(int)mv-oy;
        if(dy<0)dy=0;if(dy>=bh)dy=bh-1;
        int start=ox<0?0:ox;
        int sample=x-x%(int)mh;
        if(affine&&mh>1){if(sample<start)sample=start-1;dx=sample-ox;}
        else{dx=sample-ox;if(dx<0)dx=0;if(dx>=bw)dx=bw-1;}
    }
    if(affine) {
        unsigned matrix=((b>>9)&31)*32+6;
        int rx=dx-bw/2,ry=dy-bh/2;
        dx=(((int16_t)u16(m->oam+matrix)*rx+(int16_t)u16(m->oam+matrix+8)*ry)>>8)+w/2;
        dy=(((int16_t)u16(m->oam+matrix+16)*rx+(int16_t)u16(m->oam+matrix+24)*ry)>>8)+h/2;
        if(dx<0||dy<0||dx>=w||dy>=h) return p;
    } else {
        if(b&0x1000) dx=w-1-dx;
        if(b&0x2000) dy=h-1-dy;
    }
    unsigned tile=c&1023,bpp8=(a&0x2000)!=0;
    if(bpp8&&!(display&0x40))tile&=~1u;
    if(mode>=3 && tile<512) return p;
    unsigned rowstride=(display&0x40)?(unsigned)(w/8)*(bpp8?2:1):32;
    unsigned column=(unsigned)(dx/8)*(bpp8?2:1);
    if(display&0x40)tile+=(unsigned)(dy/8)*rowstride+column;
    else tile=(tile&~31u)+(unsigned)(dy/8)*32+((tile+column)&31);
    unsigned index;
    if(bpp8) index=vram(m,0x10000+tile*32+(dy&7)*8+(dx&7));
    else {
        unsigned packed=vram(m,0x10000+tile*32+(dy&7)*4+(dx&7)/2);
        index=(packed>>((dx&1)*4))&15;
        if(index) index+=((c>>12)&15)*16;
    }
    p.layer=4;p.priority=(uint8_t)((c>>10)&3);p.semi=((a>>10)&3)==1;
    if(!index)return p;
    p.color=palette(m,256+index);p.opaque=true;
    return p;
}
static bool above(Pixel a,Pixel b)
{
    if(a.priority!=b.priority) return a.priority<b.priority;
    if(a.layer==4 && b.layer!=4) return true;
    if(b.layer==4 && a.layer!=4) return false;
    return a.layer<b.layer;
}
/* Keep OAM order, including transparent priority effects, but reject objects
   outside this scanline before the per-pixel loop. Registers are stable while
   a line is rendered; raster effects are applied between lines by the clock. */
static unsigned line_objects(const HNMemory *m,unsigned y,unsigned indices[128])
{
    static const uint8_t widths[3][4]={{8,16,32,64},{16,32,32,64},{8,8,16,32}};
    static const uint8_t heights[3][4]={{8,16,32,64},{8,8,16,32},{16,32,32,64}};
    unsigned count=0,mh=((reg16(m,0x4C)>>8)&15)+1;
    for(unsigned n=0;n<128;n++) {
        const uint8_t *o=m->oam+n*8;unsigned a=u16(o),b=u16(o+2),shape=a>>14,size=b>>14;
        bool affine=(a&0x100)!=0;
        if(shape==3||(!affine&&(a&0x200))||((a>>10)&3)==3)continue;
        int w=widths[shape][size],h=heights[shape][size],ox=b&511,oy=a&255;
        if(affine&&(a&0x200)){w*=2;h*=2;}
        if(ox>=256)ox-=512;if(oy>=160)oy-=256;
        int right=ox+w;
        if((a&0x1000)&&right%(int)mh)right+=(int)mh-right%(int)mh;
        if((int)y<oy||(int)y>=oy+h||ox>=HN_WIDTH||right<=0)continue;
        indices[count++]=n;
    }
    return count;
}
static uint16_t blend(uint16_t a,uint16_t b,unsigned ea,unsigned eb)
{
    uint16_t out=0;
    if(ea>16) ea=16; if(eb>16) eb=16;
    for(unsigned shift=0;shift<15;shift+=5) {
        unsigned c=(((a>>shift)&31)*ea+((b>>shift)&31)*eb)>>4;
        if(c>31) c=31;
        out|=(uint16_t)(c<<shift);
    }
    return out;
}
void hn_video_render_line(const HNMemory *m,unsigned y,uint32_t *out)
{
    if(y>=HN_HEIGHT) return;
    unsigned display=reg16(m,0),mode=display&7;
    if(display&0x80){for(unsigned x=0;x<HN_WIDTH;x++)out[x]=0xFFFFFFFFu;return;}
    unsigned indices[128],object_count=(display&0x1000)?line_objects(m,y,indices):0;
    for(unsigned x=0;x<HN_WIDTH;x++) {
        Pixel obj=empty(); bool objwindow=false;
        for(unsigned index=0;index<object_count;index++) {
            unsigned n=indices[index];
            Pixel p=object(m,n,(int)x,(int)y,display,mode);
            if(p.layer!=4)continue;
            if(((u16(m->oam+n*8)>>10)&3)==2){if(p.opaque)objwindow=true;continue;}
            if(!obj.opaque){if(p.opaque)obj=p;}
            else if(p.priority<obj.priority){
                if(p.opaque)obj=p;
                else{obj.priority=p.priority;obj.semi=p.semi;}
            }
        }
        unsigned mask=63;
        if(display&0xE000) {
            if((display&0x2000)&&in_range((int)x,reg16(m,0x40))&&in_range((int)y,reg16(m,0x44)))
                mask=reg16(m,0x48)&63;
            else if((display&0x4000)&&in_range((int)x,reg16(m,0x42))&&in_range((int)y,reg16(m,0x46)))
                mask=(reg16(m,0x48)>>8)&63;
            else if((display&0x8000)&&objwindow) mask=(reg16(m,0x4A)>>8)&63;
            else mask=reg16(m,0x4A)&63;
        }
        Pixel top={palette(m,0),5,4,true,false},next=top;
        for(unsigned bg=0;bg<4;bg++) {
            if(!(display&(0x100u<<bg)) || !(mask&(1u<<bg))) continue;
            if((mode==1&&bg==3)||(mode==2&&bg<2)||(mode>=3&&bg!=2)||mode>5) continue;
            Pixel p=mode>=3?bitmap(m,(int)x,(int)y,mode,display):background(m,bg,(int)x,(int)y,mode);
            if(!p.opaque) continue;
            if(above(p,top)) { next=top; top=p; }
            else if(above(p,next)) next=p;
        }
        if(obj.opaque&&(mask&16)) {
            if(above(obj,top)) { next=top; top=obj; }
            else if(above(obj,next)) next=obj;
        }
        uint16_t color=top.color;
        unsigned ctl=reg16(m,0x50),effect=(ctl>>6)&3;
        bool first=(ctl&(1u<<top.layer))!=0,second=(ctl&(0x100u<<next.layer))!=0;
        if(mask&32) {
            if((top.semi || (first&&effect==1))&&second)
                color=blend(color,next.color,reg16(m,0x52)&31,(reg16(m,0x52)>>8)&31);
            else if(first&&(effect==2||effect==3)) {
                unsigned amount=reg16(m,0x54)&31;
                if(amount>16) amount=16;
                uint16_t adjusted=0;
                for(unsigned shift=0;shift<15;shift+=5) {
                    unsigned channel=(color>>shift)&31;
                    channel=effect==2?channel+(((31-channel)*amount)>>4):channel-((channel*amount)>>4);
                    adjusted|=(uint16_t)(channel<<shift);
                }
                color=adjusted;
            }
        }
        out[x]=rgba(color);
    }
}
void hn_video_render(const HNMemory *m,uint32_t *rgba_out)
{ for(unsigned y=0;y<HN_HEIGHT;y++) hn_video_render_line(m,y,rgba_out+y*HN_WIDTH); }
