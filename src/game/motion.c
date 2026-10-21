#include "game/motion.h"
#include "game/state.h"
#include "game/render.h"
#include "game/collision.h"
#include "game/input.h"
#include "gba/bios.h"

static int32_t magnitude(int32_t value){return value<0?(int32_t)(0u-(uint32_t)value):value;}
static GameAddr motion_slot(HNMemory *m,GameAddr selector)
{
    unsigned index=hn_read8(m,selector);
    return index<hn_read8(m,HN_RENDER_INFO+16)&&hn_read32(m,HN_RENDER_SLOTS)?hn_render_slot(m,index):0;
}
static bool transition_animation(HNMemory *m,unsigned index,GameAddr animation)
{
    if(index>=hn_read8(m,HN_RENDER_INFO+16))return false;
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,animation);return !m->fault;
}
bool hn_motion_transition(HNMemory *m,bool follower,bool keep_x)
{
    GameAddr leader=motion_slot(m,HN_RENDER_CLASS_LEADER);
    GameAddr slot=follower?motion_slot(m,HN_RENDER_CLASS_FOLLOWER):leader;if(!leader||!slot)return false;
    unsigned index=hn_read8(m,follower?HN_RENDER_CLASS_FOLLOWER:HN_RENDER_CLASS_LEADER);
    hn_write8(m,slot+0x60,hn_read8(m,follower?leader+0x60:HN_SCENE_CONTROLS+0x11));
    if(follower){
        hn_write32(m,slot+0x18,hn_read32(m,leader+0x18)+((uint32_t)hn_read16(m,0x0201DC86)<<16));
        hn_write32(m,slot+0x20,hn_read32(m,leader+0x20)+((uint32_t)hn_read16(m,0x0201DC88)<<16));
    }else{
        hn_write32(m,slot+0x18,((uint32_t)hn_read16(m,keep_x?0x0201DC60:0x0201DC84)<<16)|0x8000);
        hn_write32(m,slot+0x20,((uint32_t)hn_read16(m,keep_x?0x0201DC84:0x0201DC62)<<16)|0x8000);
    }
    if(hn_read8(m,HN_SCENE_CONTROLS+0x60)){
        unsigned variant=hn_input_query(m,2,0)>>1;
        uint32_t offset=(((uint32_t)hn_read8(m,slot+0x60)-1)<<1)+variant;
        unsigned family=hn_read8(m,0x03001F60)+(follower?1:0);
        GameAddr animation=hn_read32(m,hn_read32(m,HN_TRANSITION_ANIMATION_FAMILIES+family*4)+offset*4);
        if(!transition_animation(m,index,animation))return false;
        family=hn_read8(m,0x03001F90)+(follower?1:0);
        animation=hn_read32(m,hn_read32(m,HN_SECONDARY_ANIMATION_FAMILIES+family*4)+offset*4);
        unsigned extra=hn_read8(m,follower?0x03002270:0x03002274);
        if(extra){
            if(extra>=hn_read8(m,HN_RENDER_INFO+16))return false;
            if(hn_read32(m,hn_render_slot(m,extra)+4)!=animation&&!transition_animation(m,extra,animation))return false;
        }
        if(follower)hn_write16(m,0x0201DC96,hn_read16(m,0x0201DC94));
        else if(variant){GameAddr delay=(hn_read8(m,0x030021AB)&8)?0x0201DC96:0x0201DC94;hn_write16(m,delay,hn_read16(m,delay)>>1);}
    }
    return !m->fault;
}
bool hn_motion_interpolate(HNMemory *m,uint8_t selector)
{
    unsigned denominator=hn_read16(m,0x0201DC8E);
    if(denominator){
        uint32_t numerator=(uint32_t)(int32_t)(int16_t)hn_read16(m,0x0201DC92)<<16;
        int32_t factor=hn_runtime_divide((int32_t)numerator,(int16_t)denominator);
        GameAddr destination=selector?0x0201DC60:0x0201DC62;
        int32_t delta=(int32_t)hn_read16(m,destination)-(int32_t)hn_read16(m,0x0201DC8C);
        int32_t scaled=(int32_t)((uint32_t)delta*(uint32_t)factor)>>16;
        uint32_t angle=(uint32_t)scaled+hn_read16(m,0x0201DC90);
        angle=(angle&7)>3?(angle&0xFFF8)+8:angle&0xFFF8;
        hn_write16(m,destination,(uint16_t)angle);
    }
    return !m->fault;
}
void hn_motion_reset(HNMemory *m)
{
    hn_write16(m,0x03001FBC,0);hn_write8(m,0x030021E0,hn_read8(m,0x030021E0)&0xFB);
    const GameAddr bytes[]={0x03001FA8,0x03001F8C,0x03002C50,0x03002174,0x03000785};
    for(unsigned i=0;i<5;i++)hn_write8(m,bytes[i],0);
}
void hn_motion_control(HNMemory *m)
{
    unsigned keys=hn_input_query(m,0x30F,1),flags=hn_read8(m,0x030021E0);hn_write8(m,0x03000785,0);
    if(!(flags&8)&&(keys&1))hn_write8(m,0x03000785,8);
    else if(flags&2){if(keys&2){hn_write8(m,0x03000785,8);hn_write8(m,0x03002C50,1);}}
    else if(!(flags&1)){
        unsigned action=keys&8?3:(keys&4||keys&0x200)?2:keys&0x100?3:0;
        if(action){hn_write8(m,0x03000785,8);hn_write8(m,0x03002C50,(uint8_t)action);}
    }
}
bool hn_motion_leader(HNMemory *m,uint8_t *direction_control,uint8_t *variant,uint32_t *mode_out)
{
    GameAddr slot=motion_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return false;unsigned index=hn_read8(m,HN_RENDER_CLASS_LEADER);
    for(unsigned axis=0;axis<3;axis++)hn_write32(m,slot+0x24+axis*4,0);
    unsigned mode=1;hn_write8(m,HN_MOTION_MODE,1);
    if(!(hn_read8(m,0x030021E0)&2)&&hn_input_query(m,2,0)){
        *variant=1;*direction_control=0x80;mode=2;hn_write8(m,0x03002183,hn_read8(m,0x03002183)|1);hn_write8(m,HN_MOTION_MODE,2);
    }
    unsigned keys=hn_input_query(m,0xF0,0);
    if(!keys){hn_write32(m,0x03000778,0);if(!hn_motion_idle_render(m,false))return false;hn_write8(m,HN_MOTION_MODE,0);}
    else{
        hn_write32(m,0x03000778,8/mode);unsigned direction=keys&0x40?1:keys&0x80?2:keys&0x20?3:4;
        GameAddr pair=0x0858EC90+((mode-1)*8+(direction-1)*2)*4;
        hn_write32(m,slot+0x24,hn_read32(m,pair));hn_write32(m,slot+0x28,0);hn_write32(m,slot+0x2C,hn_read32(m,pair+4));
        unsigned animation_index=direction*2+*variant-2;GameAddr family=hn_read32(m,0x0858BF68+hn_read8(m,0x03001F60)*8);
        GameAddr animation=hn_read32(m,family+animation_index*4);*direction_control=(uint8_t)(*direction_control+direction);
        if(hn_read32(m,slot+4)!=animation){hn_render_set_animation(m,index,animation);hn_write8(m,slot+0x60,(uint8_t)direction);}
        family=hn_read32(m,0x0858BFB8+hn_read8(m,0x03001F90)*8);animation=hn_read32(m,family+animation_index*4);
        unsigned extra=hn_read8(m,0x03002274);
        if(extra){if(extra>=hn_read8(m,HN_RENDER_INFO+16))return false;if(hn_read32(m,hn_render_slot(m,extra)+4)!=animation)hn_render_set_animation(m,extra,animation);}
    }
    if(hn_read8(m,0x03000785)==8&&hn_read8(m,0x03002C50)!=1)for(unsigned axis=0;axis<3;axis++)hn_write32(m,slot+0x24+axis*4,0);
    if(!hn_collision_records(m,hn_read8(m,0x03000785),index,true)||!hn_collision_flags(m,hn_read8(m,0x03000784),index))return false;
    unsigned status=hn_read8(m,0x03001FA8);
    if(status&4){
        unsigned direction=hn_motion_direction(m,slot)&255;
        if(direction>=1&&direction<=4){
            unsigned axis=direction<=2?2:0;uint32_t coordinate=hn_read32(m,slot+0x18+axis*4),velocity=hn_read32(m,slot+0x24+axis*4);
            hn_write32(m,slot+0x24+axis*4,((coordinate+velocity+((direction==1||direction==3)?0x40000u:0))&0xFFF80000u)-coordinate);
        }
        if((status&13)==5){
            unsigned response=hn_read8(m,0x03001F8C);
            if(response>=1&&response<=4)hn_write32(m,slot+0x24+(response<=2?2:0)*4,(response==1||response==3)?0xFFFF0000u:0x10000u);
        }else if(hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,HN_MOTION_MODE,0);
    }
    *mode_out=mode;return !m->fault;
}
bool hn_motion_advance(HNMemory *m)
{
    unsigned flags=hn_read8(m,0x030021E0);
    if(flags&4){
        hn_write8(m,0x03001FA8,16);hn_write8(m,0x03001F8C,0);hn_write8(m,0x03002C50,0);hn_write8(m,0x03002174,0);
        hn_write16(m,0x03001FBC,(uint16_t)hn_read32(m,HN_CHOICE_PENDING));hn_write32(m,HN_CHOICE_PENDING,0);hn_write8(m,0x030021E0,(uint8_t)(flags&0xFB));return !m->fault;
    }
    const GameAddr bytes[]={0x03001FA8,0x03001F8C,0x03002C50,0x03002174,0x03000785};
    for(unsigned i=0;i<5;i++)hn_write8(m,bytes[i],0);
    hn_write8(m,0x03002183,hn_read8(m,0x03002183)&0xFE);hn_write8(m,HN_MOTION_MODE,0);hn_motion_control(m);
    uint8_t direction=0,variant=0;uint32_t mode;if(!hn_motion_leader(m,&direction,&variant,&mode))return false;
    unsigned index=hn_read8(m,HN_RENDER_CLASS_LEADER);if(!hn_collision_scripts(m,hn_read8(m,0x03000785),index)||!hn_collision_records(m,hn_read8(m,0x03000785),index,false))return false;
    GameAddr slot=motion_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return false;hn_write8(m,HN_MOTION_MODE,(uint8_t)hn_motion_classify(m,slot));
    for(unsigned axis=0;axis<=2;axis+=2){hn_write32(m,slot+0x18+axis*4,hn_read32(m,slot+0x18+axis*4)+hn_read32(m,slot+0x24+axis*4));hn_write32(m,slot+0x24+axis*4,0);}
    if(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)){
        if(!hn_motion_extend_route(m)||!hn_motion_follower(m))return false;slot=motion_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!slot)return false;
        for(unsigned axis=0;axis<=2;axis+=2){hn_write32(m,slot+0x18+axis*4,hn_read32(m,slot+0x18+axis*4)+hn_read32(m,slot+0x24+axis*4));hn_write32(m,slot+0x24+axis*4,0);}
    }
    GameAddr counter=HN_SCENE_CONTROLS+12;if(hn_input_query(m,0x3FF,0))hn_write8(m,counter,0);
    else if((int8_t)hn_read8(m,counter)>=0)hn_write8(m,counter,(uint8_t)(hn_read8(m,counter)+1));
    return !m->fault;
}
uint32_t hn_motion_direction(HNMemory *m,GameAddr slot)
{
    int32_t x=(int32_t)hn_read32(m,slot+0x24),z=(int32_t)hn_read32(m,slot+0x2C);
    int32_t ax=magnitude(x),az=magnitude(z);if(!ax&&!az)return 0;
    return ax>=az?(x<0?3u:4u):(z<0?1u:2u);
}
uint32_t hn_motion_classify(HNMemory *m,GameAddr slot)
{
    int32_t x=(int16_t)hn_read16(m,slot+0x26),z=(int16_t)hn_read16(m,slot+0x2E);
    if(!x&&!z)return 0;if(!z)return magnitude(x)>1?2u:1u;if(!x)return magnitude(z)>1?2u:1u;
    return (uint32_t)(x*x)+(uint32_t)(z*z)==0?2u:1u;
}
bool hn_motion_steps(HNMemory *m,uint16_t *x,uint16_t *z,uint32_t *mode)
{
    GameAddr slot=motion_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!slot)return false;
    int values[2]={(int16_t)hn_read16(m,slot+0x26),(int16_t)hn_read16(m,slot+0x2E)};
    uint16_t *outputs[2]={x,z};
    for(unsigned i=0;i<2;i++){
        int value=values[i];*outputs[i]=(uint16_t)(value<-2?-2:value<0?-1:value==0?0:value<=1?1:2);
    }
    *mode=hn_motion_classify(m,slot);return !m->fault;
}
bool hn_motion_extend_route(HNMemory *m)
{
    GameAddr slot=motion_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return false;
    unsigned end=hn_read8(m,HN_ROUTE_END);GameAddr point=HN_ROUTE_POINTS+end*8;
    int32_t x_fixed=(int32_t)hn_read32(m,slot+0x18),x=x_fixed>>16,z=(int16_t)hn_read16(m,slot+0x22);
    int32_t ox=(int16_t)hn_read16(m,point),oz=(int16_t)hn_read16(m,point+2);
    int32_t dx=(int16_t)hn_read16(m,point+4),dz=(int16_t)hn_read16(m,point+6);
    if(x==ox||z==oz){
        if(x==ox+dx&&z==oz+dz)return !m->fault;
        if(z==oz){hn_write16(m,point+4,(uint16_t)((int32_t)((uint32_t)x_fixed-((uint32_t)ox<<16))>>16));hn_write16(m,point+6,0);}
        else{hn_write16(m,point+4,0);hn_write16(m,point+6,(uint16_t)((int32_t)(hn_read32(m,slot+0x20)-((uint32_t)oz<<16))>>16));}
    }else{
        int16_t nx=(int16_t)(x-ox),nz=(int16_t)(z-oz);
        if((int32_t)nx*dz==(int32_t)nz*dx){hn_write16(m,point+4,(uint16_t)nx);hn_write16(m,point+6,(uint16_t)nz);}
        else{
            uint16_t ex=(uint16_t)(ox+dx),ez=(uint16_t)(oz+dz);end=(end+1)&31;hn_write8(m,HN_ROUTE_END,(uint8_t)end);
            point=HN_ROUTE_POINTS+end*8;hn_write16(m,point,ex);hn_write16(m,point+2,ez);
            hn_write16(m,point+4,(uint16_t)(x-(int16_t)ex));hn_write16(m,point+6,(uint16_t)(z-(int16_t)ez));
        }
    }
    return !m->fault;
}
static void follower_idle(HNMemory *m,GameAddr slot)
{
    unsigned direction=(uint8_t)((hn_read8(m,slot+0x60)&15)-1),primary=hn_read8(m,0x03001F60),secondary=hn_read8(m,0x03001F90);
    GameAddr family=hn_read32(m,0x0858BF68+((primary^1)*2+1)*4);
    hn_write32(m,HN_FOLLOWER_ANIMATION,hn_read32(m,family+direction*4));
    family=hn_read32(m,0x0858BFB8+(secondary*2+1)*4);
    hn_write32(m,HN_FOLLOWER_SECONDARY_ANIMATION,hn_read32(m,family+direction*8));
    hn_write8(m,HN_FOLLOWER_DIRECTION,0);hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x2C,0);
}
bool hn_motion_follower(HNMemory *m)
{
    GameAddr slot=motion_slot(m,HN_RENDER_CLASS_FOLLOWER),leader=motion_slot(m,HN_RENDER_CLASS_LEADER);
    if(!slot||!leader)return false;
    hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x2C,0);
    GameAddr point=HN_ROUTE_POINTS+hn_read8(m,HN_ROUTE_READ)*8;unsigned mode=hn_read8(m,HN_MOTION_MODE),direction=0;
    if(!mode||!hn_read32(m,point+4))follower_idle(m,slot);
    else{
        unsigned speed=mode==2?1:0,amount=speed+1;int32_t dx=(int16_t)hn_read16(m,point+4),dz=(int16_t)hn_read16(m,point+6);
        if(!dx){direction=dz<0?1:2;hn_write32(m,slot+0x2C,(dz<0?0u-amount:amount)<<16);}
        else if(!dz){direction=dx<0?3:4;hn_write32(m,slot+0x24,(dx<0?0u-amount:amount)<<16);}
        else{
            hn_write32(m,slot+0x24,(uint32_t)dx<<16);hn_write32(m,slot+0x2C,(uint32_t)dz<<16);
            uint16_t x,z;uint32_t classification;if(!hn_motion_steps(m,&x,&z,&classification))return false;
            hn_write32(m,slot+0x24,(uint32_t)x<<16);hn_write32(m,slot+0x2C,(uint32_t)z<<16);direction=hn_motion_direction(m,slot)&255;
        }
        unsigned index=direction*2+speed-2;
        GameAddr family=hn_read32(m,0x0858BF68+(hn_read8(m,0x03001F60)^1)*8);
        hn_write32(m,HN_FOLLOWER_ANIMATION,hn_read32(m,family+index*4));hn_write8(m,HN_FOLLOWER_DIRECTION,(uint8_t)direction);
        family=hn_read32(m,0x0858BFB8+(hn_read8(m,0x03001F90)*2+1)*4);
        hn_write32(m,HN_FOLLOWER_SECONDARY_ANIMATION,hn_read32(m,family+index*4));
    }
    int32_t dx=(int32_t)(hn_read32(m,slot+0x18)-hn_read32(m,leader+0x18)),dz=(int32_t)(hn_read32(m,slot+0x20)-hn_read32(m,leader+0x20));
    if((magnitude(dx)<=0xFFFF&&magnitude(dz)<=0x17FFFF)||(magnitude(dx)<=0x27FFFF&&magnitude(dz)<=0xFFFF)){
        follower_idle(m,slot);direction=0;if(!hn_motion_rebuild_route(m))return false;
    }
    int32_t component=(int16_t)hn_read16(m,point+4);
    uint32_t target=((uint32_t)((int16_t)hn_read16(m,point)+component)<<16)+(component>0?0xFFFF8000u:0x8000u);
    uint32_t x=hn_read32(m,slot+0x18),vx=hn_read32(m,slot+0x24);
    int32_t error_x=(int32_t)(target-(x+vx))>>16;
    component=(int16_t)hn_read16(m,point+6);
    target=((uint32_t)((int16_t)hn_read16(m,point+2)+component)<<16)+(component>0?0xFFFF8000u:0x8000u);
    uint32_t z=hn_read32(m,slot+0x20),vz=hn_read32(m,slot+0x2C);int32_t error_z=(int32_t)(target-(z+vz))>>16;
    if((!error_x&&!error_z)||(int32_t)((uint32_t)error_x*vx)<0||(int32_t)((uint32_t)error_z*vz)<0){
        unsigned current=hn_read8(m,HN_ROUTE_READ),end=hn_read8(m,HN_ROUTE_END);
        if(current!=end){
            current=(current+1)&31;hn_write8(m,HN_ROUTE_READ,(uint8_t)current);point=HN_ROUTE_POINTS+current*8;
            hn_write32(m,slot+0x24,((uint32_t)(int32_t)(int16_t)hn_read16(m,point)<<16)-hn_read32(m,slot+0x18));
            hn_write32(m,slot+0x2C,((uint32_t)(int32_t)(int16_t)hn_read16(m,point+2)<<16)-hn_read32(m,slot+0x20));
        }else{
            if(magnitude(error_x)<=1&&magnitude(error_z)<=1&&direction){
                hn_write32(m,slot+0x24,((uint32_t)((int16_t)hn_read16(m,point)+(int16_t)hn_read16(m,point+4))<<16)-x);
                hn_write32(m,slot+0x2C,((uint32_t)((int16_t)hn_read16(m,point+2)+(int16_t)hn_read16(m,point+6))<<16)-z);
            }
            if(!hn_motion_rebuild_route(m))return false;
        }
    }
    GameAddr primary=hn_read32(m,HN_FOLLOWER_ANIMATION);
    if(primary){
        bool reset_delay=true;
        if(hn_read32(m,slot+4)!=primary){
            unsigned moving=hn_read8(m,HN_FOLLOWER_DIRECTION);
            if(moving){hn_write8(m,slot+0x60,(uint8_t)moving);hn_render_set_animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),primary);}
            else{
                int32_t delay=(int32_t)hn_read32(m,HN_FOLLOWER_IDLE_DELAY);
                if(delay<=1){hn_write32(m,HN_FOLLOWER_IDLE_DELAY,(uint32_t)delay+1);reset_delay=false;}
                else hn_render_set_animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),primary);
            }
        }
        if(reset_delay)hn_write32(m,HN_FOLLOWER_IDLE_DELAY,0);
        unsigned extra=hn_read8(m,0x03002270);
        if(extra){
            if(extra>=hn_read8(m,HN_RENDER_INFO+16))return false;
            GameAddr secondary=hn_read32(m,HN_FOLLOWER_SECONDARY_ANIMATION);
            if(hn_read32(m,hn_render_slot(m,extra)+4)!=secondary)hn_render_set_animation(m,extra,secondary);
        }
    }
    return !m->fault;
}
bool hn_motion_update_following(HNMemory *m)
{
    if(!hn_read8(m,HN_RENDER_CLASS_FOLLOWER)||!hn_read8(m,0x03002170))return !m->fault;
    hn_write8(m,HN_MOTION_MODE,(hn_read8(m,0x03002183)&1)?2:1);if(!hn_motion_follower(m))return false;
    GameAddr slot=motion_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!slot)return false;
    hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+hn_read32(m,slot+0x24));
    hn_write32(m,slot+0x20,hn_read32(m,slot+0x20)+hn_read32(m,slot+0x2C));
    hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x2C,0);return !m->fault;
}

