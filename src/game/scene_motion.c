#include "game/scene_motion.h"
#include "game/state.h"
#include "game/render.h"
#include "game/random.h"
#include "gba/bios.h"

static bool valid_slot(HNMemory *m,unsigned index)
{return hn_read32(m,HN_RENDER_SLOTS)&&index<hn_read8(m,HN_RENDER_INFO+16);}
static int32_t high(HNMemory *m,GameAddr slot,unsigned field)
{return (int32_t)hn_read32(m,slot+field)>>16;}
static unsigned background(HNMemory *m,unsigned layer,unsigned axis)
{return hn_read16(m,HN_VIDEO_STATE+0x3A+layer*48+axis*2);}
static void anchor(HNMemory *m,uint16_t x,uint16_t z,uint8_t first,uint8_t second,uint8_t mode)
{
    hn_write16(m,0x0201DC84,x);hn_write16(m,0x0201DC86,z);
    hn_write8(m,HN_SCENE_CONTROLS+102,first);hn_write8(m,HN_SCENE_CONTROLS+103,second);hn_write8(m,HN_SCENE_CONTROLS+104,mode);
}
static bool rectangle(int32_t x,int32_t z,int32_t center_x,int32_t center_z)
{
    return x>(int16_t)(center_x-28)&&x<(int16_t)(center_x+16)&&
           z>(int16_t)(center_z-8)&&z<(int16_t)(center_z+8);
}
static int32_t middle(int32_t a,int32_t b)
{return a>b?b+(a-b)/2:a+(b-a)/2;}
static int32_t fixed8(int32_t numerator,int32_t denominator)
{return (int16_t)hn_runtime_divide((int32_t)((uint32_t)numerator<<16)>>8,(int16_t)denominator);}
static bool aim(HNMemory *m)
{
    if(!valid_slot(m,30))return false;
    GameAddr a=hn_render_slot(m,10),b=hn_render_slot(m,13),source=hn_render_slot(m,24),base=hn_render_slot(m,30),moving=hn_render_slot(m,26);
    int32_t mx=(int32_t)(hn_read32(m,a+0x18)-hn_read32(m,b+0x18))>>17;
    mx+=high(m,b,0x18);
    int32_t mz=(int32_t)(hn_read32(m,a+0x20)-hn_read32(m,b+0x20))>>17;
    mz+=high(m,b,0x20);
    int32_t target=(int16_t)(((int32_t)(hn_read32(m,source+0x18)+hn_read32(m,base+0x18))>>16)+background(m,0,0));
    uint16_t x=(uint16_t)(target-51-hn_read16(m,HN_SCENE_BACKGROUND_OFFSET));
    uint16_t z=(uint16_t)(((int32_t)(hn_read32(m,source+0x20)+hn_read32(m,base+0x20)+hn_read32(m,base+0x1C))>>16)+background(m,0,1)+40);
    int32_t difference=(int16_t)mx-(int16_t)x,direction=difference>0?1:-1;
    int32_t dx=(int16_t)(difference<0?-difference:difference),dz=(int16_t)((int16_t)mz-(int16_t)z);
    int32_t vx,vz;
    if(dx>dz){vx=(int16_t)((uint8_t)direction<<8);vz=fixed8((int16_t)((uint32_t)dz<<7),(int16_t)((uint32_t)dx<<7));}
    else{vx=(int16_t)(fixed8((int16_t)((uint32_t)dx<<7),(int16_t)((uint32_t)dz<<7))*direction);vz=256;}
    vx*=256;vz*=256;
    hn_write32(m,moving+0x24,(uint32_t)vx);hn_write32(m,moving+0x2C,(uint32_t)vz);
    hn_write32(m,moving+0x18,(uint32_t)x<<16);hn_write32(m,moving+0x20,(uint32_t)z<<16);
    hn_write32(m,moving+0x30,(uint32_t)(vx>>7));hn_write32(m,moving+0x38,(uint32_t)(vz>>7));return !m->fault;
}
static bool actor_rectangles(HNMemory *m)
{
    unsigned leader=hn_read8(m,HN_RENDER_CLASS_LEADER),follower=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
    if(!valid_slot(m,26)||!valid_slot(m,leader)||!valid_slot(m,follower))return false;
    GameAddr a=hn_render_slot(m,leader),b=hn_render_slot(m,follower),probe=hn_render_slot(m,26);
    hn_write8(m,HN_SCENE_SHARED_STATE,0);
    int32_t ax=high(m,a,0x18),az=high(m,a,0x20),bx=high(m,b,0x18),bz=high(m,b,0x20);
    int32_t x=high(m,probe,0x18),z=high(m,probe,0x20);
    if(rectangle(x,z,ax,az))hn_write8(m,HN_SCENE_SHARED_STATE,1);
    if(rectangle(x,z,bx,bz))hn_write8(m,HN_SCENE_SHARED_STATE,hn_read8(m,HN_SCENE_SHARED_STATE)|2);
    if(rectangle(x,z,middle(ax,bx),middle(az,bz)))hn_write8(m,HN_SCENE_SHARED_STATE,hn_read8(m,HN_SCENE_SHARED_STATE)|3);
    if(z>216||x<-48||x>320)hn_write8(m,HN_SCENE_SHARED_STATE,hn_read8(m,HN_SCENE_SHARED_STATE)|4);
    return !m->fault;
}
static bool slot_rectangle(HNMemory *m,GameAddr arguments)
{
    hn_write8(m,HN_SCENE_SHARED_STATE,0);
    unsigned index=hn_read8(m,arguments);if(!valid_slot(m,30)||!valid_slot(m,index))return false;
    int32_t left=(int8_t)hn_read8(m,arguments+1),top=(int8_t)hn_read8(m,arguments+2);
    int32_t right=(int8_t)hn_read8(m,arguments+3),bottom=(int8_t)hn_read8(m,arguments+4);
    GameAddr slot=hn_render_slot(m,index),base=hn_render_slot(m,30),probe=hn_render_slot(m,12);
    int32_t x=(int16_t)(high(m,slot,0x18)+background(m,0,0));x=(int16_t)(x+high(m,base,0x18));
    x=(int16_t)(x-hn_read16(m,HN_SCENE_BACKGROUND_OFFSET)-31);
    int32_t z=(int16_t)(high(m,slot,0x1C)+background(m,0,1));z=(int16_t)(z+high(m,base,0x1C));z=(int16_t)(z+high(m,base,0x20));
    int32_t px=(int16_t)(high(m,probe,0x18)+16),pz=(int16_t)(high(m,probe,0x20)+16);
    if(hn_read8(m,slot+0x60)&&px>x+left&&px<x+right&&pz>z+top&&pz<z+bottom)hn_write8(m,HN_SCENE_SHARED_STATE,1);
    return !m->fault;
}
static bool clone_sources(HNMemory *m)
{
    if(!valid_slot(m,30))return false;
    GameAddr source=hn_render_slot(m,24),base=hn_render_slot(m,30);
    int32_t x=(int16_t)(((int32_t)(hn_read32(m,source+0x18)+hn_read32(m,base+0x18))>>16)+background(m,0,0));
    x=(int16_t)(x-hn_read16(m,HN_SCENE_BACKGROUND_OFFSET)-31);x=(int16_t)(x-32);
    int32_t z=(int16_t)(((int32_t)(hn_read32(m,source+0x20)+hn_read32(m,base+0x20)+hn_read32(m,base+0x1C))>>16)+background(m,0,1)+42);
    if(!hn_render_configure(m,23,1,0x72,x,0,z,0x088F4BC9)||!hn_render_configure(m,26,1,0x72,x,0,(int16_t)(z+16),0x088F4BF0)||
       !hn_render_configure(m,27,1,0x72,(int16_t)(x+36),0,z,0x088F4BD6)||!hn_render_configure(m,28,1,0x72,(int16_t)(x+36),0,(int16_t)(z+16),0x088F4BF0))return false;
    hn_write8(m,HN_SCENE_SHARED_STATE+4,0);hn_write8(m,HN_SCENE_SHARED_STATE+5,1);return !m->fault;
}
static bool clone_pair(HNMemory *m)
{
    if(hn_read8(m,HN_SCENE_SHARED_STATE+4)>6)return !m->fault;
    uint8_t countdown=(uint8_t)(hn_read8(m,HN_SCENE_SHARED_STATE+5)-1);hn_write8(m,HN_SCENE_SHARED_STATE+5,countdown);
    if(countdown)return !m->fault;
    unsigned phase=hn_read8(m,HN_SCENE_SHARED_STATE+4),first=hn_read16(m,HN_SCENE_CLONE_PAIRS+phase*4),second=hn_read16(m,HN_SCENE_CLONE_PAIRS+phase*4+2);
    if(!valid_slot(m,28)||!valid_slot(m,first)||!valid_slot(m,second))return false;
    GameAddr a=hn_render_slot(m,first),b=hn_render_slot(m,second),source=hn_render_slot(m,26),source2=hn_render_slot(m,28);
    hn_write8(m,HN_SCENE_SHARED_STATE+5,1);
    for(unsigned i=0;i<104;i+=4)hn_write32(m,a+i,hn_read32(m,source+i));
    for(unsigned i=0;i<104;i+=4)hn_write32(m,b+i,hn_read32(m,source2+i));
    hn_write32(m,source+0x1C,hn_read32(m,source+0x1C)+0x100000);hn_write32(m,source2+0x1C,hn_read32(m,source2+0x1C)+0x100000);
    hn_write32(m,a+4,0x088F4BE3);hn_write32(m,b+4,0x088F4BE3);
    GameAddr frame=hn_read16(m,a+0x4C)==5?0x088976AC:0x088976B4;hn_write32(m,a,frame);hn_write32(m,b,frame);
    hn_write8(m,HN_SCENE_SHARED_STATE+4,(uint8_t)(hn_read8(m,HN_SCENE_SHARED_STATE+4)+1));return !m->fault;
}
HNSceneResult hn_scene_motion_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x0801ED1C:anchor(m,112,152,16,19,0);break;
    case 0x0801ED50:anchor(m,176,200,16,19,1);break;
    case 0x0801ED84:anchor(m,256,144,16,19,0);break;
    case 0x0801EDBC:anchor(m,56,192,31,32,0);break;
    case 0x0801EDF0:anchor(m,96,160,19,16,0);break;
    case 0x0801EE24:anchor(m,120,128,19,16,0);break;
    case 0x0801EE58:anchor(m,80,152,16,19,0);break;
    case 0x0801EE8C:anchor(m,88,96,22,19,0);break;
    case 0x0801EEC0:anchor(m,120,64,22,23,0);break;
    case 0x0801EEF4:anchor(m,160,80,16,19,0);break;
    case 0x0801EF28:anchor(m,136,104,23,24,0);break;
    case 0x0801EF5C:anchor(m,112,104,16,19,0);break;
    case 0x0801EF90:anchor(m,168,176,16,20,0);break;
    case 0x0801EFC4:anchor(m,144,112,16,19,0);break;
    case 0x0801EFF8:anchor(m,216,104,19,16,0);break;
    case 0x0801F02C:anchor(m,160,32,16,19,1);break;
    case 0x0801F060:anchor(m,136,136,16,19,0);break;
    case 0x0801F094:anchor(m,152,168,16,19,0);break;
    case 0x0801F0C8:anchor(m,184,88,10,13,1);break;
    case 0x0801F0FC:if(!hn_render_modes(m,9,2,3,hn_read8(m,HN_SCENE_CONTROLS+102)))return HN_SCENE_RESOURCE;break;
    case 0x080B9B68:hn_write16(m,HN_SCENE_BACKGROUND_OFFSET,0xFFE1);break;
    case 0x080B9B7C:case 0x080B9BCC:{
        unsigned index=hn_read8(m,arguments),axis=hn_read8(m,arguments+1);uint32_t value=hn_read32(m,arguments+2);
        if(axis<3){if(!valid_slot(m,index))return HN_SCENE_RESOURCE;hn_write32(m,hn_render_slot(m,index)+((callback&~1u)==0x080B9B7C?0x24:0x18)+axis*4,value);}
        *consumed=6;break;
    }
    case 0x080B9CD8:if(!slot_rectangle(m,arguments))return HN_SCENE_RESOURCE;*consumed=5;break;
    case 0x080B9DF8:if(!aim(m))return HN_SCENE_RESOURCE;break;
    case 0x080B9F14:if(!actor_rectangles(m))return HN_SCENE_RESOURCE;break;
    case 0x080BA0F8:if(!clone_sources(m))return HN_SCENE_RESOURCE;break;
    case 0x080BA1FC:if(!clone_pair(m))return HN_SCENE_RESOURCE;break;
    case 0x080BA2B4:{
        unsigned leader=hn_read8(m,HN_RENDER_CLASS_LEADER),follower=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
        if(!valid_slot(m,28)||!valid_slot(m,leader)||!valid_slot(m,follower))return HN_SCENE_RESOURCE;
        GameAddr a=hn_render_slot(m,23),b=hn_render_slot(m,28);hn_write8(m,HN_SCENE_SHARED_STATE,0);
        int32_t left=(int16_t)(high(m,a,0x18)-32),top=(int16_t)(high(m,a,0x20)+high(m,a,0x1C));
        int32_t right=(int16_t)(high(m,b,0x18)+24),bottom=(int16_t)(high(m,b,0x20)+high(m,b,0x1C));
        for(unsigned i=0;i<2;i++){GameAddr slot=hn_render_slot(m,i?follower:leader);int32_t x=high(m,slot,0x18),z=high(m,slot,0x20);
            if(x>left&&x<right&&z>top&&z<bottom)hn_write8(m,HN_SCENE_SHARED_STATE,hn_read8(m,HN_SCENE_SHARED_STATE)|(1u<<i));}
        break;
    }
    case 0x080BA398:hn_write16(m,0x0201DC84,(uint16_t)(hn_random_range(m,1,4)*60+60));break;
    case 0x080BA3C0:{
        uint16_t value=(uint16_t)(int8_t)hn_read8(m,arguments);
        hn_write8(m,0x030021E0,hn_read8(m,0x030021E0)|4);hn_write32(m,HN_CHOICE_PENDING,value);*consumed=1;break;
    }
    case 0x080BA3D8:{
        if(!valid_slot(m,30))return HN_SCENE_RESOURCE;
        GameAddr source=hn_render_slot(m,24),base=hn_render_slot(m,30);
        int32_t x=(int16_t)(((int32_t)(hn_read32(m,source+0x18)+hn_read32(m,base+0x18))>>16)+background(m,0,0));
        hn_write8(m,HN_SCENE_SHARED_STATE+2,(uint8_t)(x<high(m,hn_render_slot(m,10),0x18)));break;
    }
    case 0x080BA428:{
        uint16_t value=(uint16_t)(hn_read16(m,HN_SCENE_BACKGROUND_OFFSET)+(hn_read8(m,HN_SCENE_SHARED_STATE+2)?-2:2));
        hn_write16(m,HN_SCENE_BACKGROUND_OFFSET,value);
        if((uint16_t)(value-65)<=0x7FBE)hn_write8(m,HN_SCENE_SHARED_STATE+2,1);
        else if((uint16_t)(value-0x8000)<=0x7F89)hn_write8(m,HN_SCENE_SHARED_STATE+2,0);break;
    }
    case 0x080BA488:{
        hn_write8(m,HN_SCENE_SHARED_STATE,0);unsigned value=hn_read16(m,HN_SCENE_BACKGROUND_OFFSET);
        if((uint16_t)(value+0x7FFF)<=0x7FDF)hn_write16(m,HN_SCENE_BACKGROUND_OFFSET,(uint16_t)(value+1));
        else if(value!=0xFFE1)hn_write16(m,HN_SCENE_BACKGROUND_OFFSET,(uint16_t)(value-1));else hn_write8(m,HN_SCENE_SHARED_STATE,1);break;
    }
    case 0x080BA4D8:hn_write16(m,HN_SCENE_BACKGROUND_OFFSET,(uint16_t)((int32_t)(int8_t)hn_read8(m,arguments)|((uint32_t)(int32_t)(int8_t)hn_read8(m,arguments+1)<<8)));*consumed=2;break;
    case 0x080BA4F4:hn_write16(m,0x0201DC84,(uint16_t)(104-background(m,1,0)));break;
    case 0x080BA514:{
        if(!valid_slot(m,37))return HN_SCENE_RESOURCE;GameAddr slot=hn_render_slot(m,37);
        if((!hn_read8(m,slot+0x60)||hn_read32(m,slot+4)!=0x088F4D77)&&!hn_render_configure(m,37,1,0,0,0,0,0x088F4D77))return HN_SCENE_RESOURCE;break;
    }
    case 0x080BA55C:hn_write8(m,HN_SCENE_CONTROLS+0x1AF,hn_read8(m,HN_CHOICE_STATE));break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_scene_motion_animation(HNMemory *m,GameAddr callback,uint32_t *consumed,uint32_t *yield)
{
    if((callback&~1u)!=0x080B9C1C&&(callback&~1u)!=0x080B9C64)return HN_SCENE_UNKNOWN;
    if(!valid_slot(m,30))return HN_SCENE_RESOURCE;GameAddr slot=hn_render_slot(m,30);
    int32_t first=(int16_t)(background(m,1,0)-hn_read16(m,HN_SCENE_BACKGROUND_OFFSET)),second=(int16_t)(background(m,0,0)-73);
    hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)-((uint32_t)(first-second)<<16));
    if((callback&~1u)==0x080B9C64){
        first=(int16_t)(background(m,1,1)+56);second=(int16_t)(background(m,0,1)+3);
        hn_write32(m,slot+0x20,hn_read32(m,slot+0x20)-((uint32_t)(first-second)<<16)+0xFFCF8000u+hn_read32(m,slot+0x1C));
    }
    *consumed=0;*yield=1;return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
