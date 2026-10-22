#include "game/render.h"
#include "game/state.h"
#include "gba/bios.h"
#include <string.h>
void hn_render_init(HNRender *r,HNMemory *m,const HNAnimationCallback *callbacks,size_t count)
{memset(r,0,sizeof(*r));r->memory=m;r->callbacks=callbacks;r->callback_count=count;}
GameAddr hn_render_slot(HNMemory *m,uint32_t index)
{return hn_read32(m,HN_RENDER_SLOTS)+index*0x68;}
uint8_t hn_render_find_slot(HNMemory *m,GameAddr slot)
{
    GameAddr base=hn_read32(m,HN_RENDER_SLOTS);
    unsigned delta=slot-base,index=delta/0x68;
    return delta%0x68||index>=hn_read8(m,HN_RENDER_INFO+0x10)?255:(uint8_t)index;
}
int32_t hn_render_class(HNMemory *m,GameAddr slot)
{
    static const GameAddr classes[]={0x03002C0C,0x03001F50,0x03002C04,0x03002C00};
    unsigned id=hn_render_find_slot(m,slot);
    for(unsigned i=0;i<4;i++)if(hn_read8(m,classes[i])==id)return (int32_t)i+42;
    return -1;
}
void hn_render_error(HNMemory *m,uint16_t code)
{
    unsigned i=hn_read32(m,0x03002C78);
    hn_write16(m,0x03002DE0+i*2,code);hn_write32(m,0x03002C78,(i+1)&7);
}
uint32_t hn_render_direction_raw(HNMemory *m,uint8_t from,uint8_t to,bool eight_directions)
{
    GameAddr a=hn_render_slot(m,from),b=hn_render_slot(m,to);
    int32_t dx=(int32_t)(hn_read32(m,b+0x18)-hn_read32(m,a+0x18))>>16;
    int32_t dz=(int32_t)(hn_read32(m,b+0x20)-hn_read32(m,a+0x20))>>16;
    uint16_t angle=hn_bios_arctan2(dx,dz);
    if(eight_directions){
        if((uint16_t)(angle-0x1000)<=0x1FFF)return 4;if((uint16_t)(angle-0x3000)<=0x1FFF)return 5;
        if((uint16_t)(angle-0x5000)<=0x1FFF)return 6;if((uint16_t)(angle-0x7000)<=0x1FFF)return 7;
        if((uint16_t)(angle+0x7000)<=0x1FFF)return 8;if((uint16_t)(angle+0x5000)<=0x1FFF)return 1;
        if((uint16_t)(angle+0x3000)<=0x1FFF)return 2;return 3;
    }
    if((uint16_t)(angle-0x2000)<=0x3FFF)return 2;if((uint16_t)(angle-0x6000)<=0x3FFF)return 3;
    if((uint16_t)(angle+0x6000)<=0x3FFF)return 1;return 4;
}
uint32_t hn_render_direction_between(HNMemory *m,uint8_t from,uint8_t to,bool eight_directions)
{
    if(from>=hn_read8(m,HN_RENDER_INFO+16)||to>=hn_read8(m,HN_RENDER_INFO+16)||!hn_read32(m,HN_RENDER_SLOTS))return 0;
    return hn_render_direction_raw(m,from,to,eight_directions);
}
void hn_render_set_animation(HNMemory *m,uint32_t index,GameAddr animation)
{
    hn_render_set_animation_pointer(m,hn_render_slot(m,index),animation);
}
void hn_render_set_animation_pointer(HNMemory *m,GameAddr slot,GameAddr animation)
{
    hn_write32(m,slot+4,animation);hn_write16(m,slot+0x4C,0);hn_write8(m,slot+0x63,0);
    for(unsigned i=0;i<4;i++){hn_write32(m,slot+8+i*4,0);hn_write16(m,slot+0x56+i*2,0);}
    unsigned flag=hn_read8(m,0x03002182),value=hn_read8(m,slot+0x61);
    if(flag&128){hn_write8(m,0x03002182,(uint8_t)(flag&127));value|=64;}
    else value&=~64u;
    hn_write8(m,slot+0x61,(uint8_t)value);
}
void hn_render_flags(HNMemory *m,uint32_t index,uint8_t mask,uint8_t value)
{
    GameAddr p=hn_render_slot(m,index)+0x61;
    hn_write8(m,p,(uint8_t)((hn_read8(m,p)&~mask)|value));
}
bool hn_render_number(HNMemory *m,uint8_t index,uint32_t value,uint32_t x_word,uint32_t y_word)
{
    value&=65535;uint16_t x=(uint16_t)x_word,y=(uint16_t)y_word;bool displayed=false;
    for(unsigned place=100;place;place/=10){
        unsigned digit=value/place;value%=place;
        if(digit||displayed||place==1){
            GameAddr animation=hn_read32(m,HN_NUMBER_ANIMATIONS+digit*4);
            if(!hn_render_configure(m,index,1,0x71,(int16_t)x,(int16_t)y,0,animation))return false;
            hn_render_flags(m,index,12,4);index++;displayed=true;
        }
        x=(uint16_t)(x+8);
    }
    return !m->fault;
}
bool hn_render_modes(HNMemory *m,uint8_t index,uint8_t mode,uint8_t value,uint8_t field65)
{
    if(index>=hn_read8(m,HN_RENDER_INFO+16)||!hn_read32(m,HN_RENDER_SLOTS))return false;
    return hn_render_modes_raw(m,index,mode,value,field65);
}
bool hn_render_modes_raw(HNMemory *m,uint8_t index,uint8_t mode,uint8_t value,uint8_t field65)
{
    uint8_t field62=(uint8_t)(((mode&3)<<6)|((value&3)<<4));GameAddr slot=hn_render_slot(m,index);
    if(mode<=1)field62|=field65&15;else hn_write8(m,slot+0x65,field65);
    hn_write8(m,slot+0x62,field62);return !m->fault;
}
bool hn_render_configure(HNMemory *m,uint8_t index,uint8_t field60,uint8_t field62,
                         int32_t x,int32_t y,int32_t z,GameAddr animation)
{
    if(index>=hn_read8(m,HN_RENDER_INFO+0x10)||!hn_read32(m,HN_RENDER_SLOTS))return false;
    return hn_render_configure_raw(m,index,field60,field62,x,y,z,animation);
}
bool hn_render_configure_raw(HNMemory *m,uint8_t index,uint8_t field60,uint8_t field62,
                             int32_t x,int32_t y,int32_t z,GameAddr animation)
{
    if(index==hn_read8(m,0x03001F50))hn_write8(m,0x03002170,0);
    hn_render_slot_reset(m,index);GameAddr slot=hn_render_slot(m,index);
    hn_write8(m,slot+0x60,field60);hn_write8(m,slot+0x62,field62);hn_render_flags(m,index,12,8);
    hn_write32(m,slot+0x18,((uint32_t)(uint16_t)x<<16)+0x8000);
    hn_write32(m,slot+0x1C,((uint32_t)(uint16_t)y<<16)+0x8000);
    hn_write32(m,slot+0x20,((uint32_t)(uint16_t)z<<16)+0x8000);
    hn_render_set_animation(m,index,animation);return !m->fault;
}
void hn_render_physics(HNMemory *m,GameAddr slot)
{
    for(unsigned i=0;i<3;i++){
        hn_write32(m,slot+0x18+i*4,hn_read32(m,slot+0x18+i*4)+hn_read32(m,slot+0x24+i*4));
        hn_write32(m,slot+0x24+i*4,hn_read32(m,slot+0x24+i*4)+hn_read32(m,slot+0x30+i*4));
    }
    for(unsigned i=0;i<2;i++){
        uint16_t value=(uint16_t)(hn_read16(m,slot+0x3C+i*2)+hn_read16(m,slot+0x44+i*2));
        if((int16_t)value>511)value=512;
        hn_write16(m,slot+0x3C+i*2,value);
    }
    hn_write16(m,slot+0x40,(uint16_t)(hn_read16(m,slot+0x40)+hn_read16(m,slot+0x48)));
}
uint8_t hn_render_find_affine(HNMemory *m)
{
    unsigned i=0;while(i<32&&hn_read8(m,HN_AFFINE_USAGE+i)!=255)i++;
    return (uint8_t)i;
}
void hn_render_screen_position(HNMemory *m,GameAddr slot)
{
    uint32_t camera_x=hn_read32(m,HN_RENDER_INFO),camera_y=hn_read32(m,HN_RENDER_INFO+4);
    unsigned x=(hn_read32(m,slot+0x18)+camera_x)>>16;
    unsigned y=(uint16_t)((hn_read32(m,slot+0x1C)>>16)+(hn_read32(m,slot+0x20)>>16)+(camera_y>>16));
    unsigned flags=hn_read8(m,slot+0x62);GameAddr reference=slot;
    if((flags&192)==128){
        reference=hn_render_slot(m,hn_read8(m,slot+0x65));
        x=(uint16_t)(x+(hn_read32(m,reference+0x18)>>16));
        y=(uint16_t)(y+(hn_read32(m,reference+0x1C)>>16)+(hn_read32(m,reference+0x20)>>16));
    }
    unsigned layers=hn_read8(m,reference+0x62)&15,ox=0,oy=0;
    for(unsigned i=0;i<4;i++)if(layers&(1u<<i)){
        GameAddr p=HN_LAYER_OFFSETS+i*44;
        ox=(hn_read32(m,p)+hn_read32(m,p+0x10))>>16;
        oy=(hn_read32(m,p+4)+hn_read32(m,p+0x14))>>16;break;
    }
    if(flags&16)x=(uint16_t)(x+ox);
    if(flags&32)y=(uint16_t)(y+oy);
    hn_write16(m,slot+0x4E,x<=511?(uint16_t)(x-(camera_x>>16)):hn_read16(m,HN_RENDER_INFO+8));
    hn_write16(m,slot+0x50,y<=255?(uint16_t)(y-(camera_y>>16)):hn_read16(m,HN_RENDER_INFO+10));
}
void hn_render_mark_affine(HNMemory *m)
{
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10);
    for(unsigned i=0;i<count;i++){
        unsigned matrix=hn_read8(m,hn_render_slot(m,i)+0x64);
        if(matrix!=255)hn_write8(m,HN_AFFINE_USAGE+matrix,1);
    }
}
bool hn_render_assign_affine(HNMemory *m)
{
    for(unsigned i=0;i<32;i++)hn_write8(m,HN_AFFINE_USAGE+i,255);
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10),next=0;
    for(unsigned i=0;i<count;i++){
        GameAddr slot=hn_render_slot(m,i);
        if(!hn_read8(m,slot+0x60)||hn_read8(m,slot+0x64)==255)continue;
        bool found=false;
        for(unsigned j=0;j<i;j++){
            GameAddr previous=hn_render_slot(m,j);
            if(hn_read8(m,previous+0x60)&&hn_read8(m,previous+0x64)!=255&&
               hn_read32(m,slot+0x3C)==hn_read32(m,previous+0x3C)&&
               hn_read16(m,slot+0x40)==hn_read16(m,previous+0x40)){
                hn_write8(m,slot+0x64,hn_read8(m,previous+0x64));found=true;break;
            }
        }
        if(!found){
            if(next>=32)return false;
            hn_write8(m,slot+0x64,(uint8_t)next);hn_write8(m,HN_AFFINE_USAGE+next++,1);
        }
    }
    return !m->fault;
}
bool hn_render_reorder(HNMemory *m,uint8_t moving,uint8_t anchor,uint8_t mode)
{
    if(mode>1)return false;
    GameAddr order=hn_read32(m,HN_RENDER_ORDER),priority=hn_read32(m,HN_RENDER_PRIORITY);
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10),remaining=count,entry=0,write=0;
    bool found=false;
    for(unsigned i=0;i<count;i++){
        unsigned id=hn_read8(m,order+i);hn_write8(m,priority+i*8+5,(uint8_t)id);
        if(id==anchor)found=true;
    }
    if(!found)return false;
    for(;;){
        unsigned id=hn_read8(m,priority+entry*8+5);
        if(id==anchor)break;
        if(id!=moving)hn_write8(m,order+write++,(uint8_t)id);
        entry++;remaining--;
    }
    if(mode){hn_write8(m,order+write++,hn_read8(m,priority+entry*8+5));entry++;}
    hn_write8(m,order+write++,moving);remaining--;
    while(remaining--){
        unsigned id=hn_read8(m,priority+entry++*8+5);
        if(id!=moving)hn_write8(m,order+write++,(uint8_t)id);
    }
    return !m->fault;
}
