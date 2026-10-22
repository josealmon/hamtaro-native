#include "game/bank4.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/background.h"
#include "game/presets.h"
#include "game/audio.h"

static bool raw_animation(HNMemory *m,uint32_t index,GameAddr image)
{hn_render_set_animation(m,index,image);return !m->fault;}
static bool animation(HNMemory *m,uint8_t index,GameAddr image)
{if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);return raw_animation(m,index,image);}
static bool full_field(HNMemory *m,uint32_t index,uint32_t value)
{hn_write8(m,hn_render_slot(m,index)+0x60,(uint8_t)value);return !m->fault;}
static bool field(HNMemory *m,uint8_t index,uint8_t value){return full_field(m,index,value);}
static bool field_animation(HNMemory *m,uint8_t index,uint8_t value,GameAddr image)
{if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);return field(m,index,value)&&raw_animation(m,index,image);}
static void indicator(HNMemory *m,uint8_t index,GameAddr image)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,image);}
static void priority(HNMemory *m,uint8_t index,uint8_t value){hn_render_flags(m,index,12,(uint8_t)(value<<2));}
static void cell_position(HNMemory *m,uint32_t index,int32_t x,int32_t y,int32_t z)
{GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x18,((uint32_t)(uint16_t)x<<16)+0x8000);hn_write32(m,slot+0x1C,((uint32_t)(uint16_t)y<<16)+0x8000);hn_write32(m,slot+0x20,((uint32_t)(uint16_t)z<<16)+0x8000);}
static void set_flag(HNMemory *m,uint32_t id,uint32_t value)
{GameAddr address=0x03002180+(id>>3);uint8_t bit=(uint8_t)(1u<<(id&7));hn_write8(m,address,(uint8_t)((hn_read8(m,address)&~bit)|(value?bit:0)));}
bool hn_bank4_clear_neighbors(HNMemory *m,uint32_t index_word)
{uint8_t index=(uint8_t)index_word;return field(m,(uint8_t)(index-1),0)&&field(m,(uint8_t)(index+1),0);}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
/* Source-shaped presets; resource addresses refer to the player's private ROM. */
HNSceneResult hn_bank4_presets(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080678C0:{ /* ClearMapBank4Slots9And11Field60 */
        REQUIRE(field(m,9, 0));
        REQUIRE(field(m,11, 0));
        break;
    }
    case 0x080678D8:{ /* ClearMapBank4Slots12And14Field60 */
        REQUIRE(field(m,12, 0));
        REQUIRE(field(m,14, 0));
        break;
    }
    case 0x080678F0:{ /* ClearMapBank4Slots15And17Field60 */
        REQUIRE(field(m,15, 0));
        REQUIRE(field(m,17, 0));
        break;
    }
    case 0x08067908:{ /* ClearMapBank4Slots18And20Field60 */
        REQUIRE(field(m,18, 0));
        REQUIRE(field(m,20, 0));
        break;
    }
    case 0x08067920:{ /* ClearMapBank4Slots28And30Field60 */
        REQUIRE(field(m,28, 0));
        REQUIRE(field(m,30, 0));
        break;
    }
    case 0x08067938:{ /* ApplyMapBank4Preset67938 */
        REQUIRE(hn_bank4_presets(m,0x080678C0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x080678D8)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,10, 0x088A6CCE));
        REQUIRE(animation(m,13, 0x088AB38D));
        REQUIRE(hn_render_configure_raw(m,9, 1, 176, 0, 0, 0, 0x088E2FEF));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088E3025));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088E304A));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088E306F));
        REQUIRE(hn_render_modes_raw(m,9, 2, 3, 10));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        indicator(m,10, 0x088A6CCE);
        break;
    }
    case 0x08067A10:{ /* ApplyMapBank4Preset67A10 */
        REQUIRE(hn_bank4_presets(m,0x080678C0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x080678D8)==HN_SCENE_OK);
        REQUIRE(animation(m,10, 0x088A5D14));
        REQUIRE(animation(m,13, 0x088A9E8D));
        indicator(m,10, 0x088A5D14);
        break;
    }
    case 0x08067A44:{ /* ApplyMapBank4Preset67A44 */
        REQUIRE(hn_bank4_presets(m,0x080678C0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x080678D8)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E13AB));
        REQUIRE(hn_graphics_initialize_raw(m,5, 1, 0x088E13E3));
        REQUIRE(hn_graphics_initialize_raw(m,6, 1, 0x088E13EB));
        REQUIRE(animation(m,10, 0x088A6BE3));
        REQUIRE(animation(m,13, 0x088AB103));
        REQUIRE(hn_render_configure_raw(m,9, 1, 176, 0, 0, 0, 0x088E2D51));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088E2E0A));
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088E2DEC));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088E2E91));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088E2F40));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088E2F2C));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088E2FDB));
        REQUIRE(hn_render_modes_raw(m,9, 2, 3, 10));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, 10));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 13));
        indicator(m,10, 0x088A6BE3);
        break;
    }
    case 0x08067BA4:{ /* ApplyMapBank4Preset67BA4 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 8) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        REQUIRE(animation(m,index, 0x088D3501));
        break;
    }
    case 0x08067BD8:{ /* ApplyMapBank4Preset67BD8 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 8) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        REQUIRE(animation(m,index, 0x088D3516));
        break;
    }
    case 0x08067C0C:{ /* ApplyMapBank4Preset67C0C */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,16, 0x088D38B6));
        REQUIRE(animation(m,19, 0x088D39B5));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E2FEF));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088E3025));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088E304A));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088E306F));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        indicator(m,16, 0x088D38B6);
        break;
    }
    case 0x08067CE4:{ /* ApplyMapBank4Preset67CE4 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D33D4));
        break;
    }
    case 0x08067CFC:{ /* ApplyMapBank4Preset67CFC */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,16, 0x088D325B));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D3412));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x08067D4C:{ /* ApplyMapBank4Preset67D4C */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D3491));
        break;
    }
    case 0x08067D64:{ /* ApplyMapBank4Preset67D64 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D347C));
        break;
    }
    case 0x08067D7C:{ /* ApplyMapBank4Preset67D7C */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,16, 0x088D327F));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D3412));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x08067DCC:{ /* ApplyMapBank4Preset67DCC */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 8 || hn_read8(m,HN_SCENE_CONTROLS+0x82) == 10) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        REQUIRE(animation(m,index, 0x088D3429));
        break;
    }
    case 0x08067E04:{ /* ApplyMapBank4Preset67E04 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 8) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        REQUIRE(animation(m,index, 0x088D325B));
        break;
    }
    case 0x08067E38:{ /* ApplyMapBank4Preset67E38 */
        uint8_t index;
        uint32_t offset;
        GameAddr animations;
        GameAddr animation_word;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 10) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        animations = 0x0200165C;
        offset = (uint8_t)hn_render_direction_raw(m,index, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,index, animation_word));
        {
            uint8_t auxiliary = index - 1;
            REQUIRE(hn_render_configure_raw(m,auxiliary, 1, 176, 0, 0, 0, 0x088D3608));
            REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, index));
        }
        break;
    }
    case 0x08067EBC:{ /* ApplyMapBank4Preset67EBC */
        uint8_t index;
        uint32_t offset;
        GameAddr animations;
        GameAddr animation_word;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 10) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        animations = 0x0200166C;
        offset = (uint8_t)hn_render_direction_raw(m,index, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,index, animation_word));
        break;
    }
    case 0x08067F00:{ /* ApplyMapBank4Preset67F00 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D36C3));
        indicator(m,19, 0x088D36C3);
        break;
    }
    case 0x08067F24:{ /* ApplyMapBank4Preset67F24 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D371D));
        indicator(m,19, 0x088D371D);
        break;
    }
    case 0x08067F48:{ /* ApplyMapBank4Preset67F48 */
        uint8_t index;
        uint32_t offset;
        GameAddr animations;
        GameAddr animation_word;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 8) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        animations = 0x0200167C;
        offset = (uint8_t)hn_render_direction_raw(m,index, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,index, animation_word));
        break;
    }
    case 0x08067F8C:{ /* ApplyMapBank4Preset67F8C */
        uint8_t index;
        uint32_t offset;
        GameAddr animations;
        GameAddr animation_word;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 10) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        animations = 0x0200165C;
        offset = (uint8_t)hn_render_direction_raw(m,index, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,index, animation_word));
        break;
    }
    case 0x08067FD0:{ /* ApplyMapBank4Preset67FD0 */
        GameAddr animations;
        uint32_t offset;
        GameAddr animation_word;
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        animations = 0x0200165C;
        offset = (uint8_t)hn_render_direction_raw(m,16, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,16, animation_word));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D3412));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x08068034:{ /* ApplyMapBank4Preset68034 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D34A6));
        break;
    }
    case 0x0806804C:{ /* ApplyMapBank4Preset6804C */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 8) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        REQUIRE(animation(m,index, 0x088D352B));
        break;
    }
    case 0x08068080:{ /* ApplyMapBank4Preset68080 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,16, 0x088D356D));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D3584));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080680D0:{ /* ApplyMapBank4Preset680D0 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D3655));
        break;
    }
    case 0x080680E8:{ /* ApplyMapBank4Preset680E8 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D35B6));
        break;
    }
    case 0x08068100:{ /* ApplyMapBank4Preset68100 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D37C4));
        indicator(m,16, 0x088D37C4);
        break;
    }
    case 0x08068124:{ /* ApplyMapBank4Preset68124 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 8 || hn_read8(m,HN_SCENE_CONTROLS+0x82) == 10) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        REQUIRE(animation(m,index, 0x088D3555));
        break;
    }
    case 0x0806815C:{ /* ApplyMapBank4Preset6815C */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x82) == 8) {
            index = 19;
            REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        } else {
            index = 16;
            REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        }
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,index, 0x088D3617));
        break;
    }
    case 0x08068194:{ /* ApplyMapBank4Preset68194 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088CDAAA));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDAB6));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080681D8:{ /* ApplyMapBank4Preset681D8 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,19, 0x088D35FC));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088D3608));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08068228:{ /* ApplyMapBank4Preset68228 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D34F5));
        break;
    }
    case 0x08068240:{ /* ApplyMapBank4Preset68240 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088CDB3B));
        break;
    }
    case 0x08068258:{ /* ApplyMapBank4Preset68258 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E13AB));
        REQUIRE(animation(m,16, 0x088CDB50));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDB6B));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(animation(m,19, 0x088D3273));
        break;
    }
    case 0x080682B8:{ /* ApplyMapBank4Preset682B8 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088CC776));
        REQUIRE(animation(m,19, 0x088D34A6));
        break;
    }
    case 0x080682E0:{ /* ApplyMapBank4Preset682E0 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D3470));
        break;
    }
    case 0x080682F8:{ /* ApplyMapBank4Preset682F8 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088CDB82));
        indicator(m,16, 0x088CDB82);
        break;
    }
    case 0x0806831C:{ /* ApplyMapBank4Preset6831C */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088CDABE));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDACA));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088CDAD2));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x08068384:{ /* ApplyMapBank4Preset68384 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D3273));
        break;
    }
    case 0x0806839C:{ /* ApplyMapBank4Preset6839C */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,16, 0x088DB8CA));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DB8D6));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080683EC:{ /* ApplyMapBank4Preset683EC */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088DB5DC));
        break;
    }
    case 0x08068404:{ /* ApplyMapBank4Preset68404 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088DB8E1));
        break;
    }
    case 0x0806841C:{ /* ApplyMapBank4Preset6841C */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E138B));
        REQUIRE(animation(m,16, 0x088DB93D));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DB95A));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806846C:{ /* ApplyMapBank4Preset6846C */
        REQUIRE(hn_bank4_presets(m,0x08067920)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,29, 0x088EBA82));
        REQUIRE(hn_render_configure_raw(m,28, 1, 176, 0, 0, 0, 0x088EBA8A));
        REQUIRE(hn_render_modes_raw(m,28, 2, 3, 29));
        break;
    }
    case 0x080684BC:{ /* ApplyMapBank4Preset684BC */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,16, 0x088DB97B));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DB987));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806850C:{ /* ApplyMapBank4Preset6850C */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088DB99E));
        break;
    }
    case 0x08068524:{ /* ApplyMapBank4Preset68524 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,16, 0x088DB97B));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DB987));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(animation(m,19, 0x088D3623));
        break;
    }
    case 0x08068584:{ /* ApplyMapBank4Preset68584 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088DC6A1));
        REQUIRE(animation(m,19, 0x088D325B));
        indicator(m,16, 0x088DC6A1);
        break;
    }
    case 0x080685B8:{ /* ApplyMapBank4Preset685B8 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088DB9DB));
        break;
    }
    case 0x080685D0:{ /* ApplyMapBank4Preset685D0 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D3655));
        break;
    }
    case 0x080685E8:{ /* ApplyMapBank4Preset685E8 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D366A));
        break;
    }
    case 0x08068600:{ /* ApplyMapBank4Preset68600 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088CAB5E));
        REQUIRE(animation(m,19, 0x088D3273));
        break;
    }
    case 0x08068628:{ /* ApplyMapBank4Preset68628 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1338));
        REQUIRE(animation(m,16, 0x088CDBC7));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDBDC));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088CDBFB));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x080686A0:{ /* ApplyMapBank4Preset686A0 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1338));
        REQUIRE(animation(m,16, 0x088CDC2F));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDC3B));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088CDC4C));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x08068718:{ /* ApplyMapBank4Preset68718 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(animation(m,16, 0x088CDC5D));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDC69));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x08068768:{ /* ApplyMapBank4Preset68768 */
        GameAddr animations;
        uint32_t offset;
        GameAddr animation_word;
        REQUIRE(hn_bank4_presets(m,0x08067920)==HN_SCENE_OK);
        animations = 0x0200168C;
        offset = (uint8_t)hn_render_direction_raw(m,29, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,29, animation_word));
        break;
    }
    case 0x08068794:{ /* ApplyMapBank4Preset68794 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E13AB));
        REQUIRE(hn_graphics_initialize_raw(m,5, 1, 0x088E13E3));
        REQUIRE(hn_graphics_initialize_raw(m,6, 1, 0x088E13EB));
        REQUIRE(animation(m,16, 0x088CDC89));
        REQUIRE(animation(m,19, 0x088D3802));
        cell_position(m,19, 128, 0, 72);
        REQUIRE(hn_render_configure_raw(m,9, 1, 176, 0, 0, 0, 0x088E2D51));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088E2E0A));
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088E2DEC));
        REQUIRE(hn_render_configure_raw(m,24, 1, 176, 0, 0, 0, 0x088CDD09));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088E2E91));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088E2F40));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088E2F2C));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088E2FDB));
        REQUIRE(hn_render_modes_raw(m,9, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,24, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 19));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 19));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        indicator(m,16, 0x088CDC89);
        break;
    }
    case 0x08068924:{ /* ApplyMapBank4Preset68924 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D3561));
        break;
    }
    case 0x0806893C:{ /* ApplyMapBank4Preset6893C */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        cell_position(m,19, 128, 0, 72);
        REQUIRE(animation(m,19, 0x088D3795));
        break;
    }
    case 0x08068960:{ /* ApplyMapBank4Preset68960 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D37AF));
        break;
    }
    case 0x08068978:{ /* ApplyMapBank4Preset68978 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D33D4));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088D33FB));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x080689BC:{ /* ApplyMapBank4Preset689BC */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E13AB));
        REQUIRE(animation(m,16, 0x088CDC7D));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDB6B));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(animation(m,19, 0x088D3742));
        indicator(m,19, 0x088D3742);
        break;
    }
    case 0x08068A28:{ /* ApplyMapBank4Preset68A28 */
        REQUIRE(hn_bank4_presets(m,0x08067908)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088D3742));
        indicator(m,19, 0x088D3742);
        break;
    }
    case 0x08068A4C:{ /* ApplyMapBank4Preset68A4C */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088CDC1A));
        break;
    }
    case 0x08068A64:{ /* ApplyMapBank4Preset68A64 */
        REQUIRE(hn_bank4_presets(m,0x080678D8)==HN_SCENE_OK);
        REQUIRE(animation(m,13, 0x088B6A9F));
        REQUIRE(field(m,13, 2));
        break;
    }
    case 0x08068A84:{ /* ApplyMapBank4Preset68A84 */
        REQUIRE(hn_bank4_presets(m,0x08067920)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(animation(m,29, 0x088EBA63));
        REQUIRE(hn_render_configure_raw(m,28, 1, 176, 0, 0, 0, 0x088EBA70));
        REQUIRE(hn_render_modes_raw(m,28, 2, 3, 29));
        break;
    }
    case 0x08068AD4:{ /* ApplyMapBank4Preset68AD4 */
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088D328B));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08068B08:{ /* ApplyMapBank4Preset68B08 */
        REQUIRE(hn_bank4_presets(m,0x080678F0)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D327F));
        break;
    }
    case 0x08068F60:{ /* ApplyMapBank4Preset68F60 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBA13));
        break;
    }
    case 0x08068F7C:{ /* ApplyMapBank4Preset68F7C */
        GameAddr animations;
        uint32_t offset;
        GameAddr animation_word;
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        animations = 0x0200169C;
        offset = (uint8_t)hn_render_direction_raw(m,19, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,19, animation_word));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DBD93));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08068FE4:{ /* ApplyMapBank4Preset68FE4 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBA28));
        break;
    }
    case 0x08069000:{ /* ApplyMapBank4Preset69000 */
        GameAddr animations;
        uint32_t offset;
        GameAddr animation_word;
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        animations = 0x0200169C;
        offset = (uint8_t)hn_render_direction_raw(m,19, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,19, animation_word));
        break;
    }
    case 0x08069030:{ /* ApplyMapBank4Preset69030 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBA52));
        break;
    }
    case 0x0806904C:{ /* ApplyMapBank4Preset6904C */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DB93D));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DB95A));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08069090:{ /* ApplyMapBank4Preset69090 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,19, 0x088DBD87));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DBD93));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x080690E4:{ /* ApplyMapBank4Preset690E4 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E138B));
        REQUIRE(animation(m,19, 0x088DB8E1));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DB8F6));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08069138:{ /* ApplyMapBank4Preset69138 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBA72));
        break;
    }
    case 0x08069154:{ /* ApplyMapBank4Preset69154 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBA8B));
        break;
    }
    case 0x08069170:{ /* ApplyMapBank4Preset69170 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DB9DB));
        break;
    }
    case 0x0806918C:{ /* ApplyMapBank4Preset6918C */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,19, 0x088DB8CA));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DB8D6));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x080691E0:{ /* ApplyMapBank4Preset691E0 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBAA0));
        break;
    }
    case 0x080691FC:{ /* ApplyMapBank4Preset691FC */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBAD8));
        break;
    }
    case 0x08069218:{ /* ApplyMapBank4Preset69218 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,19, 0x088DBAE4));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DBAF0));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x0806926C:{ /* ApplyMapBank4Preset6926C */
        REQUIRE(animation(m,10, 0x088AD641));
        REQUIRE(animation(m,13, 0x088B598D));
        REQUIRE(field(m,10, 2));
        REQUIRE(field(m,13, 2));
        break;
    }
    case 0x0806929C:{ /* ApplyMapBank4Preset6929C */
        break;
    }
    case 0x080692A0:{ /* ApplyMapBank4Preset692A0 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,19, 0x088DB59A));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DBD93));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x080692F4:{ /* ApplyMapBank4Preset692F4 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(animation(m,19, 0x088DBB13));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DBB28));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08069348:{ /* ApplyMapBank4Preset69348 */
        break;
    }
    case 0x0806934C:{ /* ApplyMapBank4Preset6934C */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(animation(m,19, 0x088DB59A));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DB5B1));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x080693A0:{ /* ApplyMapBank4Preset693A0 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBB3F));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x080693C8:{ /* ApplyMapBank4Preset693C8 */
        break;
    }
    case 0x080693CC:{ /* ApplyMapBank4Preset693CC */
        break;
    }
    case 0x080693D0:{ /* ApplyMapBank4Preset693D0 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBB4B));
        break;
    }
    case 0x080693EC:{ /* ApplyMapBank4Preset693EC */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_bank4_clear_neighbors(m,22));
        REQUIRE(hn_bank4_clear_neighbors(m,25));
        REQUIRE(animation(m,16, 0x088CDD82));
        REQUIRE(animation(m,22, 0x088CA5F9));
        REQUIRE(animation(m,25, 0x088CA5F9));
        break;
    }
    case 0x0806942C:{ /* ApplyMapBank4Preset6942C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_bank4_clear_neighbors(m,22));
        REQUIRE(hn_bank4_clear_neighbors(m,25));
        REQUIRE(animation(m,16, 0x088CE88F));
        REQUIRE(animation(m,22, 0x088CE88F));
        REQUIRE(animation(m,25, 0x088CE88F));
        break;
    }
    case 0x08069468:{ /* ApplyMapBank4Preset69468 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 1) {
            index = 16;
            REQUIRE(hn_bank4_clear_neighbors(m,16));
        } else if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 2) {
            index = 22;
            REQUIRE(hn_bank4_clear_neighbors(m,22));
        } else {
            index = 25;
            REQUIRE(hn_bank4_clear_neighbors(m,25));
        }
        switch ((uint8_t)hn_render_direction_raw(m,index, 10,false)) {
        case 1: REQUIRE(animation(m,index, 0x088CDD96)); break;
        case 2: REQUIRE(animation(m,index, 0x088CDD96)); break;
        case 3: REQUIRE(animation(m,index, 0x088CDDA6)); break;
        case 4: REQUIRE(animation(m,index, 0x088CDD9E)); break;
        }
        break;
    }
    case 0x080694F8:{ /* ApplyMapBank4Preset694F8 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 1) {
            index = 16;
            REQUIRE(hn_bank4_clear_neighbors(m,16));
        } else if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 2) {
            index = 22;
            REQUIRE(hn_bank4_clear_neighbors(m,22));
        } else {
            index = 25;
            REQUIRE(hn_bank4_clear_neighbors(m,25));
        }
        switch ((uint8_t)hn_render_direction_raw(m,index, 10,false)) {
        case 1: REQUIRE(animation(m,index, 0x088CDDBA)); break;
        case 2: REQUIRE(animation(m,index, 0x088CDDBA)); break;
        case 3: REQUIRE(animation(m,index, 0x088CDDCA)); break;
        case 4: REQUIRE(animation(m,index, 0x088CDDC2)); break;
        }
        break;
    }
    case 0x08069588:{ /* ApplyMapBank4Preset69588 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 1) {
            index = 16;
            REQUIRE(hn_bank4_clear_neighbors(m,16));
        } else if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 2) {
            index = 22;
            REQUIRE(hn_bank4_clear_neighbors(m,22));
        } else {
            index = 25;
            REQUIRE(hn_bank4_clear_neighbors(m,25));
        }
        switch ((uint8_t)hn_render_direction_raw(m,index, 10,false)) {
        case 1: REQUIRE(animation(m,index, 0x088CDDDE)); break;
        case 2: REQUIRE(animation(m,index, 0x088CDDDE)); break;
        case 3: REQUIRE(animation(m,index, 0x088CDDEE)); break;
        case 4: REQUIRE(animation(m,index, 0x088CDDE6)); break;
        }
        break;
    }
    case 0x08069618:{ /* ApplyMapBank4Preset69618 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,16, 0x088CAC9C));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CB755));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806966C:{ /* ApplyMapBank4Preset6966C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088CB98F));
        switch (hn_read8(m,HN_SCENE_CONTROLS+0x1A0)) {
        case 1: REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDE66)); break;
        case 2: REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDE59)); break;
        case 4: REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CDE4C)); break;
        }
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x08069700:{ /* ApplyMapBank4Preset69700 */
        REQUIRE(hn_bank4_clear_neighbors(m,22));
        REQUIRE(hn_bank4_clear_neighbors(m,25));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        switch (hn_read8(m,HN_SCENE_CONTROLS+0x1A1)) {
        case 1:
            REQUIRE(animation(m,22, 0x088CDE7B));
            REQUIRE(animation(m,25, 0x088CDE7B));
            break;
        case 2:
            REQUIRE(animation(m,22, 0x088CDE73));
            REQUIRE(animation(m,25, 0x088CDE7B));
            break;
        case 3:
            REQUIRE(animation(m,22, 0x088CDE73));
            REQUIRE(animation(m,25, 0x088CDE73));
            break;
        }
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088CB755));
        REQUIRE(hn_render_configure_raw(m,24, 1, 176, 0, 0, 0, 0x088CB755));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        REQUIRE(hn_render_modes_raw(m,24, 2, 3, 25));
        break;
    }
    case 0x080697DC:{ /* ApplyMapBank4Preset697DC */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1338));
        REQUIRE(animation(m,16, 0x088CACA8));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CACBD));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088CACD6));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x08069854:{ /* ApplyMapBank4Preset69854 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_bank4_clear_neighbors(m,22));
        REQUIRE(hn_bank4_clear_neighbors(m,25));
        REQUIRE(animation(m,16, 0x088CE17D));
        switch (hn_read8(m,HN_SCENE_CONTROLS+0x1A1)) {
        case 1:
            REQUIRE(animation(m,22, 0x088CDE44));
            REQUIRE(animation(m,25, 0x088CDE44));
            break;
        case 2:
            REQUIRE(animation(m,22, 0x088CDE3C));
            REQUIRE(animation(m,25, 0x088CDE44));
            break;
        case 3:
            REQUIRE(animation(m,22, 0x088CDE3C));
            REQUIRE(animation(m,25, 0x088CDE3C));
            break;
        }
        break;
    }
    case 0x080698F0:{ /* ApplyMapBank4Preset698F0 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088CC3E4));
        break;
    }
    case 0x0806990C:{ /* ApplyMapBank4Preset6990C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088CDE83));
        break;
    }
    case 0x08069928:{ /* ApplyMapBank4Preset69928 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 1) {
            index = 16;
            REQUIRE(hn_bank4_clear_neighbors(m,16));
        } else if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 2) {
            index = 22;
            REQUIRE(hn_bank4_clear_neighbors(m,22));
        } else {
            index = 25;
            REQUIRE(hn_bank4_clear_neighbors(m,25));
        }
        switch ((uint8_t)hn_render_direction_raw(m,index, 10,false)) {
        case 1: REQUIRE(animation(m,index, 0x088CDD96)); break;
        case 2: REQUIRE(animation(m,index, 0x088CDD96)); break;
        case 3: REQUIRE(animation(m,index, 0x088CDDA6)); break;
        case 4: REQUIRE(animation(m,index, 0x088CDD9E)); break;
        }
        break;
    }
    case 0x080699B8:{ /* ApplyMapBank4Preset699B8 */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 1) {
            index = 16;
            REQUIRE(hn_bank4_clear_neighbors(m,16));
        } else if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 2) {
            index = 22;
            REQUIRE(hn_bank4_clear_neighbors(m,22));
        } else {
            index = 25;
            REQUIRE(hn_bank4_clear_neighbors(m,25));
        }
        switch ((uint8_t)hn_render_direction_raw(m,index, 10,false)) {
        case 1: REQUIRE(animation(m,index, 0x088CDDBA)); break;
        case 2: REQUIRE(animation(m,index, 0x088CDDBA)); break;
        case 3: REQUIRE(animation(m,index, 0x088CDDCA)); break;
        case 4: REQUIRE(animation(m,index, 0x088CDDC2)); break;
        }
        break;
    }
    case 0x08069A48:{ /* ApplyMapBank4Preset69A48 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(field(m,30, 0));
        REQUIRE(animation(m,19, 0x088DBAD8));
        break;
    }
    case 0x08069A6C:{ /* ApplyMapBank4Preset69A6C */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DB583));
        break;
    }
    case 0x08069A88:{ /* ApplyMapBank4Preset69A88 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(animation(m,19, 0x088DBB99));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DBB28));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08069ADC:{ /* ApplyMapBank4Preset69ADC */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBD36));
        break;
    }
    case 0x08069AF8:{ /* ApplyMapBank4Preset69AF8 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBD4B));
        break;
    }
    case 0x08069B14:{ /* ApplyMapBank4Preset69B14 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DB97B));
        break;
    }
    case 0x08069B30:{ /* ApplyMapBank4Preset69B30 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DBA72));
        REQUIRE(hn_render_configure_raw(m,30, 1, 176, 0, 0, 0, 0x088EBBD5));
        REQUIRE(hn_render_modes_raw(m,30, 2, 3, 19));
        break;
    }
    case 0x08069B74:{ /* ApplyMapBank4Preset69B74 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_bank4_clear_neighbors(m,22));
        REQUIRE(hn_bank4_clear_neighbors(m,25));
        REQUIRE(animation(m,16, 0x088CE13E));
        REQUIRE(animation(m,22, 0x088CE13E));
        REQUIRE(animation(m,25, 0x088CE13E));
        break;
    }
    case 0x08069BB0:{ /* ApplyMapBank4Preset69BB0 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(field(m,30, 0));
        REQUIRE(animation(m,19, 0x088DBA72));
        break;
    }
    case 0x08069BD4:{ /* ApplyMapBank4Preset69BD4 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,16, 0x088CAB5E));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CABA2));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088CABB9));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,22, 1, 114, 88, 0, 136, 0x088CA67D));
        REQUIRE(hn_render_configure_raw(m,25, 1, 114, 136, 0, 104, 0x088CA67D));
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088CA695));
        REQUIRE(hn_render_configure_raw(m,24, 1, 176, 0, 0, 0, 0x088CA68D));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        REQUIRE(hn_render_modes_raw(m,24, 2, 3, 25));
        REQUIRE(hn_render_configure_raw(m,23, 1, 176, 0, 0, 0, 0x088CB755));
        REQUIRE(hn_render_configure_raw(m,26, 1, 176, 0, 0, 0, 0x088CB755));
        REQUIRE(hn_render_modes_raw(m,23, 2, 3, 22));
        REQUIRE(hn_render_modes_raw(m,26, 2, 3, 25));
        break;
    }
    case 0x08069D04:{ /* ApplyMapBank4Preset69D04 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088DB5DC));
        break;
    }
    case 0x08069D20:{ /* ApplyMapBank4Preset69D20 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088CE13E));
        break;
    }
    case 0x08069D3C:{ /* ApplyMapBank4Preset69D3C */
        uint8_t index;
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 1) {
            index = 16;
            REQUIRE(hn_bank4_clear_neighbors(m,16));
        } else if (hn_read8(m,HN_SCENE_CONTROLS+0x1A1) == 2) {
            index = 22;
            REQUIRE(hn_bank4_clear_neighbors(m,22));
        } else {
            index = 25;
            REQUIRE(hn_bank4_clear_neighbors(m,25));
        }
        REQUIRE(animation(m,index, 0x088CDEAF));
        break;
    }
    case 0x08069D88:{ /* ApplyMapBank4Preset69D88 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088CA5C1));
        break;
    }
    case 0x08069DA4:{ /* ApplyMapBank4Preset69DA4 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088CAB5E));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CABA2));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088CABB9));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x08069E10:{ /* ApplyMapBank4Preset69E10 */
        GameAddr animations;
        uint32_t offset;
        GameAddr animation_word;
        animations = 0x020016AC;
        offset = (uint8_t)hn_render_direction_raw(m,16, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,16, animation_word));
        break;
    }
    case 0x08069E38:{ /* ApplyMapBank4Preset69E38 */
        GameAddr animations;
        uint32_t offset;
        GameAddr animation_word;
        animations = 0x020016AC;
        offset = (uint8_t)hn_render_direction_raw(m,19, 10,false) * 4;
        animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,19, animation_word));
        break;
    }
    case 0x08069E98:{ /* ConfigureMapBank4Preset69E98 */
        REQUIRE(animation(m,10, 0x088A5C6D));
        REQUIRE(animation(m,13, 0x088A9DDC));
        indicator(m,10, 0x088A5C6D);
        break;
    }
    case 0x08069EC4:{ /* ConfigureMapBank4Preset69EC4 */
        GameAddr animations = 0x020016AC;
        uint32_t offset = (uint8_t)hn_render_direction_raw(m,16, 10,false) * 4;
        GameAddr animation_word = hn_read32(m,(offset -= 4)+animations);
        REQUIRE(animation(m,16, animation_word));
        break;
    }
    case 0x08069EEC:{ /* ConfigureMapBank4Preset69EEC */
        REQUIRE(animation(m,16, 0x088D5A16));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x08069F28:{ /* ConfigureMapBank4Preset69F28 */
        REQUIRE(field(m,17, 0));
        REQUIRE(animation(m,16, 0x088D594C));
        break;
    }
    case 0x08069F44:{ /* ConfigureMapBank4Preset69F44 */
        REQUIRE(animation(m,16, 0x088D58D2));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088D58E7));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x08069F80:{ /* ConfigureMapBank4Preset69F80 */
        REQUIRE(animation(m,19, 0x088D65B7));
        break;
    }
    case 0x08069F94:{ /* ConfigureMapBank4Preset69F94 */
        REQUIRE(field(m,17, 0));
        REQUIRE(animation(m,16, 0x088D5B4C));
        REQUIRE(animation(m,19, 0x088D5BE9));
        indicator(m,16, 0x088D5B4C);
        break;
    }
    case 0x08069FC8:{ /* ConfigureMapBank4Preset69FC8 */
        REQUIRE(animation(m,16, 0x088D583C));
        indicator(m,16, 0x088D583C);
        break;
    }
    case 0x0806A1F4:{ /* ApplyMapBank4Preset6A1F4 */
        REQUIRE(hn_background_configure_raw(m,4, 132, 0x088EC613));
        break;
    }
    case 0x0806A20C:{ /* ApplyMapBank4Preset6A20C */
        hn_audio_track_parameter(m,0x030050A0, 255, 0,0);
        break;
    }
    case 0x0806A224:{ /* ApplyMapBank4Preset6A224 */
        hn_audio_track_parameter(m,0x030050A0, 255, 64,0);
        break;
    }
    case 0x0806A23C:{ /* ApplyMapBank4Preset6A23C */
        hn_audio_track_parameter(m,0x030050A0, 255, 256,0);
        break;
    }
    case 0x0806A2C0:{ /* ConfigureMapBank4Preset6A2C0 */
        REQUIRE(animation(m,16, 0x088DFCF7));
        break;
    }
    case 0x0806A2D4:{ /* ConfigureMapBank4Preset6A2D4 */
        REQUIRE(animation(m,16, 0x088DFE4D));
        break;
    }
    case 0x0806A2E8:{ /* ConfigureMapBank4Preset6A2E8 */
        REQUIRE(animation(m,16, 0x088DFE96));
        break;
    }
    case 0x0806A2FC:{ /* ConfigureMapBank4Preset6A2FC */
        REQUIRE(animation(m,16, 0x088E0116));
        break;
    }
    case 0x0806A310:{ /* ConfigureMapBank4Preset6A310 */
        REQUIRE(animation(m,16, 0x088DFD22));
        break;
    }
    case 0x0806A324:{ /* ConfigureMapBank4Preset6A324 */
        REQUIRE(animation(m,16, 0x088DFE38));
        break;
    }
    case 0x0806A338:{ /* ConfigureMapBank4Preset6A338 */
        REQUIRE(animation(m,16, 0x088E013D));
        break;
    }
    case 0x0806A34C:{ /* ConfigureMapBank4Preset6A34C */
        REQUIRE(animation(m,16, 0x088DFCF7));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E34F0));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806A388:{ /* ConfigureMapBank4Preset6A388 */
        REQUIRE(animation(m,16, 0x088DFCC9));
        break;
    }
    case 0x0806A39C:{ /* ConfigureMapBank4Preset6A39C */
        REQUIRE(animation(m,13, 0x088B6FBB));
        break;
    }
    case 0x0806A3B0:{ /* ConfigureMapBank4Preset6A3B0 */
        REQUIRE(animation(m,10, 0x088ADA36));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088ADA54));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        hn_write8(m,0x03001FAC,10);
        hn_write32(m,0x03001F7C,0x088ADA36);
        break;
    }
    case 0x0806A404:{ /* ConfigureMapBank4Preset6A404 */
        switch ((uint8_t)hn_render_direction_raw(m,10, 13,false)) {
        case 2: REQUIRE(animation(m,10, 0x088AC08F)); break;
        case 1: REQUIRE(animation(m,10, 0x088AC0C6)); break;
        case 4: REQUIRE(animation(m,10, 0x088AC134)); break;
        case 3: REQUIRE(animation(m,10, 0x088AC0FD)); break;
        }
        switch ((uint8_t)hn_render_direction_raw(m,13, 10,false)) {
        case 2: REQUIRE(animation(m,13, 0x088B4002)); break;
        case 1: REQUIRE(animation(m,13, 0x088B4055)); break;
        case 4: REQUIRE(animation(m,13, 0x088B40C3)); break;
        case 3: REQUIRE(animation(m,13, 0x088B408C)); break;
        }
        break;
    }
    case 0x0806A4B0:{ /* ConfigureMapBank4Preset6A4B0 */
        REQUIRE(animation(m,19, 0x088C6B61));
        break;
    }
    case 0x0806A4C4:{ /* ConfigureMapBank4Preset6A4C4 */
        REQUIRE(animation(m,19, 0x088C6BDF));
        hn_write8(m,0x03001FAC,19);
        hn_write32(m,0x03001F7C,0x088C6BDF);
        break;
    }
    case 0x0806A4EC:{ /* ConfigureMapBank4Preset6A4EC */
        REQUIRE(field(m,15, 0));
        switch ((uint8_t)hn_render_direction_raw(m,19, 10,false)) {
        case 2: REQUIRE(animation(m,19, 0x088C6A1F)); break;
        case 1: REQUIRE(animation(m,19, 0x088C6A2B)); break;
        case 4: REQUIRE(animation(m,19, 0x088C6A43)); break;
        case 3: REQUIRE(animation(m,19, 0x088C6A37)); break;
        }
        break;
    }
    case 0x0806A54C:{ /* ConfigureMapBank4Preset6A54C */
        REQUIRE(animation(m,19, 0x088C6B6D));
        break;
    }
    case 0x0806A560:{ /* ConfigureMapBank4Preset6A560 */
        REQUIRE(animation(m,19, 0x088C6B2A));
        break;
    }
    case 0x0806A574:{ /* ConfigureMapBank4Preset6A574 */
        REQUIRE(animation(m,19, 0x088C6A94));
        break;
    }
    case 0x0806A588:{ /* ConfigureMapBank4Preset6A588 */
        REQUIRE(animation(m,19, 0x088C6AA0));
        break;
    }
    case 0x0806A59C:{ /* ConfigureMapBank4Preset6A59C */
        REQUIRE(animation(m,19, 0x088C6BBC));
        break;
    }
    case 0x0806A5B0:{ /* ConfigureMapBank4Preset6A5B0 */
        REQUIRE(animation(m,13, 0x088B551C));
        break;
    }
    case 0x0806A5C4:{ /* ConfigureMapBank4Preset6A5C4 */
        switch ((uint8_t)hn_render_direction_raw(m,13, 19,false)) {
        case 2: REQUIRE(animation(m,13, 0x088B4002)); break;
        case 1: REQUIRE(animation(m,13, 0x088B4055)); break;
        case 4: REQUIRE(animation(m,13, 0x088B40C3)); break;
        case 3: REQUIRE(animation(m,13, 0x088B408C)); break;
        }
        break;
    }
    case 0x0806A61C:{ /* ConfigureMapBank4Preset6A61C */
        REQUIRE(animation(m,19, 0x088C6A88));
        break;
    }
    case 0x0806A630:{ /* ConfigureMapBank4Preset6A630 */
        REQUIRE(animation(m,19, 0x088C6BC8));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088C6BD4));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x0806A66C:{ /* ConfigureMapBank4Preset6A66C */
        REQUIRE(animation(m,19, 0x088C6B6D));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088C6B79));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x0806A6A8:{ /* ConfigureMapBank4Preset6A6A8 */
        switch ((uint8_t)hn_render_direction_raw(m,10, 19,false)) {
        case 2: REQUIRE(animation(m,10, 0x088AC08F)); break;
        case 1: REQUIRE(animation(m,10, 0x088AC0C6)); break;
        case 4: REQUIRE(animation(m,10, 0x088AC134)); break;
        case 3: REQUIRE(animation(m,10, 0x088AC0FD)); break;
        }
        switch ((uint8_t)hn_render_direction_raw(m,13, 19,false)) {
        case 2: REQUIRE(animation(m,13, 0x088B4002)); break;
        case 1: REQUIRE(animation(m,13, 0x088B4055)); break;
        case 4: REQUIRE(animation(m,13, 0x088B40C3)); break;
        case 3: REQUIRE(animation(m,13, 0x088B408C)); break;
        }
        break;
    }
    case 0x0806A754:{ /* ConfigureMapBank4Preset6A754 */
        REQUIRE(animation(m,10, 0x088AD641));
        REQUIRE(animation(m,13, 0x088B5999));
        break;
    }
    case 0x0806A774:{ /* ConfigureMapBank4Preset6A774 */
        switch ((uint8_t)hn_render_direction_raw(m,10, 13,false)) {
        case 2: REQUIRE(animation(m,10, 0x088AC08F)); break;
        case 1: REQUIRE(animation(m,10, 0x088AC0C6)); break;
        case 4: REQUIRE(animation(m,10, 0x088AC134)); break;
        case 3: REQUIRE(animation(m,10, 0x088AC0FD)); break;
        }
        break;
    }
    case 0x0806A7CC:{ /* ConfigureMapBank4Preset6A7CC */
        switch ((uint8_t)hn_render_direction_raw(m,13, 10,false)) {
        case 2: REQUIRE(animation(m,13, 0x088B4002)); break;
        case 1: REQUIRE(animation(m,13, 0x088B4055)); break;
        case 4: REQUIRE(animation(m,13, 0x088B40C3)); break;
        case 3: REQUIRE(animation(m,13, 0x088B408C)); break;
        }
        break;
    }
    case 0x0806A824:{ /* ConfigureMapBank4Preset6A824 */
        REQUIRE(animation(m,13, 0x088B6A5D));
        break;
    }
    case 0x0806A838:{ /* ConfigureMapBank4Preset6A838 */
        REQUIRE(animation(m,13, 0x088B6A9F));
        REQUIRE(animation(m,10, 0x088AECA8));
        break;
    }
    case 0x0806A858:{ /* ConfigureMapBank4Preset6A858 */
        REQUIRE(animation(m,10, 0x088ACB54));
        REQUIRE(animation(m,13, 0x088B7042));
        break;
    }
    case 0x0806A878:{ /* ConfigureMapBank4Preset6A878 */
        REQUIRE(animation(m,13, 0x088B59B5));
        break;
    }
    case 0x0806A8B4:{ /* ApplyMapBank4Preset6A8B4 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(animation(m,10, 0x088A6932));
        REQUIRE(animation(m,13, 0x088AAEA0));
        indicator(m,10, 0x088A6932);
        break;
    }
    case 0x0806A8EC:{ /* ApplyMapBank4Preset6A8EC */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(animation(m,10, 0x088A5133));
        REQUIRE(animation(m,13, 0x088A8E4F));
        indicator(m,10, 0x088A5133);
        break;
    }
    case 0x0806A924:{ /* ApplyMapBank4Preset6A924 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C2F27));
        break;
    }
    case 0x0806A940:{ /* ApplyMapBank4Preset6A940 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C2850));
        break;
    }
    case 0x0806A95C:{ /* ApplyMapBank4Preset6A95C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C2EE2));
        break;
    }
    case 0x0806A978:{ /* ApplyMapBank4Preset6A978 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C27EC));
        break;
    }
    case 0x0806A994:{ /* ApplyMapBank4Preset6A994 */
        break;
    }
    case 0x0806A998:{ /* ApplyMapBank4Preset6A998 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C2815));
        break;
    }
    case 0x0806A9B4:{ /* ApplyMapBank4Preset6A9B4 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C2F12));
        break;
    }
    case 0x0806A9D0:{ /* ApplyMapBank4Preset6A9D0 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C28DC));
        break;
    }
    case 0x0806A9EC:{ /* ApplyMapBank4Preset6A9EC */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088C28F1));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 25));
        break;
    }
    case 0x0806AA2C:{ /* ApplyMapBank4Preset6AA2C */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C290D));
        break;
    }
    case 0x0806AA48:{ /* ApplyMapBank4Preset6AA48 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088C28F1));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 25));
        break;
    }
    case 0x0806AA88:{ /* ApplyMapBank4Preset6AA88 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C40C7));
        break;
    }
    case 0x0806AAA4:{ /* ApplyMapBank4Preset6AAA4 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C28D0));
        break;
    }
    case 0x0806AAC0:{ /* ApplyMapBank4Preset6AAC0 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C2EFA));
        break;
    }
    case 0x0806AADC:{ /* ApplyMapBank4Preset6AADC */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C40BB));
        break;
    }
    case 0x0806AAF8:{ /* ApplyMapBank4Preset6AAF8 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(animation(m,10, 0x088AD701));
        REQUIRE(animation(m,13, 0x088B6A5D));
        break;
    }
    case 0x0806AB24:{ /* ApplyMapBank4Preset6AB24 */
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(animation(m,13, 0x088B6A9F));
        break;
    }
    case 0x0806AB40:{ /* ApplyMapBank4Preset6AB40 */
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(animation(m,13, 0x088B6FBB));
        break;
    }
    case 0x0806AB5C:{ /* ApplyMapBank4Preset6AB5C */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C29B2));
        indicator(m,19, 0x088C29B2);
        break;
    }
    case 0x0806AB84:{ /* ApplyMapBank4Preset6AB84 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(animation(m,10, 0x088AC08F));
        break;
    }
    case 0x0806ABA0:{ /* ApplyMapBank4Preset6ABA0 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C2922));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088C292E));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x0806ABE4:{ /* ApplyMapBank4Preset6ABE4 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,16, 0x088C19D8));
        REQUIRE(animation(m,19, 0x088C294D));
        priority(m,19, 1);
        indicator(m,19, 0x088C294D);
        break;
    }
    case 0x0806AC24:{ /* ApplyMapBank4Preset6AC24 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C2941));
        break;
    }
    case 0x0806AC40:{ /* ApplyMapBank4Preset6AC40 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C0DBF));
        break;
    }
    case 0x0806AC5C:{ /* ApplyMapBank4Preset6AC5C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C19E4));
        break;
    }
    case 0x0806AC78:{ /* ApplyMapBank4Preset6AC78 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C151B));
        break;
    }
    case 0x0806AC94:{ /* ApplyMapBank4Preset6AC94 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,16, 0x088C1A12));
        REQUIRE(animation(m,19, 0x088C29D5));
        indicator(m,16, 0x088C1A12);
        break;
    }
    case 0x0806ACCC:{ /* ApplyMapBank4Preset6ACCC */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C289D));
        break;
    }
    case 0x0806ACE8:{ /* ApplyMapBank4Preset6ACE8 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088C28A9));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x0806AD28:{ /* ApplyMapBank4Preset6AD28 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C0DA7));
        break;
    }
    case 0x0806AD44:{ /* ApplyMapBank4Preset6AD44 */
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,12, 1, 114, 0, 0, 0, 0x088E3B5F));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        break;
    }
    case 0x0806AD8C:{ /* ApplyMapBank4Preset6AD8C */
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(animation(m,13, 0x088B59B5));
        REQUIRE(hn_render_configure_raw(m,12, 1, 114, 0, 0, 0, 0x088B59C1));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        REQUIRE(hn_render_configure_raw(m,14, 1, 114, 0, 0, 0, 0x088B59D2));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        break;
    }
    case 0x0806ADF8:{ /* ApplyMapBank4Preset6ADF8 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C1A8F));
        break;
    }
    case 0x0806AE14:{ /* ApplyMapBank4Preset6AE14 */
        break;
    }
    case 0x0806AE18:{ /* ApplyMapBank4Preset6AE18 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(animation(m,10, 0x088AD701));
        break;
    }
    case 0x0806AE34:{ /* ApplyMapBank4Preset6AE34 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(animation(m,10, 0x088AC08F));
        break;
    }
    case 0x0806AE50:{ /* ApplyMapBank4Preset6AE50 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088C2752));
        break;
    }
    case 0x0806AE6C:{ /* ApplyMapBank4Preset6AE6C */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1338));
        REQUIRE(hn_render_configure_raw(m,18, 1, 114, 0, 0, 0, 0x088C27F8));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x0806AEB4:{ /* ApplyMapBank4Preset6AEB4 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(animation(m,10, 0x088AC134));
        break;
    }
    case 0x0806AED0:{ /* ApplyMapBank4Preset6AED0 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1338));
        REQUIRE(hn_render_configure_raw(m,15, 1, 114, 0, 0, 0, 0x088C19F0));
        REQUIRE(hn_render_configure_raw(m,17, 1, 114, 0, 0, 0, 0x088C1A01));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806AF3C:{ /* ApplyMapBank4Preset6AF3C */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(animation(m,10, 0x088AC134));
        REQUIRE(animation(m,13, 0x088B408C));
        break;
    }
    case 0x0806AFD0:{ /* ConfigureMapBank4Preset6AFD0 */
        REQUIRE(animation(m,19, 0x088ECE40));
        break;
    }
    case 0x0806AFE4:{ /* ConfigureMapBank4Preset6AFE4 */
        REQUIRE(animation(m,19, 0x088ECE61));
        break;
    }
    case 0x0806AFF8:{ /* ConfigureMapBank4Preset6AFF8 */
        REQUIRE(animation(m,19, 0x088ECE80));
        break;
    }
    case 0x0806B00C:{ /* ConfigureMapBank4Preset6B00C */
        REQUIRE(animation(m,19, 0x088ECE9A));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088E30FE));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        hn_write8(m,0x03001FAC,19);
        hn_write32(m,0x03001F7C,0x088ECE9A);
        break;
    }
    case 0x0806B060:{ /* ConfigureMapBank4Preset6B060 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5CF6));
        break;
    }
    case 0x0806B078:{ /* ConfigureMapBank4Preset6B078 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5ED5));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D5F0D));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088D5ED5);
        break;
    }
    case 0x0806B0D0:{ /* ConfigureMapBank4Preset6B0D0 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5CFE));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D5D15));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088D5CFE);
        break;
    }
    case 0x0806B128:{ /* ConfigureMapBank4Preset6B128 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5D44));
        break;
    }
    case 0x0806B140:{ /* ConfigureMapBank4Preset6B140 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D6191));
        break;
    }
    case 0x0806B158:{ /* ConfigureMapBank4Preset6B158 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D6191));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D619D));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806B198:{ /* ConfigureMapBank4Preset6B198 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5E06));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088D5E1A));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088D5E2B));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 16));
        break;
    }
    case 0x0806B200:{ /* ConfigureMapBank4Preset6B200 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5E06));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D5E12));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088D5E1A));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088D5E2B));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 16));
        break;
    }
    case 0x0806B28C:{ /* ConfigureMapBank4Preset6B28C */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D61A5));
        if (hn_read8(m,HN_SCENE_CONTROLS+0x89) > 4) {
            REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D61B1));
            REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        }
        break;
    }
    case 0x0806B2DC:{ /* ConfigureMapBank4Preset6B2DC */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5EC1));
        if (hn_read8(m,HN_SCENE_CONTROLS+0x89) > 4) {
            REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D5ECD));
            REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        }
        break;
    }
    case 0x0806B32C:{ /* ConfigureMapBank4Preset6B32C */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D61A5));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E34F0));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806B36C:{ /* ClearMapBank4Slots15And17And18Field60 */
        REQUIRE(field(m,15, 0));
        REQUIRE(field(m,17, 0));
        REQUIRE(field(m,18, 0));
        break;
    }
    case 0x0806B38C:{ /* ClearMapBank4Slot22Field60 */
        REQUIRE(field(m,22, 0));
        break;
    }
    case 0x0806B39C:{ /* ConfigureMapBank4Preset6B39C */
        REQUIRE(animation(m,19, 0x088DFEA2));
        break;
    }
    case 0x0806B3B0:{ /* ConfigureMapBank4Preset6B3B0 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5E3C));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D5E51));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806B3F0:{ /* ConfigureMapBank4Preset6B3F0 */
        REQUIRE(animation(m,19, 0x088DFEBF));
        break;
    }
    case 0x0806B404:{ /* ConfigureMapBank4Preset6B404 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5F1B));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D5F27));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806B444:{ /* ConfigureMapBank4Preset6B444 */
        REQUIRE(animation(m,19, 0x088DFEBF));
        REQUIRE(hn_render_configure_raw(m,22, 1, 176, 0, 0, 0, 0x088DFEAE));
        REQUIRE(hn_render_modes_raw(m,22, 2, 3, 19));
        break;
    }
    case 0x0806B480:{ /* ConfigureMapBank4Preset6B480 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(animation(m,16, 0x088D5E3C));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D5E51));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088D5E59));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806B4E8:{ /* ConfigureMapBank4Preset6B4E8 */
        REQUIRE(animation(m,19, 0x088DFD16));
        REQUIRE(hn_render_configure_raw(m,22, 1, 176, 0, 0, 0, 0x088DFEAE));
        REQUIRE(hn_render_modes_raw(m,22, 2, 3, 19));
        break;
    }
    case 0x0806B524:{ /* ConfigureMapBank4Preset6B524 */
        REQUIRE(hn_bank4_presets(m,0x0806B36C)==HN_SCENE_OK);
        REQUIRE(hn_bank4_presets(m,0x0806B38C)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E13F3));
        REQUIRE(field(m,18, 0));
        REQUIRE(animation(m,16, 0x088D5F2F));
        REQUIRE(animation(m,19, 0x088DFF10));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D5FC2));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088D6005));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088D5F2F);
        break;
    }
    case 0x0806B5C4:{ /* ConfigureMapBank4Preset6B5C4 */
        REQUIRE(animation(m,10, 0x088AC134));
        REQUIRE(animation(m,13, 0x088B59B5));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088B59C1));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088B59D2));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        break;
    }
    case 0x0806B634:{ /* ConfigureMapBank4Preset6B634 */
        REQUIRE(animation(m,10, 0x088AC134));
        REQUIRE(animation(m,13, 0x088B6BB5));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088B6C1C));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088B6C57));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        break;
    }
    case 0x0806B6A4:{ /* ConfigureMapBank4Preset6B6A4 */
        REQUIRE(animation(m,19, 0x088DFD16));
        REQUIRE(hn_render_configure_raw(m,22, 1, 176, 0, 0, 0, 0x088DFEAE));
        REQUIRE(hn_render_modes_raw(m,22, 2, 3, 19));
        break;
    }
    case 0x0806B6E0:{ /* ConfigureMapBank4Preset6B6E0 */
        REQUIRE(animation(m,19, 0x088ECE54));
        indicator(m,19, 0x088ECE54);
        break;
    }
    case 0x0806B700:{ /* ConfigureMapBank4Preset6B700 */
        REQUIRE(animation(m,16, 0x088ECFF7));
        break;
    }
    case 0x0806B714:{ /* ConfigureMapBank4Preset6B714 */
        REQUIRE(animation(m,16, 0x088ED004));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088ED004);
        break;
    }
    case 0x0806B73C:{ /* ConfigureMapBank4Preset6B73C */
        REQUIRE(animation(m,16, 0x088ED019));
        break;
    }
    case 0x0806B750:{ /* ConfigureMapBank4Preset6B750 */
        REQUIRE(animation(m,16, 0x088ED021));
        break;
    }
    case 0x0806B764:{ /* ConfigureMapBank4Preset6B764 */
        REQUIRE(animation(m,16, 0x088ED038));
        break;
    }
    case 0x0806B778:{ /* ConfigureMapBank4Preset6B778 */
        REQUIRE(animation(m,16, 0x088ED040));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088E19BC));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088ED040);
        break;
    }
    case 0x0806B7CC:{ /* ConfigureMapBank4Preset6B7CC */
        REQUIRE(animation(m,16, 0x088ECFF7));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088ED06E));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806B808:{ /* ConfigureMapBank4Preset6B808 */
        REQUIRE(animation(m,10, 0x088ADA0E));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088ADA54));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        REQUIRE(animation(m,13, 0x088B4F3F));
        break;
    }
    case 0x0806B878:{ /* ApplyMapBank4Preset6B878 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DFCF7));
        break;
    }
    case 0x0806B894:{ /* ApplyMapBank4Preset6B894 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DFD03));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806B8DC:{ /* ApplyMapBank4Preset6B8DC */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DFE4D));
        break;
    }
    case 0x0806B8F8:{ /* ApplyMapBank4Preset6B8F8 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088E0116));
        break;
    }
    case 0x0806B914:{ /* ApplyMapBank4Preset6B914 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088E013D));
        break;
    }
    case 0x0806B930:{ /* ApplyMapBank4Preset6B930 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DFE96));
        break;
    }
    case 0x0806B94C:{ /* ApplyMapBank4Preset6B94C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088E0158));
        break;
    }
    case 0x0806B968:{ /* ApplyMapBank4Preset6B968 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088E01AA));
        break;
    }
    case 0x0806B984:{ /* ApplyMapBank4Preset6B984 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,10, 0x088AC08F));
        REQUIRE(animation(m,13, 0x088B4002));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088DFD03));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        break;
    }
    case 0x0806B9E8:{ /* ApplyMapBank4Preset6B9E8 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(animation(m,10, 0x088AC0C6));
        break;
    }
    case 0x0806BAD4:{ /* MapBank4ReturnZero6BAD4 */
        break;
    }
    case 0x0806BCF0:{ /* ConfigureMapBank4Preset6BCF0 */
        REQUIRE(animation(m,19, 0x088C6DC4));
        break;
    }
    case 0x0806BD04:{ /* ConfigureMapBank4Preset6BD04 */
        REQUIRE(animation(m,10, 0x088A6711));
        REQUIRE(animation(m,13, 0x088AB681));
        indicator(m,10, 0x088A6711);
        break;
    }
    case 0x0806BD30:{ /* ConfigureMapBank4Preset6BD30 */
        REQUIRE(animation(m,19, 0x088DBA28));
        break;
    }
    case 0x0806BD44:{ /* ConfigureMapBank4Preset6BD44 */
        REQUIRE(animation(m,19, 0x088DB93D));
        break;
    }
    case 0x0806BD58:{ /* ConfigureMapBank4Preset6BD58 */
        REQUIRE(animation(m,19, 0x088DB8E1));
        break;
    }
    case 0x0806BD6C:{ /* ConfigureMapBank4Preset6BD6C */
        REQUIRE(animation(m,19, 0x088DC13C));
        break;
    }
    case 0x0806BD80:{ /* ConfigureMapBank4Preset6BD80 */
        REQUIRE(animation(m,19, 0x088DB9DB));
        break;
    }
    case 0x0806BD94:{ /* ConfigureMapBank4Preset6BD94 */
        REQUIRE(animation(m,19, 0x088DC165));
        indicator(m,19, 0x088DC165);
        break;
    }
    case 0x0806BDB4:{ /* ConfigureMapBank4Preset6BDB4 */
        REQUIRE(animation(m,19, 0x088DBD36));
        break;
    }
    case 0x0806BDC8:{ /* ConfigureMapBank4Preset6BDC8 */
        REQUIRE(animation(m,19, 0x088DB949));
        indicator(m,19, 0x088DB949);
        break;
    }
    case 0x0806BF20:{ /* ConfigureMapBank4Preset6BF20 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        break;
    }
    case 0x0806BF2C:{ /* ConfigureMapBank4Preset6BF2C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        break;
    }
    case 0x0806BF38:{ /* ConfigureMapBank4Preset6BF38 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        break;
    }
    case 0x0806BF44:{ /* ConfigureMapBank4Preset6BF44 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        break;
    }
    case 0x0806BF50:{ /* ConfigureMapBank4Preset6BF50 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        break;
    }
    case 0x0806BF5C:{ /* ConfigureMapBank4Preset6BF5C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        break;
    }
    case 0x0806BF68:{ /* ConfigureMapBank4Preset6BF68 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        break;
    }
    case 0x0806BF74:{ /* ConfigureMapBank4Preset6BF74 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        break;
    }
    case 0x0806BF80:{ /* ConfigureMapBank4Preset6BF80 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(animation(m,10, 0x088AC08F));
        REQUIRE(animation(m,13, 0x088B4002));
        break;
    }
    case 0x0806BFAC:{ /* ConfigureMapBank4Preset6BFAC */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        break;
    }
    case 0x0806BFB8:{ /* ConfigureMapBank4Preset6BFB8 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        break;
    }
    case 0x0806BFC4:{ /* ConfigureMapBank4Preset6BFC4 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C7750));
        break;
    }
    case 0x0806BFDC:{ /* ConfigureMapBank4Preset6BFDC */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C77CB));
        break;
    }
    case 0x0806BFF4:{ /* ConfigureMapBank4Preset6BFF4 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088C77D7));
        break;
    }
    case 0x0806C00C:{ /* ConfigureMapBank4Preset6C00C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        hn_write16(m,0x0201DC40+(0x2C)*2,40);
        REQUIRE(animation(m,16, 0x088C79AE));
        break;
    }
    case 0x0806C030:{ /* ConfigureMapBank4Preset6C030 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6DB8));
        REQUIRE(hn_render_configure_raw(m,(uint8_t)((hn_read8(m,0x03002C00) - 1)), 1, 176, 0, 0xFFFFFFFF, 1, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)((hn_read8(m,0x03002C00) - 1)), 2, 3, 19));
        break;
    }
    case 0x0806C094:{ /* ConfigureMapBank4Preset6C094 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6DD0));
        break;
    }
    case 0x0806C0AC:{ /* ConfigureMapBank4Preset6C0AC */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6DE5));
        break;
    }
    case 0x0806C0C4:{ /* ConfigureMapBank4Preset6C0C4 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6DE5));
        REQUIRE(hn_render_configure_raw(m,(uint8_t)((hn_read8(m,0x03002C00) - 1)), 1, 176, 0, 0xFFFFFFFF, 1, 0x088C6DF1));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)((hn_read8(m,0x03002C00) - 1)), 2, 3, 19));
        break;
    }
    case 0x0806C118:{ /* ConfigureMapBank4Preset6C118 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ED2CB));
        break;
    }
    case 0x0806C130:{ /* ConfigureMapBank4Preset6C130 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ED2D8));
        break;
    }
    case 0x0806C148:{ /* ConfigureMapBank4Preset6C148 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ED2E0));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0xFFFFFFFF, 1, 0x088ED419));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x0806C190:{ /* ConfigureMapBank4Preset6C190 */
        REQUIRE(hn_bank4_clear_neighbors(m,hn_read8(m,0x03002C00)));
        break;
    }
    case 0x0806C1CC:{ /* ConfigureMapBank4Preset6C1CC */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DC148));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088DC148);
        break;
    }
    case 0x0806C1FC:{ /* ConfigureMapBank4Preset6C1FC */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(animation(m,16, 0x088DB8E1));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088DB8F6));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806C248:{ /* ConfigureMapBank4Preset6C248 */
        REQUIRE(animation(m,16, 0x088DB93D));
        break;
    }
    case 0x0806C25C:{ /* ConfigureMapBank4Preset6C25C */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(animation(m,16, 0x088DBD87));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088DBD93));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806C2A8:{ /* ConfigureMapBank4Preset6C2A8 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DB583));
        break;
    }
    case 0x0806C2C0:{ /* ConfigureMapBank4Preset6C2C0 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DC1DC));
        break;
    }
    case 0x0806C2D8:{ /* ConfigureMapBank4Preset6C2D8 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DB729));
        break;
    }
    case 0x0806C2F0:{ /* ConfigureMapBank4Preset6C2F0 */
        REQUIRE(animation(m,16, 0x088DB768));
        break;
    }
    case 0x0806C304:{ /* ConfigureMapBank4Preset6C304 */
        REQUIRE(animation(m,16, 0x088DBA8B));
        break;
    }
    case 0x0806C318:{ /* ConfigureMapBank4Preset6C318 */
        REQUIRE(animation(m,16, 0x088DB97B));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088DB987));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806C354:{ /* ConfigureMapBank4Preset6C354 */
        REQUIRE(animation(m,16, 0x088DBA52));
        break;
    }
    case 0x0806C368:{ /* ConfigureMapBank4Preset6C368 */
        REQUIRE(animation(m,16, 0x088DBDD3));
        break;
    }
    case 0x0806C37C:{ /* ConfigureMapBank4Preset6C37C */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,16, 0x088DB8CA));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088DB8D6));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806C3C8:{ /* ConfigureMapBank4Preset6C3C8 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DB9B3));
        break;
    }
    case 0x0806C3E0:{ /* ConfigureMapBank4Preset6C3E0 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DBD60));
        break;
    }
    case 0x0806C3F8:{ /* ConfigureMapBank4Preset6C3F8 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088D352B));
        break;
    }
    case 0x0806C410:{ /* ConfigureMapBank4Preset6C410 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088D32D2));
        break;
    }
    case 0x0806C428:{ /* ConfigureMapBank4Preset6C428 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088D2F47));
        break;
    }
    case 0x0806C440:{ /* ConfigureMapBank4Preset6C440 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088D2E81));
        break;
    }
    case 0x0806C458:{ /* ConfigureMapBank4Preset6C458 */
        REQUIRE(hn_bank4_clear_neighbors(m,hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0806C46C:{ /* ConfigureMapBank4Preset6C46C */
        REQUIRE(animation(m,hn_read8(m,0x03001F50), 0x088B6FBB));
        break;
    }
    case 0x0806C68C:{ /* ConfigureMapBank4Preset6C68C */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E142A));
        REQUIRE(animation(m,10, 0x088A50E9));
        REQUIRE(animation(m,13, 0x088A8E02));
        REQUIRE(hn_render_configure_raw(m,9, 1, 176, 0, 0, 0, 0x088E1A7A));
        REQUIRE(hn_render_modes_raw(m,9, 2, 3, 10));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088E1AAB));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        indicator(m,10, 0x088A50E9);
        break;
    }
    case 0x0806C714:{ /* ConfigureMapBank4Preset6C714 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0x12) == hn_read8(m,0x03002C00)) {
            REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6FE1));
            REQUIRE(animation(m,22, 0x088C7886));
        } else {
            REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C6FED));
            REQUIRE(animation(m,22, 0x088C788E));
        }
        break;
    }
    case 0x0806C768:{ /* ConfigureMapBank4Preset6C768 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6CEE));
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C6CEE));
        break;
    }
    case 0x0806C794:{ /* ConfigureMapBank4Preset6C794 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6FD5));
        REQUIRE(hn_render_configure_raw(m,(uint8_t)((hn_read8(m,0x03002C00) - 1)), 1, 176, 0, 0, 0, 0x088E3AF1));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)((hn_read8(m,0x03002C00) - 1)), 2, 3, hn_read8(m,0x03002C00)));
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C6FD5));
        REQUIRE(hn_render_configure_raw(m,(uint8_t)((hn_read8(m,0x03002C04) - 1)), 1, 176, 0, 0, 0, 0x088E3AFB));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)((hn_read8(m,0x03002C04) - 1)), 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0806C824:{ /* ConfigureMapBank4Preset6C824 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6FF9));
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C700B));
        break;
    }
    case 0x0806C850:{ /* ConfigureMapBank4Preset6C850 */
        REQUIRE(animation(m,22, 0x088C7886));
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088E3B05));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        break;
    }
    case 0x0806C88C:{ /* ConfigureMapBank4Preset6C88C */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C709E));
        REQUIRE(animation(m,22, 0x088C788E));
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088E3B2D));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        break;
    }
    case 0x0806C8DC:{ /* ConfigureMapBank4Preset6C8DC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C6E87));
        REQUIRE(hn_render_configure_raw(m,(uint8_t)((hn_read8(m,0x03002C04) - 1)), 1, 176, 0, 0, 0, 0x088C6FA6));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)((hn_read8(m,0x03002C04) - 1)), 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0806C92C:{ /* ConfigureMapBank4Preset6C92C */
        REQUIRE(hn_render_configure_raw(m,(uint8_t)((hn_read8(m,0x03002C00) - 1)), 1, 176, 0, 0, 0, 0x088E3A1A));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)((hn_read8(m,0x03002C00) - 1)), 2, 3, hn_read8(m,0x03002C00)));
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088E3B55));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        REQUIRE(hn_render_configure_raw(m,9, 1, 176, 0, 0, 0, 0x088E399F));
        REQUIRE(hn_render_modes_raw(m,9, 2, 3, 10));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088E399F));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        break;
    }
    case 0x0806C9D8:{ /* ConfigureMapBank4Preset6C9D8 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C6DC4));
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C6E87));
        REQUIRE(animation(m,22, 0x088C798E));
        REQUIRE(field(m,(uint8_t)((hn_read8(m,0x03002C04) - 1)), 0));
        break;
    }
    case 0x0806CA20:{ /* ConfigureMapBank4Preset6CA20 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C6FD5));
        REQUIRE(hn_render_configure_raw(m,(uint8_t)((hn_read8(m,0x03002C04) - 1)), 1, 176, 0, 0, 0, 0x088E398B));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)((hn_read8(m,0x03002C04) - 1)), 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0806CA70:{ /* ConfigureMapBank4Preset6CA70 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C7069));
        break;
    }
    case 0x0806CA88:{ /* ConfigureMapBank4Preset6CA88 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C4C51));
        break;
    }
    case 0x0806CAA0:{ /* ConfigureMapBank4Preset6CAA0 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088C7160));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04), 1, 114, 168, 0, 96, 0x088C77F8));
        REQUIRE(field(m,22, 0));
        REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C00) - 1), 1, 176, 0, 0, 0, 0x088E1A7A));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)(hn_read8(m,0x03002C00) - 1), 2, 3, hn_read8(m,0x03002C00)));
        REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 1, 176, 0, 0, 0, 0x088C7845));
        REQUIRE(hn_render_modes_raw(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 2, 3, hn_read8(m,0x03002C04)));
        indicator(m,hn_read8(m,0x03002C00), 0x088C7160);
        break;
    }
    case 0x0806CB5C:{ /* ConfigureMapBank4Preset6CB5C */
        REQUIRE(animation(m,22, 0x088C7886));
        break;
    }
    case 0x0806CB70:{ /* ConfigureMapBank4Preset6CB70 */
        REQUIRE(animation(m,10, 0x088AD70D));
        REQUIRE(hn_preset_apply(m,0x080136A0)==HN_SCENE_OK);
        break;
    }
    case 0x0806CB88:{ /* ConfigureMapBank4Preset6CB88 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C0C), 0x088A5A06));
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088A9AE0));
        indicator(m,hn_read8(m,0x03002C0C), 0x088A5A06);
        break;
    }
    case 0x0806CBC0:{ /* ConfigureMapBank4Preset6CBC0 */
        REQUIRE(raw_animation(m,25, 0x088EDC5A));
        break;
    }
    case 0x0806CBD4:{ /* ConfigureMapBank4Preset6CBD4 */
        REQUIRE(raw_animation(m,26, 0x088EDC72));
        break;
    }
    case 0x0806CBE8:{ /* ConfigureMapBank4Preset6CBE8 */
        REQUIRE(raw_animation(m,27, 0x088EDC8A));
        break;
    }
    case 0x0806CBFC:{ /* ConfigureMapBank4Preset6CBFC */
        REQUIRE(raw_animation(m,25, 0x088EDC52));
        break;
    }
    case 0x0806CC10:{ /* ConfigureMapBank4Preset6CC10 */
        REQUIRE(raw_animation(m,26, 0x088EDC6A));
        break;
    }
    case 0x0806CC24:{ /* ConfigureMapBank4Preset6CC24 */
        REQUIRE(raw_animation(m,27, 0x088EDC82));
        break;
    }
    case 0x0806CC38:{ /* ConfigureMapBank4Preset6CC38 */
        REQUIRE(field_animation(m,hn_read8(m,0x03002C00), 2, 0x088DC148));
        indicator(m,hn_read8(m,0x03002C00), 0x088DC148);
        break;
    }
    case 0x0806CC60:{ /* ConfigureMapBank4Preset6CC60 */
        REQUIRE(field_animation(m,19, 2, 0x088DB93D));
        break;
    }
    case 0x0806CC74:{ /* ConfigureMapBank4Preset6CC74 */
        REQUIRE(field_animation(m,19, 2, 0x088DB583));
        break;
    }
    case 0x0806CC88:{ /* ConfigureMapBank4Preset6CC88 */
        REQUIRE(field_animation(m,19, 2, 0x088DBA72));
        break;
    }
    case 0x0806CC9C:{ /* ConfigureMapBank4Preset6CC9C */
        REQUIRE(field_animation(m,19, 2, 0x088DBD36));
        break;
    }
    case 0x0806CCB0:{ /* ConfigureMapBank4Preset6CCB0 */
        REQUIRE(field_animation(m,19, 4, 0x088DB5DC));
        break;
    }
    case 0x0806CCC4:{ /* ConfigureMapBank4Preset6CCC4 */
        REQUIRE(field_animation(m,19, 2, 0x088DBA28));
        break;
    }
    case 0x0806CCD8:{ /* ConfigureMapBank4Preset6CCD8 */
        REQUIRE(field_animation(m,19, 2, 0x088DBDA6));
        break;
    }
    case 0x0806CCEC:{ /* ConfigureMapBank4Preset6CCEC */
        REQUIRE(field_animation(m,19, 2, 0x088EDC9A));
        break;
    }
    case 0x0806CD00:{ /* ConfigureMapBank4Preset6CD00 */
        REQUIRE(field_animation(m,19, 2, 0x088DB97B));
        break;
    }
    case 0x0806CD14:{ /* ConfigureMapBank4Preset6CD14 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1364));
        REQUIRE(raw_animation(m,10, 0x088ADA36));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088ADA54));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        indicator(m,hn_read8(m,0x03002C0C), 0x088ADA36);
        REQUIRE(raw_animation(m,13, 0x088B59B5));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088B59C1));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088B59D2));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        REQUIRE(raw_animation(m,16, 0x088EDCEC));
        REQUIRE(raw_animation(m,24, 0x088EDD57));
        break;
    }
    case 0x0806CDDC:{ /* ConfigureMapBank4Preset6CDDC */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1338));
        REQUIRE(raw_animation(m,10, 0x088AD6D3));
        REQUIRE(hn_render_configure_raw(m,9, 1, 176, 0, 0, 0, 0x088AD6DF));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088AD6F0));
        REQUIRE(hn_render_modes_raw(m,9, 2, 3, 10));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        REQUIRE(raw_animation(m,13, 0x088B59E3));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088B59EF));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088B5A00));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        break;
    }
    case 0x0806CEA0:{ /* ConfigureMapBank4Preset6CEA0 */
        REQUIRE(raw_animation(m,19, 0x088EDC42));
        break;
    }
    case 0x0806CEB4:{ /* ConfigureMapBank4Preset6CEB4 */
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088DB5F3));
        break;
    }
    case 0x0806CED8:{ /* ConfigureMapBank4Preset6CED8 */
        REQUIRE(field_animation(m,19, 2, 0x088DBDE8));
        break;
    }
    case 0x0806CEEC:{ /* ConfigureMapBank4Preset6CEEC */
        REQUIRE(field_animation(m,19, 2, 0x088DB9B3));
        break;
    }
    case 0x0806CF00:{ /* ConfigureMapBank4Preset6CF00 */
        REQUIRE(field_animation(m,22, 4, 0x088EDC92));
        break;
    }
    case 0x0806CF14:{ /* ConfigureMapBank4Preset6CF14 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E138B));
        REQUIRE(field_animation(m,19, 2, 0x088DB8E1));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088DB8F6));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        break;
    }
    case 0x0806CF60:{ /* ConfigureMapBank4Preset6CF60 */
        REQUIRE(field_animation(m,19, 2, 0x088DB9DB));
        break;
    }
    case 0x0806CF74:{ /* ConfigureMapBank4Preset6CF74 */
        REQUIRE(field_animation(m,19, 2, 0x088DC13C));
        break;
    }
    case 0x0806CF88:{ /* ConfigureMapBank4Preset6CF88 */
        REQUIRE(raw_animation(m,16, 0x088C9B0C));
        REQUIRE(raw_animation(m,15, 0x088C9B21));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088C9B33));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806CFD0:{ /* ConfigureMapBank4Preset6CFD0 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(raw_animation(m,10, 0x088ADA0E));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088E3961));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        REQUIRE(raw_animation(m,13, 0x088B5611));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088E3961));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        break;
    }
    case 0x0806D048:{ /* ConfigureMapBank4Preset6D048 */
        break;
    }
    case 0x0806D04C:{ /* ConfigureMapBank4Preset6D04C */
        REQUIRE(field_animation(m,22, 4, 0x088EDC9A));
        break;
    }
    case 0x0806D060:{ /* ConfigureMapBank4Preset6D060 */
        REQUIRE(raw_animation(m,16, 0x088C9AF3));
        REQUIRE(raw_animation(m,15, 0x088C9AFF));
        break;
    }
    case 0x0806D080:{ /* ConfigureMapBank4Preset6D080 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(raw_animation(m,16, 0x088C9AF3));
        REQUIRE(raw_animation(m,15, 0x088C9AFF));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088E38F4));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806D0D8:{ /* ConfigureMapBank4Facing8Preset6D0D8 */
        uint32_t direction = (uint8_t)hn_render_direction_raw(m,16, 10,true);
        if (direction <= 4) {
            REQUIRE(raw_animation(m,16, 0x088C9BB7));
            REQUIRE(raw_animation(m,15, 0x088C9BC3));
        } else if (direction <= 6) {
            REQUIRE(raw_animation(m,16, 0x088C9AF3));
            REQUIRE(raw_animation(m,15, 0x088C9AFF));
        } else {
            REQUIRE(raw_animation(m,16, 0x088C9B9E));
            REQUIRE(raw_animation(m,15, 0x088C9BAA));
        }
        break;
    }
    case 0x0806D13C:{ /* ConfigureMapBank4Preset6D13C */
        REQUIRE(raw_animation(m,16, 0x088C9B51));
        REQUIRE(raw_animation(m,15, 0x088C9B66));
        break;
    }
    case 0x0806D15C:{ /* ConfigureMapBank4Preset6D15C */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(raw_animation(m,16, 0x088C9AF3));
        REQUIRE(raw_animation(m,15, 0x088C9AFF));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088E3961));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806D1B4:{ /* ConfigureMapBank4Preset6D1B4 */
        REQUIRE(raw_animation(m,16, 0x088C9B45));
        REQUIRE(raw_animation(m,15, 0x088C9AFF));
        break;
    }
    case 0x0806D1D4:{ /* ConfigureMapBank4Preset6D1D4 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(raw_animation(m,16, 0x088C9BD0));
        REQUIRE(raw_animation(m,15, 0x088C9BDC));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088E38F4));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806D22C:{ /* ConfigureMapBank4Preset6D22C */
        REQUIRE(raw_animation(m,16, 0x088C9BE9));
        REQUIRE(raw_animation(m,15, 0x088C9AFF));
        break;
    }
    case 0x0806D24C:{ /* ConfigureMapBank4Preset6D24C */
        REQUIRE(raw_animation(m,16, 0x088C9BF5));
        REQUIRE(raw_animation(m,15, 0x088C9C01));
        break;
    }
    case 0x0806D26C:{ /* ConfigureMapBank4Preset6D26C */
        REQUIRE(raw_animation(m,16, 0x088C9C54));
        REQUIRE(raw_animation(m,15, 0x088C9C83));
        indicator(m,16, 0x088C9C54);
        break;
    }
    case 0x0806D298:{ /* ConfigureMapBank4Preset6D298 */
        REQUIRE(raw_animation(m,16, 0x088C9C27));
        REQUIRE(raw_animation(m,15, 0x088C9C3C));
        break;
    }
    case 0x0806D2B8:{ /* ConfigureMapBank4Preset6D2B8 */
        break;
    }
    case 0x0806D2BC:{ /* ConfigureMapBank4Preset6D2BC */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C0C), 0x088AC08F));
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088B4002));
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088EDCB2));
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088EDC9A));
        REQUIRE(raw_animation(m,24, 0x088EDD1D));
        break;
    }
    case 0x0806D318:{ /* ConfigureMapBank4Preset6D318 */
        REQUIRE(hn_preset_apply(m,0x08018D9C)==HN_SCENE_OK);
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C0C), 0x088ADCE6));
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088B55BA));
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088EDCA2));
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088EDC9A));
        REQUIRE(raw_animation(m,24, 0x088EDD0D));
        break;
    }
    case 0x0806D378:{ /* ScriptCallbackSetMapBank4Half114C */
        hn_write16(m,0x0300114C,0x8000);
        break;
    }
    case 0x0806E568:{ /* MapBank4PathNoop */
        break;
    }
    case 0x0806E56C:{ /* ConfigureMapBank4Preset6E56C */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(raw_animation(m,16, 0x088C6CEE));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088C6D06));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806E5B8:{ /* ConfigureMapBank4Preset6E5B8 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088B6ADD));
        break;
    }
    case 0x0806E5D0:{ /* ConfigureMapBank4Preset6E5D0 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088B5611));
        break;
    }
    case 0x0806E5E8:{ /* ConfigureMapBank4Preset6E5E8 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C0C), 0x088ADA0E));
        break;
    }
    case 0x0806E600:{ /* ConfigureMapBank4Preset6E600 */
        REQUIRE(raw_animation(m,16, 0x088C6DC4));
        break;
    }
    case 0x0806E614:{ /* ConfigureMapBank4Preset6E614 */
        REQUIRE(raw_animation(m,16, 0x088C6DE5));
        break;
    }
    case 0x0806E684:{ /* ConfigureMapBank4Preset6E684 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(raw_animation(m,16, 0x088C6E87));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088EE142));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806E6D0:{ /* ConfigureMapBank4Preset6E6D0 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(raw_animation(m,16, 0x088C6F62));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088EE158));
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088EE16E));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x0806E740:{ /* ConfigureMapBank4Preset6E740 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E13AB));
        REQUIRE(raw_animation(m,16, 0x088C6DB8));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088C6EE0));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806E78C:{ /* ConfigureMapBank4Preset6E78C */
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088B5999));
        break;
    }
    case 0x0806E7A4:{ /* ConfigureMapBank4Preset6E7A4 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088B6A9F));
        break;
    }
    case 0x0806E7BC:{ /* ConfigureMapBank4Preset6E7BC */
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088B70A6));
        break;
    }
    case 0x0806E7D4:{ /* ConfigureMapBank4Preset6E7D4 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C0C), 0x088AF6B3));
        break;
    }
    case 0x0806E7EC:{ /* ConfigureMapBank4Preset6E7EC */
        REQUIRE(raw_animation(m,hn_read8(m,0x03001F50), 0x088B70B2));
        break;
    }
    case 0x0806E804:{ /* ClearMapBank4Slots15And17Field60Word */
        REQUIRE(full_field(m,15, 0));
        REQUIRE(full_field(m,17, 0));
        break;
    }
    case 0x0806E81C:{ /* ConfigureMapBank4Preset6E81C */
        REQUIRE(field_animation(m,hn_read8(m,0x03002C00), 2, 0x088DC148));
        indicator(m,hn_read8(m,0x03002C00), 0x088DC148);
        break;
    }
    case 0x0806E844:{ /* ConfigureMapBank4Preset6E844 */
        REQUIRE(raw_animation(m,19, 0x088DB93D));
        break;
    }
    case 0x0806E858:{ /* ConfigureMapBank4Preset6E858 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(raw_animation(m,19, 0x088DBD87));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088DBD93));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        break;
    }
    case 0x0806E8A4:{ /* ConfigureMapBank4Preset6E8A4 */
        REQUIRE(raw_animation(m,19, 0x088DB9B3));
        break;
    }
    case 0x0806E8B8:{ /* ConfigureMapBank4Preset6E8B8 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(raw_animation(m,19, 0x088DB8CA));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088DB8D6));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        break;
    }
    case 0x0806E904:{ /* ConfigureMapBank4Preset6E904 */
        REQUIRE(raw_animation(m,19, 0x088DB583));
        break;
    }
    case 0x0806E918:{ /* ConfigureMapBank4Preset6E918 */
        REQUIRE(raw_animation(m,19, 0x088DB9DB));
        break;
    }
    case 0x0806E92C:{ /* ConfigureMapBank4Preset6E92C */
        REQUIRE(raw_animation(m,19, 0x088DBA28));
        break;
    }
    case 0x0806E940:{ /* ConfigureMapBank4Preset6E940 */
        REQUIRE(raw_animation(m,19, 0x088DB99E));
        break;
    }
    case 0x0806E954:{ /* ConfigureMapBank4Preset6E954 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(raw_animation(m,19, 0x088DB8E1));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088DB8F6));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        break;
    }
    case 0x0806E9A0:{ /* ConfigureMapBank4Preset6E9A0 */
        REQUIRE(raw_animation(m,19, 0x088DC1DC));
        break;
    }
    case 0x0806E9B4:{ /* ConfigureMapBank4Preset6E9B4 */
        REQUIRE(raw_animation(m,19, 0x088DB8CA));
        break;
    }
    case 0x0806E9C8:{ /* ConfigureMapBank4Preset6E9C8 */
        indicator(m,16, 0x088C6E50);
        break;
    }
    case 0x0806E9DC:{ /* ConfigureMapBank4Preset6E9DC */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_preset_apply(m,0x08014810)==HN_SCENE_OK);
        REQUIRE(hn_preset_apply(m,0x08019EE8)==HN_SCENE_OK);
        break;
    }
    case 0x0806E9F8:{ /* ConfigureMapBank4Preset6E9F8 */
        REQUIRE(hn_preset_apply(m,0x080148D0)==HN_SCENE_OK);
        REQUIRE(hn_preset_apply(m,0x0801A684)==HN_SCENE_OK);
        break;
    }
    case 0x0806EA08:{ /* ConfigureMapBank4Preset6EA08 */
        REQUIRE(hn_preset_apply(m,0x08014AF8)==HN_SCENE_OK);
        REQUIRE(hn_preset_apply(m,0x0801A598)==HN_SCENE_OK);
        break;
    }
    case 0x0806EA18:{ /* ConfigureMapBank4Preset6EA18 */
        set_flag(m,23, 1);
        REQUIRE(hn_preset_apply(m,0x08014984)==HN_SCENE_OK);
        break;
    }
    case 0x0806EA54:{ /* ApplyMapBank4Preset6EA54 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E13FB));
        REQUIRE(animation(m,10, 0x088A4FE5));
        REQUIRE(animation(m,13, 0x088A8D07));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088E19BC));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088E19BC));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        indicator(m,10, 0x088A4FE5);
        break;
    }
    case 0x0806EAE4:{ /* ApplyMapBank4Preset6EAE4 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DFE2A));
        indicator(m,16, 0x088DFE2A);
        break;
    }
    case 0x0806EB0C:{ /* ApplyMapBank4Preset6EB0C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DFE38));
        break;
    }
    case 0x0806EB28:{ /* ApplyMapBank4Preset6EB28 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,16, 0x088DFE4D));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DFE64));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806EB7C:{ /* ApplyMapBank4Preset6EB7C */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DFE96));
        break;
    }
    case 0x0806EB98:{ /* ApplyMapBank4Preset6EB98 */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,16, 0x088DFCC9));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DFCE1));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0806EBEC:{ /* ApplyMapBank4Preset6EBEC */
        REQUIRE(hn_bank4_clear_neighbors(m,16));
        REQUIRE(animation(m,16, 0x088DFCC9));
        break;
    }
    case 0x0806EC08:{ /* ApplyMapBank4Preset6EC08 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ED004));
        indicator(m,19, 0x088ED004);
        break;
    }
    case 0x0806EC30:{ /* ApplyMapBank4Preset6EC30 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ECFF7));
        break;
    }
    case 0x0806EC4C:{ /* ApplyMapBank4Preset6EC4C */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ED038));
        break;
    }
    case 0x0806EC68:{ /* ApplyMapBank4Preset6EC68 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(animation(m,19, 0x088ECFF7));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088ED06E));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x0806ECBC:{ /* ApplyMapBank4Preset6ECBC */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ED019));
        break;
    }
    case 0x0806ECD8:{ /* ApplyMapBank4Preset6ECD8 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ED038));
        break;
    }
    case 0x0806ECF4:{ /* ApplyMapBank4Preset6ECF4 */
        REQUIRE(hn_bank4_clear_neighbors(m,10));
        REQUIRE(hn_bank4_clear_neighbors(m,13));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1364));
        REQUIRE(animation(m,10, 0x088ADA0E));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088ADA54));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        REQUIRE(animation(m,13, 0x088B4F3F));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088E1740));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        REQUIRE(field(m,10, 2));
        REQUIRE(field(m,13, 2));
        break;
    }
    case 0x0806ED90:{ /* ApplyMapBank4Preset6ED90 */
        REQUIRE(hn_bank4_clear_neighbors(m,19));
        REQUIRE(animation(m,19, 0x088ED040));
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088E19BC));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        indicator(m,19, 0x088ED040);
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank4_preset_callback(HNMemory *m,GameAddr callback,uint32_t *consumed)
{
    switch(callback&~1u){
    case 0x08067938:
    case 0x08067A10:
    case 0x08067A44:
    case 0x08067BA4:
    case 0x08067BD8:
    case 0x08067C0C:
    case 0x08067CE4:
    case 0x08067CFC:
    case 0x08067D4C:
    case 0x08067D64:
    case 0x08067D7C:
    case 0x08067DCC:
    case 0x08067E04:
    case 0x08067E38:
    case 0x08067EBC:
    case 0x08067F00:
    case 0x08067F24:
    case 0x08067F48:
    case 0x08067F8C:
    case 0x08067FD0:
    case 0x08068034:
    case 0x0806804C:
    case 0x08068080:
    case 0x080680D0:
    case 0x080680E8:
    case 0x08068100:
    case 0x08068124:
    case 0x0806815C:
    case 0x08068194:
    case 0x080681D8:
    case 0x08068228:
    case 0x08068240:
    case 0x08068258:
    case 0x080682B8:
    case 0x080682E0:
    case 0x080682F8:
    case 0x0806831C:
    case 0x08068384:
    case 0x0806839C:
    case 0x080683EC:
    case 0x08068404:
    case 0x0806841C:
    case 0x0806846C:
    case 0x080684BC:
    case 0x0806850C:
    case 0x08068524:
    case 0x08068584:
    case 0x080685B8:
    case 0x080685D0:
    case 0x080685E8:
    case 0x08068600:
    case 0x08068628:
    case 0x080686A0:
    case 0x08068718:
    case 0x08068768:
    case 0x08068794:
    case 0x08068924:
    case 0x0806893C:
    case 0x08068960:
    case 0x08068978:
    case 0x080689BC:
    case 0x08068A28:
    case 0x08068A4C:
    case 0x08068A64:
    case 0x08068A84:
    case 0x08068AD4:
    case 0x08068B08:
    case 0x08068F60:
    case 0x08068F7C:
    case 0x08068FE4:
    case 0x08069000:
    case 0x08069030:
    case 0x0806904C:
    case 0x08069090:
    case 0x080690E4:
    case 0x08069138:
    case 0x08069154:
    case 0x08069170:
    case 0x0806918C:
    case 0x080691E0:
    case 0x080691FC:
    case 0x08069218:
    case 0x0806926C:
    case 0x0806929C:
    case 0x080692A0:
    case 0x080692F4:
    case 0x08069348:
    case 0x0806934C:
    case 0x080693A0:
    case 0x080693C8:
    case 0x080693CC:
    case 0x080693D0:
    case 0x080693EC:
    case 0x0806942C:
    case 0x08069468:
    case 0x080694F8:
    case 0x08069588:
    case 0x08069618:
    case 0x0806966C:
    case 0x08069700:
    case 0x080697DC:
    case 0x08069854:
    case 0x080698F0:
    case 0x0806990C:
    case 0x08069928:
    case 0x080699B8:
    case 0x08069A48:
    case 0x08069A6C:
    case 0x08069A88:
    case 0x08069ADC:
    case 0x08069AF8:
    case 0x08069B14:
    case 0x08069B30:
    case 0x08069B74:
    case 0x08069BB0:
    case 0x08069BD4:
    case 0x08069D04:
    case 0x08069D20:
    case 0x08069D3C:
    case 0x08069D88:
    case 0x08069DA4:
    case 0x08069E10:
    case 0x08069E38:
    case 0x0806A1F4:
    case 0x0806A20C:
    case 0x0806A224:
    case 0x0806A23C:
    case 0x0806A8B4:
    case 0x0806A8EC:
    case 0x0806A924:
    case 0x0806A940:
    case 0x0806A95C:
    case 0x0806A978:
    case 0x0806A994:
    case 0x0806A998:
    case 0x0806A9B4:
    case 0x0806A9D0:
    case 0x0806A9EC:
    case 0x0806AA2C:
    case 0x0806AA48:
    case 0x0806AA88:
    case 0x0806AAA4:
    case 0x0806AAC0:
    case 0x0806AADC:
    case 0x0806AAF8:
    case 0x0806AB24:
    case 0x0806AB40:
    case 0x0806AB5C:
    case 0x0806AB84:
    case 0x0806ABA0:
    case 0x0806ABE4:
    case 0x0806AC24:
    case 0x0806AC40:
    case 0x0806AC5C:
    case 0x0806AC78:
    case 0x0806AC94:
    case 0x0806ACCC:
    case 0x0806ACE8:
    case 0x0806AD28:
    case 0x0806AD44:
    case 0x0806AD8C:
    case 0x0806ADF8:
    case 0x0806AE14:
    case 0x0806AE18:
    case 0x0806AE34:
    case 0x0806AE50:
    case 0x0806AE6C:
    case 0x0806AEB4:
    case 0x0806AED0:
    case 0x0806AF3C:
    case 0x0806B878:
    case 0x0806B894:
    case 0x0806B8DC:
    case 0x0806B8F8:
    case 0x0806B914:
    case 0x0806B930:
    case 0x0806B94C:
    case 0x0806B968:
    case 0x0806B984:
    case 0x0806B9E8:
    case 0x0806BAD4:
    case 0x0806D378:
    case 0x0806EA54:
    case 0x0806EAE4:
    case 0x0806EB0C:
    case 0x0806EB28:
    case 0x0806EB7C:
    case 0x0806EB98:
    case 0x0806EBEC:
    case 0x0806EC08:
    case 0x0806EC30:
    case 0x0806EC4C:
    case 0x0806EC68:
    case 0x0806ECBC:
    case 0x0806ECD8:
    case 0x0806ECF4:
    case 0x0806ED90:
        *consumed=0;return hn_bank4_presets(m,callback);
    default:return HN_SCENE_UNKNOWN;
    }
}
