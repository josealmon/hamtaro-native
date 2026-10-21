#include "game/graphics.h"
#include "game/state.h"
#include "game/video.h"
#include "game/render.h"
#include "game/raster.h"
#include "gba/bios.h"
static bool fail(HNGraphics *g,HNGraphicsFault fault,unsigned op,GameAddr address)
{g->fault=fault;g->fault_opcode=op;g->fault_address=address;return false;}
void hn_graphics_init(HNGraphics *g,HNMemory *m)
{g->memory=m;g->fault=HN_GRAPHICS_OK;g->fault_opcode=0;g->fault_address=0;}
bool hn_graphics_configure(HNMemory *m,uint8_t index,GameAddr data)
{
    if(index>=hn_read8(m,HN_GRAPHICS_COUNT))return false;
    GameAddr s=hn_read32(m,HN_GRAPHICS_SCRIPTS)+index*44;
    hn_write32(m,s+0x18,data);hn_write8(m,s,1);hn_write8(m,s+1,0);hn_write8(m,s+2,0);
    hn_write16(m,s+0x1C,0);hn_write16(m,s+0x1E,0);hn_write16(m,s+0x20,0);
    for(unsigned i=0;i<4;i++){hn_write8(m,s+3+i,0);hn_write32(m,s+8+i*4,0);}
    return !m->fault;
}
bool hn_palette_prepare(HNMemory *m,uint32_t amount,uint32_t first,uint32_t count)
{
    first&=65535;count&=65535;amount&=255;
    if(!count||first>=512||count>512-first)return false;
    const GameAddr current[3]={HN_PALETTE_RED,HN_PALETTE_GREEN,HN_PALETTE_BLUE};
    const GameAddr target[3]={HN_PALETTE_TARGET_RED,HN_PALETTE_TARGET_GREEN,HN_PALETTE_TARGET_BLUE};
    const GameAddr delta[3]={HN_PALETTE_DELTA_RED,HN_PALETTE_DELTA_GREEN,HN_PALETTE_DELTA_BLUE};
    for(unsigned i=first;i<first+count&&!m->fault;i++){
        GameAddr source=HN_PALETTE_SOURCE+(i&255)*2,output=hn_read32(m,HN_PALETTE_OUTPUT)+i*2;
        GameAddr transition=HN_PALETTE_TRANSITIONS+i*8,accumulator=HN_PALETTE_ACCUMULATORS+i*6;
        unsigned desired=hn_read16(m,source),old=hn_read16(m,output);
        if(desired==old){hn_write16(m,transition,0);continue;}
        hn_write16(m,transition,1);desired=hn_read16(m,source);hn_write16(m,HN_PALETTE_COLOR,(uint16_t)desired);
        for(unsigned channel=0;channel<3;channel++)hn_write16(m,target[channel],(uint16_t)((desired>>(channel*5))&31));
        old=hn_read16(m,output);hn_write16(m,HN_PALETTE_COLOR,(uint16_t)old);
        for(unsigned channel=0;channel<3;channel++)hn_write16(m,current[channel],(uint16_t)((old>>(channel*5))&31));
        for(unsigned channel=0;channel<3;channel++){
            unsigned value=hn_read16(m,current[channel]);uint16_t negative=(uint16_t)(0u-(value<<8));
            hn_write16(m,delta[channel],negative);
            int16_t step=(int16_t)((hn_read16(m,target[channel])<<8)+negative);
            step=(int16_t)(step>>(amount<16?amount:15));hn_write16(m,target[channel],(uint16_t)step);
            hn_write16(m,accumulator+channel*2,(uint16_t)(value<<8));hn_write16(m,transition+2+channel*2,(uint16_t)step);
        }
    }
    return !m->fault;
}
bool hn_palette_update(HNMemory *m,uint32_t first,uint32_t count)
{
    first&=65535;count&=65535;if(!count||first>=512||count>512-first)return false;
    for(unsigned i=first;i<first+count&&!m->fault;i++){
        GameAddr transition=HN_PALETTE_TRANSITIONS+i*8,accumulator=HN_PALETTE_ACCUMULATORS+i*6;
        if(!hn_read16(m,transition))continue;
        unsigned values[3];
        for(unsigned channel=0;channel<3;channel++){
            uint16_t value=(uint16_t)(hn_read16(m,accumulator+channel*2)+hn_read16(m,transition+2+channel*2));
            hn_write16(m,accumulator+channel*2,value);values[channel]=value&0x1F00;
        }
        hn_write16(m,HN_PALETTE_RED,(uint16_t)values[0]);hn_write16(m,HN_PALETTE_GREEN,(uint16_t)values[1]);
        hn_write16(m,HN_PALETTE_BLUE,(uint16_t)values[2]);
        uint16_t packed=(uint16_t)((values[0]>>8)|(values[1]>>3)|(values[2]<<2));
        hn_write16(m,HN_PALETTE_COLOR,packed);hn_write16(m,hn_read32(m,HN_PALETTE_OUTPUT)+i*2,packed);
    }
    return !m->fault;
}
void hn_mosaic_registers(HNMemory *m,uint8_t selector,uint8_t x,uint8_t y)
{
    uint8_t value=(uint8_t)((x&15)|(y<<4));
    if(selector==0||selector==2)hn_write8(m,HN_MOSAIC,value);
    if(selector==1||selector==2)hn_write8(m,HN_MOSAIC+1,value);
}
void hn_mosaic_sizes(HNMemory *m,uint8_t selector,uint8_t x,uint8_t y)
{
    if(selector==0||selector==2){hn_write8(m,HN_MOSAIC+4,x);hn_write8(m,HN_MOSAIC+5,y);}
    if(selector==1||selector==2){hn_write8(m,HN_MOSAIC+6,x);hn_write8(m,HN_MOSAIC+7,y);}
}
static void cursor(HNMemory *m,GameAddr s,GameAddr p,unsigned bytes)
{hn_write32(m,s+0x18,p+bytes);}
static void transition_end(HNMemory *m,GameAddr s)
{
    unsigned remaining=(hn_read8(m,s+1)-1)&255;hn_write8(m,s+1,(uint8_t)remaining);
    if(!remaining)hn_write8(m,s,1);else hn_write16(m,s+0x1C,hn_read8(m,s+2));
}
static bool begin_palette(HNMemory *m,GameAddr s,GameAddr p,unsigned count,unsigned offset)
{
    unsigned group=hn_read8(m,p),first=hn_read16(m,p+2);
    hn_write8(m,s+1,(uint8_t)(hn_read8(m,HN_PALETTE_GROUP_SIZES+group)-offset));
    hn_write8(m,s+2,hn_read8(m,p+1));hn_write16(m,s+0x1E,(uint16_t)first);
    cursor(m,s,p,4);hn_write8(m,s,0x84);hn_write16(m,s+0x20,(uint16_t)count);
    return hn_palette_prepare(m,group,first,(uint16_t)count);
}
bool hn_graphics_command(HNGraphics *g,GameAddr s,unsigned op,GameAddr p,uint32_t *running)
{
    if(g->fault)return false;HNMemory *m=g->memory;*running=1;
    switch(op){
    case 0x80:case 0x8B: {
        int index=3;while(index>=0&&!hn_read8(m,s+3+index))index--;
        if(index<0){if(op==0x80){hn_write8(m,s,0);*running=0;}else cursor(m,s,p,0);}
        else if(op==0x80){hn_write8(m,s+3+index,0);cursor(m,s,hn_read32(m,s+8+index*4),0);}
        else{
            unsigned count=(hn_read8(m,s+3+index)-1)&255;hn_write8(m,s+3+index,(uint8_t)count);
            cursor(m,s,count?hn_read32(m,s+8+index*4):p,0);
        }break;
    }
    case 0x81:hn_write16(m,s+0x1C,hn_read16(m,p));cursor(m,s,p,2);break;
    case 0x82:cursor(m,s,hn_read32(m,p),0);break;
    case 0x83:case 0x8C: {
        unsigned bytes=hn_read8(m,p+1),width=op==0x83?1:2,count=bytes/width;GameAddr target=hn_read32(m,p+2);
        hn_write16(m,s+0x1C,hn_read8(m,p));
        for(unsigned i=0;i<count&&!m->fault;i++){
            if(width==1)hn_write8(m,target+i,hn_read8(m,p+6+i));
            else hn_write16(m,target+i*2,hn_read16(m,p+6+i*2));
        }
        cursor(m,s,p,6+count*width);break;
    }
    case 0x84:case 0x86:case 0x89: {
        unsigned count=op==0x89?hn_read8(m,p):hn_read16(m,HN_GRAPHICS_DECOMPRESSED);
        unsigned offset=op==0x84?0:hn_read8(m,p+(op==0x89?1:0));p+=op==0x89?2:op==0x86?1:0;
        if(!begin_palette(m,s,p,count,offset))return fail(g,HN_GRAPHICS_RESOURCE,op,p);*running=0;break;
    }
    case 0x85:
        if(!hn_palette_update(m,hn_read16(m,s+0x1E),hn_read16(m,s+0x20)))return fail(g,HN_GRAPHICS_RESOURCE,op,p);
        transition_end(m,s);*running=0;break;
    case 0x87: {
        unsigned count=hn_read8(m,p+1);GameAddr target=hn_read32(m,p+2),source=hn_read32(m,p+6);
        hn_write16(m,s+0x1C,hn_read8(m,p));
        for(unsigned i=0;i<count&&!m->fault;i++)hn_write16(m,target+i*2,hn_read16(m,source+i*2));cursor(m,s,p,10);break;
    }
    case 0x88: {
        hn_write16(m,s+0x1C,hn_read8(m,p));GameAddr target=hn_read32(m,p+1),source=hn_read32(m,p+5);
        hn_write32(m,HN_GRAPHICS_DECOMPRESSED,(hn_read32(m,source)>>8)>>1);
        if(!hn_bios_lz77(m,source,target,0x40000))return fail(g,HN_GRAPHICS_RESOURCE,op,p);cursor(m,s,p,9);break;
    }
    case 0x8A:case 0xAF: {
        unsigned i=0;while(i<4&&hn_read8(m,s+3+i))i++;
        if(op==0xAF){if(i<4){hn_write8(m,s+3+i,1);hn_write32(m,s+8+i*4,p+4);cursor(m,s,hn_read32(m,p),0);}else cursor(m,s,p,4);}
        else{
            if(i<4){unsigned count=hn_read8(m,p);hn_write8(m,s+3+i,(uint8_t)(count?count:1));hn_write32(m,s+8+i*4,p+1);}
            cursor(m,s,p,1);
        }break;
    }
    case 0x8D: {
        hn_write16(m,s+0x1C,hn_read8(m,p));unsigned width=hn_read8(m,p+1),rows=hn_read8(m,p+2);
        GameAddr target=hn_read32(m,p+3);
        for(unsigned row=0;row<rows&&!m->fault;row++)for(unsigned i=0;i<width;i++)
            hn_write8(m,target+row*width+width-1-i,hn_read8(m,p+7+i));
        cursor(m,s,p,7+width);break;
    }
    case 0x8E: {
        hn_write16(m,s+0x1C,hn_read8(m,p));unsigned effect=hn_read8(m,p+1),a=hn_read8(m,p+2),b=hn_read8(m,p+3);
        if(effect!=255)hn_video_blend_effect(m,effect&3);
        hn_video_blend_targets(m,a==255?0:(uint8_t)(a&63),b==255?0:(uint8_t)(b&63));
        hn_video_blend_coefficients(m,hn_read8(m,p+4),hn_read8(m,p+5),hn_read8(m,p+6));cursor(m,s,p,7);break;
    }
    case 0x8F:case 0x90: {
        hn_write8(m,s,0x91);hn_write8(m,s+2,hn_read8(m,p));unsigned target=hn_read8(m,p+1);
        hn_write16(m,0x04000050,(uint16_t)(((op-0x8D)<<6)|(target==255?0:target&63)));
        unsigned parameters=hn_read8(m,p+2),magnitude=parameters&15;
        unsigned remaining=hn_read8(m,HN_PALETTE_GROUP_SIZES+magnitude),step=16u>>magnitude;
        unsigned value=parameters&16?((0u-step)<<8)|16:step<<8;
        remaining=(remaining-hn_read8(m,HN_PALETTE_GROUP_SIZES+hn_read8(m,p+3)))&255;
        hn_write8(m,s+1,(uint8_t)((remaining&128)?0:remaining));
        if(hn_read8(m,p+4))value=(value&0xFF00)|hn_read8(m,p+4);
        hn_write16(m,s+0x1E,(uint16_t)value);cursor(m,s,p,5);break;
    }
    case 0x91: {
        unsigned old=hn_read16(m,s+0x1E),delta=old>>8,value=(old+delta)&255;
        value=value&128?0:value>16?16:value;hn_write16(m,0x04000054,(uint16_t)value);
        hn_write16(m,s+0x1E,(uint16_t)((delta<<8)|value));
        unsigned remaining=hn_read8(m,s+1);
        if(!remaining)hn_write8(m,s,1);else{hn_write8(m,s+1,(uint8_t)(remaining-1));hn_write16(m,s+0x1C,hn_read8(m,s+2));}
        *running=0;break;
    }
    case 0x92:case 0x93:case 0x94: {
        unsigned value=hn_read8(m,hn_read32(m,p)),expected=hn_read8(m,p+4);
        bool take=op==0x92?value==expected:op==0x93?value>expected:value<expected;
        cursor(m,s,take?hn_read32(m,p+5):p+9,0);break;
    }
    case 0x95:hn_video_blend_effect(m,0);hn_video_blend_targets(m,0,0);hn_video_blend_coefficients(m,0,0,0);cursor(m,s,p,0);break;
    case 0x96: {
        hn_write8(m,s,0x97);hn_write16(m,s+0x1C,hn_read8(m,p));hn_write8(m,s+2,hn_read8(m,p));
        hn_video_blend_effect(m,1);hn_video_blend_targets(m,hn_read8(m,p+1)&63,hn_read8(m,p+2)&63);
        unsigned remaining=hn_read8(m,p+3);if(!remaining)remaining=1;hn_write8(m,s+1,(uint8_t)remaining);
        int a=hn_read8(m,p+4),b=hn_read8(m,p+5),end_a=hn_read8(m,p+6),end_b=hn_read8(m,p+7);
        hn_video_blend_coefficients(m,(uint8_t)a,(uint8_t)b,0);
        hn_write16(m,s+0x22,(uint16_t)(a*256));hn_write16(m,s+0x24,(uint16_t)(b*256));
        hn_write16(m,s+0x26,(uint16_t)((end_a-a)*256/(int)remaining));hn_write16(m,s+0x28,(uint16_t)((end_b-b)*256/(int)remaining));
        cursor(m,s,p,8);break;
    }
    case 0x97:case 0x9F:case 0xA0: {
        uint16_t a=(uint16_t)(hn_read16(m,s+0x22)+hn_read16(m,s+0x26));
        uint16_t b=(uint16_t)(hn_read16(m,s+0x24)+hn_read16(m,s+0x28));
        hn_write16(m,s+0x22,a);hn_write16(m,s+0x24,b);
        if(op==0x97){hn_video_blend_effect(m,1);hn_video_blend_coefficients(m,(uint8_t)(a>>8),(uint8_t)(b>>8),0);}
        else{
            unsigned selector=hn_read16(m,s+0x1E)&255;
            if(hn_read8(m,s)==0x9F){hn_mosaic_registers(m,(uint8_t)selector,(uint8_t)(a>>8),(uint8_t)(b>>8));hn_write8(m,HN_MOSAIC+2,hn_read8(m,HN_MOSAIC+2)|1);}
            else hn_mosaic_sizes(m,(uint8_t)selector,(uint8_t)(a>>8),(uint8_t)(b>>8));
            hn_mosaic_update(m);hn_video_mark_backgrounds(m,15);
        }
        transition_end(m,s);*running=0;break;
    }
    case 0x98: {
        hn_mosaic_init(m);hn_video_mark_backgrounds(m,15);
        for(unsigned i=0;i<hn_read8(m,HN_RENDER_INFO+16);i++){
            GameAddr slot=hn_render_slot(m,i);hn_write8(m,slot+0x61,hn_read8(m,slot+0x61)&0xEF);
        }cursor(m,s,p,0);break;
    }
    case 0x99:case 0x9A: {
        unsigned selector=hn_read8(m,p);uint8_t x=hn_read8(m,p+1),y=hn_read8(m,p+2);
        if(op==0x99){hn_mosaic_registers(m,(uint8_t)selector,x,y);hn_write8(m,HN_MOSAIC+2,hn_read8(m,HN_MOSAIC+2)|1);}
        else hn_mosaic_sizes(m,(uint8_t)selector,x,y);
        hn_video_mark_backgrounds(m,15);hn_mosaic_update(m);cursor(m,s,p,3);break;
    }
    case 0x9B: {
        unsigned mask=hn_read8(m,p);
        for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){
            GameAddr control=HN_VIDEO_STATE+0x38+i*48;hn_write16(m,control,hn_read16(m,control)|64);
        }
        hn_video_mark_backgrounds(m,15);
        if(mask&0x60)for(unsigned i=0;i<hn_read8(m,HN_RENDER_INFO+16);i++){
            GameAddr slot=hn_render_slot(m,i);
            if((mask&0x40)||hn_read8(m,slot+0x60))hn_write8(m,slot+0x61,hn_read8(m,slot+0x61)|16);
        }
        hn_write8(m,HN_MOSAIC+3,(uint8_t)mask);cursor(m,s,p,1);break;
    }
    case 0x9C:hn_write8(m,HN_MOSAIC+2,hn_read8(m,p));hn_mosaic_update(m);cursor(m,s,p,1);break;
    case 0x9D: {
        unsigned first=hn_read8(m,p),last=hn_read8(m,p+1),count=hn_read8(m,HN_RENDER_INFO+16);if(last==255)last=count-1;
        if(last>=count)return fail(g,HN_GRAPHICS_RESOURCE,op,p);
        for(unsigned i=first;i<=last;i++){GameAddr slot=hn_render_slot(m,i);hn_write8(m,slot+0x61,hn_read8(m,slot+0x61)|16);}
        cursor(m,s,p,2);break;
    }
    case 0x9E: {
        hn_write8(m,s,hn_read8(m,p));hn_write16(m,s+0x1C,hn_read8(m,p+1));hn_write8(m,s+2,hn_read8(m,p+1));
        unsigned remaining=hn_read8(m,p+2);if(!remaining)remaining=1;hn_write8(m,s+1,(uint8_t)remaining);
        hn_write16(m,s+0x1E,hn_read8(m,p+3));
        int x=hn_read8(m,p+4)*256,y=hn_read8(m,p+5)*256,end_x=hn_read8(m,p+6)*256,end_y=hn_read8(m,p+7)*256;
        hn_write16(m,s+0x22,(uint16_t)x);hn_write16(m,s+0x24,(uint16_t)y);
        hn_write16(m,s+0x26,(uint16_t)((end_x-x)/(int)remaining));hn_write16(m,s+0x28,(uint16_t)((end_y-y)/(int)remaining));
        cursor(m,s,p,8);*running=0;break;
    }
    case 0xA1:case 0xA2:case 0xA3:case 0xA4:case 0xA5:case 0xA6:case 0xA7:case 0xA8:case 0xA9:case 0xAD:case 0xAE:
        if(!hn_raster_command(m,s,op,p,running))return fail(g,HN_GRAPHICS_RESOURCE,op,p);break;
    case 0xAA: {
        unsigned enabled=hn_read8(m,p)&1,mask=hn_read8(m,p+1),value=hn_read8(m,HN_DISPLAY_SHADOW+1);
        for(unsigned i=0;i<3;i++)if(mask&(1u<<i))value=(value&~(1u<<(5+i)))|(enabled<<(5+i));
        hn_write8(m,HN_DISPLAY_SHADOW+1,(uint8_t)value);hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|1);cursor(m,s,p,2);break;
    }
    case 0xAB:hn_write16(m,0x04000048,hn_read16(m,p));hn_write16(m,0x0400004A,hn_read16(m,p+2));cursor(m,s,p,4);break;
    case 0xAC: {
        GameAddr target=hn_read8(m,p)?0x04000042:0x04000040;
        hn_write16(m,target,hn_read16(m,p+1));hn_write16(m,target+4,hn_read16(m,p+3));cursor(m,s,p,5);break;
    }
    default:return fail(g,HN_GRAPHICS_OPCODE,op,p);
    }
    return m->fault?fail(g,HN_GRAPHICS_MEMORY,op,m->fault_address):true;
}
bool hn_graphics_step(HNGraphics *g,GameAddr state,uint32_t *running)
{
    if(g->fault)return false;HNMemory *m=g->memory;GameAddr p=hn_read32(m,state+0x18);unsigned type=hn_read8(m,state);
    if(type==0x84||type==0x91||type==0x97||type==0x9F||type==0xA0||type==0xAE)
        return hn_graphics_command(g,state,type==0x84?0x85:type,p+1,running);
    unsigned op=hn_read8(m,p++);
    if(m->fault)return fail(g,HN_GRAPHICS_MEMORY,op,m->fault_address);
    if(op<0x80){
        hn_write16(m,state+0x1C,(uint16_t)op);GameAddr target=0x05000000+hn_read16(m,p)*2,source=hn_read32(m,p+2);
        if(!hn_bios_lz77(m,source,target,0x40000))return fail(g,HN_GRAPHICS_RESOURCE,op,p);
        cursor(m,state,p,6);*running=0;return true;
    }
    return hn_graphics_command(g,state,op,p,running);
}
bool hn_graphics_update(HNGraphics *g,unsigned command_budget)
{
    if(g->fault)return false;HNMemory *m=g->memory;GameAddr state=hn_read32(m,HN_GRAPHICS_SCRIPTS);
    for(unsigned i=0;i<hn_read8(m,HN_GRAPHICS_COUNT);i++,state+=44){
        unsigned type=hn_read8(m,state);if(!type||type==255)continue;
        bool complete=false;
        for(unsigned j=0;j<command_budget;j++){
            unsigned delay=hn_read16(m,state+0x1C);
            if(delay){hn_write16(m,state+0x1C,(uint16_t)(delay-1));complete=true;break;}
            uint32_t running;if(!hn_graphics_step(g,state,&running))return false;
            if(!running){complete=true;break;}
        }
        if(!complete)return fail(g,HN_GRAPHICS_BUDGET,0,hn_read32(m,state+0x18));
    }
    return m->fault?fail(g,HN_GRAPHICS_MEMORY,0,m->fault_address):true;
}
