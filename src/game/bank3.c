#include "game/bank3.h"
#include "game/presets.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/audio.h"
#include "game/world.h"

static bool raw_animation(HNMemory *m,uint32_t index,GameAddr resource)
{hn_render_set_animation(m,index,resource);return !m->fault;}
static bool animation(HNMemory *m,uint8_t index,GameAddr resource)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    return raw_animation(m,index,resource);
}
static void indicator(HNMemory *m,uint8_t index,GameAddr resource)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,resource);}
static bool field(HNMemory *m,uint32_t index,uint8_t value)
{hn_write8(m,hn_render_slot(m,index)+0x60,value);return !m->fault;}

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

static GameAddr song_player(HNMemory *m,unsigned song)
{
    unsigned index=hn_read16(m,HN_AUDIO_SONG_RECORDS+song*8+4);
    return hn_read32(m,HN_AUDIO_PLAYER_RECORDS+index*12);
}
HNSceneResult hn_bank3_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    (void)arguments;
    switch(callback&~1u){
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
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank3_task(HNMemory *m,GameAddr callback)
{
    uint32_t ignored;HNSceneResult result=hn_bank3_callback(m,callback,0,&ignored);
    return result==HN_SCENE_UNKNOWN?late_presets(m,callback):result;
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