bool hn_motion_idle_render(HNMemory *m,bool follower)
{
    unsigned index=hn_read8(m,follower?HN_RENDER_CLASS_FOLLOWER:HN_RENDER_CLASS_LEADER);
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10);if(index>=count||!hn_read32(m,HN_RENDER_SLOTS))return false;
    GameAddr slot=hn_render_slot(m,index);unsigned direction=(uint8_t)((hn_read8(m,slot+0x60)&15)-1);
    unsigned primary=hn_read8(m,0x03001F60),secondary=hn_read8(m,0x03001F90);
    if(!follower)hn_write32(m,0x03000778,0);
    GameAddr family=hn_read32(m,0x0858BF68+((follower?(1^primary):primary)*2+1)*4);
    GameAddr animation=hn_read32(m,family+direction*4);
    if(hn_read32(m,slot+4)!=animation)hn_render_set_animation(m,index,animation);
    family=hn_read32(m,0x0858BFB8+(secondary*2+(follower?1:0))*4);
    animation=hn_read32(m,family+direction*8);index=hn_read8(m,follower?0x03002270:0x03002274);
    if(index){
        if(index>=count)return false;
        if(hn_read32(m,hn_render_slot(m,index)+4)!=animation)hn_render_set_animation(m,index,animation);
    }
    return !m->fault;
}

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
