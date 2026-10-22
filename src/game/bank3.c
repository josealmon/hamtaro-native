#include "game/bank3.h"
#include "game/presets.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/audio.h"
#include "game/world.h"
#include "game/random.h"

static bool raw_animation(HNMemory *m,uint32_t index,GameAddr resource)
{hn_render_set_animation(m,index,resource);return !m->fault;}
static bool animation(HNMemory *m,uint8_t index,GameAddr resource)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    return raw_animation(m,index,resource);
}
static void indicator(HNMemory *m,uint8_t index,GameAddr resource)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,resource);}
static bool field(HNMemory *m,uint8_t index,uint8_t value)
{hn_write8(m,hn_render_slot(m,index)+0x60,value);return !m->fault;}
static bool hide_neighbors(HNMemory *m,uint8_t index)
{return field(m,(uint8_t)(index-1),0)&&field(m,(uint8_t)(index+1),0);}
static bool configure_auxiliary(HNMemory *m,uint8_t index,uint8_t parent,GameAddr resource)
{return hn_render_configure_raw(m,index,1,0xB0,0,0,0,resource)&&hn_render_modes_raw(m,index,2,3,parent);}
static bool cell_position(HNMemory *m,uint8_t index,int32_t x,int32_t y,int32_t z)
{
    GameAddr slot=hn_render_slot(m,index);
    hn_write32(m,slot+0x18,((uint32_t)(uint16_t)x<<16)+0x8000);
    hn_write32(m,slot+0x1C,((uint32_t)(uint16_t)y<<16)+0x8000);
    hn_write32(m,slot+0x20,((uint32_t)(uint16_t)z<<16)+0x8000);return !m->fault;
}
static bool field_animation(HNMemory *m,uint8_t index,uint8_t value,GameAddr resource)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    return field(m,index,value)&&raw_animation(m,index,resource);
}
static bool call_scene(HNMemory *m,GameAddr callback,GameAddr arguments)
{uint32_t ignored;return hn_scene_callback(m,callback,arguments,&ignored)==HN_SCENE_OK;}

/* Complete late-scene void family. Resource words name the player's ROM. */
static HNSceneResult late_presets(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x08050FC8:{ /* SetMapBank3Animation50FC8 */
        if(!animation(m,hn_read8(m,0x03002C04), 0x088EB057))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08050FE0:{ /* SetMapBank3Animation50FE0 */
        if(!animation(m,hn_read8(m,0x03002C04), 0x088EB05F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08050FF8:{ /* ConfigureMapBank3LatePreset50FF8 */
        if(!animation(m,hn_read8(m,0x03002C04), 0x088EB080))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04), 0x088EB080);
        break;
    }
    case 0x0805101C:{ /* SetMapBank3Animation5101C */
        if(!animation(m,hn_read8(m,0x03002C04), 0x088EB094))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051034:{ /* SetMapBank3Animation51034 */
        if(!animation(m,hn_read8(m,0x03002C04), 0x088EB09C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0805104C:{ /* ConfigureMapBank3LatePreset5104C */
        if(!animation(m,hn_read8(m,0x03002C04), 0x088EB0BD))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04), 0x088EB0BD);
        break;
    }
    case 0x08051070:{ /* ConfigureMapBank3LatePreset51070 */
        if(!hn_render_configure_raw(m,17, 1, 114, 104, 0, 8, 0x088EB0D1))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,18, 1, 114, 128, 0, 8, 0x088EB0D1))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,19, 1, 114, 88, 0, 32, 0x088EB109))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,20, 1, 114, 144, 0, 32, 0x088EB109))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,17);
        hn_write32(m,0x03001F7C,0x088EB0D1);
        if(!hn_graphics_configure_raw(m,4,0x088E139B))return HN_SCENE_RESOURCE;
        (void)hn_audio_song(m,0x3D,HN_SONG_START);
        break;
    }
    case 0x080511AC:{ /* MapBank3NoOp511AC */
        break;
    }
    case 0x080511B0:{ /* ConfigureMapBank3LatePreset511B0 */
        if(hn_preset_apply(m,0x08015F1C)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801AE10)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(late_presets(m,0x08051640)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080511C4:{ /* ConfigureMapBank3LatePreset511C4 */
        if(hn_preset_apply(m,0x080162B8)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801AC78)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CA000))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,15, 2, 3, 16))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,0x088E13B3))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,5,0x088CA066))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051218:{ /* ConfigureMapBank3LatePreset51218 */
        if(!raw_animation(m,10, 0x088EB799))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,11, 0x088EB7C4))return HN_SCENE_RESOURCE;
        if(!field(m,11, 1))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,9, 1, 176, 0, 0, 0, 0x088CA000))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,9, 2, 3, 10))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,0x088E13BB))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,5,0x088CA066))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051284:{ /* ConfigureMapBank3LatePreset51284 */
        if(hn_preset_apply(m,0x08015F64)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801AE98)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088CA000))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,18, 2, 3, 19))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,0x088E13C3))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,5,0x088CA066))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080512D8:{ /* ConfigureMapBank3LatePreset512D8 */
        if(hn_preset_apply(m,0x08015EA4)!=HN_SCENE_OK)return HN_SCENE_RESOURCE; if(hn_preset_apply(m,0x0801AD44)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080512E8:{ /* ConfigureMapBank3LatePreset512E8 */
        if(hn_preset_apply(m,0x08015EEC)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801AD88)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(late_presets(m,0x080515E8)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080512FC:{ /* ConfigureMapBank3LatePreset512FC */
        if(hn_preset_apply(m,0x08016FEC)!=HN_SCENE_OK)return HN_SCENE_RESOURCE; if(hn_preset_apply(m,0x0801B720)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0805130C:{ /* ConfigureMapBank3LatePreset5130C */
        if(hn_preset_apply(m,0x0801707C)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801B7A8)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088CA000))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,15, 2, 3, 16))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,0x088E13B3))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,5,0x088CA066))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051360:{ /* ConfigureMapBank3LatePreset51360 */
        if(hn_preset_apply(m,0x080170AC)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801B7EC)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(late_presets(m,0x080515E8)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051374:{ /* ConfigureMapBank3LatePreset51374 */
        if(hn_preset_apply(m,0x080170C4)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801B830)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(late_presets(m,0x08051640)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051388:{ /* ConfigureMapBank3LatePreset51388 */
        if(!raw_animation(m,13, 0x088EB80B))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,14, 0x088EB83E))return HN_SCENE_RESOURCE;
        if(!field(m,14, 1))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088CA000))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,12, 2, 3, 13))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,0x088E13CB))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,5,0x088CA066))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080513F4:{ /* ConfigureMapBank3LatePreset513F4 */
        if(!raw_animation(m,22, 0x088EB885))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,23, 0x088EB8B0))return HN_SCENE_RESOURCE;
        if(!field(m,23, 1))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088CA000))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,21, 2, 3, 22))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,0x088E13D3))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,5,0x088CA066))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051460:{ /* ConfigureMapBank3LatePreset51460 */
        if(hn_preset_apply(m,0x0801707C)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801B7A8)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(late_presets(m,0x08051698)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,0x088E1412))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,27, 1, 0, 0, 0, 0, 0x088E33FB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088EB8DD))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,29, 1, 176, 0, 12, -16, 0x088E33EA))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,29, 2, 3, 16))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,30, 1, 176, -64, 12, -32, 0x088E33EA))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,30, 2, 3, 16))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,31, 1, 176, 64, 12, -32, 0x088E33EA))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,31, 2, 3, 16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051524:{ /* ConfigureMapBank3LatePreset51524 */
        if(hn_preset_apply(m,0x080162B8)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801AC78)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(late_presets(m,0x08051698)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,0x088E1412))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,27, 1, 0, 0, 0, 0, 0x088E33FB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088EB8DD))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,29, 1, 176, 0, 12, -16, 0x088E33EA))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,29, 2, 3, 16))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,30, 1, 176, -64, 12, -32, 0x088E33EA))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,30, 2, 3, 16))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,31, 1, 176, 64, 12, -32, 0x088E33EA))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,31, 2, 3, 16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080515E8:{ /* ConfigureMapBank3LatePreset515E8 */
        if(hn_preset_apply(m,0x08015F04)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801ADCC)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(!raw_animation(m,10, 0x088EB799))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,11, 0x088EB7D2))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,13, 0x088EB813))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,14, 0x088EB84C))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,22, 0x088EB885))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,23, 0x088EB8BE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051640:{ /* ConfigureMapBank3LatePreset51640 */
        if(hn_preset_apply(m,0x08015F34)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801AE54)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(!raw_animation(m,10, 0x088EB7A1))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,11, 0x088EB7E0))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,13, 0x088EB81B))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,14, 0x088EB85A))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,22, 0x088EB88D))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,23, 0x088EB8CC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08051698:{ /* ConfigureMapBank3LatePreset51698 */
        if(hn_preset_apply(m,0x08015F64)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(hn_preset_apply(m,0x0801AE98)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
        if(!raw_animation(m,10, 0x088EB799))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,11, 0x088EB7C4))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,13, 0x088EB80B))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,14, 0x088EB83E))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,22, 0x088EB885))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,23, 0x088EB8B0))return HN_SCENE_RESOURCE;
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}

