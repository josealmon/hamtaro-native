#include "game/collision.h"
#include "game/state.h"
#include "game/render.h"
#include "gba/bios.h"

enum { STATUS=0x03001FA8,RESPONSE=0x03001F8C,CONTACT=0x03002174,RECORDS=0x030021F0 };
static int32_t absolute(int32_t value){return value<0?(int32_t)(0u-(uint32_t)value):value;}
static GameAddr render_slot(HNMemory *m,unsigned index)
{return index<=hn_read8(m,HN_RENDER_INFO+16)&&hn_read32(m,HN_RENDER_SLOTS)?hn_render_slot(m,index):0;}
uint32_t hn_collision_facing(HNMemory *m,GameAddr slot){return (hn_read8(m,slot+0x60)-1u)&3;}
static GameAddr cell(HNMemory *m,GameAddr map,int32_t x,int32_t z)
{
    int32_t tx=(int32_t)((uint32_t)x<<13)>>16,tz=(int32_t)((uint32_t)z<<13)>>16;
    unsigned width=hn_read16(m,HN_COLLISION_DIMENSIONS);
    if(tx<0)tx+=(int32_t)width;if(tz<0)tz+=hn_read16(m,HN_COLLISION_DIMENSIONS+2);
    return map+((uint32_t)tz*width+(uint32_t)tx)*2;
}
GameAddr hn_collision_sample(HNMemory *m,GameAddr map,int32_t x,int32_t z,uint32_t direction,uint32_t sample)
{
    direction&=255;sample&=255;if(direction>=4||(sample>=6&&sample!=255))return 0;
    GameAddr offsets=sample==255?0x0858ED50+direction*4:0x0858ECD0+(direction*6+sample)*2;
    int32_t dx=sample==255?(int16_t)hn_read16(m,offsets):(int8_t)hn_read8(m,offsets);
    int32_t dz=sample==255?(int16_t)hn_read16(m,offsets+2):(int8_t)hn_read8(m,offsets+1);
    return cell(m,map,(int32_t)((uint32_t)x+(uint32_t)dx),(int32_t)((uint32_t)z+(uint32_t)dz));
}
GameAddr hn_collision_ahead(HNMemory *m,GameAddr map,int32_t x,int32_t z,uint32_t direction)
{
    direction&=255;if(direction>=4)return 0;
    x=(int32_t)((uint32_t)x+(uint32_t)(int32_t)(int16_t)hn_read16(m,0x0858ED50+direction*4));
    z=(int32_t)((uint32_t)z+(uint32_t)(int32_t)(int16_t)hn_read16(m,0x0858ED52+direction*4));
    int32_t tx=(int32_t)((uint32_t)x<<13)>>16,tz=(int32_t)((uint32_t)z<<13)>>16;
    int32_t distance=(int16_t)hn_read16(m,0x0858ED48+direction*2);
    tx+=(int16_t)hn_read16(m,0x0858ED38+direction*4)*distance;tz+=(int16_t)hn_read16(m,0x0858ED3A+direction*4)*distance;
    unsigned width=hn_read16(m,HN_COLLISION_DIMENSIONS);if(tx<0)tx+=(int32_t)width;if(tz<0)tz+=hn_read16(m,HN_COLLISION_DIMENSIONS+2);
    return map+((uint32_t)tz*width+(uint32_t)tx)*2;
}
uint32_t hn_collision_response_direction(uint32_t mask,uint32_t direction)
{
    mask&=255;direction&=255;
    if(mask==1||mask==32||mask==3||mask==48||(direction>1&&(mask==56||mask==60)))
        return mask==1||mask==3?(direction<=1?4u:2u):(direction<=1?3u:1u);
    return 0;
}
uint32_t hn_collision_mask(HNMemory *m,GameAddr map,int32_t x,int32_t z)
{
    uint32_t mask=0;
    for(unsigned direction=0;direction<4;direction++)for(unsigned sample=0;sample<6;sample++){
        GameAddr offset=0x0858ED00+(direction*6+sample)*2;
        int32_t sx=(int32_t)((uint32_t)x+(uint32_t)(int32_t)(int8_t)hn_read8(m,offset));
        int32_t sz=(int32_t)((uint32_t)z+(uint32_t)(int32_t)(int8_t)hn_read8(m,offset+1));
        unsigned edge=hn_read16(m,0x0858ED30+direction*2);if(hn_read16(m,cell(m,map,sx,sz))&edge)mask|=edge;
    }
    return mask;
}
void hn_collision_stage(HNMemory *m,uint32_t script)
{hn_write16(m,0x03001FBC,(uint16_t)script);hn_write8(m,STATUS,hn_read8(m,STATUS)|2);hn_write8(m,0x03000794,0);}
static int32_t projected(HNMemory *m,GameAddr slot,unsigned axis,bool truncate,uint32_t bias)
{
    int32_t value=(int32_t)(hn_read32(m,slot+0x18+axis*4)+hn_read32(m,slot+0x24+axis*4)+bias);
    return truncate?value/65536:value>>16;
}
static bool circle_hit(HNMemory *m,GameAddr record,int32_t x,int32_t z,bool strict,uint32_t *hit)
{
    GameAddr slot=render_slot(m,hn_read16(m,record));if(!slot)return false;
    int16_t cx=(int16_t)(projected(m,slot,0,true,0)+hn_read16(m,record+8));
    int16_t cz=(int16_t)(projected(m,slot,2,true,0)+hn_read16(m,record+10));
    int16_t dx=(int16_t)((int16_t)x-cx),dz=(int16_t)((int16_t)z-cz);
    uint32_t distance=(uint32_t)(dx*dx)+(uint32_t)(dz*dz),radius=hn_read16(m,record+12);radius*=radius;
    *hit=strict?radius>distance:radius>=distance;return !m->fault;
}
static bool rectangle_hit(HNMemory *m,GameAddr record,int32_t x,int32_t z,bool strict,uint32_t *hit)
{
    GameAddr slot=render_slot(m,hn_read16(m,record));if(!slot)return false;
    int32_t cx=projected(m,slot,0,false,0),cz=projected(m,slot,2,false,0);
    int16_t left=(int16_t)(hn_read16(m,record+8)+cx-40),top=(int16_t)(hn_read16(m,record+10)+cz-32);
    int16_t right=(int16_t)(cx+hn_read16(m,record+12)),bottom=(int16_t)(cz+hn_read16(m,record+14)+8);
    x=(int16_t)x;z=(int16_t)z;
    *hit=x>=left&&z>=top&&(strict?x<right&&z<bottom:x<=right&&z<=bottom);return !m->fault;
}
bool hn_collision_contains(HNMemory *m,GameAddr record,int32_t x,int32_t z,uint32_t *hit)
{return hn_read16(m,record+4)?rectangle_hit(m,record,x,z,false,hit):circle_hit(m,record,x,z,false,hit);}
bool hn_collision_query(HNMemory *m,uint32_t render,GameAddr record,uint32_t *script)
{
    GameAddr slot=render_slot(m,render&255);if(!slot)return false;uint32_t hit;
    bool rectangle=hn_read16(m,record+4)!=0;
    int32_t x=projected(m,slot,0,!rectangle,rectangle?0:0x100000),z=projected(m,slot,2,!rectangle,rectangle?0:0x180000);
    bool ok=rectangle?rectangle_hit(m,record,x,z,true,&hit):circle_hit(m,record,x,z,true,&hit);
    if(ok)*script=hit?hn_read16(m,record+2):0;return ok&&!m->fault;
}
bool hn_collision_response(HNMemory *m,uint32_t render,GameAddr record)
{
    GameAddr obstacle=render_slot(m,hn_read16(m,record)),slot=render_slot(m,render&255);if(!obstacle||!slot)return false;
    if(!hn_read16(m,record+4)){
        int16_t cx=(int16_t)(projected(m,obstacle,0,true,0)+hn_read16(m,record+8));
        int16_t cz=(int16_t)(projected(m,obstacle,2,true,0)+hn_read16(m,record+10));
        int16_t dx=(int16_t)((int16_t)projected(m,slot,0,true,0x100000)-cx);
        int16_t dz=(int16_t)(projected(m,slot,2,true,0x180000)-cz);
        uint32_t radius=hn_read16(m,record+12),distance=(uint32_t)(dx*dx)+(uint32_t)(dz*dz);if(radius*radius<=distance)return !m->fault;
        uint16_t angle=hn_bios_arctan2(dx,dz);unsigned index=angle>>8;
        int16_t ox=(int16_t)(((int32_t)(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+(index+64)*2)*(int32_t)radius)/256);
        int16_t oz=(int16_t)(((int32_t)(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+index*2)*(int32_t)radius)/256);
        uint32_t vx=(((uint32_t)((int32_t)ox+cx-16)<<16)+0x8000)-hn_read32(m,slot+0x18);
        hn_write32(m,slot+0x24,vx);
        uint32_t vz=(((uint32_t)((int32_t)oz+cz-24)<<16)+0x8000)-hn_read32(m,slot+0x20);hn_write32(m,slot+0x2C,vz);
        if(vx||vz){
            unsigned direction=absolute((int32_t)vx)>=absolute((int32_t)vz)?((int32_t)vx<0?4:3):((int32_t)vz<0?1:2);
            hn_write8(m,RESPONSE,(uint8_t)direction);
        }
    }else{
        int32_t cx=projected(m,obstacle,0,false,0),cz=projected(m,obstacle,2,false,0);
        int32_t left=(int16_t)(hn_read16(m,record+8)+cx-40),top=(int16_t)(hn_read16(m,record+10)+cz-32);
        int32_t right=(int16_t)(cx+hn_read16(m,record+12)),bottom=(int16_t)(cz+hn_read16(m,record+14)+8);
        int32_t x=projected(m,slot,0,false,0),z=(int16_t)projected(m,slot,2,false,0);
        if(x<left||x>=right||z<top||z>=bottom)return !m->fault;
        int32_t width=right-left,height=bottom-top;
        uint32_t product=(uint32_t)width*(uint32_t)z;
        int32_t first=(int32_t)(product-((uint32_t)(x-left)*(uint32_t)height+(uint32_t)top*(uint32_t)width));
        int32_t second=(int32_t)(product-((uint32_t)(x-left)*(uint32_t)(top-bottom)+(uint32_t)width*(uint32_t)bottom));
        unsigned axis=first>=0?(second>=0?2:0):(second>=0?0:2);
        int32_t boundary=first>=0?(second>=0?bottom:left):(second>=0?right:top);
        uint32_t velocity=(((uint32_t)boundary<<16)+0x8000)-hn_read32(m,slot+0x18+axis*4);
        hn_write32(m,slot+0x24+axis*4,velocity);
        if(velocity)hn_write8(m,RESPONSE,(uint8_t)(first>=0?(second>=0?2:3):(second>=0?4:1)));
    }
    hn_write8(m,STATUS,hn_read8(m,STATUS)|8);hn_write8(m,CONTACT,(uint8_t)hn_read16(m,record));return !m->fault;
}
bool hn_collision_has_record(HNMemory *m,int32_t x,int32_t z,uint32_t *hit)
{
    *hit=0;
    for(unsigned i=0;i<32;i++){
        GameAddr record=hn_read32(m,RECORDS+i*4);if(!record)return !m->fault;
        unsigned index=hn_read16(m,record);if(index>hn_read8(m,HN_RENDER_INFO+16))return !m->fault;
        GameAddr slot=render_slot(m,index);if(!slot)return false;
        if((hn_read16(m,record+6)&2)&&hn_read8(m,slot+0x60)){
            if(!hn_collision_contains(m,record,x,z,hit))return false;if(*hit)return true;
        }
    }
    return !m->fault;
}
bool hn_collision_records(HNMemory *m,uint32_t control,uint32_t render,bool resolve)
{
    for(unsigned i=0;i<32;i++){
        GameAddr record=hn_read32(m,RECORDS+i*4);if(!record)return !m->fault;
        unsigned index=hn_read16(m,record);if(index>hn_read8(m,HN_RENDER_INFO+16))return !m->fault;
        GameAddr slot=render_slot(m,index);if(!slot)return false;unsigned flags=hn_read16(m,record+6);
        if(!hn_read8(m,slot+0x60)||(bool)(flags&2)!=resolve)continue;
        if(resolve){if(!hn_collision_response(m,render,record))return false;}
        else{uint32_t script;if(!hn_collision_query(m,render,record,&script))return false;
            if(script&&((control&8)||(flags&1))){hn_collision_stage(m,script);hn_write8(m,CONTACT,(uint8_t)index);return !m->fault;}}
    }
    return !m->fault;
}
bool hn_collision_flags(HNMemory *m,uint32_t control,uint32_t render)
{
    GameAddr slot=render_slot(m,render&255);if(!slot)return false;unsigned direction;
    if(hn_read8(m,STATUS)&8){
        int32_t vx=(int32_t)hn_read32(m,slot+0x24),vz=(int32_t)hn_read32(m,slot+0x2C);
        direction=absolute(vx)>=absolute(vz)?(vx<0?2:3):(unsigned)((uint32_t)~vz>>31);
    }else direction=hn_collision_facing(m,slot);
    int32_t x=projected(m,slot,0,true,0),z=projected(m,slot,2,true,0);GameAddr map=hn_read32(m,HN_COLLISION_MAP);
    if(control&1){
        unsigned sampled=0,edge=hn_read16(m,0x0858ED30+direction*2);GameAddr samples[6];
        for(unsigned sample=0;sample<6;sample++)samples[sample]=hn_collision_sample(m,map,x,z,direction,sample);
        for(unsigned sample=0;sample<6;sample++)if(hn_read16(m,samples[sample])&edge)sampled|=1u<<sample;
        if(sampled){
            unsigned flags=hn_read8(m,STATUS);if((flags&7)!=5){hn_write8(m,STATUS,(uint8_t)(flags|4));hn_write8(m,RESPONSE,0);}
            if(control&2){unsigned response=hn_collision_response_direction(sampled,direction);if(response){
                uint32_t mask=hn_collision_mask(m,map,x,z),hit;
                if(!(mask&hn_read16(m,0x0858ED30+(response-1)*2))){
                    if(!hn_collision_has_record(m,x,z,&hit))return false;
                    if(!hit){hn_write8(m,STATUS,hn_read8(m,STATUS)|5);hn_write8(m,RESPONSE,(uint8_t)response);}
                }
            }}
        }
    }
    return !m->fault;
}
bool hn_collision_scripts(HNMemory *m,uint32_t control,uint32_t render)
{
    GameAddr slot=render_slot(m,render&255),leader=render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER));if(!slot||!leader)return false;
    unsigned direction=hn_collision_facing(m,leader);int32_t x=projected(m,slot,0,false,0),z=projected(m,slot,2,false,0);
    GameAddr map=hn_read32(m,HN_COLLISION_MAP);
    unsigned center=hn_read16(m,hn_collision_sample(m,map,x,z,direction,255)),ahead=hn_read16(m,hn_collision_ahead(m,map,x,z,direction));
    if(center&0x400)hn_collision_stage(m,center&63);
    else if(ahead&0x800)hn_collision_stage(m,(ahead&0x3C0)>>6);
    else if(control&8){if(center&63)hn_collision_stage(m,center&63);else if(ahead&0x3C0)hn_collision_stage(m,(ahead&0x3C0)>>6);else hn_write8(m,STATUS,hn_read8(m,STATUS)|2);}
    return !m->fault;
}
