#include "game/render.h"
#include "game/state.h"
#include "game/transfer.h"
#include "gba/bios.h"
static bool fail(HNRender *r,HNRenderFault fault,unsigned op,GameAddr address)
{r->fault=fault;r->fault_opcode=op;r->fault_address=address;return false;}
static GameAddr base(HNMemory *m,GameAddr slot)
{
    for(unsigned i=4;i;i--)if(hn_read16(m,slot+0x56+(i-1)*2)&0x8000)
        return hn_read32(m,slot+8+(i-1)*4);
    return hn_read32(m,slot+4);
}
static void advance(HNMemory *m,GameAddr slot,GameAddr *cursor,unsigned bytes)
{
    *cursor+=bytes;
    hn_write16(m,slot+0x4C,(uint16_t)(hn_read16(m,slot+0x4C)+bytes));
}
static void jump(HNMemory *m,GameAddr slot,GameAddr *cursor)
{
    GameAddr target=hn_read32(m,*cursor+1);unsigned offset=target-base(m,slot);
    *cursor=target;
    if(offset<=0x7FFF)hn_write16(m,slot+0x4C,(uint16_t)offset);
    else{hn_write32(m,slot+4,target);hn_write16(m,slot+0x4C,0);hn_render_error(m,0x101);}
}
static void affine(HNMemory *m,GameAddr slot)
{
    if(hn_read8(m,slot+0x64)==255){
        uint8_t index=hn_render_find_affine(m);hn_write8(m,slot+0x64,index);
        hn_write8(m,HN_AFFINE_USAGE+index,1);
    }
}
bool hn_animation_command(HNRender *r,GameAddr slot,unsigned op,GameAddr *cursor,uint32_t *yield)
{
    if(r->fault)return false;
    HNMemory *m=r->memory;GameAddr data=*cursor;*yield=0;
    switch(op){
    case 0x80: {
        unsigned i=4;
        while(i&&!(hn_read16(m,slot+0x56+(i-1)*2)&0x8000))i--;
        if(!i){hn_write8(m,slot+0x60,0);*yield=1;break;}
        i--;unsigned offset=hn_read16(m,slot+0x56+i*2)&0x7FFF;
        hn_write16(m,slot+0x4C,(uint16_t)offset);hn_write16(m,slot+0x56+i*2,0);
        hn_write32(m,slot+8+i*4,0);*cursor=base(m,slot)+offset;break;
    }
    case 0x81: {
        GameAddr target=hn_read32(m,data+1);hn_write32(m,slot+4,target);
        *cursor=target;hn_write16(m,slot+0x4C,0);break;
    }
    case 0x82:jump(m,slot,cursor);break;
    case 0x83:
        hn_write16(m,slot+0x4C,hn_read16(m,data+1));*cursor=hn_read32(m,slot+4)+hn_read16(m,slot+0x4C);break;
    case 0x84:case 0x85:case 0x86:case 0x8A:case 0x8B:case 0x8C: {
        bool half=op>=0x8A;unsigned kind=half?op-0x8A:op-0x84;
        GameAddr address=hn_read32(m,data+1);
        unsigned left=half?hn_read16(m,address):hn_read8(m,address);
        unsigned right=half?hn_read16(m,data+5):hn_read8(m,data+5);
        bool take=kind==0?left==right:kind==1?left<right:left>right;
        if(take){*cursor=data+(half?6:5);jump(m,slot,cursor);}
        else advance(m,slot,cursor,half?11:10);
        break;
    }
    case 0x87:case 0x89: {
        unsigned bytes=op==0x87?2:5;
        if(hn_read16(m,slot+0x5C)){hn_render_error(m,1);advance(m,slot,cursor,bytes);break;}
        unsigned i=0;while(i<4&&hn_read16(m,slot+0x56+i*2))i++;
        if(op==0x87){
            hn_write16(m,slot+0x56+i*2,hn_read8(m,data+1));
            advance(m,slot,cursor,2);hn_write32(m,slot+8+i*4,data+2);
        }else{
            GameAddr target=hn_read32(m,data+1);
            hn_write32(m,slot+8+i*4,target);*cursor=target;
            hn_write16(m,slot+0x56+i*2,(uint16_t)((hn_read16(m,slot+0x4C)+5)|0x8000));
            hn_write16(m,slot+0x4C,0);
        }
        break;
    }
    case 0x88: {
        if(!hn_read16(m,slot+0x56)){hn_render_error(m,2);advance(m,slot,cursor,1);break;}
        unsigned i=4;while(i&&!hn_read16(m,slot+0x56+(i-1)*2))i--;i--;
        uint16_t count=(uint16_t)(hn_read16(m,slot+0x56+i*2)-1);hn_write16(m,slot+0x56+i*2,count);
        if(!count){hn_write32(m,slot+8+i*4,0);advance(m,slot,cursor,1);break;}
        GameAddr target=hn_read32(m,slot+8+i*4),origin=hn_read32(m,slot+4);
        for(unsigned j=i+1;j;j--)if(hn_read16(m,slot+0x56+(j-1)*2)&0x8000){origin=hn_read32(m,slot+8+(j-1)*4);break;}
        unsigned offset=target-origin;
        if(offset>0x7FFF){hn_render_error(m,3);advance(m,slot,cursor,1);}
        else{*cursor=target;hn_write16(m,slot+0x4C,(uint16_t)offset);}
        break;
    }
    case 0x8D: {
        GameAddr address=hn_read32(m,data+1);HNAnimationFunction function=NULL;
        for(size_t i=0;i<r->callback_count;i++)if((r->callbacks[i].address&~1u)==(address&~1u)){function=r->callbacks[i].function;break;}
        if(!function)return fail(r,HN_RENDER_CALLBACK,op,address);
        unsigned bytes=function(m,slot,data+5,yield);advance(m,slot,cursor,5+bytes);break;
    }
    case 0x8E:case 0x8F: {
        unsigned index=(uint8_t)hn_render_class(m,slot);
        unsigned left=hn_read16(m,0x0201DC40+index*2),right=hn_read16(m,data+1);
        if(op==0x8E?left<right:left>right){*cursor=data+2;jump(m,slot,cursor);}
        else advance(m,slot,cursor,7);
        break;
    }
    case 0x90:advance(m,slot,cursor,1);break;
    case 0x91:case 0x9F:case 0xCE:case 0xCF:break;
    case 0x92:case 0x93:case 0x94:case 0x95:case 0x96:case 0x97:
    case 0x98:case 0x99:case 0x9A:case 0x9B:case 0x9C:case 0x9D: {
        unsigned group=(op-0x92)/3,width=1u<<((op-0x92)%3);
        GameAddr address=hn_read32(m,data+1);uint32_t value=0;
        if(group)value=width==1?hn_read8(m,address):width==2?hn_read16(m,address):hn_read32(m,address);
        if(group==0)value=width==1?hn_read8(m,data+5):width==2?hn_read16(m,data+5):hn_read32(m,data+5);
        else if(group==1)value++;
        else if(group==2)value--;
        else{
            uint32_t add=width==1?(uint32_t)(int32_t)(int8_t)hn_read8(m,data+5):
                width==2?(uint32_t)(int32_t)(int16_t)hn_read16(m,data+5):hn_read32(m,data+5);
            value+=add;
        }
        if(width==1)hn_write8(m,address,(uint8_t)value);
        else if(width==2)hn_write16(m,address,(uint16_t)value);
        else hn_write32(m,address,value);
        advance(m,slot,cursor,5+((group==0||group==3)?width:0));break;
    }
    case 0x9E: {
        unsigned index=(uint8_t)hn_render_class(m,slot);GameAddr address=0x0201DC40+index*2;
        hn_write16(m,address,(uint16_t)(hn_read16(m,address)+hn_read16(m,data+1)));
        advance(m,slot,cursor,3);break;
    }
    case 0xA0:case 0xA1:case 0xA2:
        hn_write8(m,slot+(op==0xA0?0x61:op==0xA1?0x60:0x62),hn_read8(m,data+1));
        advance(m,slot,cursor,2);break;
    case 0xA3:case 0xA4:case 0xA5:
        hn_write32(m,slot+0x18+(op-0xA3)*4,((uint32_t)hn_read16(m,data+1)<<16)+0x8000);
        advance(m,slot,cursor,3);break;
    case 0xA6:case 0xA7:case 0xA8: {
        unsigned offset=0x18+(op-0xA6)*4;GameAddr other=hn_render_slot(m,hn_read8(m,data+1));
        hn_write32(m,slot+offset,((uint32_t)hn_read16(m,data+2)<<16)+hn_read32(m,other+offset));
        advance(m,slot,cursor,4);break;
    }
    case 0xA9:case 0xAA:case 0xAB: {
        unsigned offset=0x18+(op-0xA9)*4;
        hn_write32(m,slot+offset,hn_read32(m,slot+offset)+((uint32_t)hn_read16(m,data+1)<<16));
        advance(m,slot,cursor,3);break;
    }
    case 0xAC:case 0xAD:case 0xAE:case 0xAF:case 0xB0:case 0xB1:
        hn_write32(m,slot+0x24+(op-0xAC)*4,hn_read32(m,data+1));advance(m,slot,cursor,5);break;
    case 0xB2:
        hn_write8(m,HN_AFFINE_USAGE+hn_read8(m,slot+0x64),255);hn_write8(m,slot+0x64,255);
        hn_write16(m,slot+0x44,0);hn_write16(m,slot+0x46,0);
        hn_write16(m,slot+0x3C,256);hn_write16(m,slot+0x3E,256);advance(m,slot,cursor,1);break;
    case 0xB3:case 0xB4:case 0xB5:case 0xB6:case 0xBC:case 0xBD: {
        unsigned offset=op<=0xB4?0x44+(op-0xB3)*2:op<=0xB6?0x3C+(op-0xB5)*2:op==0xBC?0x40:0x48;
        affine(m,slot);hn_write16(m,slot+offset,hn_read16(m,data+1));advance(m,slot,cursor,3);break;
    }
    case 0xB7:
        hn_write8(m,slot+0x66,hn_read8(m,slot+0x62));
        hn_write8(m,slot+0x62,(uint8_t)((hn_read8(m,slot+0x62)&63)|hn_read8(m,data+1)));
        advance(m,slot,cursor,2);break;
    case 0xB8:
        hn_write8(m,slot+0x62,hn_read8(m,slot+0x66));advance(m,slot,cursor,1);break;
    case 0xB9:case 0xBA:case 0xBB: {
        unsigned offset=0x18+(op-0xB9)*4;
        hn_write32(m,slot+offset,(hn_read32(m,slot+offset)&0xFFFF0000u)+hn_read16(m,data+1));
        advance(m,slot,cursor,3);break;
    }
    case 0xBE:case 0xBF:
        affine(m,slot);hn_write8(m,slot+0x5E+(op-0xBE),hn_read8(m,data+1));advance(m,slot,cursor,2);break;
    case 0xC0:
        hn_write8(m,hn_read32(m,HN_RENDER_STATES)+hn_read8(m,data+1)*80+0x38,hn_read8(m,data+2));
        advance(m,slot,cursor,3);break;
    case 0xC1:case 0xC2:
        hn_write16(m,slot+0x52+(op-0xC1)*2,hn_read16(m,data+1));advance(m,slot,cursor,3);break;
    case 0xC3:
        hn_write8(m,hn_render_slot(m,hn_read8(m,data+1))+0x60,hn_read8(m,data+2));advance(m,slot,cursor,3);break;
    case 0xC4:case 0xC5: {
        unsigned value=hn_read8(m,slot+0x61);
        hn_write8(m,slot+0x61,(uint8_t)(op==0xC4?value|64:value&~64u));advance(m,slot,cursor,1);break;
    }
    case 0xC6:
        hn_write8(m,slot+0x63,hn_read8(m,data));hn_write32(m,slot,hn_read32(m,data+1));
        advance(m,slot,cursor,5);*yield=hn_read8(m,slot+0x63)!=0;break;
    case 0xC7:
        hn_write8(m,slot+0x63,hn_read8(m,data+1));advance(m,slot,cursor,2);*yield=hn_read8(m,slot+0x63)!=0;break;
    case 0xC8:case 0xC9: {
        GameAddr destination=hn_read32(m,data+1),source=hn_read32(m,data+5);
        if(op==0xC8)hn_transfer_queue(m,source,destination,hn_read16(m,data+9));
        else hn_transfer_tiles(m,source,destination,hn_read8(m,data+9),hn_read8(m,data+10));
        advance(m,slot,cursor,11);break;
    }
    case 0xCA:case 0xCB: {
        GameAddr destination,source;
        if(op==0xCA){destination=hn_read32(m,data+1);source=hn_read32(m,data+5);}
        else{
            unsigned set=(uint8_t)(((int)hn_render_find_slot(m,slot)-10)/3);
            destination=hn_read32(m,HN_RENDER_TILE_SETS+set*284);source=hn_read32(m,data+1);
        }
        if(!hn_bios_lz77(m,source,destination,0x40000))return fail(r,HN_RENDER_RESOURCE,op,source);
        advance(m,slot,cursor,op==0xCA?9:5);break;
    }
    case 0xCC: {
        int set=((int)hn_render_find_slot(m,slot)-10)/3;
        GameAddr tiles=HN_RENDER_TILE_SETS+(uint32_t)(set*284);
        hn_transfer_tiles(m,hn_read32(m,tiles),hn_read32(m,tiles+4),hn_read8(m,tiles+280),hn_read8(m,tiles+281));
        advance(m,slot,cursor,1);break;
    }
    case 0xCD: {
        int set=((int)hn_render_find_slot(m,slot)-10)/3;unsigned frame=hn_read8(m,data+2);
        hn_write8(m,slot+0x63,hn_read8(m,data+1));
        hn_write32(m,slot,hn_read32(m,HN_RENDER_TILE_SETS+8+(uint32_t)(set*284)+frame*4));
        advance(m,slot,cursor,3);*yield=hn_read8(m,slot+0x63)!=0;break;
    }
    case 0xD0:case 0xD1:
        if(!(hn_read8(m,0x03002182)&64)&&!(hn_read8(m,slot+0x61)&64))return fail(r,HN_RENDER_RESOURCE,op,data);
        advance(m,slot,cursor,3);break;
    default:return fail(r,HN_RENDER_OPCODE,op,data);
    }
    if(m->fault)return fail(r,HN_RENDER_MEMORY,op,m->fault_address);
    return true;
}
bool hn_render_animation(HNRender *r,GameAddr slot,unsigned budget)
{
    if(r->fault)return false;
    HNMemory *m=r->memory;GameAddr cursor=base(m,slot)+hn_read16(m,slot+0x4C);
    for(unsigned i=0;i<budget;i++){
        unsigned op=hn_read8(m,cursor);if(op<0x80)op=0xC6;
        unsigned delay=hn_read8(m,slot+0x63);
        if(delay){hn_write8(m,slot+0x63,(uint8_t)(delay-1));if(delay!=1)return true;continue;}
        uint32_t yield;
        if(!hn_animation_command(r,slot,op,&cursor,&yield))return false;
        if(yield)return true;
    }
    return fail(r,HN_RENDER_BUDGET,hn_read8(m,cursor),cursor);
}
