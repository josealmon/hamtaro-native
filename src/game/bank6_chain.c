#include "game/bank6.h"
#include "game/state.h"
#include "game/render.h"

enum { CLASSES=0x0201DC40,POSITION=0x03002BB0,FIRST=0x020018D8,SECOND=0x020018DD,THIRD=0x020018E2 };
typedef struct { uint32_t sine,cosine;int32_t angle; } ChainAngle;
static uint8_t leader(HNMemory *m){return hn_read8(m,HN_RENDER_CLASS_LEADER);}
static uint8_t follower(HNMemory *m){return hn_read8(m,HN_RENDER_CLASS_FOLLOWER);}
static bool flag(HNMemory *m,unsigned id){return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void animation(HNMemory *m,uint8_t index,GameAddr image)
{if(index==follower(m))hn_write8(m,0x03002170,0);hn_render_set_animation(m,index,image);}
static void allocate(HNMemory *m,GameAddr indices,unsigned count)
{
    for(unsigned i=0;i<count;i++){
        GameAddr slot=hn_render_slot(m,hn_read8(m,indices+i));
        /* These chains use zero, rather than 255, as their allocation sentinel. */
        if(!hn_read8(m,slot+0x64)){hn_write8(m,slot+0x64,hn_render_find_affine(m));hn_write8(m,HN_AFFINE_USAGE+hn_read8(m,slot+0x64),1);}
    }
}
static uint32_t sample(HNMemory *m,unsigned index)
{return (uint32_t)(int32_t)(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+index*2)<<8;}
static ChainAngle interpolate(HNMemory *m,uint16_t phase,bool reverse)
{
    uint32_t raw=hn_read16(m,HN_RENDER_SINE_TABLE+(phase&255)*2);
    int32_t fixed=(int32_t)((reverse?0u-raw:raw)<<16),angle=(int16_t)(fixed>>20);
    unsigned first=(uint32_t)angle&255,second=(uint32_t)(angle+1)&255,fraction=((uint32_t)fixed&0xF0000)>>16;
    uint32_t sine=sample(m,first),cosine=sample(m,first+64);
    sine+=(uint32_t)((int32_t)(sample(m,second)-sine)>>4)*fraction;
    cosine+=(uint32_t)((int32_t)(sample(m,second+64)-cosine)>>4)*fraction;
    ChainAngle result={sine,cosine,angle};return result;
}
static void initial_position(HNMemory *m)
{hn_write32(m,POSITION,((uint32_t)hn_read16(m,CLASSES+68)+128)<<16);hn_write32(m,POSITION+4,0xFF308000);hn_write32(m,POSITION+8,(uint32_t)hn_read16(m,CLASSES+70)<<16);}
static void linear_chain(HNMemory *m,GameAddr indices,ChainAngle a,uint32_t x,uint32_t y,uint32_t z)
{
    uint32_t next_x=x+a.sine*96,next_y=y+a.cosine*96;
    for(unsigned i=0;i<5;i++){
        GameAddr slot=hn_render_slot(m,hn_read8(m,indices+i));
        hn_write32(m,slot+0x18,next_x);hn_write32(m,slot+0x1C,next_y-(i<<21));hn_write32(m,slot+0x20,z);hn_write16(m,slot+0x40,(uint16_t)((uint32_t)a.angle<<8));
        next_x+=a.sine<<5;next_y+=a.cosine<<5;
    }
}
static void first_chain(HNMemory *m,GameAddr arguments)
{
    allocate(m,FIRST,5);if(!hn_read16(m,CLASSES+84))initial_position(m);
    unsigned phase=hn_read16(m,CLASSES+84);ChainAngle a=interpolate(m,(uint16_t)phase,true);
    uint32_t x=hn_read32(m,POSITION),y=hn_read32(m,POSITION+4),z=hn_read32(m,POSITION+8);
    linear_chain(m,FIRST,a,x,y,z);
    if(!arguments||phase>65)return;
    y-=0x100000;
    uint8_t index=leader(m);GameAddr base=hn_read32(m,HN_RENDER_SLOTS),slot=base+index*104;
    if(phase>54){
        hn_write16(m,slot+0x40,(uint16_t)((uint32_t)(a.angle-21)<<8));
        hn_write32(m,base+leader(m)*104+0x20,z);
        hn_write16(m,base+follower(m)*104+0x40,0xE400);hn_write32(m,base+follower(m)*104+0x20,z);
        GameAddr last=base+21*104;hn_write16(m,last+0x40,(uint16_t)((uint32_t)a.angle<<8));
        slot=base+leader(m)*104;
        if(phase==65){
            /* The terminal phase retains both actors' Y coordinates. */
            hn_write16(m,slot+0x40,0);hn_write32(m,slot+0x18,x-0x760000);
            slot=base+follower(m)*104;hn_write16(m,slot+0x40,0);hn_write32(m,slot+0x18,x-0x680000);
        }else{
            bool early=phase<=57;
            hn_write32(m,slot+0x18,x-(early?0x5E0000:0x5F0000));hn_write32(m,slot+0x1C,y+(early?0xBC0000:0xB80000));
            slot=base+follower(m)*104;hn_write32(m,slot+0x18,x-(early?0x500000:0x510000));hn_write32(m,slot+0x1C,y+0xA60000);
            hn_write32(m,last+0x18,x-(early?0x910000:0x920000));hn_write32(m,last+0x1C,y+0xAC0000);
        }
    }else{
        uint32_t target_z=z-(a.sine<<1),rotation=(uint32_t)a.angle<<8;
        hn_write32(m,slot+0x18,x+a.sine*208-(a.cosine<<1));hn_write32(m,slot+0x1C,y+a.cosine*208);hn_write32(m,slot+0x20,target_z);hn_write16(m,slot+0x40,(uint16_t)rotation);
        slot=base+follower(m)*104;
        hn_write32(m,slot+0x18,x+a.sine*180-(a.cosine<<1));hn_write32(m,slot+0x1C,y+a.cosine*180);hn_write32(m,slot+0x20,target_z);hn_write16(m,slot+0x40,(uint16_t)rotation);
    }
}
static void second_chain(HNMemory *m,GameAddr arguments)
{
    /* A null call discards both original BIOS byte loads; neither value is used. */
    uint8_t first=arguments?hn_read8(m,arguments):0,second=arguments?hn_read8(m,arguments+1):0;
    allocate(m,SECOND,5);unsigned phase=hn_read16(m,CLASSES+84);
    if(!phase){
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS),slot=base+24*104;
        hn_write32(m,POSITION,hn_read32(m,slot+0x18));hn_write32(m,POSITION+4,hn_read32(m,slot+0x1C)-0x600000);hn_write32(m,POSITION+8,hn_read32(m,slot+0x20));
        if(arguments){hn_write16(m,base+first*104+0x40,0);if(second)hn_write16(m,base+second*104+0x40,0);}
    }
    ChainAngle a=interpolate(m,hn_read8(m,CLASSES+84),false);
    uint32_t x=hn_read32(m,POSITION),y=hn_read32(m,POSITION+4),z=hn_read32(m,POSITION+8);
    linear_chain(m,SECOND,a,x,y,z);y-=0x100000;
    if(arguments){
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS),slot=base+first*104;uint16_t rotation=(uint16_t)((uint32_t)a.angle<<8);
        hn_write32(m,slot+0x18,x+a.sine*208);hn_write32(m,slot+0x1C,y+a.cosine*208);hn_write32(m,slot+0x20,z);hn_write16(m,slot+0x40,rotation);
        if(second){slot=base+second*104;hn_write32(m,slot+0x18,x+a.sine*180);hn_write32(m,slot+0x1C,y+a.cosine*180);hn_write32(m,slot+0x20,z);hn_write16(m,slot+0x40,rotation);}
    }
}
static void third_chain(HNMemory *m)
{
    allocate(m,THIRD,6);if(!hn_read16(m,CLASSES+84))initial_position(m);
    ChainAngle a=interpolate(m,hn_read16(m,CLASSES+84),false);
    uint32_t x=hn_read32(m,POSITION),y=hn_read32(m,POSITION+4),z=hn_read32(m,POSITION+8);
    for(unsigned i=0;i<6;i++){
        GameAddr slot=hn_render_slot(m,hn_read8(m,THIRD+i));
        hn_write32(m,slot+0x18,x);hn_write32(m,slot+0x1C,y);hn_write32(m,slot+0x20,z);
        hn_bank6_planar_transform(m,slot,a.sine,a.cosine,i<5?0u-8:0u-75,i<5?96+i*32:216);
        hn_write32(m,slot+0x1C,hn_read32(m,slot+0x1C)-(i<5?i<<21:0x280000));hn_write16(m,slot+0x40,(uint16_t)((0u-(uint32_t)a.angle)<<8));
    }
}
HNSceneResult hn_bank6_chain_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080A2DA8:first_chain(m,arguments);break;
    case 0x080A30E4:{GameAddr slot=hn_render_slot(m,hn_read8(m,arguments));hn_write32(m,slot+0x18,hn_read32(m,POSITION));hn_write32(m,slot+0x20,hn_read32(m,POSITION+8));*consumed=1;break;}
    case 0x080A3108:second_chain(m,arguments);*consumed=2;break;
    case 0x080A371C:third_chain(m);break;
    case 0x080A38D4:{
        uint8_t index=(uint8_t)(hn_read8(m,0x03002C00)-1);hn_write8(m,hn_render_slot(m,index)+0x60,1);animation(m,index,0x088F407D);
        hn_write8(m,0x03001FAC,(uint8_t)(hn_read8(m,0x03002C00)-1));hn_write32(m,0x03001F7C,0x088F407D);break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank6_chain_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080A371C:case 0x080A38D4:{uint32_t ignored;return hn_bank6_chain_callback(m,callback,0,&ignored);}
    default:return HN_SCENE_UNKNOWN;
    }
}
static uint32_t rectangles(HNMemory *m,bool overlap)
{
    GameAddr slot=hn_render_slot(m,leader(m));int x=(int16_t)hn_read16(m,slot+0x1A),z=(int16_t)hn_read16(m,slot+0x22);
    int min_z=(int16_t)hn_read16(m,CLASSES+76),max_z=(int16_t)hn_read16(m,CLASSES+78),min_x=(int16_t)hn_read16(m,CLASSES+80),max_x=(int16_t)hn_read16(m,CLASSES+82);
    bool inside=overlap?min_z<z+40&&z<max_z&&min_x<x+40&&x<max_x:min_z<z&&z+40<max_z&&min_x<x&&x+40<max_x;
    if(inside==overlap){
        uint8_t facing=hn_read8(m,slot+0x60);
        if(facing>=1&&facing<=4)animation(m,leader(m),facing==1?0x088AC0C6:facing==2?0x088AC08F:facing==3?0x088AC0FD:0x088AC134);
        hn_write8(m,0x030021E0,hn_read8(m,0x030021E0)|4);hn_write16(m,0x03002A80,hn_read8(m,HN_SCENE_CONTROLS+0xA3));return 3;
    }
    return 0;
}
HNSceneResult hn_bank6_chain_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield)
{
    (void)arguments;*consumed=0;
    switch(callback&~1u){
    case 0x080A3318:if(hn_render_find_slot(m,slot)==22)first_chain(m,0);else second_chain(m,0);break;
    case 0x080A333C:
        *yield=0;if((int32_t)(hn_read32(m,slot+0x1C)+hn_read32(m,slot+0x28))<=0x7FFF)break;
        {unsigned pair=slot==hn_render_slot(m,leader(m))?34:36;
        hn_write32(m,slot+0x18,((uint32_t)hn_read16(m,CLASSES+pair*2)<<16)+0x8000);hn_write32(m,slot+0x20,(uint32_t)hn_read16(m,CLASSES+pair*2+2)<<16);
        hn_write32(m,slot+0x1C,0x8000);for(unsigned i=0;i<3;i++)hn_write32(m,slot+0x24+i*4,0);*consumed=3;}break;
    case 0x080A33C0:case 0x080A34AC:*consumed=rectangles(m,(callback&~1u)==0x080A34AC);*yield=1;break;
    case 0x080A3594:{
        uint32_t index=hn_render_find_slot(m,slot);GameAddr counter=CLASSES+(index+205)*2;uint32_t old=hn_read16(m,counter);
        hn_write16(m,counter,(uint16_t)(old+1));uint32_t curve=(((index-25)<<8)+(uint32_t)(int32_t)(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+(old&255)*2))*(old<<6),step;
        if(flag(m,0x2AF)){step=old-64;old=step<<16;step=(step<<17)+curve*3;if((int32_t)step>0x2400000)hn_write16(m,counter,48);}
        else{step=384-old;old=step<<16;step=(step<<17)-curve*3;if((int32_t)step<=0x3FFFFF)hn_write16(m,counter,48);}
        *consumed=flag(m,0x2AE)?0:8;hn_write32(m,slot+0x18,old+0x8000);hn_write32(m,slot+0x1C,curve+0xFEE08000);hn_write32(m,slot+0x20,0x01208000);*yield=0;break;
    }
    case 0x080A3664:{
        uint32_t target=slot==hn_render_slot(m,leader(m))?0x908000:0xB88000;
        hn_write32(m,slot+0x24,(uint32_t)((int32_t)(target-hn_read32(m,slot+0x18))/32));hn_write32(m,slot+0x2C,(uint32_t)((int32_t)(0xB08000-hn_read32(m,slot+0x20))/32));*yield=0;break;
    }
    case 0x080A36C4:
        for(unsigned i=0;i<4;i++){GameAddr target=hn_render_slot(m,hn_read32(m,0x086BF9F4+i*4));uint32_t y=hn_read32(m,target+0x1C);if((int32_t)(y+hn_read32(m,target+0x20))>0x80000)hn_write32(m,target+0x1C,y-0x400000);}*yield=0;break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
