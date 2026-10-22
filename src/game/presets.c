#include "game/presets.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"

static bool valid(HNMemory *m,unsigned index)
{return hn_read32(m,HN_RENDER_SLOTS)&&index<hn_read8(m,HN_RENDER_INFO+16);}
static bool animation(HNMemory *m,uint8_t index,GameAddr resource,bool boot_gate)
{
    if(!valid(m,index))return false;
    if(boot_gate&&index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,resource);return !m->fault;
}
static bool field(HNMemory *m,uint8_t index,uint8_t value)
{
    if(!valid(m,index))return false;
    hn_write8(m,hn_render_slot(m,index)+0x60,value);return !m->fault;
}
static bool clear(HNMemory *m,uint32_t mask)
{
    for(unsigned index=0;mask;index++,mask>>=1)if((mask&1)&&!field(m,(uint8_t)index,0))return false;
    return !m->fault;
}
static void indicator(HNMemory *m,uint8_t index,GameAddr resource)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,resource);}
static bool linked(HNMemory *m,uint8_t index,uint8_t parent,GameAddr resource,uint8_t mode)
{return hn_render_configure(m,index,1,mode,0,0,0,resource)&&hn_render_modes(m,index,2,3,parent);}
static bool class_link(HNMemory *m,GameAddr selector,int delta,GameAddr graphics,GameAddr resource)
{
    if(graphics&&!hn_graphics_configure(m,4,graphics))return false;
    if(!hn_render_configure(m,(uint8_t)(hn_read8(m,selector)+delta),1,0xB0,0,0,0,resource))return false;
    /* The render setup may alias the selector: reload it for the second call. */
    return hn_render_modes(m,(uint8_t)(hn_read8(m,selector)+delta),2,3,hn_read8(m,selector));
}
static bool apply(HNMemory *m,GameAddr callback)
{return hn_preset_apply(m,callback)==HN_SCENE_OK;}
HNSceneResult hn_preset_apply(HNMemory *m,GameAddr callback)
{
    bool ok=true;
    switch(callback&~1u){
    /* Actor helpers used by the complete scene-render preset family. Resource
       words identify the player's ROM; no resource bytes are embedded here. */
    case 0x08013384:ok=animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),0x088B5325,true);break;
    case 0x080136B8:ok=animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),0x088B6FD7,true);break;
    case 0x08013FEC:ok=animation(m,hn_read8(m,0x03002C04),0x088C18D6,false);break;
    case 0x08014400:ok=animation(m,hn_read8(m,0x03002C04),0x088BC636,true);break;
    case 0x0801499C:ok=animation(m,hn_read8(m,0x03002C00),0x088C5C71,false);break;
    case 0x080149E4:ok=animation(m,hn_read8(m,0x03002C00),0x088C5CAF,false);break;
    case 0x0801536C:ok=animation(m,hn_read8(m,0x03002C04),0x088CAB5E,false);break;
    case 0x0801542C:ok=animation(m,hn_read8(m,0x03002C04),0x088CACA8,true);break;
    case 0x0801548C:ok=animation(m,hn_read8(m,0x03002C04),0x088CACFB,true);break;
    case 0x08016A6C:ok=animation(m,hn_read8(m,0x03002C00),0x088D2FE5,false);break;
    case 0x08016BD8:ok=animation(m,hn_read8(m,0x03002C00),0x088D33D4,false);break;
    case 0x0801707C:ok=animation(m,hn_read8(m,0x03002C04),0x088D5C88,false);break;
    case 0x0801710C:ok=animation(m,hn_read8(m,0x03002C00),0x088D48A9,false);break;
    case 0x08017C6C:ok=animation(m,hn_read8(m,0x03002C04),0x088DC493,false);break;
    case 0x080182FC:ok=animation(m,hn_read8(m,0x03002C00),0x088DBD87,false);break;
    case 0x08014034:ok=animation(m,hn_read8(m,0x03002C04),0x088C18FB,false);break;
    case 0x08017694:ok=animation(m,hn_read8(m,0x03002C04),0x088D836C,true);break;
    case 0x080176AC:ok=animation(m,hn_read8(m,0x03002C00),0x088D836C,true);break;
    case 0x080176C4:ok=animation(m,hn_read8(m,0x03002C04),0x088D839E,true);break;
    case 0x080178D0:ok=animation(m,hn_read8(m,0x03002C00),0x088D8F67,true);break;
    case 0x08018E2C:ok=field(m,(uint8_t)(hn_read8(m,0x03002C04)-1),0);break;
    case 0x08018E90:ok=field(m,(uint8_t)(hn_read8(m,0x03002C00)-1),0);break;
    case 0x08019A0C:ok=class_link(m,0x03002C00,-1,0x088E1364,0x088E39A9);break;
    case 0x08019FE4:ok=class_link(m,0x03002C04,-1,0,0x088E3AC9);break;
    case 0x0801A028:ok=class_link(m,0x03002C00,-1,0,0x088E3AC9);break;
    case 0x0801A3B0:ok=class_link(m,0x03002C04,-1,0x088E138B,0x088C286A);break;
    case 0x080192A4:ok=class_link(m,HN_RENDER_CLASS_FOLLOWER,-1,0x088E135C,0x088E3920);break;
    case 0x08019584:ok=class_link(m,HN_RENDER_CLASS_FOLLOWER,-1,0x088E1330,0x088E3961);break;
    case 0x080195D8:ok=class_link(m,0x03002C04,-1,0x088E1330,0x088E3961);break;
    case 0x080197D0:ok=class_link(m,0x03002C04,-1,0x088E1330,0x088E3980);break;
    case 0x08019878:ok=class_link(m,0x03002C04,-1,0x088E137B,0x088E398B);break;
    case 0x08019BB0:ok=class_link(m,0x03002C00,-1,0x088E135C,0x088E3A24);break;
    case 0x0801A458:ok=class_link(m,0x03002C00,-1,0x088E1338,0x088C5C7D);break;
    case 0x0801A4AC:ok=class_link(m,0x03002C00,-1,0x088E137B,0x088C5CC4);break;
    case 0x0801A500:ok=class_link(m,0x03002C00,1,0x088E137B,0x088C5CD5);break;
    case 0x0801AA10:ok=class_link(m,0x03002C04,-1,0x088E1330,0x088CACBD);break;
    case 0x0801AAB8:ok=class_link(m,0x03002C04,1,0x088E1330,0x088CACD6);break;
    case 0x0801B7A8:ok=class_link(m,0x03002C04,1,0,0x088D449D);break;
    case 0x0801BAFC:ok=hn_render_configure(m,(uint8_t)(hn_read8(m,0x03002C00)-1),1,0xB0,0,0,0,0x088D48C5)&&
        hn_render_configure(m,(uint8_t)(hn_read8(m,0x03002C00)+1),1,0xB0,0,0,0,0x088D48D6)&&
        hn_render_modes(m,(uint8_t)(hn_read8(m,0x03002C00)-1),2,3,hn_read8(m,0x03002C00))&&
        hn_render_modes(m,(uint8_t)(hn_read8(m,0x03002C00)+1),2,3,hn_read8(m,0x03002C00));break;
    case 0x080CC878:hn_write8(m,HN_SCENE_CONTROLS,(uint8_t)(hn_read32(m,0x03004ED4)>>31));break;
    case 0x080CC8A8:break;
    case 0x080CC8AC:ok=apply(m,0x080136B8)&&apply(m,0x080192A4);break;
    case 0x080CC8BC:ok=apply(m,0x0801499C)&&apply(m,0x0801A458);break;
    case 0x080CC8CC:ok=apply(m,0x080149E4)&&apply(m,0x0801A4AC)&&apply(m,0x0801A500);break;
    case 0x080CC8E0:ok=apply(m,0x08013384)&&apply(m,0x08019584);break;
    case 0x080CC8F0:ok=animation(m,16,0x088BB59C,true)&&linked(m,15,16,0x088BB5B8,0xB0);break;
    case 0x080CC92C:ok=animation(m,19,0x088BE3E2,true);break;
    case 0x080CC940:ok=animation(m,19,0x088BE520,true)&&linked(m,18,19,0x088BE55A,0xB0);break;
    case 0x080CC97C:ok=animation(m,16,0x088BB771,true)&&linked(m,15,16,0x088BB78D,0xB0);break;
    case 0x080CC9B8:ok=apply(m,0x08013FEC)&&apply(m,0x08019878);break;
    case 0x080CC9C8:ok=animation(m,19,0x088DFE96,true);break;
    case 0x080CC9DC:ok=animation(m,19,0x088DFEBF,true);break;
    case 0x080CC9F0:ok=animation(m,16,0x088D5E3C,true)&&
        hn_render_configure(m,15,1,0xB0,0,0,0,0x088D5E51)&&hn_render_configure(m,17,1,0xB0,0,0,0,0x088D5E59)&&
        hn_render_modes(m,15,2,3,16)&&hn_render_modes(m,17,2,3,16);break;
    case 0x080CCA54:ok=hn_graphics_configure(m,4,0x088E13AB)&&animation(m,16,0x088CDC7D,true)&&linked(m,15,16,0x088CDB6B,0xB0)&&field(m,17,0);break;
    case 0x080CCAA8:ok=animation(m,25,0x088EA739,true)&&linked(m,24,25,0x088EA74B,0xB0);if(ok)indicator(m,25,0x088EA739);break;
    case 0x080CCAF0:ok=hn_graphics_configure(m,4,0x088DB4C3)&&hn_graphics_configure(m,5,0x088D111A)&&
        hn_render_configure(m,16,1,0x72,40,0,120,0x088DBB99)&&hn_render_configure(m,25,1,0x72,144,0,128,0x088EA6B9)&&hn_render_modes(m,14,2,3,16);break;
    case 0x080CCB5C:ok=animation(m,19,0x088D2D83,true)&&
        hn_render_configure(m,18,1,0x72,0,0,0,0x088D2D98)&&hn_render_configure(m,20,1,0x72,0,0,0,0x088D2DB7)&&
        hn_render_modes(m,18,2,3,19)&&hn_render_modes(m,20,2,3,19);break;
    case 0x080CCBC0:ok=animation(m,19,0x088D85AB,true);if(ok)indicator(m,19,0x088D85AB);break;
    case 0x080CCBE0:ok=animation(m,22,0x088E9579,true);break;
    case 0x080CCBF4:ok=animation(m,22,0x088E95A0,true);break;
    case 0x080CCC08:ok=apply(m,0x0801536C)&&
        hn_render_configure(m,15,1,0x72,0,0,0,0x088CABA2)&&hn_render_configure(m,17,1,0x72,0,0,0,0x088CABB9)&&
        hn_render_modes(m,15,2,3,16)&&hn_render_modes(m,17,2,3,16);break;
    case 0x080CCC64:ok=apply(m,0x0801548C)&&apply(m,0x080195D8);break;
    case 0x080CCC74:ok=animation(m,22,0x088DE338,true);break;
    case 0x080CCC88:ok=apply(m,0x080182FC)&&apply(m,0x08019BB0);break;
    case 0x080CCC98:ok=apply(m,0x08017C6C)&&
        hn_render_configure(m,15,1,0x72,0,0,0,0x088DC49F)&&hn_render_configure(m,17,1,0x72,0,0,0,0x088DC4B0)&&
        hn_render_modes(m,15,2,3,16)&&hn_render_modes(m,17,2,3,16);break;
    case 0x080CCCF4:ok=apply(m,0x08014400)&&apply(m,0x080197D0);break;
    case 0x080CCD04:ok=apply(m,0x08016BD8)&&linked(m,18,19,0x088D33FB,0x72);break;
    case 0x080CCD38:ok=apply(m,0x0801542C)&&apply(m,0x0801AA10)&&apply(m,0x0801AAB8);break;
    case 0x080CCD4C:ok=apply(m,0x08016A6C)&&
        hn_render_configure(m,18,1,0x72,0,0,0,0x088D300D)&&hn_render_configure(m,20,1,0x72,0,0,0,0x088D301B)&&
        hn_render_modes(m,18,2,3,19)&&hn_render_modes(m,20,2,3,19);break;
    case 0x080CCDA8:ok=apply(m,0x0801707C)&&apply(m,0x0801B7A8)&&linked(m,15,16,0x088CA000,0xB0)&&hn_graphics_configure(m,4,0x088E13B3);break;
    case 0x080CCDF0:ok=animation(m,19,0x088BF92A,true);break;
    case 0x080CCE04:ok=animation(m,19,0x088BF962,true)&&
        hn_render_configure(m,18,1,0x72,0,0,0,0x088BF997)&&hn_render_configure(m,20,1,0x72,0,0,0,0x088BF9B6)&&
        hn_render_modes(m,18,2,3,19)&&hn_render_modes(m,20,2,3,19);break;
    case 0x080CCE68:ok=apply(m,0x0801710C)&&apply(m,0x0801BAFC);break;
    case 0x080CCE78:if(hn_read8(m,HN_SCENE_CONTROLS+0xC6)==7)ok=animation(m,22,0x088F4248,true);break;
    case 0x080CCE98:if(hn_read8(m,HN_SCENE_CONTROLS+0xC6)==7)ok=animation(m,22,0x088F4256,true);break;
    case 0x080CCEB8:if(hn_read8(m,HN_SCENE_CONTROLS+0xC6)==7)ok=animation(m,22,0x088F426F,true);break;
    case 0x080CCED8:if(hn_read8(m,HN_SCENE_CONTROLS+0xC6)==7)ok=animation(m,22,0x088F4288,true);break;
    case 0x080CCEF8:ok=clear(m,1u<<11);break;
    case 0x080CCF08:ok=clear(m,(1u<<12)|(1u<<14));break;
    case 0x080CCF20:ok=clear(m,(1u<<18)|(1u<<20));break;
    case 0x080CCF38:ok=clear(m,(1u<<15)|(1u<<17));break;
    case 0x080CCF50:ok=apply(m,0x080CCEF8)&&apply(m,0x080CCF08)&&apply(m,0x080CCF20)&&apply(m,0x080CCF38);break;
    case 0x080CCF68:case 0x080CD104:case 0x080CD108:case 0x080CD150:case 0x080CD16C:case 0x080CD1B0:
    case 0x080CD1CC:case 0x080CD2B4:case 0x080CD32C:case 0x080CD330:case 0x080CD334:case 0x080CD338:break;
    case 0x080CCF6C:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5BF9,true);break;
    case 0x080CCF84:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5C60,true);if(ok)indicator(m,19,0x088C5C60);break;
    case 0x080CCFA8:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5C71,true)&&linked(m,18,19,0x088C5C7D,0xB0);break;
    case 0x080CCFEC:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5A97,true);break;
    case 0x080CD004:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5C8E,true);break;
    case 0x080CD01C:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5CA3,true);break;
    case 0x080CD034:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5CAF,true)&&linked(m,18,19,0x088C5CC4,0xB0)&&linked(m,20,19,0x088C5CD5,0xB0);break;
    case 0x080CD09C:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5CE6,true);break;
    case 0x080CD0B4:ok=apply(m,0x080CCF20)&&hn_graphics_configure(m,4,0x088E135C)&&animation(m,19,0x088C5D0D,true)&&linked(m,18,19,0x088C5D29,0xB0);break;
    case 0x080CD10C:ok=apply(m,0x080CCF38)&&animation(m,16,0x088BA5F7,true)&&linked(m,15,16,0x088BA603,0x72);break;
    case 0x080CD154:ok=apply(m,0x080CCF38)&&animation(m,16,0x088BA5EB,true);break;
    case 0x080CD170:ok=apply(m,0x080CCEF8)&&apply(m,0x080CCF08)&&animation(m,10,0x088AC134,true)&&animation(m,13,0x088B50AB,true);break;
    case 0x080CD198:ok=apply(m,0x080CCF08)&&animation(m,13,0x088B50E3,true);break;
    case 0x080CD1B4:ok=apply(m,0x080CCF08)&&animation(m,13,0x088B551C,true);break;
    case 0x080CD1D0:ok=apply(m,0x080CCF08)&&animation(m,13,0x088B5136,true);break;
    case 0x080CD1E8:ok=apply(m,0x080CCEF8)&&apply(m,0x080CCF08)&&animation(m,10,0x088ADA0E,true)&&linked(m,11,10,0x088ADA54,0x72)&&field(m,11,2)&&
        animation(m,13,0x088B5611,true)&&linked(m,12,13,0x088ADA54,0x72)&&field(m,13,2);break;
    case 0x080CD26C:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5D40,true);break;
    case 0x080CD284:ok=apply(m,0x080CCF20)&&animation(m,19,0x088C5D4C,true);break;
    case 0x080CD29C:ok=apply(m,0x080CCF38)&&animation(m,16,0x088BA4B4,true);break;
    case 0x080CD2B8:ok=apply(m,0x080CCF38)&&animation(m,16,0x088BA456,true);break;
    case 0x080CD2D0:ok=apply(m,0x080CCF08)&&animation(m,13,0x088B4002,true);break;
    case 0x080CD2E8:ok=apply(m,0x080CCF08)&&animation(m,13,0x088B79D7,true)&&linked(m,12,13,0x088B79F3,0xB0);break;
    default:return HN_SCENE_UNKNOWN;
    }
    return ok&&!m->fault?HN_SCENE_OK:HN_SCENE_RESOURCE;
}
HNSceneResult hn_preset_callback(HNMemory *m,GameAddr callback,uint32_t *consumed)
{
    /* These source functions explicitly return zero. Void helpers are reachable
       only through the task ABI; Main must never invent a consumed count. */
    switch(callback&~1u){
    case 0x080CC878:case 0x080CCF68:case 0x080CCF6C:case 0x080CCF84:case 0x080CCFA8:case 0x080CCFEC:
    case 0x080CD004:case 0x080CD01C:case 0x080CD034:case 0x080CD09C:case 0x080CD0B4:case 0x080CD104:
    case 0x080CD108:case 0x080CD10C:case 0x080CD150:case 0x080CD154:case 0x080CD16C:case 0x080CD170:
    case 0x080CD198:case 0x080CD1B0:case 0x080CD1B4:case 0x080CD1CC:case 0x080CD1D0:case 0x080CD1E8:
    case 0x080CD26C:case 0x080CD284:case 0x080CD29C:case 0x080CD2B4:case 0x080CD2B8:case 0x080CD2D0:
    case 0x080CD2E8:case 0x080CD32C:case 0x080CD330:case 0x080CD334:case 0x080CD338:
        *consumed=0;return hn_preset_apply(m,callback);
    default:return HN_SCENE_UNKNOWN;
    }
}
