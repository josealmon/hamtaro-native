#include "game/particles.h"
#include "game/state.h"
#include "game/render.h"
static bool valid(HNMemory *m,unsigned index)
{return hn_read32(m,HN_RENDER_SLOTS)&&index<hn_read8(m,HN_RENDER_INFO+16);}
static GameAddr parameters(HNMemory *m,unsigned index)
{return hn_read32(m,HN_SCENE_SHARED_STATE+4)+index*28;}
static bool reset_position(HNMemory *m,uint8_t index,int32_t x,int32_t z)
{
    if(!valid(m,index))return false;GameAddr slot=hn_render_slot(m,index);
    hn_write32(m,slot+0x18,((uint32_t)(uint16_t)x<<16)+0x8000);
    hn_write32(m,slot+0x1C,0xFF008000);hn_write32(m,slot+0x20,((uint32_t)(uint16_t)z<<16)+0x8000);return !m->fault;
}
static void scales(HNMemory *m,GameAddr slot,unsigned index)
{
    hn_write16(m,slot+0x3C,256);hn_write16(m,slot+0x3E,256);
    hn_write16(m,slot+0x44,(uint16_t)hn_read32(m,parameters(m,index)+24));
    hn_write16(m,slot+0x46,(uint16_t)hn_read32(m,parameters(m,index)+24));
}
bool hn_particles_initialize(HNMemory *m)
{
    for(unsigned index=0;index<hn_read8(m,HN_SCENE_SHARED_STATE+1)&&!m->fault;index++){
        GameAddr p=parameters(m,index);uint32_t x=hn_read32(m,p)-16,z=hn_read32(m,p+4)+256;
        unsigned slot_index=hn_read32(m,HN_PARTICLE_INDICES+index*4);
        if(!hn_render_configure(m,hn_read8(m,HN_PARTICLE_INDICES+index*4),1,114,(int16_t)x,-256,(int16_t)z,0x088E6A70)||!valid(m,slot_index))return false;
        GameAddr slot=hn_render_slot(m,slot_index);p=parameters(m,index);
        hn_write32(m,slot+0x24,hn_read32(m,p+8));hn_write32(m,slot+0x2C,hn_read32(m,p+12));
        hn_write32(m,slot+0x30,hn_read32(m,p+16));hn_write32(m,slot+0x38,hn_read32(m,p+20));scales(m,slot,index);
    }
    return !m->fault;
}
HNSceneResult hn_particles_callback(HNMemory *m,GameAddr callback,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x0802EE5C:
        hn_write8(m,HN_SCENE_SHARED_STATE,1);hn_write8(m,HN_SCENE_SHARED_STATE+1,14);hn_write32(m,HN_SCENE_SHARED_STATE+4,HN_PARTICLE_PARAMETERS);
        if(!hn_particles_initialize(m))return HN_SCENE_RESOURCE;break;
    case 0x0802EF34:{
        GameAddr states=hn_read32(m,HN_RENDER_STATES);if(!states)return HN_SCENE_RESOURCE;
        if(!hn_read8(m,states+4*80+0x38))hn_write8(m,HN_SCENE_SHARED_STATE,0);
        for(unsigned index=0;index<hn_read8(m,HN_SCENE_SHARED_STATE+1)&&!m->fault;index++){
            unsigned slot_index=hn_read32(m,HN_PARTICLE_INDICES+index*4);if(!valid(m,slot_index))return HN_SCENE_RESOURCE;
            GameAddr slot=hn_render_slot(m,slot_index);
            if(hn_read32(m,slot+0x18)+0x200000u>0x1800000u||(int32_t)hn_read32(m,slot+0x20)>0x1F00000){
                GameAddr p=parameters(m,index);uint32_t x=hn_read32(m,p)-16,z=hn_read32(m,p+4)+256;
                if(!reset_position(m,(uint8_t)slot_index,(int16_t)x,(int16_t)z))return HN_SCENE_RESOURCE;
                slot=hn_render_slot(m,slot_index);p=parameters(m,index);
                hn_write32(m,slot+0x24,hn_read32(m,p+8));hn_write32(m,slot+0x2C,hn_read32(m,p+12));scales(m,slot,index);
            }
        }
        break;
    }
    case 0x0802F008:
        for(unsigned index=0;index<hn_read8(m,HN_SCENE_SHARED_STATE+1)&&!m->fault;index++){
            unsigned slot_index=(uint8_t)hn_read32(m,HN_PARTICLE_INDICES+index*4);if(!valid(m,slot_index))return HN_SCENE_RESOURCE;
            hn_write8(m,hn_render_slot(m,slot_index)+0x60,0);
        }
        break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