#define BANK3_REQUIRE(expression) do {if(!(expression))return HN_SCENE_RESOURCE;} while(0)
static HNSceneResult main_presets(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x0804B4B0:{ /* SetMapBank3PairIndicator4B4B0 */
        GameAddr resource = (GameAddr)0x088A53A1;
        BANK3_REQUIRE(animation(m,10, resource));
        BANK3_REQUIRE(animation(m,13, (GameAddr)0x088A92EC));
        indicator(m,10, resource);
        break;
    }
    case 0x0804B4DC:{ /* SetMapBank3PairIndicator4B4DC */
        GameAddr resource = (GameAddr)0x088A540C;
        BANK3_REQUIRE(animation(m,10, resource));
        BANK3_REQUIRE(animation(m,13, (GameAddr)0x088A9354));
        indicator(m,10, resource);
        break;
    }
    case 0x0804B508:{ /* ConfigureMapBank3Auxiliary4B508 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08015564)==HN_SCENE_OK);
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1D3) == 4)
            BANK3_REQUIRE(configure_auxiliary(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), hn_read8(m,0x03002C04), (GameAddr)0x088CA357));
        break;
    }
    case 0x0804B540:{ /* ConfigureMapBank3Auxiliary4B540 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08015AE4)==HN_SCENE_OK);
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1D3) == 4) {
            BANK3_REQUIRE(configure_auxiliary(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), hn_read8(m,0x03002C04), (GameAddr)0x088CA35F));
            BANK3_REQUIRE(field(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 0));
        }
        break;
    }
    case 0x0804B588:{ /* ConfigureMapBank3Auxiliary4B588 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08015B44)==HN_SCENE_OK);
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1D3) == 4)
            BANK3_REQUIRE(configure_auxiliary(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), hn_read8(m,0x03002C04), (GameAddr)0x088CA3AB));
        break;
    }
    case 0x0804B5C0:{ /* ConfigureMapBank3Auxiliary4B5C0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08015B74)==HN_SCENE_OK);
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1D3) == 4) {
            BANK3_REQUIRE(configure_auxiliary(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), hn_read8(m,0x03002C04), (GameAddr)0x088CA31C));
            BANK3_REQUIRE(cell_position(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 0, -1, 1));
        }
        break;
    }
    case 0x0804B610:{ /* SetMapBank3Class2Indicator */
        GameAddr index = 0x03002C04;
        GameAddr resource;
        BANK3_REQUIRE(animation(m,hn_read8(m,index), resource = (GameAddr)0x088CD3FC));
        indicator(m,hn_read8(m,index), resource);
        break;
    }
    case 0x0804B6E8:{ /* EmptyMapBank3Callback4B6E8 */

        break;
    }
    case 0x0804B6EC:{ /* EmptyMapBank3Callback4B6EC */

        break;
    }
    case 0x0804B6F0:{ /* EmptyMapBank3Callback4B6F0 */

        break;
    }
    case 0x0804B6F4:{ /* EmptyMapBank3Callback4B6F4 */

        break;
    }
    case 0x0804B6F8:{ /* EmptyMapBank3Callback4B6F8 */

        break;
    }
    case 0x0804B6FC:{ /* EmptyMapBank3Callback4B6FC */

        break;
    }
    case 0x0804B700:{ /* EmptyMapBank3Callback4B700 */

        break;
    }
    case 0x0804B704:{ /* EmptyMapBank3Callback4B704 */

        break;
    }
    case 0x0804B708:{ /* EmptyMapBank3Callback4B708 */

        break;
    }
    case 0x0804B70C:{ /* EmptyMapBank3Callback4B70C */

        break;
    }
    case 0x0804B710:{ /* EmptyMapBank3Callback4B710 */

        break;
    }
    case 0x0804B714:{ /* EmptyMapBank3Callback4B714 */

        break;
    }
    case 0x0804B718:{ /* EmptyMapBank3Callback4B718 */

        break;
    }
    case 0x0804B71C:{ /* EmptyMapBank3Callback4B71C */

        break;
    }
    case 0x0804B720:{ /* EmptyMapBank3Callback4B720 */

        break;
    }
    case 0x0804B724:{ /* EmptyMapBank3Callback4B724 */

        break;
    }
    case 0x0804B728:{ /* EmptyMapBank3Callback4B728 */

        break;
    }
    case 0x0804B72C:{ /* EmptyMapBank3Callback4B72C */

        break;
    }
    case 0x0804B730:{ /* StartMapBank3Song60 */
        BANK3_REQUIRE(hn_audio_song(m,60,HN_SONG_START));
        break;
    }
    case 0x0804B73C:{ /* EmptyMapBank3Callback4B73C */

        break;
    }
    case 0x0804B740:{ /* ConfigureMapBank3Slot15_4B740 */
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088CA0F6));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0804B770:{ /* ConfigureMapBank3Slot15_4B770 */
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088CA11E));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0804B7A0:{ /* ConfigureMapBank3Slot15_4B7A0 */
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088CA0D3));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0804B7D0:{ /* ConfigureMapBank3Slot15_4B7D0 */
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088CA0B7));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0804B800:{ /* ConfigureMapBank3Slot15_4B800 */
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088CA131));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0804B830:{ /* ConfigureMapBank3Slot15_4B830 */
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088CA13F));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0804CB60:{ /* EmptyMapBank3Callback4CB60 */

        break;
    }
    case 0x0804CB64:{ /* SetMapBank3Slot16Preset4CB64 */
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088D12AE));
        break;
    }
    case 0x0804CB78:{ /* ConfigureMapBank3Slot15Linked16 */
        GameAddr resource = (GameAddr)0x088D13CC;
        BANK3_REQUIRE(animation(m,16, resource));
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088E18D8));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,(uint32_t)resource);
        break;
    }
    case 0x0804CBCC:{ /* SetMapBank3Slot16Indicator4CBCC */
        GameAddr resource = (GameAddr)0x088D142E;
        BANK3_REQUIRE(animation(m,16, resource));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,(uint32_t)resource);
        break;
    }
    case 0x0804CBF4:{ /* SetMapBank3Slot16Indicator4CBF4 */
        GameAddr resource = (GameAddr)0x088D143A;
        BANK3_REQUIRE(animation(m,16, resource));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,(uint32_t)resource);
        break;
    }
    case 0x0804CC1C:{ /* ConfigureMapBank3Slot17_4CC1C */
        BANK3_REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, (GameAddr)0x088CA40D));
        BANK3_REQUIRE(hn_render_modes_raw(m,17, 2, 3, 19));
        break;
    }
    case 0x0804CC4C:{ /* ConfigureMapBank3Slot17_4CC4C */
        BANK3_REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, (GameAddr)0x088CA3C0));
        BANK3_REQUIRE(hn_render_modes_raw(m,17, 2, 3, 19));
        break;
    }
    case 0x0804CC7C:{ /* ConfigureMapBank3Slot17_4CC7C */
        BANK3_REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, (GameAddr)0x088CA3CD));
        BANK3_REQUIRE(hn_render_modes_raw(m,17, 2, 3, 19));
        break;
    }
    case 0x0804CCAC:{ /* ConfigureMapBank3Slot17_4CCAC */
        BANK3_REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, (GameAddr)0x088CA405));
        BANK3_REQUIRE(hn_render_modes_raw(m,17, 2, 3, 19));
        break;
    }
    case 0x0804D0C4:{ /* ScriptCallbackSetMapBank3PairPreset4D0C4 */
        GameAddr resource;
        BANK3_REQUIRE(hide_neighbors(m,10));
        BANK3_REQUIRE(hide_neighbors(m,13));
        resource = (GameAddr)0x088A575C;
        BANK3_REQUIRE(animation(m,10, resource));
        BANK3_REQUIRE(animation(m,13, (GameAddr)0x088A96CB));
        indicator(m,10, resource);
        break;
    }
    case 0x0804D0FC:{ /* ScriptCallbackSetMapBank3Slot16Preset4D0FC */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088E0116));
        break;
    }
    case 0x0804D118:{ /* ScriptCallbackSetMapBank3Slot16Preset4D118 */
        GameAddr resource;
        BANK3_REQUIRE(hide_neighbors(m,16));
        resource = (GameAddr)0x088E00EA;
        BANK3_REQUIRE(animation(m,16, resource));
        indicator(m,16, resource);
        break;
    }
    case 0x0804D140:{ /* ScriptCallbackSetMapBank3Slot16Preset4D140 */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088E00DE));
        break;
    }
    case 0x0804D15C:{ /* ScriptCallbackSetMapBank3Slot16Preset4D15C */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088E004C));
        break;
    }
    case 0x0804D178:{ /* ScriptCallbackSetMapBank3Slot16Preset4D178 */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088E0085));
        break;
    }
    case 0x0804D194:{ /* ScriptCallbackConfigureMapBank3Slot24_4D194 */
        BANK3_REQUIRE(hn_render_configure_raw(m,24, 1, 114, 208, 156, 0, (GameAddr)0x088E8809));
        break;
    }
    case 0x0804D1BC:{ /* ScriptCallbackHideMapBank3Slot24 */
        BANK3_REQUIRE(field(m,24, 0));
        break;
    }
    case 0x0804D1CC:{ /* ScriptCallbackSetMapBank3Slot16Preset4D1CC */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088DFFFB));
        break;
    }
    case 0x0804D1E8:{ /* ScriptCallbackSetMapBank3Slot16Preset4D1E8 */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088E0091));
        break;
    }
    case 0x0804D204:{ /* ScriptCallbackSetMapBank3Slot16Preset4D204 */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088E009D));
        break;
    }
    case 0x0804D220:{ /* ScriptCallbackConfigureMapBank3Slot15_4D220 */
        GameAddr resource;
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1340));
        resource = (GameAddr)0x088E00A9;
        BANK3_REQUIRE(animation(m,16, resource));
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088E16F9));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        indicator(m,16, resource);
        break;
    }
    case 0x0804D280:{ /* ScriptCallbackConfigureMapBank3Slot24_4D280 */
        BANK3_REQUIRE(hn_render_configure_raw(m,24, 1, 114, 224, 104, 0, (GameAddr)0x088E8809));
        break;
    }
    case 0x0804D2A8:{ /* ScriptCallbackSetMapBank3Slot16Preset4D2A8 */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088E01AA));
        break;
    }
    case 0x0804D2C4:{ /* ScriptCallbackConfigureMapBank3Slot15_4D2C4 */
        BANK3_REQUIRE(hide_neighbors(m,16));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088DFE4D));
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088DFE64));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0804D318:{ /* ScriptCallbackSetMapBank3Slot16Preset4D318 */
        GameAddr resource;
        BANK3_REQUIRE(hide_neighbors(m,16));
        resource = (GameAddr)0x088E0010;
        BANK3_REQUIRE(animation(m,16, resource));
        indicator(m,16, resource);
        break;
    }
    case 0x0804D340:{ /* ScriptCallbackConfigureMapBank3SlotsExcept3 */
        GameAddr resource;
        uint32_t zero;
        BANK3_REQUIRE(field(m,3, 0));
        zero = 0;
        BANK3_REQUIRE(hn_render_configure_raw(m,4, 1, 0, 0, zero, zero, resource = (GameAddr)0x0889F620));
        BANK3_REQUIRE(hn_render_configure_raw(m,5, 1, 0, 0, zero, zero, resource));
        BANK3_REQUIRE(hn_render_configure_raw(m,6, 1, 0, 0, zero, zero, resource));
        BANK3_REQUIRE(hn_render_configure_raw(m,7, 1, 0, 0, zero, zero, resource));
        break;
    }
    case 0x0804D3A8:{ /* ScriptCallbackConfigureMapBank3SlotsExcept4 */
        GameAddr resource;
        uint32_t zero;
        zero = 0;
        BANK3_REQUIRE(hn_render_configure_raw(m,3, 1, 0, 0, zero, zero, resource = (GameAddr)0x0889F620));
        BANK3_REQUIRE(field(m,4, 0));
        BANK3_REQUIRE(hn_render_configure_raw(m,5, 1, 0, 0, zero, zero, resource));
        BANK3_REQUIRE(hn_render_configure_raw(m,6, 1, 0, 0, zero, zero, resource));
        BANK3_REQUIRE(hn_render_configure_raw(m,7, 1, 0, 0, zero, zero, resource));
        break;
    }
    case 0x0804D410:{ /* ScriptCallbackConfigureMapBank3SlotsExcept5 */
        GameAddr resource;
        uint32_t zero;
        zero = 0;
        BANK3_REQUIRE(hn_render_configure_raw(m,3, 1, 0, 0, zero, zero, resource = (GameAddr)0x0889F620));
        BANK3_REQUIRE(hn_render_configure_raw(m,4, 1, 0, 0, zero, zero, resource));
        BANK3_REQUIRE(field(m,5, 0));
        BANK3_REQUIRE(hn_render_configure_raw(m,6, 1, 0, 0, zero, zero, resource));
        BANK3_REQUIRE(hn_render_configure_raw(m,7, 1, 0, 0, zero, zero, resource));
        break;
    }
    case 0x0804D478:{ /* ScriptCallbackConfigureMapBank3SlotsExcept6 */
        GameAddr resource;
        uint32_t zero;
        zero = 0;
        BANK3_REQUIRE(hn_render_configure_raw(m,3, 1, 0, 0, zero, zero, resource = (GameAddr)0x0889F620));
        BANK3_REQUIRE(hn_render_configure_raw(m,4, 1, 0, 0, zero, zero, resource));
        BANK3_REQUIRE(hn_render_configure_raw(m,5, 1, 0, 0, zero, zero, resource));
        BANK3_REQUIRE(field(m,6, 0));
        BANK3_REQUIRE(hn_render_configure_raw(m,7, 1, 0, 0, zero, zero, resource));
        break;
    }
    case 0x0804DBDC:{ /* ResetMapBank3ChainTimer1C4 */
        hn_write8(m,HN_SCENE_CONTROLS+0x1C4,(uint8_t)(hn_random_range(m,60, 180)));
        break;
    }
    case 0x0804DBFC:{ /* ResetMapBank3ChainTimer1C5 */
        hn_write8(m,HN_SCENE_CONTROLS+0x1C5,(uint8_t)(hn_random_range(m,60, 180)));
        break;
    }
    case 0x0804EF98:{ /* EndMapBank3ChainFlagGraphics */
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088D142E));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,3, 0, 0));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 0, 0));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,5, 0, 0));
        break;
    }
    case 0x0804EFC8:{ /* MapBank3NoOp4EFC8 */

        break;
    }
    case 0x0804EFCC:{ /* ConfigureMapBank3RenderPreset4EFCC */
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088D14A7));
        BANK3_REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, (GameAddr)0x088E19EB));
        BANK3_REQUIRE(hn_render_modes_raw(m,14, 2, 3, 16));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088D14A7);
        break;
    }
    case 0x0804F020:{ /* ConfigureMapBank3RenderPreset4F020 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,1, 1, 0x088E902A));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,2, 1, 0x088E9058));
        BANK3_REQUIRE(hn_render_configure_raw(m,31, 1, 114, 0, 0, 0, (GameAddr)0x088E92A8));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,3, 1, 0x088E92C7));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E92D9));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,5, 1, 0x088E92EE));
        hn_write8(m,0x03001FAC,31);
        hn_write32(m,0x03001F7C,0x088E92A8);
        break;
    }
    case 0x0804F0A0:{ /* ConfigureMapBank3RenderPreset4F0A0 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,1, 1, 0x088E902A));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,2, 1, 0x088E9058));
        BANK3_REQUIRE(hn_render_configure_raw(m,31, 1, 114, 0, 0, 0, (GameAddr)0x088E92A8));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,3, 1, 0x088E9303));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E9315));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,5, 1, 0x088E932A));
        hn_write8(m,0x03001FAC,31);
        hn_write32(m,0x03001F7C,0x088E92A8);
        break;
    }
    case 0x0804F120:{ /* ConfigureMapBank3RenderPreset4F120 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,1, 1, 0x088E902A));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,2, 1, 0x088E9058));
        BANK3_REQUIRE(hn_render_configure_raw(m,31, 1, 114, 0, 0, 0, (GameAddr)0x088E92A8));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,3, 1, 0x088E933F));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E9354));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,5, 0, 0));
        hn_write8(m,0x03001FAC,31);
        hn_write32(m,0x03001F7C,0x088E92A8);
        break;
    }
    case 0x0804F19C:{ /* MapBank3NoOp4F19C */

        break;
    }
    case 0x0804F1A0:{ /* SetMapBank3Slot22DirectionAnimation4F1A0 */
        int32_t direction = (uint8_t)hn_render_direction_raw(m,22, hn_read8(m,0x03002C0C),false);
        switch (direction) {
        case 4: BANK3_REQUIRE(raw_animation(m,22, (GameAddr)0x088E9561)); break;
        case 1: BANK3_REQUIRE(raw_animation(m,22, (GameAddr)0x088E9559)); break;
        case 2: BANK3_REQUIRE(raw_animation(m,22, (GameAddr)0x088E9551)); break;
        }
        break;
    }
    case 0x0804F1F0:{ /* SetMapBank3Animation4F1F0 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E9559));
        break;
    }
    case 0x0804F204:{ /* SetMapBank3Animation4F204 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E9561));
        break;
    }
    case 0x0804F218:{ /* SetMapBank3Slot22DirectionAnimation4F218 */
        int32_t direction = (uint8_t)hn_render_direction_raw(m,22, hn_read8(m,0x03002C0C),false);
        switch (direction) {
        case 4: BANK3_REQUIRE(raw_animation(m,22, (GameAddr)0x088E9571)); break;
        case 2: BANK3_REQUIRE(raw_animation(m,22, (GameAddr)0x088E9569)); break;
        }
        break;
    }
    case 0x0804F254:{ /* SetMapBank3Animation4F254 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E9569));
        break;
    }
    case 0x0804F268:{ /* SetMapBank3Animation4F268 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E9571));
        break;
    }
    case 0x0804F27C:{ /* ApplyMapBank3RenderPreset4F27C */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E9579));
        hn_write8(m,0x03001FAC,22);
        hn_write32(m,0x03001F7C,0x088E9579);
        break;
    }
    case 0x0804F2A4:{ /* SetMapBank3Animation4F2A4 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E958B));
        break;
    }
    case 0x0804F2B8:{ /* SetMapBank3Animation4F2B8 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E958B));
        break;
    }
    case 0x0804F2CC:{ /* SetMapBank3Animation4F2CC */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E95A0));
        break;
    }
    case 0x0804F2E0:{ /* SetMapBank3Animation4F2E0 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E95A8));
        break;
    }
    case 0x0804F2F4:{ /* SetMapBank3Animation4F2F4 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088E95B0));
        break;
    }
    case 0x0804F308:{ /* ApplyMapBank3RenderPreset4F308 */
        BANK3_REQUIRE(animation(m,19, (GameAddr)0x088D9413));
        hn_write8(m,0x03001FAC,19);
        hn_write32(m,0x03001F7C,0x088D9413);
        break;
    }
    case 0x0804F330:{ /* SetMapBank3Animation4F330 */
        BANK3_REQUIRE(animation(m,25, (GameAddr)0x088E950C));
        break;
    }
    case 0x0804F344:{ /* SetMapBank3Animation4F344 */
        BANK3_REQUIRE(animation(m,25, (GameAddr)0x088E9524));
        break;
    }
    case 0x0804F358:{ /* SetMapBank3Animation4F358 */
        BANK3_REQUIRE(animation(m,25, (GameAddr)0x088E952C));
        break;
    }
    case 0x0804F36C:{ /* SetMapBank3Animation4F36C */
        BANK3_REQUIRE(animation(m,25, (GameAddr)0x088E9534));
        break;
    }
    case 0x0804F380:{ /* SetMapBank3Animation4F380 */
        BANK3_REQUIRE(animation(m,25, (GameAddr)0x088E953C));
        break;
    }
    case 0x0804F394:{ /* SetMapBank3Animation4F394 */
        BANK3_REQUIRE(animation(m,25, (GameAddr)0x088E9544));
        break;
    }
    case 0x0804F3A8:{ /* ApplyMapBank3RenderPreset4F3A8 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08012F58)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08013324)==HN_SCENE_OK);
        break;
    }
    case 0x0804F3B8:{ /* ConfigureMapBank3RenderPreset4F3B8 */
        BANK3_REQUIRE(hn_render_configure_raw(m,24, 1, 176, 0, 0, 0, (GameAddr)0x088E9422));
        BANK3_REQUIRE(hn_render_modes_raw(m,24, 2, 3, 25));
        break;
    }
    case 0x0804F3E8:{ /* ConfigureMapBank3RenderPreset4F3E8 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        BANK3_REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, (GameAddr)0x088CC423));
        BANK3_REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        break;
    }
    case 0x0804F428:{ /* ConfigureMapBank3RenderPreset4F428 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        BANK3_REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, (GameAddr)0x088E3920));
        BANK3_REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        break;
    }
    case 0x0804F468:{ /* ConfigureMapBank3RenderPreset4F468 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1364));
        BANK3_REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, (GameAddr)0x088E39A9));
        BANK3_REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        break;
    }
    case 0x0804F4A8:{ /* ConfigureMapBank3RenderPreset4F4A8 */
        BANK3_REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, (GameAddr)0x088D2700));
        BANK3_REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        BANK3_REQUIRE(hn_render_configure_raw(m,23, 1, 176, 0, 0, 0, (GameAddr)0x088D272D));
        BANK3_REQUIRE(hn_render_modes_raw(m,23, 2, 3, 22));
        break;
    }
    case 0x0804F500:{ /* ConfigureMapBank3RenderPreset4F500 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        BANK3_REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, (GameAddr)0x088CC423));
        BANK3_REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088CC40C);
        break;
    }
    case 0x0804F558:{ /* ConfigureMapBank3RenderPreset4F558 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        BANK3_REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, (GameAddr)0x088E3961));
        BANK3_REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        break;
    }
    case 0x0804F598:{ /* ApplyMapBank3RenderPreset4F598 */
        BANK3_REQUIRE(field(m,21, 0));
        BANK3_REQUIRE(field(m,23, 0));
        break;
    }
    case 0x0804F5E4:{ /* SetMapBank3Class3ControlDirection */
        int32_t direction;
        BANK3_REQUIRE(call_scene(m,0x0801C634,0x03002C0C));
        direction = hn_read8(m,HN_SCENE_CONTROLS+0);
        switch (direction) {
        case 2: BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088CA426)); break;
        case 1: BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088CA41A)); break;
        case 4: BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088CA432)); break;
        case 3: BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088CA43E)); break;
        }
        break;
    }
    case 0x0804F668:{ /* SetMapBank3Class2ControlDirection */
        int32_t direction;
        BANK3_REQUIRE(call_scene(m,0x0801C634,0x03002C0C));
        direction = hn_read8(m,HN_SCENE_CONTROLS+0);
        switch (direction) {
        case 2: BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088CA426)); break;
        case 1: BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088CA41A)); break;
        case 4: BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088CA432)); break;
        case 3: BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088CA43E)); break;
        }
        break;
    }
    case 0x0804F6EC:{ /* ConfigureMapBank3RenderPreset4F6EC */
        BANK3_REQUIRE(hn_preset_apply(m,0x08015D5C)==HN_SCENE_OK);
        indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088CD247);
        break;
    }
    case 0x0804F708:{ /* MapBank3NoOp4F708 */

        break;
    }
    case 0x0804F70C:{ /* ConfigureMapBank3RenderPreset4F70C */
        uint8_t auxiliary;
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) + 1);
        BANK3_REQUIRE(configure_auxiliary(m,auxiliary, hn_read8(m,0x03002C04), (GameAddr)0x088CA16A));
        break;
    }
    case 0x0804F72C:{ /* ConfigureMapBank3AuxiliaryByAnimation */
        uint8_t index = hn_read8(m,0x03002C04);
        if (hn_read32(m,hn_render_slot(m,index)+4) == 0x088CBEEA)
            BANK3_REQUIRE(configure_auxiliary(m,(uint8_t)(index + 1), index, (GameAddr)0x088CA1D0));
        else
            BANK3_REQUIRE(configure_auxiliary(m,(uint8_t)(index + 1), index, (GameAddr)0x088CA1DD));
        BANK3_REQUIRE(cell_position(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 0, -1, 1));
        break;
    }
    case 0x0804F794:{ /* ConfigureMapBank3RenderPreset4F794 */
        uint8_t auxiliary;
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 1, 176, 0, 0, 0, (GameAddr)0x088E9691));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) + 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0804F7D8:{ /* ConfigureMapBank3RenderPreset4F7D8 */
        uint8_t auxiliary;
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 1, 176, 0, 0, 0, (GameAddr)0x088E969F));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) + 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0804F81C:{ /* MapBank3NoOp4F81C */

        break;
    }
    case 0x0804F820:{ /* ConfigureMapBank3RenderPreset4F820 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08014124)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08019C04)==HN_SCENE_OK);
        BANK3_REQUIRE(main_presets(m,0x0804F794)==HN_SCENE_OK);
        break;
    }
    case 0x0804F838:{ /* ApplyMapBank3RenderPreset4F838 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801413C)==HN_SCENE_OK);
        BANK3_REQUIRE(main_presets(m,0x0804F794)==HN_SCENE_OK);
        break;
    }
    case 0x0804F84C:{ /* ConfigureMapBank3RenderPreset4F84C */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08014154)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08019728)==HN_SCENE_OK);
        BANK3_REQUIRE(main_presets(m,0x0804F794)==HN_SCENE_OK);
        break;
    }
    case 0x0804F864:{ /* ApplyMapBank3RenderPreset4F864 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801416C)==HN_SCENE_OK);
        BANK3_REQUIRE(main_presets(m,0x0804F794)==HN_SCENE_OK);
        break;
    }
    case 0x0804F878:{ /* ApplyMapBank3RenderPreset4F878 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08014184)==HN_SCENE_OK);
        BANK3_REQUIRE(main_presets(m,0x0804F794)==HN_SCENE_OK);
        break;
    }
    case 0x0804F88C:{ /* ApplyMapBank3RenderPreset4F88C */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801419C)==HN_SCENE_OK);
        BANK3_REQUIRE(main_presets(m,0x0804F794)==HN_SCENE_OK);
        break;
    }
    case 0x0804F8A0:{ /* ConfigureMapBank3RenderPreset4F8A0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080141B4)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08019C04)==HN_SCENE_OK);
        break;
    }
    case 0x0804F8B4:{ /* ApplyMapBank3RenderPreset4F8B4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080141CC)==HN_SCENE_OK);
        break;
    }
    case 0x0804F8C4:{ /* ConfigureMapBank3RenderPreset4F8C4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080141E4)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080197D0)==HN_SCENE_OK);
        break;
    }
    case 0x0804F8D8:{ /* ConfigureMapBank3RenderPreset4F8D8 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080141FC)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080199B8)==HN_SCENE_OK);
        break;
    }
    case 0x0804F8EC:{ /* ConfigureMapBank3RenderPreset4F8EC */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08014244)==HN_SCENE_OK);
        BANK3_REQUIRE(main_presets(m,0x0804F7D8)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080199B8)==HN_SCENE_OK);
        break;
    }
    case 0x0804F904:{ /* ConfigureMapBank3RenderPreset4F904 */
        uint8_t auxiliary;
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801425C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 1, 176, 0, 0, 0, (GameAddr)0x088C8275));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) + 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0804F960:{ /* ConfigureMapBank3RenderPreset4F960 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08014274)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08019E40)==HN_SCENE_OK);
        break;
    }
    case 0x0804F974:{ /* ConfigureMapBank3RenderPreset4F974 */
        uint8_t auxiliary;
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,5, 1, 0x088E1330));
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088C845D));
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 1, 176, 0, 0, 0, (GameAddr)0x088C84F6));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) + 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0804F9E4:{ /* ConfigureMapBank3RenderPreset4F9E4 */
        uint8_t auxiliary;
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080142A4)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 1, 176, 0, 0, 0, (GameAddr)0x088C82E5));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) + 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0804FA40:{ /* ApplyMapBank3RenderPreset4FA40 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080142BC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FA50:{ /* ConfigureMapBank3RenderPreset4FA50 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080142D4)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08019C9C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FA64:{ /* ApplyMapBank3RenderPreset4FA64 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08014304)==HN_SCENE_OK);
        break;
    }
    case 0x0804FA74:{ /* ApplyMapBank3RenderPreset4FA74 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08014334)==HN_SCENE_OK);
        break;
    }
    case 0x0804FA84:{ /* ApplyMapBank3RenderPreset4FA84 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801422C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FA94:{ /* ConfigureMapBank3RenderPreset4FA94 */
        BANK3_REQUIRE(hn_preset_apply(m,0x080190C0)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08016060)==HN_SCENE_OK);
        break;
    }
    case 0x0804FAA4:{ /* ConfigureMapBank3RenderPreset4FAA4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08019250)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801575C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FAB4:{ /* ApplyMapBank3RenderPreset4FAB4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x080156F0)==HN_SCENE_OK);
        indicator(m,19, (GameAddr)0x088CB58B);
        break;
    }
    case 0x0804FACC:{ /* MapBank3NoOp4FACC */

        break;
    }
    case 0x0804FAD0:{ /* ConfigureMapBank3RenderPreset4FAD0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x0801682C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08019488)==HN_SCENE_OK);
        break;
    }
    case 0x0804FAE0:{ /* ConfigureMapBank3RenderPreset4FAE0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08016090)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B008)==HN_SCENE_OK);
        break;
    }
    case 0x0804FBA4:{ /* ConfigureMapBank3RenderPreset4FBA4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08013084)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08013954)==HN_SCENE_OK);
        break;
    }
    case 0x0804FBB4:{ /* ApplyMapBank3RenderPreset4FBB4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x0801682C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E2C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FBC4:{ /* ConfigureMapBank3RenderPreset4FBC4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08012EE0)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801A168)==HN_SCENE_OK);
        break;
    }
    case 0x0804FBD4:{ /* ConfigureMapBank3RenderPreset4FBD4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x0801691C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B1AC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FBE4:{ /* ConfigureMapBank3RenderPreset4FBE4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x0801694C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B200)==HN_SCENE_OK);
        break;
    }
    case 0x0804FBF4:{ /* ConfigureMapBank3RenderPreset4FBF4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08015444)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AA64)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AB0C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AB60)==HN_SCENE_OK);
        break;
    }
    case 0x0804FC0C:{ /* ApplyMapBank3RenderPreset4FC0C */
        BANK3_REQUIRE(hn_preset_apply(m,0x080155AC)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E90)==HN_SCENE_OK);
        break;
    }
    case 0x0804FC1C:{ /* ConfigureMapBank3RenderPreset4FC1C */
        BANK3_REQUIRE(hn_preset_apply(m,0x080160F0)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AB98)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801ABEC)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AC40)==HN_SCENE_OK);
        break;
    }
    case 0x0804FC34:{ /* ConfigureMapBank3RenderPreset4FC34 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08015DD0)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AD00)==HN_SCENE_OK);
        indicator(m,hn_read8(m,0x03002C00), (GameAddr)0x088CD247);
        break;
    }
    case 0x0804FC54:{ /* ConfigureMapBank3RenderPreset4FC54 */
        uint8_t auxiliary;
        BANK3_REQUIRE(hn_preset_apply(m,0x08015ED4)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C00) - 1), 1, 176, 0, 0, 0, (GameAddr)0x088CD4F6));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C00) - 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C00)));
        break;
    }
    case 0x0804FC9C:{ /* ConfigureMapBank3RenderPreset4FC9C */
        BANK3_REQUIRE(hn_preset_apply(m,0x0801773C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BB70)==HN_SCENE_OK);
        break;
    }
    case 0x0804FCAC:{ /* ApplyMapBank3RenderPreset4FCAC */
        BANK3_REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088D85AB));
        indicator(m,hn_read8(m,0x03002C00), (GameAddr)0x088D85AB);
        break;
    }
    case 0x0804FCD0:{ /* ConfigureMapBank3RenderPreset4FCD0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x080175D4)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BC18)==HN_SCENE_OK);
        break;
    }
    case 0x0804FCE0:{ /* ConfigureMapBank3RenderPreset4FCE0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08017664)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BC5C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BCA0)==HN_SCENE_OK);
        break;
    }
    case 0x0804FCF4:{ /* ConfigureMapBank3RenderPreset4FCF4 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08017840)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BBC4)==HN_SCENE_OK);
        break;
    }
    case 0x0804FD04:{ /* ConfigureMapBank3RenderPreset4FD04 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08017004)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B764)==HN_SCENE_OK);
        break;
    }
    case 0x0804FD14:{ /* ConfigureMapBank3RenderPreset4FD14 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08016270)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x080194DC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FD24:{ /* ConfigureMapBank3RenderPreset4FD24 */
        BANK3_REQUIRE(hn_preset_apply(m,0x080155AC)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08019250)==HN_SCENE_OK);
        break;
    }
    case 0x0804FD34:{ /* ConfigureMapBank3RenderPreset4FD34 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08015444)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AA64)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AB0C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FD48:{ /* ConfigureMapBank3RenderPreset4FD48 */
        BANK3_REQUIRE(hn_preset_apply(m,0x080160F0)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801AB98)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801ABEC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FD5C:{ /* ApplyMapBank3RenderPreset4FD5C */
        BANK3_REQUIRE(animation(m,10, (GameAddr)0x088A4EBC));
        BANK3_REQUIRE(animation(m,13, (GameAddr)0x088A8C01));
        indicator(m,10, (GameAddr)0x088A4EBC);
        break;
    }
    case 0x0804FD88:{ /* ApplyMapBank3RenderPreset4FD88 */
        BANK3_REQUIRE(animation(m,10, (GameAddr)0x088A52E3));
        BANK3_REQUIRE(animation(m,13, (GameAddr)0x088A9234));
        indicator(m,10, (GameAddr)0x088A52E3);
        break;
    }
    case 0x0804FDB4:{ /* MapBank3NoOp4FDB4 */

        break;
    }
    case 0x0804FDB8:{ /* ApplyMapBank3RenderPreset4FDB8 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014BC4)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014BAC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FDD8:{ /* ApplyMapBank3RenderPreset4FDD8 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) {
            BANK3_REQUIRE(hn_preset_apply(m,0x08014BF4)==HN_SCENE_OK);
            indicator(m,hn_read8(m,0x03002C00), (GameAddr)0x088C8BAB);
        } else {
            BANK3_REQUIRE(hn_preset_apply(m,0x08014BDC)==HN_SCENE_OK);
            indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8BAB);
        }
        break;
    }
    case 0x0804FE1C:{ /* ApplyMapBank3RenderPreset4FE1C */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014C24)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014C0C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FE3C:{ /* ApplyMapBank3RenderPreset4FE3C */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014C54)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014C3C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FE5C:{ /* ApplyMapBank3RenderPreset4FE5C */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014C84)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014C6C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FE7C:{ /* ApplyMapBank3RenderPreset4FE7C */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014CB4)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014C9C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FE9C:{ /* ApplyMapBank3RenderPreset4FE9C */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014CE4)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014CCC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FEBC:{ /* ApplyMapBank3RenderPreset4FEBC */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014D14)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014CFC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FEDC:{ /* ApplyMapBank3RenderPreset4FEDC */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014D44)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014D2C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FEFC:{ /* ApplyMapBank3RenderPreset4FEFC */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014D74)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014D5C)==HN_SCENE_OK);
        break;
    }
    case 0x0804FF1C:{ /* SetMapBank3Animation4FF1C */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088E9DF7));
        break;
    }
    case 0x0804FF34:{ /* ApplyMapBank3RenderPreset4FF34 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4)
            BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088E9E04));
        else
            BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088E9E04));
        break;
    }
    case 0x0804FF70:{ /* ApplyMapBank3RenderPreset4FF70 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) {
            BANK3_REQUIRE(hn_preset_apply(m,0x08014DA4)==HN_SCENE_OK);
            indicator(m,hn_read8(m,0x03002C00), (GameAddr)0x088C8C63);
        } else {
            BANK3_REQUIRE(hn_preset_apply(m,0x08014D8C)==HN_SCENE_OK);
            indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8C63);
        }
        break;
    }
    case 0x0804FFB4:{ /* ApplyMapBank3RenderPreset4FFB4 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014DD4)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014DBC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FFD4:{ /* ApplyMapBank3RenderPreset4FFD4 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014E04)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014DEC)==HN_SCENE_OK);
        break;
    }
    case 0x0804FFF4:{ /* ConfigureMapBank3RenderPreset4FFF4 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) {
            BANK3_REQUIRE(hn_preset_apply(m,0x08014E34)==HN_SCENE_OK);
            BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C00) - 1), 1, 176, 0, 0, 0, (GameAddr)0x088C908C));
            BANK3_REQUIRE(hn_render_modes_raw(m,(uint8_t)(hn_read8(m,0x03002C00) - 1), 2, 3, hn_read8(m,0x03002C00)));
        } else {
            BANK3_REQUIRE(hn_preset_apply(m,0x08014E1C)==HN_SCENE_OK);
            BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 1, 176, 0, 0, 0, (GameAddr)0x088C908C));
            BANK3_REQUIRE(hn_render_modes_raw(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 2, 3, hn_read8(m,0x03002C04)));
        }
        break;
    }
    case 0x08050088:{ /* SetMapBank3Animation50088 */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088E9E0C));
        break;
    }
    case 0x080500A0:{ /* ConfigureMapBank3RenderPreset500A0 */
        uint8_t auxiliary;
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8D4D));
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 1, 176, 0, 0, 0, (GameAddr)0x088C8DBE));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) - 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C04)));
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088C9099));
        indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8D4D);
        break;
    }
    case 0x0805010C:{ /* SetMapBank3Animation5010C */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088E9E19));
        break;
    }
    case 0x08050124:{ /* ApplyMapBank3RenderPreset50124 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014EAC)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014E94)==HN_SCENE_OK);
        break;
    }
    case 0x08050144:{ /* ApplyMapBank3RenderPreset50144 */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8C7D));
        indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8C7D);
        break;
    }
    case 0x08050168:{ /* ApplyMapBank3RenderPreset50168 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0xA7) == 4) BANK3_REQUIRE(hn_preset_apply(m,0x08014E7C)==HN_SCENE_OK);
        else BANK3_REQUIRE(hn_preset_apply(m,0x08014E64)==HN_SCENE_OK);
        break;
    }
    case 0x08050188:{ /* ConfigureMapBank3RenderPreset50188 */
        uint8_t auxiliary;
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8BAB));
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 1, 176, 0, 0, 0, (GameAddr)0x088C8BE9));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) - 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C04)));
        indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8BAB);
        break;
    }
    case 0x080501E4:{ /* ConfigureMapBank3RenderPreset501E4 */
        uint8_t auxiliary;
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8C05));
        indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8C05);
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 1, 176, 0, 0, 0, (GameAddr)0x088C8C1C));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C04) - 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x08050240:{ /* SetMapBank3Animation50240 */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088E9E04));
        break;
    }
    case 0x08050258:{ /* ApplyMapBank3RenderPreset50258 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08014D8C)==HN_SCENE_OK);
        indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8C63);
        break;
    }
    case 0x08050274:{ /* SetMapBank3Animation50274 */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088E9DEF));
        break;
    }
    case 0x0805028C:{ /* ApplyMapBank3RenderPreset5028C */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8ED6));
        indicator(m,hn_read8(m,0x03002C04), (GameAddr)0x088C8ED6);
        break;
    }
    case 0x080502B0:{ /* ConfigureMapBank3RenderPreset502B0 */
        BANK3_REQUIRE(hn_render_configure_raw(m,20, 1, 114, 0, 0, 0, (GameAddr)0x088E9E26));
        indicator(m,20, (GameAddr)0x088E9E26);
        break;
    }
    case 0x080507CC:{ /* MapBank3NoOp507CC */

        break;
    }
    case 0x080507D0:{ /* ConfigureMapBank3RenderPreset507D0 */
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,5, 1, 0x088E135C));
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,5, 1, 0x088E13AB));
        BANK3_REQUIRE(animation(m,10, (GameAddr)0x088A51F4));
        BANK3_REQUIRE(animation(m,13, (GameAddr)0x088A9104));
        BANK3_REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, (GameAddr)0x088E1B11));
        BANK3_REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, (GameAddr)0x088E1B3C));
        BANK3_REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        BANK3_REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        indicator(m,10, (GameAddr)0x088A51F4);
        break;
    }
    case 0x08050868:{ /* ApplyMapBank3RenderPreset50868 */
        if ((uint8_t)(hn_read8(m,HN_SCENE_CONTROLS+0x93) - 5) <= 1)
            BANK3_REQUIRE(animation(m,25, (GameAddr)0x088EA6E8));
        else BANK3_REQUIRE(hn_preset_apply(m,0x080168A4)==HN_SCENE_OK);
        break;
    }
    case 0x08050894:{ /* ConfigureMapBank3RenderPreset50894 */
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088D1469));
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B158)==HN_SCENE_OK);
        break;
    }
    case 0x080508AC:{ /* ConfigureMapBank3RenderPreset508AC */
        BANK3_REQUIRE(hn_preset_apply(m,0x0801694C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B200)==HN_SCENE_OK);
        break;
    }
    case 0x080508BC:{ /* ConfigureMapBank3RenderPreset508BC */
        BANK3_REQUIRE(hn_preset_apply(m,0x08016A6C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B480)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B4D4)==HN_SCENE_OK);
        break;
    }
    case 0x080508D0:{ /* ConfigureMapBank3RenderPreset508D0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08016D7C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B6CC)==HN_SCENE_OK);
        break;
    }
    case 0x080508E0:{ /* ConfigureMapBank3RenderPreset508E0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08016B60)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B384)==HN_SCENE_OK);
        break;
    }
    case 0x080508F0:{ /* ConfigureMapBank3RenderPreset508F0 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08016B00)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B2DC)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B330)==HN_SCENE_OK);
        break;
    }
    case 0x08050904:{ /* ConfigureMapBank3RenderPreset50904 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08016B78)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B3D8)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801B42C)==HN_SCENE_OK);
        break;
    }
    case 0x08050918:{ /* SetMapBank3Animation50918 */
        BANK3_REQUIRE(animation(m,16, (GameAddr)0x088DB88B));
        break;
    }
    case 0x0805092C:{ /* ConfigureMapBank3RenderPreset5092C */
        BANK3_REQUIRE(hn_preset_apply(m,0x08017E74)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BCE4)==HN_SCENE_OK);
        break;
    }
    case 0x0805093C:{ /* ConfigureMapBank3RenderPreset5093C */
        BANK3_REQUIRE(hn_preset_apply(m,0x080181F8)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08019D44)==HN_SCENE_OK);
        break;
    }
    case 0x0805094C:{ /* ApplyMapBank3RenderPreset5094C */
        BANK3_REQUIRE(animation(m,25, (GameAddr)0x088EA722));
        BANK3_REQUIRE(animation(m,24, (GameAddr)0x088E394D));
        break;
    }
    case 0x0805096C:{ /* ConfigureMapBank3RenderPreset5096C */
        BANK3_REQUIRE(hn_preset_apply(m,0x08017C6C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BD7C)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BDD0)==HN_SCENE_OK);
        break;
    }
    case 0x08050980:{ /* ConfigureMapBank3RenderPreset50980 */
        BANK3_REQUIRE(hn_preset_apply(m,0x08018090)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x0801BD38)==HN_SCENE_OK);
        break;
    }
    case 0x08050990:{ /* SetMapBank3Animation50990 */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C04), (GameAddr)0x088DC14B));
        break;
    }
    case 0x08050CEC:{ /* ApplyMapBank3RenderPreset50CEC */
        BANK3_REQUIRE(hn_preset_apply(m,0x08016210)==HN_SCENE_OK);
        BANK3_REQUIRE(hn_preset_apply(m,0x08018E64)==HN_SCENE_OK);
        break;
    }
    case 0x08050CFC:{ /* ConfigureMapBank3SqrtMotion */
        hn_write16(m,0x0201DC40+(0x57)*2,352);
        BANK3_REQUIRE(hn_render_configure_raw(m,22, 1, 114, -40, -8, -40, (GameAddr)0x088EAD94));
        BANK3_REQUIRE(field_animation(m,23, 1, (GameAddr)0x088EADAB));
        break;
    }
    case 0x08050DDC:{ /* SetMapBank3Animation50DDC */
        BANK3_REQUIRE(animation(m,21, (GameAddr)0x088EAD55));
        break;
    }
    case 0x08050DF0:{ /* ApplyMapBank3RenderPreset50DF0 */
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088CF0A3));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00));
        hn_write32(m,0x03001F7C,0x088CF0A3);
        break;
    }
    case 0x08050E20:{ /* ConfigureMapBank3RenderPreset50E20 */
        uint8_t auxiliary;
        BANK3_REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1403));
        BANK3_REQUIRE(animation(m,hn_read8(m,0x03002C00), (GameAddr)0x088CD2E8));
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C00) - 1), 1, 176, 0, 0, 0, (GameAddr)0x088CD31D));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C00) - 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C00)));
        BANK3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C00) + 1), 1, 176, 0, 0, 0, (GameAddr)0x088CD334));
        auxiliary = (uint8_t)(hn_read8(m,0x03002C00) + 1);
        BANK3_REQUIRE(hn_render_modes_raw(m,auxiliary, 2, 3, hn_read8(m,0x03002C00)));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00));
        hn_write32(m,0x03001F7C,0x088CD2E8);
        break;
    }
    case 0x08050EC4:{ /* SetMapBank3Animation50EC4 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088DE501));
        break;
    }
    case 0x08050ED8:{ /* ConfigureMapBank3RenderPreset50ED8 */
        BANK3_REQUIRE(animation(m,22, (GameAddr)0x088DE50E));
        BANK3_REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, (GameAddr)0x088DE52A));
        BANK3_REQUIRE(hn_render_modes_raw(m,12, 2, 3, 22));
        BANK3_REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, (GameAddr)0x088DE584));
        BANK3_REQUIRE(hn_render_modes_raw(m,14, 2, 3, 22));
        break;
    }
    case 0x08050F3C:{ /* ConfigureMapBank3RenderPreset50F3C */
        BANK3_REQUIRE(field(m,15, 0));
        BANK3_REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04), 1, 114, 144, 0, 88, (GameAddr)0x088C2EE2));
        break;
    }
    case 0x08050F70:{ /* ApplyMapBank3RenderPreset50F70 */
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,0x088C4878);
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
#undef BANK3_REQUIRE

