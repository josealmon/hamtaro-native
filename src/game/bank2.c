#include "game/bank2.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/input.h"
#include "game/random.h"
#include "game/motion.h"

static bool valid(HNMemory *m,unsigned index)
{return hn_read32(m,HN_RENDER_SLOTS)&&index<hn_read8(m,HN_RENDER_INFO+16);}
static bool select_animation(HNMemory *m,uint8_t index,GameAddr resource,bool gate)
{
    if(!valid(m,index))return false;
    if(gate&&index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,resource);return !m->fault;
}
static bool field(HNMemory *m,uint8_t index,uint8_t value,unsigned offset)
{if(!valid(m,index))return false;hn_write8(m,hn_render_slot(m,index)+offset,value);return !m->fault;}
static bool field_animation(HNMemory *m,uint8_t index,uint8_t value,GameAddr resource)
{
    if(!valid(m,index))return false;
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    return field(m,index,value,0x60)&&select_animation(m,index,resource,false);
}
static bool priority(HNMemory *m,uint8_t index,uint8_t value)
{if(!valid(m,index))return false;hn_render_flags(m,index,12,(uint8_t)(value<<2));return !m->fault;}
static void indicator(HNMemory *m,uint8_t index,GameAddr resource)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,resource);}
static bool flag(HNMemory *m,unsigned id)
{return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}

