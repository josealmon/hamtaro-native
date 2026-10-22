#include "game/presentation.h"
#include "game/state.h"
#include "game/render.h"
#include "game/world.h"
#include "game/video.h"
#include "game/transfer.h"
#include "gba/bios.h"

static bool flag(HNMemory *m,unsigned id){return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void animation(HNMemory *m,uint8_t index,GameAddr image)
{if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);hn_render_set_animation(m,index,image);}
static void clear_pair(HNMemory *m,uint8_t first)
{hn_write8(m,hn_render_slot(m,first)+0x60,0);hn_write8(m,hn_render_slot(m,first+2)+0x60,0);}
bool hn_presentation_actor_height(HNMemory *m,GameAddr slot,unsigned operation)
{
    if(operation>2)return false;
    bool lower=operation==1;
    if(!operation)lower=flag(m,0x1AD)&&hn_read32(m,slot+0x18)-0x508000u<=0x800000&&hn_read32(m,slot+0x20)-0x1E8000u<=0x620000;
    uint32_t y=hn_read32(m,slot+0x1C);
    hn_write32(m,slot+0x1C,lower?((int32_t)y<=-0xC8000?0u-0xC8000:y-0x20000):((int32_t)y>0x7FFF?0x8000:y+0x20000));
    return !m->fault;
}
uint32_t hn_presentation_region_motion(HNMemory *m,GameAddr slot,GameAddr z_moving,GameAddr x_moving,GameAddr z_idle,GameAddr x_idle)
{
    uint32_t x=hn_read32(m,slot+0x18),minimum=flag(m,0x7E)?0x100000:0x600000;
    if(x>minimum&&(int32_t)x<=0x71FFFF&&(int32_t)hn_read32(m,slot+0x20)<=0x6BFFFF){
        if(hn_read32(m,slot+4)!=z_moving){hn_write8(m,slot+0x60,1);hn_render_set_animation_pointer(m,slot,z_moving);}
        hn_write32(m,slot+0x20,hn_read32(m,slot+0x20)+0x20000);return 1;
    }
    if(x-0x720000u<=0x3EFFFF){
        if(hn_read32(m,slot+4)!=x_moving){hn_write8(m,slot+0x60,3);hn_render_set_animation_pointer(m,slot,x_moving);}
        hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+0x20000);return 1;
    }
    if(hn_read8(m,slot+0x60)==1){if(hn_read32(m,slot+4)==z_moving)hn_render_set_animation_pointer(m,slot,z_idle);}
    else if(hn_read32(m,slot+4)==x_moving)hn_render_set_animation_pointer(m,slot,x_idle);
    return 0;
}
static void active_range(HNMemory *m)
{
    unsigned count=hn_read8(m,HN_RENDER_INFO+16);
    if(count<2||46>=count)hn_render_error(m,4);
    else{hn_write8(m,HN_RENDER_INFO+17,0);hn_write8(m,HN_RENDER_INFO+18,46);}
}
static void affine(HNMemory *m)
{
    GameAddr p=HN_LAYER_OFFSETS+88,v=HN_VIDEO_STATE+0x98;
    hn_write16(m,v+2,(uint16_t)((hn_read32(m,p)+hn_read32(m,p+0x10))>>16));
    hn_write16(m,v+4,(uint16_t)((hn_read32(m,p+4)+hn_read32(m,p+0x14))>>16));hn_video_mark_backgrounds(m,4);
    unsigned angle=hn_read16(m,p+0x1C);int sine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+angle*2),cosine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+(angle+64)*2);
    int rx=(int16_t)hn_runtime_divide(65536,(int16_t)hn_read16(m,p+0x18)),ry=(int16_t)hn_runtime_divide(65536,(int16_t)hn_read16(m,p+0x1A));
    int matrix[4]={(int16_t)(cosine*rx/256),(int16_t)(sine*rx/256),(int16_t)((int16_t)-sine*ry/256),(int16_t)(cosine*ry/256)};
    for(unsigned i=0;i<4;i++)hn_write16(m,p+0x22+i*2,(uint16_t)matrix[i]);
    int ax=(int16_t)hn_read16(m,p+0x1E),ay=(int16_t)hn_read16(m,p+0x20);
    uint32_t x=(uint32_t)((int32_t)(hn_read32(m,HN_VIDEO_STATE+4)-hn_read32(m,p))>>8),y=(uint32_t)((int32_t)(hn_read32(m,HN_VIDEO_STATE+8)-hn_read32(m,p+4))>>8);
    x-=(uint32_t)ax+(uint32_t)(matrix[0]*ax)+(uint32_t)(matrix[1]*ay);hn_write32(m,p+8,x);
    y-=(uint32_t)ay+(uint32_t)(matrix[2]*ax)+(uint32_t)(matrix[3]*ay);hn_write32(m,p+12,y);
    for(unsigned i=0;i<4;i++)hn_write16(m,v+6+i*2,(uint16_t)matrix[i]);hn_write32(m,v+0x10,x);hn_write32(m,v+0x14,y);
}
HNSceneResult hn_presentation_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x0801D3D0:hn_render_reset(m);break;
    case 0x0801D3DC:{
        GameAddr temporary=0x03007C00;uint16_t old=hn_read16(m,temporary);hn_write16(m,temporary,0);
        hn_write32(m,0x040000D4,temporary);hn_write32(m,0x040000D8,hn_read32(m,HN_GRAPHICS_SCRIPTS));hn_write32(m,0x040000DC,0x81000160);hn_write16(m,temporary,old);break;
    }
    case 0x0801D408:
        if(!hn_transfer_fill_words(m,hn_read32(m,HN_RENDER_STATES),0,hn_read8(m,HN_BACKGROUND_COUNT)*20)||!hn_transfer_fill_words(m,HN_LAYER_OFFSETS,0,44))return HN_SCENE_RESOURCE;
        for(unsigned i=0;i<4;i++){GameAddr p=HN_LAYER_OFFSETS+i*44;hn_write16(m,p+0x18,256);hn_write16(m,p+0x1A,256);hn_write16(m,p+0x22,256);hn_write16(m,p+0x28,256);}break;
    case 0x080CBBD0:
        hn_video_set_mode(m,1);hn_write16(m,HN_VIDEO_STATE+0x9E,256);hn_write16(m,HN_VIDEO_STATE+0xA4,256);hn_video_mark_backgrounds(m,5);active_range(m);break;
    case 0x080CBC14:hn_write8(m,HN_DISPLAY_SHADOW,hn_read8(m,HN_DISPLAY_SHADOW)&0xF0);hn_video_mark_backgrounds(m,1);break;
    case 0x080CBC34:hn_write16(m,HN_VIDEO_STATE+0x3A,240);hn_write16(m,HN_VIDEO_STATE+0x3C,128);hn_video_mark_backgrounds(m,1);break;
    case 0x080CBC50:affine(m);break;
    case 0x0802204C:{
        hn_write8(m,HN_SCENE_CONTROLS+0x60,0);GameAddr a=hn_render_slot(m,10),b=hn_render_slot(m,13);
        int32_t x13=(int32_t)hn_read32(m,b+0x18),x10=(int32_t)hn_read32(m,a+0x18),dx=(int32_t)((uint32_t)x13-(uint32_t)x10);
        if(dx<0)dx=(int32_t)(0u-(uint32_t)dx);
        int32_t z13=(int32_t)hn_read32(m,b+0x20),z10=(int32_t)hn_read32(m,a+0x20),dz=(int32_t)((uint32_t)z13-(uint32_t)z10);
        if(dz<0)dz=(int32_t)(0u-(uint32_t)dz);
        if(dx<=0x280000){if(dz<=0x280000)break;}
        else if(dz<=0x280000){hn_write8(m,HN_SCENE_CONTROLS+0x60,x10<x13?2:1);break;}
        hn_write8(m,HN_SCENE_CONTROLS+0x60,z10<z13?4:3);break;
    }
    default:{HNSceneResult r=hn_presentation_background_callback(m,callback,arguments,consumed);return r==HN_SCENE_UNKNOWN?hn_presentation_window_callback(m,callback,arguments,consumed):r;}
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_presentation_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080B8F30:clear_pair(m,9);break;
    case 0x080B8F48:clear_pair(m,12);break;
    case 0x080B8F60:clear_pair(m,15);break;
    case 0x080B8F78:clear_pair(m,18);break;
    case 0x080B8F90:clear_pair(m,28);break;
    case 0x080B8FA8:clear_pair(m,15);animation(m,hn_read8(m,0x03002C04),0x088CA4CC);break;
    case 0x080B8FC4:clear_pair(m,15);animation(m,hn_read8(m,0x03002C04),0x088CACEF);break;
    case 0x080B8FE0:clear_pair(m,15);animation(m,hn_read8(m,0x03002C04),0x088CB0AA);break;
    case 0x080B8FFC:clear_pair(m,15);animation(m,hn_read8(m,0x03002C04),0x088CDBC7);break;
    case 0x080B9018:clear_pair(m,15);animation(m,hn_read8(m,0x03002C04),0x088CBEEA);break;
    case 0x080B9034:clear_pair(m,15);animation(m,hn_read8(m,0x03002C04),0x088CCF49);break;
    case 0x080B9050:clear_pair(m,15);animation(m,hn_read8(m,0x03002C04),0x088CD4AC);break;
    case 0x080B906C:clear_pair(m,15);animation(m,13,0x088B4FE1);break;
    case 0x080B9084:clear_pair(m,15);animation(m,13,0x088B5325);break;
    case 0x080B909C:clear_pair(m,15);animation(m,13,0x088B70A6);break;
    case 0x080B90B4:clear_pair(m,15);animation(m,13,0x088B5D66);break;
    case 0x0801D3D0:case 0x0801D3DC:case 0x0801D408:case 0x080CBBD0:case 0x080CBC14:case 0x080CBC34:case 0x080CBC50:case 0x0802204C:{uint32_t ignored;return hn_presentation_callback(m,callback,0,&ignored);}
    default:{HNSceneResult r=hn_presentation_background_task(m,callback);return r==HN_SCENE_UNKNOWN?hn_presentation_window_task(m,callback):r;}
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_presentation_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080B90CC:
        if(!hn_presentation_actor_height(m,hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER)),0))return HN_SCENE_RESOURCE;
        if(flag(m,0x10)&&!hn_presentation_actor_height(m,hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER)),0))return HN_SCENE_RESOURCE;
        *yield=1;break;
    case 0x08021EEC:{
        uint32_t moving=hn_presentation_region_motion(m,hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER)),0x088AC2BA,0x088AC359,0x088AC0C6,0x088AC0FD);
        moving+=hn_presentation_region_motion(m,hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER)),0x088B4211,0x088B42B0,0x088B4055,0x088B408C);
        if(!moving)hn_write8(m,slot+0x60,0);*yield=1;break;
    }
    default:return hn_presentation_background_animation(m,callback,slot,arguments,consumed,yield);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