static GameAddr song_player(HNMemory *m,unsigned song)
{
    unsigned index=hn_read16(m,HN_AUDIO_SONG_RECORDS+song*8+4);
    return hn_read32(m,HN_AUDIO_PLAYER_RECORDS+index*12);
}
HNSceneResult hn_bank3_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    (void)arguments;
    switch(callback&~1u){
    case 0x0804D0C4:
    case 0x0804D0FC:
    case 0x0804D118:
    case 0x0804D140:
    case 0x0804D15C:
    case 0x0804D178:
    case 0x0804D194:
    case 0x0804D1BC:
    case 0x0804D1CC:
    case 0x0804D1E8:
    case 0x0804D204:
    case 0x0804D220:
    case 0x0804D280:
    case 0x0804D2A8:
    case 0x0804D2C4:
    case 0x0804D318:
    case 0x0804D340:
    case 0x0804D3A8:
    case 0x0804D410:
    case 0x0804D478:
    case 0x0804DBDC:
    case 0x0804DBFC:
    case 0x080507D0:
        *consumed=0;return main_presets(m,callback);
    case 0x08051798:
        if(!hn_scene_save_actor_slots_raw(m))return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS+0x27,hn_read8(m,HN_SCENE_CONTROLS+0x0D));
        hn_write8(m,HN_SCENE_CONTROLS+0x28,hn_read8(m,HN_SCENE_CONTROLS+0x0E));
        if(!hn_map_initialize(m,3,22))return HN_SCENE_RESOURCE;*consumed=0xFFFFFFFC;break;
    case 0x08051830:{
        GameAddr cursor=hn_read32(m,0x03002C60);
        if(!hn_map_initialize(m,hn_read8(m,HN_SCENE_CONTROLS+0x27),hn_read8(m,HN_SCENE_CONTROLS+0x28))||
           !hn_scene_restore_actor_slots_raw(m))return HN_SCENE_RESOURCE;
        hn_write32(m,0x03002C60,cursor);
        hn_write8(m,HN_SCENE_CONTROLS+0x0D,hn_read8(m,HN_SCENE_CONTROLS+0x27));
        hn_write8(m,HN_SCENE_CONTROLS+0x0E,hn_read8(m,HN_SCENE_CONTROLS+0x28));*consumed=0;break;
    }
    default:{
        HNSceneResult result=hn_bank3_games_callback(m,callback,arguments,consumed);
        return result==HN_SCENE_UNKNOWN?hn_bank3_chain_callback(m,callback,arguments,consumed):result;
    }
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank3_task(HNMemory *m,GameAddr callback)
{
    HNSceneResult result=main_presets(m,callback);
    if(result==HN_SCENE_UNKNOWN)result=late_presets(m,callback);
    if(result!=HN_SCENE_UNKNOWN)return result;
    switch(callback&~1u){
    case 0x08051798:case 0x08051830:{uint32_t ignored;return hn_bank3_callback(m,callback,0,&ignored);}
    default:
        result=hn_bank3_games_task(m,callback);
        return result==HN_SCENE_UNKNOWN?hn_bank3_chain_task(m,callback):result;
    }
}
HNSceneResult hn_bank3_animation(HNMemory *m,GameAddr callback,uint32_t *consumed,uint32_t *yield)
{
    switch(callback&~1u){
    case 0x0805110C:{
        uint8_t next=(uint8_t)(hn_read8(m,0x02001658)-1);hn_write8(m,0x02001658,next);
        if(next){
            GameAddr player=song_player(m,0x37);
            uint32_t shifted=((uint32_t)hn_read8(m,0x02001658)*5u)<<25;
            int32_t pan=(int32_t)(shifted+0x81000000u)>>24;
            hn_audio_track_parameter(m,player,255,(uint32_t)pan,2);
        }else hn_write8(m,0x02001658,22);
        *yield=0;break;
    }
    case 0x08051168:{
        uint8_t next=(uint8_t)(hn_read8(m,0x02001659)-1);hn_write8(m,0x02001659,next);
        if(next)hn_audio_tempo(m,song_player(m,0x36),128);*yield=0;break;
    }
    case 0x080516F0:
        hn_write8(m,HN_SCENE_CONTROLS+0x57,hn_read8(m,HN_SCENE_CONTROLS+0x57)^1);*yield=0;break;
    case 0x08051708:{
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS),source=base+27*104,destination=base+29*104;
        uint16_t scale=hn_read16(m,source+0x3C);
        if(hn_read8(m,HN_SCENE_CONTROLS+0x57))scale=(uint16_t)(0u-scale);
        for(unsigned i=0;i<3;i++,destination+=104){
            if(hn_read8(m,destination+0x64)==255){
                hn_write8(m,destination+0x64,hn_render_find_affine(m));
                hn_write8(m,HN_AFFINE_USAGE+hn_read8(m,destination+0x64),1);
            }
            hn_write16(m,destination+0x3C,scale);hn_write16(m,destination+0x3E,hn_read16(m,source+0x3E));
        }
        *yield=1;break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    *consumed=0;return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