/* Ordered actor/control presets; resources are identifiers in the player's ROM. */
static HNSceneResult simple_apply(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x0802D000: /* UnreferencedMapBank2ReturnZero */ {

        break;
    }
    case 0x0802D350: /* ClearSceneRenderSlots17And18 */ {
        if(!(field(m,17, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,18, 0,0x60)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D368: /* ConfigureSceneControlPreset2D368 */ {
        if(!(select_animation(m,13, 0x088A8369,false)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,10, 0x088AF60C,false)))return HN_SCENE_RESOURCE;
        if(!(field(m,21, 1,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,21, 0xB0,0x62)))return HN_SCENE_RESOURCE;
        if(!(field(m,21, 13,0x65)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,21, 0x088E180F,false)))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,13);
        hn_write32(m,0x03001F7C,0x088A8369);
        break;
    }
    case 0x0802D3C0: /* ConfigureSceneControlPreset2D3C0 */ {
        if(!(select_animation(m,13, 0x088B6B06,false)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,10, 0x088AF645,false)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D3E0: /* ConfigureSceneControlPreset2D3E0 */ {
        if(!(select_animation(m,16, 0x088C2EE2,false)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D3F4: /* ConfigureSceneControlPreset2D3F4 */ {
        if(!(select_animation(m,16, 0x088C2F12,false)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D408: /* ConfigureSceneControlPreset2D408 */ {
        if(!(select_animation(m,16, 0x088C2F27,false)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D41C: /* ConfigureSceneControlPreset2D41C */ {
        if(!(select_animation(m,16, 0x088BB59C,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0xB0, 0, 0, 0, 0x088BB5B8)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D458: /* ConfigureSceneControlPreset2D458 */ {
        if(!(select_animation(m,16, 0x088BB55A,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0xB0, 0, 0, 0, 0x088BB58F)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D494: /* ConfigureSceneControlPreset2D494 */ {
        if(!(select_animation(m,16, 0x088BB483,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0xB0, 0, 0, 0, 0x088BB514)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0xB0, 0, 0, 0, 0x088BB537)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088BB483);
        break;
    }
    case 0x0802D50C: /* ConfigureSceneControlPreset2D50C */ {
        if(!(select_animation(m,16, 0x088BB771,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0xB0, 0, 0, 0, 0x088BB78D)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0xB0, 0, 0, 0, 0x088BB795)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D570: /* ConfigureSceneControlPreset2D570 */ {
        if(!(select_animation(m,16, 0x088BB771,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0xB0, 0, 0, 0, 0x088BB78D)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D5AC: /* ConfigureSceneControlPreset2D5AC */ {
        if(!(select_animation(m,19, 0x088BE3FE,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0xB0, 0, 0, 0, 0x088BE433)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 0xB0, 0, 0, 0, 0x088BE452)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D610: /* ConfigureSceneControlPreset2D610 */ {
        if(!(hn_bank2_apply(m,0x0802D350)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BE010,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 0xB0, 0, 0, 0, 0x088E1D3B)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,20, 1, 0xB0, 0, 0, 0, 0x088E1D61)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,20, 2, 3, 19)))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,19);
        hn_write32(m,0x03001F7C,0x088BE010);
        break;
    }
    case 0x0802D68C: /* ConfigureSceneControlPreset2D68C */ {
        if(!(field(m,15, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(hn_bank2_apply(m,0x0802D350)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088BB40E,false)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BDF9E,false)))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088BB40E);
        break;
    }
    case 0x0802D6CC: /* ConfigureSceneControlPreset2D6CC */ {
        if(!(select_animation(m,19, 0x088BE520,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 0xB0, 0, 0, 0, 0x088BE55A)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D708: /* ConfigureSceneControlPreset2D708 */ {
        if(!(field(m,14, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,15, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088BB70E,false)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D72C: /* ConfigureSceneControlPreset2D72C */ {
        if(!(select_animation(m,16, 0x088BB5C0,false)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0xB0, 0, 0, 0, 0x088BB5DC)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D768: /* ConfigureSceneControlPreset2D768 */ {
        if(!(select_animation(m,28, 0x088E5718,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D77C: /* ConfigureSceneControlPreset2D77C */ {
        if(!(select_animation(m,28, 0x088E5725,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D790: /* ConfigureSceneControlPreset2D790 */ {
        if(!(select_animation(m,28, 0x088E572F,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D7A4: /* ConfigureSceneControlPreset2D7A4 */ {
        if(!(hn_render_configure(m,29, 1, 0x72, 56, 0, 32, 0x088E56BF)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D7CC: /* ConfigureSceneControlPreset2D7CC */ {
        if(!(field_animation(m,29, 0, 0)))return HN_SCENE_RESOURCE;
        if(!(field_animation(m,31, 0, 0)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802D7E8: /* ConfigureSceneControlPreset2D7E8 */ {
        if(!(select_animation(m,28, 0x088E574B,true)))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,28);
        hn_write32(m,0x03001F7C,0x088E574B);
        break;
    }
    case 0x0802D810: /* ConfigureSceneControlPreset2D810 */ {
        if(!(hn_render_configure(m,31, 1, 0x72, 80, 0, 16, 0x088E56D6)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802DADC: /* ClearSceneSlots15And17 */ {
        if(!(field(m,15, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,17, 0,0x60)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802DAF4: /* ConfigureSceneSlotPreset2DAF4 */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,10, 0x088A6141,true)))return HN_SCENE_RESOURCE;
        indicator(m,10, 0x088A6141);
        if(flag(m,0x10)){if(!(select_animation(m,13, 0x088AA4AA,true)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DB30: /* ConfigureSceneSlotPreset2DB30 */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,10, 0x088AC134,true)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D26EB,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0x72, 0, 0, 0, 0x088D2700)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0x72, 0, 0, 0, 0x088D272D)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(flag(m,0x10)){if(!(field_animation(m,13, 2, 0x088B6A5D)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DBBC: /* ConfigureSceneSlotPreset2DBBC */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D26EB,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0x72, 0, 0, 0, 0x088D2700)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0x72, 0, 0, 0, 0x088D272D)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(flag(m,0x10)){if(!(select_animation(m,13, 0x088B6A5D,true)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DC3C: /* ConfigureSceneSlotPreset2DC3C */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D2B55,true)))return HN_SCENE_RESOURCE;
        indicator(m,16, 0x088D2B55);
        break;
    }
    case 0x0802DC60: /* ConfigureSceneSlotPreset2DC60 */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D280F,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0x72, 0, 0, 0, 0x088D286B)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        indicator(m,16, 0x088D280F);
        break;
    }
    case 0x0802DCB0: /* ConfigureSceneSlotPreset2DCB0 */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D26EB,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0x72, 0, 0, 0, 0x088D2700)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0x72, 0, 0, 0, 0x088D272D)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802DD18: /* ConfigureSceneSlotPreset2DD18 */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D2A47,true)))return HN_SCENE_RESOURCE;
        indicator(m,16, 0x088D2A47);
        break;
    }
    case 0x0802DD3C: /* ConfigureSceneSlotPreset2DD3C */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D2A70,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0x72, 0, 0, 0, 0x088D2A8C)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802DD80: /* SceneSlotPresetNoop2DD80 */ {

        break;
    }
    case 0x0802DD84: /* ConfigureSceneSlotPreset2DD84 */ {
        if(!(hn_bank2_apply(m,0x0802DADC)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D2757,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802DDC4: /* ClearSceneSlots17And18 */ {
        if(!(field(m,17, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,18, 0,0x60)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802DDDC: /* ConfigureSceneSlotPreset2DDDC */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF867,true)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_SCENE_CONTROLS+0x6D)<=4){if(!(hn_render_configure(m,15, 1, 0x72, 104, 0, 32, 0x088E5DA5)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DE24: /* ConfigureSceneSlotPreset2DE24 */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF90E,true)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_SCENE_CONTROLS+0x6D)<=4){if(!(hn_render_configure(m,15, 1, 0x72, 104, 0, 32, 0x088E5DA5)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DE6C: /* ConfigureSceneSlotPreset2DE6C */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF92A,true)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_SCENE_CONTROLS+0x6D)<=4){if(!(hn_render_configure(m,15, 1, 0x72, 104, 0, 32, 0x088E5DA5)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DEB4: /* ConfigureSceneSlotPreset2DEB4 */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BFAE2,true)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_SCENE_CONTROLS+0x6D)<=4){if(!(hn_render_configure(m,15, 1, 0x72, 104, 0, 32, 0x088E5DA5)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DEFC: /* ConfigureSceneSlotPreset2DEFC */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(hn_graphics_configure(m,4,0x088E1338)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF962,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0xB0, 0, 0, 0, 0x088BF997)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 0xB0, 0, 0, 0, 0x088BF9B6)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_SCENE_CONTROLS+0x6D)<=4){if(!(hn_render_configure(m,15, 1, 0x72, 104, 0, 32, 0x088E5DA5)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DF9C: /* ConfigureSceneSlotPreset2DF9C */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BFAFE,true)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_SCENE_CONTROLS+0x6D)<=4){if(!(hn_render_configure(m,15, 1, 0x72, 104, 0, 32, 0x088E5DA5)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802DFE4: /* ConfigureSceneSlotPreset2DFE4 */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF9D5,true)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_SCENE_CONTROLS+0x6D)<=4){if(!(hn_render_configure(m,15, 1, 0x72, 104, 0, 32, 0x088E5DA5)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802E02C: /* ConfigureSceneSlotPreset2E02C */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BFAB0,true)))return HN_SCENE_RESOURCE;
        if(hn_read8(m,HN_SCENE_CONTROLS+0x6D)<=4){if(!(hn_render_configure(m,15, 1, 0x72, 104, 0, 32, 0x088E5DA5)))return HN_SCENE_RESOURCE;}
        break;
    }
    case 0x0802E074: /* SceneSlotPresetNoop2E074 */ {

        break;
    }
    case 0x0802E078: /* ConfigureSceneSlotPreset2E078 */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,10, 0x088AF3F5,true)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,13, 0x088B6A36,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E09C: /* ConfigureSceneSlotPreset2E09C */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(hn_graphics_configure(m,4,0x088E1330)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088C3B4D,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 0xB0, 0, 0, 0, 0x088C3B69)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E0EC: /* ConfigureSceneSlotPreset2E0EC */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(field(m,15, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088C3AFF,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E10C: /* ConfigureSceneSlotPreset2E10C */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088C3ACA,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E124: /* ConfigureSceneSlotPreset2E124 */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088C2F27,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E13C: /* ConfigureSceneSlotPreset2E13C */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(hn_graphics_configure(m,4,0x088E1338)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF962,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 0xB0, 0, 0, 0, 0x088BF997)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 0xB0, 0, 0, 0, 0x088BF9B6)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E1B4: /* ConfigureSceneSlotPreset2E1B4 */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF90E,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E1CC: /* ConfigureSceneSlotPreset2E1CC */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BFAE2,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E1E4: /* ConfigureSceneSlotPreset2E1E4 */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF867,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E1FC: /* ConfigureSceneSlotPreset2E1FC */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BFAFE,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E214: /* ConfigureSceneSlotPreset2E214 */ {
        if(!(hn_bank2_apply(m,0x0802DDC4)==HN_SCENE_OK))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF9D5,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E3A4: /* EmptyMapBank2RenderCallback */ {

        break;
    }
    case 0x0802E3A8: /* SetMapBank2Slot19AnimationFB6B */ {
        if(!(select_animation(m,19, 0x088BFB6B,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E3BC: /* ConfigureMapBank2LinkedSlotsE3BC */ {
        if(!(hn_graphics_configure(m,4,0x088E1338)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BFB87,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 176, 0, 0, 0, 0x088BFBBC)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 176, 0, 0, 0, 0x088BFBDB)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E42C: /* SetMapBank2Slot19Animation01A0 */ {
        if(!(select_animation(m,19, 0x088C01A0,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E440: /* ConfigureMapBank2LinkedSlotE440 */ {
        if(!(select_animation(m,19, 0x088C04A6,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 176, 0, 0, 0, 0x088E3980)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E47C: /* ConfigureMapBank2LinkedSlotE47C */ {
        if(!(select_animation(m,19, 0x088C04BB,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 176, 0, 0, 0, 0x088E39DD)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E4B8: /* SetMapBank2Slot16AnimationEAC1 */ {
        if(!(select_animation(m,16, 0x088BEAC1,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E4CC: /* SetMapBank2Slot19AnimationFDB5 */ {
        if(!(select_animation(m,19, 0x088BFDB5,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E4E0: /* SetMapBank2Slot19AnimationFE1F */ {
        if(!(select_animation(m,19, 0x088BFE1F,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E4F4: /* SetMapBank2Slot13Animation6ADD */ {
        if(!(select_animation(m,13, 0x088B6ADD,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E508: /* SetMapBank2Slot13Animation6A9F */ {
        if(!(select_animation(m,13, 0x088B6A9F,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E51C: /* SetMapBank2Slot19AnimationFE54 */ {
        if(!(select_animation(m,19, 0x088BFE54,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E530: /* SetMapBank2Slot19AnimationFE70 */ {
        if(!(select_animation(m,19, 0x088BFE70,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E544: /* ConfigureMapBank2LinkedSlotsE544 */ {
        if(!(hn_graphics_configure(m,4,0x088E1338)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BF962,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 176, 0, 0, 0, 0x088BF997)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 176, 0, 0, 0, 0x088BF9B6)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E5B4: /* ClearMapBank2LinkedSlotsAndSetAnimationFAFE */ {
        if(!(field(m,17, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,18, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BFAFE,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E5D8: /* SetMapBank2Slot19AnimationFAE2 */ {
        if(!(select_animation(m,19, 0x088BFAE2,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E5EC: /* ConfigureMapBank2LinkedSlotsE5EC */ {
        if(!(select_animation(m,16, 0x088BEC8C,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 176, 0, 0, 0, 0x088BECA1)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 176, 0, 0, 0, 0x088BECBA)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E650: /* ConfigureMapBank2LinkedSlotE650 */ {
        if(!(field(m,18, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,20, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(hn_graphics_configure(m,4,0x088E1330)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088BFAB0,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 176, 0, 0, 0, 0x088BFACC)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E6AC: /* SetMapBank2Slot19AnimationFAB0 */ {
        if(!(select_animation(m,19, 0x088BFAB0,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E6C0: /* SetMapBank2Slot16IndicatorE6C0 */ {
        GameAddr resource = 0x088BE7F3;
        if(!(select_animation(m,16, resource,true)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088C01D5,true)))return HN_SCENE_RESOURCE;
        indicator(m,16, resource);
        break;
    }
    case 0x0802E6EC: /* SetMapBank2Slot16AnimationEAF6 */ {
        if(!(select_animation(m,16, 0x088BEAF6,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E700: /* ConfigureMapBank2LinkedGroupsE700 */ {
        GameAddr resource;
        if(!(hn_graphics_configure(m,4,0x088E1338)))return HN_SCENE_RESOURCE;
        if(!(hn_graphics_configure(m,5,0x088E13AB)))return HN_SCENE_RESOURCE;
        resource = 0x088BED36;
        if(!(select_animation(m,16, resource,true)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088C04C7,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 176, 0, 0, 0, 0x088BEE8E)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 176, 0, 0, 0, 0x088BF04F)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,9, 1, 176, 0, 0, 0, 0x088BF210)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,11, 1, 176, 0, 0, 0, 0x088BF2DA)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,9, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,11, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,18, 1, 176, 0, 0, 0, 0x088C0625)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,20, 1, 176, 0, 0, 0, 0x088C07E6)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,12, 1, 176, 0, 0, 0, 0x088C09A7)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,14, 1, 176, 0, 0, 0, 0x088C0A71)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,18, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,20, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,12, 2, 3, 19)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,14, 2, 3, 19)))return HN_SCENE_RESOURCE;
        indicator(m,16, resource);
        break;
    }
    case 0x0802E86C: /* ConfigureMapBank2LinkedSlotE86C */ {
        if(!(select_animation(m,16, 0x088BED12,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 176, 0, 0, 0, 0x088E39FC)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E8D0: /* SetMapBank2Slot16AnimationE5CE */ {
        if(!(select_animation(m,16, 0x088BE5CE,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E8E4: /* ClearMapBank2LinkedSlotsAndSetAnimationE5B2 */ {
        if(!(select_animation(m,16, 0x088BE5B2,true)))return HN_SCENE_RESOURCE;
        if(!(field(m,15, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,17, 0,0x60)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E908: /* ClearMapBank2LinkedSlotsAndSetAnimationE603 */ {
        if(!(select_animation(m,16, 0x088BE603,true)))return HN_SCENE_RESOURCE;
        if(!(field(m,15, 0,0x60)))return HN_SCENE_RESOURCE;
        if(!(field(m,17, 0,0x60)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E92C: /* SetMapBank2Slot16AnimationE603 */ {
        if(!(select_animation(m,16, 0x088BE603,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E940: /* ConfigureMapBank2LinkedSlotsWithPriorityE940 */ {
        if(!(select_animation(m,16, 0x088BE603,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 176, 0, 0, 0, 0x088BE61F)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(priority(m,15, 1)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 176, 0, 0, 0, 0x088BE62D)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(priority(m,17, 1)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E9B4: /* SetMapBank2Slot16AnimationE686 */ {
        if(!(select_animation(m,16, 0x088BE686,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E9C8: /* SetMapBank2Slot16AnimationDD7F */ {
        if(!(select_animation(m,16, 0x088BDD7F,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802E9DC: /* ConfigureMapBank2LinkedSlotsAndIndicatorE9DC */ {
        GameAddr resource = 0x088BE010;
        if(!(select_animation(m,16, resource,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 176, 0, 0, 0, 0x088E1D3B)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 176, 0, 0, 0, 0x088E1D61)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,resource);
        break;
    }
    case 0x0802EA54: /* ConfigureMapBank2LinkedSlotsEA54 */ {
        if(!(select_animation(m,16, 0x088BE010,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 176, 0, 0, 0, 0x088E1D87)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,17, 1, 176, 0, 0, 0, 0x088E1DAD)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,17, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802EB78: /* SetMapBank2Slot10IndicatorEB78 */ {
        GameAddr resource = 0x088A5B33;
        if(!(select_animation(m,10, resource,true)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,13, 0x088A9CAC,true)))return HN_SCENE_RESOURCE;
        indicator(m,10, resource);
        break;
    }
    case 0x0802EBA4: /* ConfigureMapBank2LinkedSlotEBA4 */ {
        if(!(hn_graphics_configure(m,4,0x088E1330)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D8292,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 114, 0, 0, 0, 0x088D82AE)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802EBF0: /* SetMapBank2Slot16IndicatorEBF0 */ {
        GameAddr resource = 0x088D85AB;
        if(!(select_animation(m,16, resource,true)))return HN_SCENE_RESOURCE;
        indicator(m,16, resource);
        break;
    }
    case 0x0802EC10: /* ConfigureMapBank2LinkedSlotEC10 */ {
        if(!(hn_graphics_configure(m,4,0x088E135C)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D8E5A,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 114, 0, 0, 0, 0x088D8CE3)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802EC5C: /* SetMapBank2Slot16Animation8AA0 */ {
        if(!(select_animation(m,16, 0x088D8AA0,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802EC70: /* SetMapBank2Slot16Animation839E */ {
        if(!(select_animation(m,16, 0x088D839E,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802EC84: /* SetMapBank2Slot16Animation90C5 */ {
        if(!(select_animation(m,16, 0x088D90C5,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802EC98: /* ConfigureMapBank2LinkedSlotEC98 */ {
        if(!(hn_graphics_configure(m,4,0x088E1330)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,16, 0x088D7EBD,true)))return HN_SCENE_RESOURCE;
        if(!(hn_render_configure(m,15, 1, 114, 0, 0, 0, 0x088D82AE)))return HN_SCENE_RESOURCE;
        if(!(hn_render_modes(m,15, 2, 3, 16)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802ECE4: /* SetMapBank2Slot16IndicatorECE4 */ {
        GameAddr resource = 0x088D8DA8;
        if(!(select_animation(m,16, resource,true)))return HN_SCENE_RESOURCE;
        indicator(m,16, resource);
        break;
    }
    case 0x0802ED04: /* SetMapBank2Slot16Animation8CCE */ {
        if(!(select_animation(m,16, 0x088D8CCE,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802ED18: /* SetMapBank2Slot19Animation4A23 */ {
        if(!(select_animation(m,19, 0x088D4A23,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802ED2C: /* SetMapBank2Slot19Animation519F */ {
        if(!(select_animation(m,19, 0x088D519F,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802ED40: /* SetMapBank2Slot16IndicatorED40 */ {
        GameAddr resource = 0x088D85BF;
        if(!(select_animation(m,16, resource,true)))return HN_SCENE_RESOURCE;
        indicator(m,16, resource);
        break;
    }
    case 0x0802ED60: /* SetMapBank2Slot19Animation4661 */ {
        if(!(select_animation(m,19, 0x088D4661,true)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802ED74: /* SetMapBank2Slot16IndicatorED74 */ {
        GameAddr resource = 0x088D8DA8;
        if(!(select_animation(m,16, resource,true)))return HN_SCENE_RESOURCE;
        indicator(m,16, resource);
        break;
    }
    case 0x0802ED94: /* SetMapBank2Slot16IndicatorED94 */ {
        GameAddr resource = 0x088D89DE;
        if(!(select_animation(m,16, resource,true)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,19, 0x088D562D,true)))return HN_SCENE_RESOURCE;
        if(!(select_animation(m,29, 0x088E65C9,true)))return HN_SCENE_RESOURCE;
        indicator(m,16, resource);
        break;
    }
    case 0x0802EDCC: /* SetMapBank2Slot13IndicatorEDCC */ {
        GameAddr resource = 0x088A8B25;
        if(!(select_animation(m,13, resource,true)))return HN_SCENE_RESOURCE;
        indicator(m,13, resource);
        break;
    }
    case 0x0802EDEC: /* SetMapBank2Slot13Animation4F17 */ {
        if(!(select_animation(m,13, 0x088B4F17,true)))return HN_SCENE_RESOURCE;
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
static bool main_contract(GameAddr callback)
{
    switch(callback&~1u){
    case 0x0802D000:
    case 0x0802D004:
    case 0x0802D030:
    case 0x0802D190:
    case 0x0802D2EC:
    case 0x0802D838:
    case 0x0802DAF4:
    case 0x0802DB30:
    case 0x0802DBBC:
    case 0x0802DC3C:
    case 0x0802DC60:
    case 0x0802DCB0:
    case 0x0802DD18:
    case 0x0802DD3C:
    case 0x0802DD80:
    case 0x0802DD84:
    case 0x0802DD9C:
    case 0x0802DDDC:
    case 0x0802DE24:
    case 0x0802DE6C:
    case 0x0802DEB4:
    case 0x0802DEFC:
    case 0x0802DF9C:
    case 0x0802DFE4:
    case 0x0802E02C:
    case 0x0802E074:
    case 0x0802E078:
    case 0x0802E09C:
    case 0x0802E0EC:
    case 0x0802E10C:
    case 0x0802E124:
    case 0x0802E13C:
    case 0x0802E1B4:
    case 0x0802E1CC:
    case 0x0802E1E4:
    case 0x0802E1FC:
    case 0x0802E214:
    case 0x0802E8A8:
    case 0x0802E8D0:
    case 0x0802E8E4:
    case 0x0802E908:
    case 0x0802E92C:
    case 0x0802E940:
    case 0x0802E9B4:
    case 0x0802E9C8:
    case 0x0802E9DC:
    case 0x0802EA54:
    case 0x0802EAB8:
    case 0x0802EB5C:
    case 0x0802EB78:
    case 0x0802EBA4:
    case 0x0802EBF0:
    case 0x0802EC10:
    case 0x0802EC5C:
    case 0x0802EC70:
    case 0x0802EC84:
    case 0x0802EC98:
    case 0x0802ECE4:
    case 0x0802ED04:
    case 0x0802ED18:
    case 0x0802ED2C:
    case 0x0802ED40:
    case 0x0802ED60:
    case 0x0802ED74:
    case 0x0802ED94:
    case 0x0802EDCC:
    case 0x0802EDEC:
    case 0x0802EE00:
    case 0x0804B360:case 0x0804B3DC:return true;
    default:return false;
    }
}

static bool update_motion(HNMemory *m)
{
    GameAddr counter=HN_SCENE_CONTROLS+0x60;
    uint8_t count=hn_read8(m,counter);
    if(!count){
        for(unsigned i=0;i<=12;i++){
            uint8_t kind=(uint8_t)hn_random_range(m,0,3);
            if(!(i&1))kind=(uint8_t)(kind+4);
            GameAddr slot=hn_read32(m,HN_RENDER_SLOTS)+(i+22)*104;
            if(hn_read8(m,slot+0x60)){
                hn_write32(m,slot+0x28,hn_read32(m,0x085F4294+kind*8));
                hn_write32(m,slot+0x34,hn_read32(m,0x085F4298+kind*8));
            }
        }
        hn_write8(m,counter,(uint8_t)(hn_read8(m,counter)+1));
    }else hn_write8(m,counter,count==20?0:(uint8_t)(count+1));
    if(!(hn_read16(m,0x0201DC84)&31)){
        for(unsigned i=0;i<=12;i++){
            uint8_t kind=(uint8_t)((hn_random_range(m,0,31)&hn_read8(m,0x085F42D4+i))+hn_read8(m,0x085F42E1+i));
            GameAddr slot=hn_read32(m,HN_RENDER_SLOTS)+(i+22)*104;
            if(hn_read8(m,slot+0x60)){
                hn_write32(m,slot+0x24,hn_read32(m,0x085F4194+kind*8));
                hn_write32(m,slot+0x30,hn_read32(m,0x085F4198+kind*8));
            }
        }
    }
    for(unsigned i=0;i<=12;i++){
        GameAddr slot=hn_read32(m,HN_RENDER_SLOTS)+(i+22)*104;
        if(hn_read8(m,slot+0x60)){
            if((int32_t)hn_read32(m,slot+0x24)>0x3FFFF)hn_write32(m,slot+0x24,0x4000);
            if((int32_t)hn_read32(m,slot+0x24)<=-0x40000)hn_write32(m,slot+0x24,0xFFFFC000);
        }
    }
    hn_write16(m,0x0201DC84,(uint16_t)(hn_read16(m,0x0201DC84)+1));return !m->fault;
}
static bool hide_raised(HNMemory *m)
{
    GameAddr slot=hn_read32(m,HN_RENDER_SLOTS)+23*104;
    for(unsigned i=23;i<=27;i++,slot+=104)
        if((int32_t)(hn_read32(m,slot+0x1C)+hn_read32(m,slot+0x20))>-0x180000)return !m->fault;
    if(hn_read8(m,HN_SCENE_CONTROLS+0x1D3)>1&&
       !select_animation(m,hn_read8(m,0x03002C04),0x088CE7CA,true))return false;
    for(unsigned i=23;i<=27;i++)if(!field(m,(uint8_t)i,0,0x60))return false;
    return !m->fault;
}
HNSceneResult hn_bank2_apply(HNMemory *m,GameAddr callback)
{
    bool ok=true;
    switch(callback&~1u){
    case 0x0802D004:
        hn_write16(m,0x03001FBC,0);hn_input_script_mask(m,1);
        if(hn_read16(m,0x0201DC40))hn_write16(m,0x03001FBC,1);break;
    case 0x0802D030:ok=update_motion(m);break;
    case 0x0802D2EC:ok=hn_motion_restore_actors(m);break;
    case 0x0802DD9C:{
        GameAddr value=0x0201DC4C;hn_write16(m,value,(uint16_t)(hn_read16(m,value)*20));
        if(hn_read16(m,value)>999)hn_write16(m,value,999);break;
    }
    case 0x0802EAB8:{
        uint8_t direction=(uint8_t)hn_render_direction_between(m,10,16,false);
        if(direction==1)ok=select_animation(m,10,0x088AC0C6,false);
        else if(direction==3)ok=select_animation(m,10,0x088AC0FD,false);
        direction=(uint8_t)hn_render_direction_between(m,13,16,false);
        if(direction==1)ok=ok&&select_animation(m,13,0x088B4055,false);
        else if(direction==3)ok=ok&&select_animation(m,13,0x088B408C,false);
        direction=(uint8_t)hn_render_direction_between(m,16,10,false);
        if(direction==2)ok=ok&&select_animation(m,16,0x088BDD7F,false);
        else if(direction==4)ok=ok&&select_animation(m,16,0x088BDDB7,false);break;
    }
    case 0x0802EB5C:
        hn_write16(m,HN_VIDEO_STATE+0x9C,hn_read16(m,HN_VIDEO_STATE+0x3C));
        hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|16);break;
    case 0x0802EE00:{
        uint8_t direction=(uint8_t)hn_render_direction_between(m,13,10,false);
        switch(direction){
        case 1:ok=select_animation(m,13,0x088B4055,true);break;
        case 2:ok=select_animation(m,13,0x088B4002,true);break;
        case 3:ok=select_animation(m,13,0x088B408C,true);break;
        case 4:ok=select_animation(m,13,0x088B40C3,true);break;
        default:break;
        }break;
    }
    case 0x0804B360:ok=hide_raised(m);break;
    case 0x0804B3DC:{
        uint8_t mode=hn_read8(m,HN_SCENE_CONTROLS+0x3A),value;
        if(mode==1||mode==3)value=2;
        else value=mode==5&&!flag(m,0x16F)?3:1;
        hn_write8(m,HN_SCENE_CONTROLS+0x1D3,value);break;
    }
    default:return simple_apply(m,callback);
    }
    return ok&&!m->fault?HN_SCENE_OK:HN_SCENE_RESOURCE;
}
static bool panels(HNMemory *m,GameAddr arguments)
{
    uint8_t base=hn_read8(m,arguments);uint16_t x=hn_read16(m,arguments+1),z=hn_read16(m,arguments+3);
    GameAddr stage=0x0201DF8C;
    /* Each stage reads the live class value again after render writes. */
    switch(hn_read16(m,stage)){
    case 0:
        if(!hn_render_configure(m,base,1,0x72,(int16_t)(x-16),-160,(int16_t)z,0x088E58E9)||
           !hn_render_configure(m,(uint8_t)(base+1),1,0x72,(int16_t)x,-160,(int16_t)z,0x088E59EE)||
           !hn_render_configure(m,(uint8_t)(base+2),1,0x72,(int16_t)(x+96),-160,(int16_t)z,0x088E5B4B)||
           !hn_render_configure(m,(uint8_t)(base+3),1,0x72,(int16_t)(x+72),-160,(int16_t)z,0x088E594B))return false;
        hn_write16(m,stage,(uint16_t)(hn_read16(m,stage)+1));break;
    case 1:
        if(!hn_render_configure(m,(uint8_t)(base+4),1,0x72,(int16_t)(x-24),-160,(int16_t)z,0x088E5AF3)||
           !hn_render_configure(m,(uint8_t)(base+5),1,0x72,(int16_t)(x-8),-160,(int16_t)z,0x088E594B)||
           !hn_render_configure(m,(uint8_t)(base+6),1,0x72,(int16_t)(x+40),-160,(int16_t)z,0x088E59EE)||
           !hn_render_configure(m,(uint8_t)(base+7),1,0x72,(int16_t)(x+48),-160,(int16_t)z,0x088E58E4))return false;
        hn_write16(m,stage,(uint16_t)(hn_read16(m,stage)+1));break;
    case 2:
        if(!hn_render_configure(m,base,1,0x72,(int16_t)(x-16),-160,(int16_t)z,0x088E59EE)||
           !hn_render_configure(m,(uint8_t)(base+1),1,0x72,(int16_t)x,-160,(int16_t)z,0x088E58E9)||
           !hn_render_configure(m,(uint8_t)(base+2),1,0x72,(int16_t)(x+96),-160,(int16_t)z,0x088E594B)||
           !hn_render_configure(m,(uint8_t)(base+3),1,0x72,(int16_t)(x+72),-160,(int16_t)z,0x088E5B4B))return false;
        hn_write16(m,stage,(uint16_t)(hn_read16(m,stage)+1));break;
    case 3:
        if(!hn_render_configure(m,(uint8_t)(base+4),1,0x72,(int16_t)(x-8),-160,(int16_t)z,0x088E5A4B)||
           !hn_render_configure(m,(uint8_t)(base+5),1,0x72,(int16_t)x,-160,(int16_t)z,0x088E58E4)||
           !hn_render_configure(m,(uint8_t)(base+6),1,0x72,(int16_t)(x+56),-160,(int16_t)z,0x088E594B)||
           !hn_render_configure(m,(uint8_t)(base+7),1,0x72,(int16_t)(x+88),-160,(int16_t)z,0x088E5AEE))return false;
        hn_write16(m,stage,0);break;
    default:break;
    }return !m->fault;
}
HNSceneResult hn_bank2_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    if(!main_contract(callback))return HN_SCENE_UNKNOWN;
    *consumed=0;
    switch(callback&~1u){
    case 0x0802D190:
        if(!hn_motion_scene_actor(m,arguments))return HN_SCENE_RESOURCE;*consumed=1;break;
    case 0x0802D838:
        if(!panels(m,arguments))return HN_SCENE_RESOURCE;*consumed=5;break;
    case 0x0802E8A8:{
        unsigned source=hn_read8(m,arguments),destination=hn_read8(m,arguments+1);
        if(!valid(m,source)||!valid(m,destination))return HN_SCENE_RESOURCE;
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS);
        /* Original memcpy copies forward; do not apply render-setup side effects. */
        for(unsigned offset=0;offset<104;offset++)hn_write8(m,base+destination*104+offset,hn_read8(m,base+source*104+offset));
        *consumed=2;break;
    }
    default:return hn_bank2_apply(m,callback);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank2_animation(HNMemory *m,GameAddr callback,uint32_t *consumed,uint32_t *yield)
{
    if((callback&~1u)!=0x0804B43C)return HN_SCENE_UNKNOWN;
    uint8_t index=hn_read8(m,0x03002C04);
    if(!valid(m,index))return HN_SCENE_RESOURCE;
    if((int32_t)hn_read32(m,hn_render_slot(m,index)+0x20)<-0x280000)hn_write8(m,HN_SCENE_CONTROLS+0x60,1);
    *consumed=0;*yield=1;return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
