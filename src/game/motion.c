#include "game/motion.h"
#include "game/state.h"
#include "game/render.h"
#include "gba/bios.h"

bool hn_motion_route_obstacle(HNMemory *m,uint32_t x1,uint32_t z1,uint32_t x2,uint32_t z2,uint32_t *obstacle)
{
    unsigned width=hn_read16(m,HN_COLLISION_DIMENSIONS),height=hn_read16(m,HN_COLLISION_DIMENSIONS+2);
    if((uint32_t)width*height>0x20000)return false;
    /* Coordinate inputs deliberately stay unsigned, including the expanded edges. */
    uint32_t left=(x2>x1?x1:x2)>>3,top=((z2>z1?z1:z2)+8u)>>3;
    uint32_t right=((x2<x1?x1:x2)+39u)>>3,bottom=((z2<z1?z1:z2)+31u)>>3;
    if(left>width)left=width;if(right>width)right=width;
    if(top>height)top=height;if(bottom>height)bottom=height;
    GameAddr map=hn_read32(m,HN_COLLISION_MAP);*obstacle=0;
    for(unsigned row=top;row<bottom;row++)for(unsigned column=left;column<right;column++){
        if(hn_read16(m,map+(row*width+column)*2)&0xF000){*obstacle=1;return !m->fault;}
        if(m->fault)return false;
    }
    return !m->fault;
}
static bool clear_path(HNMemory *m,uint32_t x1,uint32_t z1,uint32_t x2,uint32_t z2,
                       uint32_t x3,uint32_t z3,bool *clear)
{
    uint32_t obstacle;if(!hn_motion_route_obstacle(m,x1,z1,x2,z2,&obstacle))return false;
    if(obstacle){*clear=false;return true;}
    if(!hn_motion_route_obstacle(m,x2,z2,x3,z3,&obstacle))return false;
    *clear=!obstacle;return true;
}
bool hn_motion_rebuild_route(HNMemory *m)
{
    unsigned leader=hn_read8(m,HN_RENDER_CLASS_LEADER),follower=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10);
    if(leader>=count||follower>=count||!hn_read32(m,HN_RENDER_SLOTS))return false;
    GameAddr a=hn_render_slot(m,leader),b=hn_render_slot(m,follower),point=HN_ROUTE_POINTS;
    hn_write8(m,HN_ROUTE_READ,0);hn_write8(m,HN_ROUTE_END,0);
    hn_write16(m,point,hn_read16(m,b+0x1A));hn_write16(m,point+2,hn_read16(m,b+0x22));
    int32_t dx=(int32_t)(hn_read32(m,a+0x18)-hn_read32(m,b+0x18));
    int32_t dz=(int32_t)(hn_read32(m,a+0x20)-hn_read32(m,b+0x20));
    if(!dx||!dz){hn_write16(m,point+4,(uint16_t)(dx>>16));hn_write16(m,point+6,(uint16_t)(dz>>16));return !m->fault;}
    int32_t lx=(int32_t)hn_read32(m,a+0x18)>>16,lz=(int32_t)hn_read32(m,a+0x20)>>16;
    int32_t fx=(int32_t)hn_read32(m,b+0x18)>>16,fz=(int32_t)hn_read32(m,b+0x20)>>16;
    bool x_clear,z_clear;
    if(!clear_path(m,(uint32_t)fx,(uint32_t)fz,(uint32_t)lx,(uint32_t)fz,(uint32_t)lx,(uint32_t)lz,&x_clear)||
       !clear_path(m,(uint32_t)fx,(uint32_t)fz,(uint32_t)fx,(uint32_t)lz,(uint32_t)lx,(uint32_t)lz,&z_clear))return false;
    int32_t ax=dx<0?(int32_t)(0u-(uint32_t)dx):dx,az=dz<0?(int32_t)(0u-(uint32_t)dz):dz;
    bool z_first=x_clear==z_clear?ax<=az:!x_clear;
    hn_write8(m,HN_ROUTE_END,1);
    hn_write16(m,point+4,z_first?0:(uint16_t)(dx>>16));hn_write16(m,point+6,z_first?(uint16_t)(dz>>16):0);
    hn_write16(m,point+8,(uint16_t)(z_first?fx:lx));hn_write16(m,point+10,(uint16_t)(z_first?lz:fz));
    hn_write16(m,point+12,z_first?(uint16_t)(dx>>16):0);hn_write16(m,point+14,z_first?0:(uint16_t)(dz>>16));
    return !m->fault;
}
bool hn_motion_configure(HNMemory *m,uint32_t leader,uint32_t follower,uint32_t control,uint32_t secondary,
                         uint32_t flags,uint32_t width,uint32_t height,GameAddr resource,GameAddr parameters)
{
    hn_write8(m,0x03000784,3);hn_write32(m,HN_COLLISION_MAP,HN_COLLISION_DEFAULT);
    hn_write8(m,HN_RENDER_CLASS_LEADER,(uint8_t)leader);hn_write8(m,0x03002274,(uint8_t)control);
    hn_write8(m,HN_RENDER_CLASS_FOLLOWER,(uint8_t)follower);hn_write8(m,0x03002270,(uint8_t)secondary);
    hn_write8(m,0x03001F60,0);hn_write32(m,0x03000778,0);hn_write32(m,0x0300077C,0);
    hn_write8(m,0x03002170,0);hn_write8(m,HN_SCENE_CONTROLS+12,0);
    if(resource!=HN_COLLISION_DEFAULT){
        if((hn_read16(m,resource)&15)==1){if(!hn_bios_lz77(m,resource,HN_COLLISION_DEFAULT,0x1200))return false;}
        else if(!hn_bios_cpu_set(m,resource,HN_COLLISION_DEFAULT,0x04000480))return false;
    }
    if(parameters){if(!hn_memory_copy(m,0x030021F0,parameters,128))return false;}
    else for(unsigned i=0;i<32;i++)hn_write32(m,0x030021F0+i*4,0);
    for(unsigned layer=0;layer<4;layer++)if(!layer||(flags&(1u<<(layer-1)))){
        GameAddr bounds=HN_BACKGROUND_BOUNDS+layer*12;
        hn_write16(m,bounds,0);hn_write16(m,bounds+2,(uint16_t)width);
        hn_write16(m,bounds+4,0);hn_write16(m,bounds+6,(uint16_t)height);hn_write8(m,bounds+8,1);
    }
    return !m->fault;
}
