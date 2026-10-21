#include "game/render.h"
#include "game/state.h"
#include "game/video.h"
#include "gba/bios.h"
static int16_t multiply8(int left,int right)
{return (int16_t)(((int16_t)left*(int16_t)right)/256);}
static unsigned object_size(HNMemory *m,unsigned a0,unsigned a1,unsigned axis)
{
    unsigned index=((a0&0xC000)>>11)|((a1&0xC000)>>13);
    return hn_read8(m,HN_OBJECT_SIZE_TABLE+index+axis);
}
static void swap(HNMemory *m,GameAddr a,GameAddr b)
{
    uint32_t key=hn_read32(m,a);unsigned priority=hn_read8(m,a+4),id=hn_read8(m,a+5);
    hn_write32(m,a,hn_read32(m,b));hn_write8(m,a+4,hn_read8(m,b+4));hn_write8(m,a+5,hn_read8(m,b+5));
    hn_write32(m,b,key);hn_write8(m,b+4,(uint8_t)priority);hn_write8(m,b+5,(uint8_t)id);
}
static void bubble_key(HNMemory *m,GameAddr priority,int position,unsigned remaining)
{
    while(remaining){
        GameAddr p=priority+(uint32_t)(position*8);swap(m,p,p+8);position--;remaining--;
        if(!remaining)break;p-=8;
        if(hn_read8(m,p+4)<hn_read8(m,p+12))break;
        if((int32_t)hn_read32(m,p)>=(int32_t)hn_read32(m,p+8))break;
    }
}
static void sort(HNMemory *m,GameAddr priority,unsigned first,unsigned last)
{
    /* The original insertion pass has distinct priority, key and id exits.
       A total-order comparator changes the order of some equal-key entries. */
    for(unsigned i=first;i<last;i++){
        int j=(int)i;unsigned remaining=(uint8_t)(i-first+1);
        GameAddr p=priority+i*8;unsigned left=hn_read8(m,p+4),right=hn_read8(m,p+12);
        if(left>right){
            while(remaining){
                swap(m,p,p+8);j--;remaining--;p-=8;
                if(!remaining||hn_read8(m,p+4)<=hn_read8(m,p+12))break;
            }
            if(remaining&&hn_read8(m,p+4)==hn_read8(m,p+12)&&
               (int32_t)hn_read32(m,p)<(int32_t)hn_read32(m,p+8))bubble_key(m,priority,j,remaining);
        }else if(left==right){
            int32_t left_key=(int32_t)hn_read32(m,p),right_key=(int32_t)hn_read32(m,p+8);
            if(left_key<right_key)bubble_key(m,priority,j,remaining);
            else if(left_key==right_key&&hn_read8(m,p+5)>hn_read8(m,p+13)){
                while(remaining){
                    swap(m,p,p+8);remaining--;p-=8;
                    if(!remaining||hn_read8(m,p+4)<hn_read8(m,p+12)||
                       (int32_t)hn_read32(m,p)>(int32_t)hn_read32(m,p+8)||hn_read8(m,p+5)<=hn_read8(m,p+13))break;
                }
            }
        }
    }
}
static unsigned scaled_coordinate(unsigned value,unsigned mask,int scale,unsigned half,unsigned flags,unsigned anchor,unsigned screen)
{
    bool negative=(value&((mask+1)>>1))!=0;
    unsigned magnitude=negative?(value^mask)+1:value;
    unsigned coordinate=(uint16_t)multiply8((int16_t)magnitude,scale);
    if(negative)coordinate=(uint16_t)((coordinate^mask)+1);
    unsigned delta=(uint16_t)((uint16_t)multiply8(half,scale)-half);
    if(flags&32)coordinate=(uint16_t)(coordinate-half);
    coordinate=(uint16_t)(coordinate+delta);
    delta=(uint16_t)((uint16_t)multiply8((int16_t)anchor,scale)-anchor);
    delta>>=8;
    if(mask==511&&(delta&128))delta|=256;
    delta&=mask;
    return (((coordinate-delta)&mask)+screen)&mask;
}
bool hn_render_draw(HNRender *r)
{
    if(r->fault)return false;
    HNMemory *m=r->memory;unsigned count=hn_read8(m,HN_RENDER_INFO+0x10),processed=0;
    GameAddr priority=hn_read32(m,HN_RENDER_PRIORITY);
    for(unsigned i=0;i<count&&!m->fault;i++){
        GameAddr slot=hn_render_slot(m,hn_read8(m,priority+i*8+5));unsigned flags=hn_read8(m,slot+0x62);
        if(!hn_read8(m,slot+0x60)||(flags&192))continue;
        unsigned changed=0;
        for(unsigned layer=0;layer<4;layer++){
            unsigned bit=1u<<layer;if((processed&bit)||!(flags&bit))continue;
            int x=(int16_t)(hn_read32(m,slot+0x18)>>16);
            int y=(int16_t)((hn_read32(m,slot+0x1C)>>16)+(hn_read32(m,slot+0x20)>>16));
            x=(int16_t)(x-hn_read16(m,HN_RENDER_INFO+12));y=(int16_t)(y-hn_read16(m,HN_RENDER_INFO+14));
            uint32_t ox,oy;hn_background_offset(m,(uint8_t)layer,x,y,&ox,&oy);
            GameAddr background=HN_VIDEO_STATE+0x38+layer*0x30;
            uint16_t bx=(uint16_t)(0u-(uint32_t)((int32_t)ox>>16)),by=(uint16_t)(0u-(uint32_t)((int32_t)oy>>16));
            if((flags&16)&&hn_read16(m,background+2)!=bx){hn_write16(m,background+2,bx);changed|=bit;}
            if((flags&32)&&hn_read16(m,background+4)!=by){hn_write16(m,background+4,by);changed|=bit;}
            processed|=bit;
        }
        hn_video_mark_backgrounds(m,(uint8_t)changed);
    }
    if(!hn_render_assign_affine(m)){
        r->fault=HN_RENDER_RESOURCE;r->fault_address=HN_AFFINE_USAGE;return false;
    }
    for(unsigned i=0;i<count;i++){
        GameAddr entry=priority+i*8;unsigned id=hn_read8(m,entry+5);
        hn_write32(m,entry,(uint32_t)hn_render_key(m,id));
        hn_write8(m,entry+4,(hn_read8(m,hn_render_slot(m,id)+0x61)&12)>>2);
    }
    unsigned first=hn_read8(m,HN_RENDER_INFO+0x11),last=hn_read8(m,HN_RENDER_INFO+0x12);
    sort(m,priority,first,last);
    unsigned drawn=0;
    for(unsigned i=0;i<count&&!m->fault;i++){
        GameAddr slot=hn_render_slot(m,hn_read8(m,priority+i*8+5)),frame=hn_read32(m,slot);
        if(!hn_read8(m,slot+0x60)||!frame)continue;
        unsigned matrix=hn_read8(m,slot+0x64),flags=hn_read8(m,slot+0x61),palette=hn_read8(m,slot+0x67)&240;
        int sx=(int16_t)hn_read16(m,slot+0x3C),sy=(int16_t)hn_read16(m,slot+0x3E);
        unsigned rotation=hn_read16(m,slot+0x40);
        if(matrix!=255){
            int16_t values[4];
            hn_bios_obj_matrix((int16_t)hn_runtime_divide(65536,sx),(int16_t)hn_runtime_divide(65536,sy),(uint16_t)rotation,values);
            for(unsigned j=0;j<4;j++)hn_write16(m,HN_OAM_SHADOW+matrix*32+6+j*8,(uint16_t)values[j]);
        }
        hn_render_screen_position(m,slot);
        unsigned screen_x=hn_read16(m,slot+0x4E),screen_y=hn_read16(m,slot+0x50),parts=hn_read8(m,frame);
        if(parts>=128)parts=0;
        int ax=-(int8_t)hn_read8(m,slot+0x5E),ay=(int8_t)hn_read8(m,slot+0x5F);
        int sine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+(rotation>>8)*2);
        int cosine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+((rotation>>8)+64)*2);
        for(unsigned j=0;j<parts&&!m->fault;j++){
            GameAddr part=frame+2+j*6,output=HN_OAM_SHADOW+drawn++*8;
            unsigned a0=hn_read16(m,part),a1=hn_read16(m,part+2),a2=hn_read16(m,part+4);
            if(matrix==255){
                a0=(((a0&0xFF00)|(((a0&255)+screen_y)&255))&0xE3FF)|((flags&16)<<8)|((flags&3)<<10);
                a1=(a1&0xFE00)|(((a1&511)+screen_x)&511);a2=(a2&0xF3FF)|((flags&12)<<8);
            }else{
                unsigned x=scaled_coordinate(a1&511,511,sx,object_size(m,a0,a1,0)>>1,flags,hn_read16(m,slot+0x52),screen_x);
                unsigned y=scaled_coordinate(a0&255,255,sy,object_size(m,a0,a1,1)>>1,flags,hn_read16(m,slot+0x54),screen_y);
                int dy=ay-(int16_t)(multiply8(ax,sine)+multiply8(ay,cosine));
                int dx=ax-(int16_t)(multiply8(ax,cosine)-multiply8(ay,sine));
                y=(y+(uint16_t)dy)&255;x=(x-(uint16_t)dx)&511;
                a0=(((a0&0xFF00)|y|256|((flags&32)<<4))&0xE3FF)|((flags&16)<<8)|((flags&3)<<10);
                a1=(((a1&0xFE00)|x)&0xC1FF)|((matrix<<9)&0x3E00);a2|=(flags&12)<<8;
            }
            if(palette)a2=(a2&0x0FFF)|(palette<<8);
            hn_write16(m,output,(uint16_t)a0);hn_write16(m,output+2,(uint16_t)a1);hn_write16(m,output+4,(uint16_t)a2);
        }
    }
    for(;drawn<128;drawn++){
        GameAddr output=HN_OAM_SHADOW+drawn*8;
        hn_write16(m,output,160);hn_write16(m,output+2,240);hn_write16(m,output+4,0);
    }
    if(m->fault){r->fault=HN_RENDER_MEMORY;r->fault_address=m->fault_address;return false;}
    return true;
}
bool hn_render_update(HNRender *r,unsigned budget)
{
    if(r->fault)return false;
    HNMemory *m=r->memory;
    for(unsigned i=0;i<32;i++)hn_write8(m,HN_AFFINE_USAGE+i,255);
    hn_render_mark_affine(m);
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10);
    for(unsigned i=0;i<count;i++){
        GameAddr slot=hn_render_slot(m,i);
        if(hn_read8(m,slot+0x60)){
            hn_render_physics(m,slot);
            if(!(hn_read8(m,slot+0x61)&128)&&!hn_render_animation(r,slot,budget))return false;
        }
    }
    if(!hn_render_draw(r))return false;
    hn_write16(m,HN_RENDER_FRAME_COUNT,(uint16_t)(hn_read16(m,HN_RENDER_FRAME_COUNT)+1));
    return !m->fault;
}
