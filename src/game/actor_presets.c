#include "game/actor_presets.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"

static bool raw_animation(HNMemory *m,uint32_t index,GameAddr resource)
{hn_render_set_animation(m,index,resource);return !m->fault;}
static bool animation(HNMemory *m,uint8_t index,GameAddr resource)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    return raw_animation(m,index,resource);
}
static void indicator(HNMemory *m,uint8_t index,GameAddr resource)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,resource);}
static bool auxiliary(HNMemory *m,uint8_t index,uint8_t parent,GameAddr resource)
{
    /* The original wrapper captures these arguments before configuring the
       slot; unlike two separate source calls, its selectors are not reloaded. */
    return hn_render_configure_raw(m,index,1,0xB0,0,0,0,resource)&&hn_render_modes_raw(m,index,2,3,parent);
}
static bool field(HNMemory *m,uint8_t index,uint8_t value)
{hn_write8(m,hn_render_slot(m,index)+0x60,value);return !m->fault;}
static bool priority(HNMemory *m,uint8_t index,uint8_t value)
{hn_render_flags(m,index,12,(uint8_t)(value<<2));return !m->fault;}

/* Void/F4 presets. These functions have no defined Main consumed count.
   Constants identify resources in the player's ROM; no resource data is here. */
static HNSceneResult base_actor_presets(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x08012D78:{ /* ActorPresetNoop12D78 */

        break;
    }
    case 0x08012D7C:{ /* SetActorAnimationPreset12D7C */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088AC08F)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012D94:{ /* SetActorAnimationPreset12D94 */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088AC0C6)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012DAC:{ /* SetActorAnimationPreset12DAC */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088B0A5F)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012DC4:{ /* SetActorAnimationPreset12DC4 */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088ACAF7)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012DDC:{ /* ConfigureActorPreviousSlot12DDC */
        do {if(!(hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1, 1, 0xB0, 0, 0, 0, 0x088E1740)))return HN_SCENE_RESOURCE;} while(0);
        do {if(!(hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1, 2, 3, hn_read8(m,0x03002C0C))))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012E20:{ /* SetActorAnimationPreset12E20 */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088ACC20)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012E38:{ /* SetActorAnimationPreset12E38 */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088AECA8)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012E50:{ /* SetActorAnimationPreset12E50 */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088ACB54)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012E68:{ /* SetActorAnimationPreset12E68 */
        do {if(!(animation(m,hn_read8(m,0x03001F50), 0x088ACB54)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012E80:{ /* SetActorAnimationPreset12E80 */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088AD1F9)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012E98:{ /* SetActorAnimationPreset12E98 */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088AD381)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012EB0:{ /* SetActorAnimationPreset12EB0 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088AC134)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012EC8:{ /* SetActorAnimationPreset12EC8 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088AD98E)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012EE0:{ /* SetActorAnimationPreset12EE0 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088AD9DC)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012EF8:{ /* SetActorAnimationPreset12EF8 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088ADA0E)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012F10:{ /* SetActorAnimationPreset12F10 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088ADE4C)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012F28:{ /* SetActorAnimationPreset12F28 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088B0CA0)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012F40:{ /* SetActorAnimationPreset12F40 */
        do {if(!(animation(m,hn_read8(m,0x03002C0C), 0x088B08D3)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012F58:{ /* SetActorAnimationPreset12F58 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088AD641)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012F70:{ /* SetActorAnimationPreset12F70 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088AD680)))return HN_SCENE_RESOURCE;} while(0);
        break;
    }
    case 0x08012F88:{ /* SetActorAnimationAndIndicator12F88 */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088A5724)))return HN_SCENE_RESOURCE;} while(0);
        indicator(m,hn_read8(m,0x03002C0C), 0x088A5724);
        break;
    }
    case 0x08012FAC:{ /* ConfigureActorNextSlot12FAC */
        do {if(!(raw_animation(m,hn_read8(m,0x03002C0C), 0x088A5788)))return HN_SCENE_RESOURCE;} while(0);
        do {if(!(hn_graphics_initialize_raw(m,4, 1, (uint32_t)0x088E135C)))return HN_SCENE_RESOURCE;} while(0);
        do {if(!(hn_render_configure_raw(m,hn_read8(m,0x03002C0C) + 1, 1, 0xB0, 0, 0, 0, 0x088E1ADF)))return HN_SCENE_RESOURCE;} while(0);
        do {if(!(hn_render_modes_raw(m,hn_read8(m,0x03002C0C) + 1, 2, 3, hn_read8(m,0x03002C0C))))return HN_SCENE_RESOURCE;} while(0);
        indicator(m,hn_read8(m,0x03002C0C), 0x088A5788);
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}

HNSceneResult hn_actor_presets_apply(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x08013014:{ /* SetActorAnimationAndIndicator13014 */
        if(!raw_animation(m,hn_read8(m,0x03002C0C),0x088A590A))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),0x088A590A);
        break;
    }
    case 0x08013038:{ /* ConfigureActorNeighborSlots13038 */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088A5AB7))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),0x088A5AB7);
        if(!auxiliary(m,hn_read8(m,0x03002C0C) - 1,hn_read8(m,0x03002C0C),0x088E1D87))return HN_SCENE_RESOURCE;
        if(!auxiliary(m,hn_read8(m,0x03002C0C) + 1,hn_read8(m,0x03002C0C),0x088E1DAD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013084:{ /* ConfigureActorPreviousSlotAndGraphics13084 */
        if(!raw_animation(m,hn_read8(m,0x03002C0C),0x088A5ECC))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),0x088A5ECC);
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088E1FD6))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E138B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080130EC:{ /* ConfigureActorPreviousSlotAndGraphics130EC */
        if(!raw_animation(m,hn_read8(m,0x03002C0C),0x088A6AB9))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),0x088A6AB9);
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088E2CF0))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013154:{ /* SetActorAnimationAndIndicator13154 */
        if(!raw_animation(m,hn_read8(m,0x03002C0C),0x088A6B7E))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),0x088A6B7E);
        break;
    }
    case 0x08013178:{ /* SetActorAnimationAndIndicator13178 */
        if(!raw_animation(m,hn_read8(m,0x03002C0C),0x088A6DC1))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),0x088A6DC1);
        break;
    }
    case 0x0801319C:{ /* SetActorAnimationAndIndicator1319C */
        if(!raw_animation(m,hn_read8(m,0x03002C0C),0x088A6DDB))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),0x088A6DDB);
        break;
    }
    case 0x080131C0:{ /* SetActorAnimationPreset131C0 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B745B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080131D8:{ /* SetActorAnimationPreset131D8 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B4F17))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080131F0:{ /* SetActorAnimationPreset131F0 */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B4F17))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013208:{ /* SetActorAnimationPreset13208 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B7F79))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013220:{ /* SetActorAnimationPreset13220 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B4F23))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013238:{ /* SetActorAnimationPreset13238 */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B4F23))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013250:{ /* SetActorAnimationPreset13250 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B4F3F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013268:{ /* SetActorAnimationPreset13268 */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B4F3F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013280:{ /* ConfigureFollowerPreviousSlot13280 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E1740))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080132C4:{ /* SetActorAnimationPreset132C4 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B68F8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080132DC:{ /* SetActorAnimationPreset132DC */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B4F90))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080132F4:{ /* SetActorAnimationPreset132F4 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B4FE1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801330C:{ /* SetActorAnimationPreset1330C */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B4FE1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013324:{ /* SetActorAnimationPreset13324 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B50AB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801333C:{ /* SetActorAnimationPreset1333C */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B50E3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013354:{ /* SetActorAnimationPreset13354 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B8840))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801336C:{ /* SetActorAnimationPreset1336C */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B8840))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801339C:{ /* SetActorAnimationPreset1339C */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B5325))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080133B4:{ /* SetActorAnimationPreset133B4 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B5331))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080133CC:{ /* SetActorAnimationPreset133CC */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B53A0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080133E4:{ /* SetActorAnimationPreset133E4 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088B549D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080133FC:{ /* SetActorAnimationPreset133FC */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6A36))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013414:{ /* SetActorAnimationPreset13414 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088B6A36))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801342C:{ /* SetActorAnimationPreset1342C */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6A5D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013444:{ /* SetActorAnimationPreset13444 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6A83))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801345C:{ /* SetActorAnimationPreset1345C */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B408C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013474:{ /* SetActorAnimationPreset13474 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6A9F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801348C:{ /* SetActorAnimationPreset1348C */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B6A9F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080134A4:{ /* SetActorAnimationPreset134A4 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6ADD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080134BC:{ /* SetActorAnimationPreset134BC */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B6ADD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080134D4:{ /* SetActorAnimationPreset134D4 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B79D7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080134EC:{ /* SetActorAnimationPreset134EC */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088B4002))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013504:{ /* ConfigureActorNeighborSlots13504 */
        if(!raw_animation(m,hn_read8(m,0x03002C0C),0x088B4002))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088B4039))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) + 1,1,0xB0,0,0,0,0x088B4047))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) + 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013584:{ /* ClearActorNeighborField60_13584 */
        if(!field(m,hn_read8(m,0x03002C0C) - 1,0))return HN_SCENE_RESOURCE;
        if(!field(m,hn_read8(m,0x03002C0C) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080135B0:{ /* SetActorAnimationAndIndicator135B0 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B55BA))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,hn_read8(m,0x03001F50));
        hn_write32(m,0x03001F7C,(uint32_t)0x088B55BA);
        break;
    }
    case 0x080135E0:{ /* SetActorAnimationAndIndicator135E0 */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B55BA))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C0C));
        hn_write32(m,0x03001F7C,(uint32_t)0x088B55BA);
        break;
    }
    case 0x08013610:{ /* SetActorClassAnimationPreset13610 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B5611))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013628:{ /* SetActorClassAnimationPreset13628 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B598D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013640:{ /* SetActorClassAnimationPreset13640 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B574A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013658:{ /* SetActorClassAnimationPreset13658 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6BB5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013670:{ /* SetActorClassAnimationPreset13670 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B57E9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013688:{ /* SetActorClassAnimationPreset13688 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6F54))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080136A0:{ /* SetActorClassAnimationPreset136A0 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6FBB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080136D0:{ /* SetActorClassAnimationPreset136D0 */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B6FD7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080136E8:{ /* SetActorClassAnimationPreset136E8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088B6FD7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013700:{ /* SetActorClassAnimationPreset13700 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B7042))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013718:{ /* SetActorClassAnimationPreset13718 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B551C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013730:{ /* SetActorClassAnimationPreset13730 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B5551))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013748:{ /* SetActorClassAnimationPreset13748 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B5926))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013760:{ /* SetActorClassAnimationPreset13760 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B59B5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013778:{ /* SetActorClassAnimationPreset13778 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B5999))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013790:{ /* SetActorClassAnimationPreset13790 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B5A5B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080137A8:{ /* SetActorClassAnimationPreset137A8 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B4055))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080137C0:{ /* SetActorClassAnimationPreset137C0 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B40C3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080137D8:{ /* SetActorClassAnimationPreset137D8 */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B40C3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080137F0:{ /* SetActorClassAnimationPreset137F0 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B70A6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013808:{ /* SetActorClassAnimationPreset13808 */
        if(!animation(m,hn_read8(m,0x03002C0C),0x088B70A6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013820:{ /* SetActorClassAnimationPreset13820 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088B70A6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013838:{ /* SetActorClassAnimationPreset13838 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B70B2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013850:{ /* SetActorClassAnimationAndIndicator13850 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088A7588))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088A7588);
        break;
    }
    case 0x08013874:{ /* SetActorClassAnimationAndIndicator13874 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088A8B25))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03001F50),0x088A8B25);
        break;
    }
    case 0x08013898:{ /* SetActorClass2AnimationAndInlineIndicator13898 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088A8D95))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,(uint32_t)0x088A8D95);
        break;
    }
    case 0x080138C8:{ /* SetActorClassAnimationPreset138C8 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088A965A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080138E0:{ /* SetActorClassAnimationPreset138E0 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088A96F4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080138F8:{ /* SetActorClassAnimationPreset138F8 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088A9428))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013910:{ /* ConfigureFollowerNeighborSlots13910 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088A9BE9))return HN_SCENE_RESOURCE;
        if(!auxiliary(m,hn_read8(m,0x03001F50) - 1,hn_read8(m,0x03001F50),0x088E1D87))return HN_SCENE_RESOURCE;
        if(!auxiliary(m,hn_read8(m,0x03001F50) + 1,hn_read8(m,0x03001F50),0x088E1DAD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013954:{ /* ConfigureFollowerPreviousSlotAndGraphics13954 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088AA090))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E1FAD))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E138B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080139B4:{ /* ConfigureFollowerPreviousSlotAndGraphics139B4 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088AB01E))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E2D2B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,5,(uint32_t)0x088E1432))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013A14:{ /* SetActorClassAnimationPreset13A14 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088AB0A1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013A2C:{ /* SetActorClassAnimationPreset13A2C */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088AB6CE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013A44:{ /* SetActorClassAnimationPreset13A44 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088AB6E5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013A5C:{ /* SetFollowerAnimationTowardActor13A5C */
        switch((uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03001F50),hn_read8(m,0x03002C0C),false)){
        case 3:
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088B408C))return HN_SCENE_RESOURCE;
        break;
        case 4:
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088B40C3))return HN_SCENE_RESOURCE;
        break;
        case 1:
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088B4055))return HN_SCENE_RESOURCE;
        break;
        case 2:
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088B4002))return HN_SCENE_RESOURCE;
        break;
        }
        break;
    }
    case 0x08013AD0:{ /* SetSecondaryAnimationPreset13AD0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C77EC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013AE8:{ /* SetSecondaryAnimationPreset13AE8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C7723))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013B00:{ /* SetSecondaryAnimationAndIndicator13B00 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C7750))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088C7750);
        break;
    }
    case 0x08013B24:{ /* SetSecondaryAnimationPreset13B24 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C77D7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013B3C:{ /* SetSecondaryAnimationPreset13B3C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C701D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013B54:{ /* SetSecondaryAnimationPreset13B54 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C7036))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013B6C:{ /* SetSecondaryAnimationPreset13B6C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C6D27))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013B84:{ /* SetSecondaryAnimationPreset13B84 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C6DC4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013B9C:{ /* SetSecondaryAnimationPreset13B9C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C2F12))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013BB4:{ /* SetSecondaryAnimationPreset13BB4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C2EE2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013BCC:{ /* SetSecondaryAnimationPreset13BCC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C2F27))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013BE4:{ /* SetSecondaryAnimationPreset13BE4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C3B4D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013BFC:{ /* SetSecondaryAnimationPreset13BFC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C3AFF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013C14:{ /* SetSecondaryAnimationPreset13C14 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C3ACA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013C2C:{ /* SetSecondaryAnimationPreset13C2C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C44ED))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013C44:{ /* SetSecondaryAnimationPreset13C44 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C452A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013C5C:{ /* SetSecondaryAnimationPreset13C5C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4689))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013C74:{ /* ConfigureFixedRenderPreviousSlot13C74 */
        if(!animation(m,16,0x088C4823))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,15,1,0xB0,0,0,0,0x088C4840))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,15,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013CB0:{ /* ConfigureSecondaryPreviousSlot13CB0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4878))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088C48B6))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013D00:{ /* ConfigureFixedRenderPreviousSlot13D00 */
        if(!animation(m,16,0x088C4919))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,15,1,0xB0,0,0,0,0x088C492E))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,15,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013D3C:{ /* SetFixedRenderAnimationPreset13D3C */
        if(!animation(m,16,0x088C49B9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013D50:{ /* SetFixedRenderAnimationPreset13D50 */
        if(!animation(m,16,0x088C49E0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013D64:{ /* SetFixedRenderAnimationPreset13D64 */
        if(!animation(m,16,0x088C49F5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013D78:{ /* SetSecondaryAnimationPreset13D78 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4A01))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013D90:{ /* SetSecondaryAnimationPreset13D90 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4A22))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013DA8:{ /* SetSecondaryAnimationPreset13DA8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4A66))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013DC0:{ /* SetSecondaryAnimationPreset13DC0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4A8D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013DD8:{ /* SetSecondaryAnimationPreset13DD8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4AA2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013DF0:{ /* SetSecondaryAnimationPreset13DF0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4AAE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013E08:{ /* SetSecondaryAnimationPreset13E08 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4AC3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013E20:{ /* SetSecondaryAnimationPreset13E20 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4B51))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013E38:{ /* SetSecondaryAnimationPreset13E38 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4BF1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013E50:{ /* SetSecondaryAnimationPreset13E50 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C41CD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013E68:{ /* ConfigureSecondaryPreviousSlotOnly13E68 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088C41D9))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013EAC:{ /* ConfigureSecondaryNextSlotOnly13EAC */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088C4203))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013EF0:{ /* SetSecondaryAnimationPreset13EF0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C422D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013F08:{ /* SetSecondaryAnimationPreset13F08 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C3B96))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013F20:{ /* SetSecondaryAnimationPreset13F20 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C3EEC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013F38:{ /* SetSecondaryAnimationPreset13F38 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4036))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013F50:{ /* SetSecondaryAnimationPreset13F50 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C404E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013F68:{ /* SetSecondaryAnimationPreset13F68 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C40C7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013F80:{ /* SetSecondaryAnimationPreset13F80 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4063))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013F98:{ /* SetSecondaryAnimationPreset13F98 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C2EFA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013FB0:{ /* SetSecondaryAnimationPreset13FB0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C2F06))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013FC8:{ /* SetSecondaryAnimationAndIndicator13FC8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C4ACF))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088C4ACF);
        break;
    }
    case 0x08014004:{ /* SetSecondaryAnimationPreset14004 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C18D6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801401C:{ /* SetSecondaryAnimationPreset1401C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088C18EF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801404C:{ /* SetSecondaryAnimationPreset1404C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088C1919))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014064:{ /* SetSecondaryAnimationPreset14064 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C1933))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801407C:{ /* SetSecondaryAnimationPreset1407C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088C1948))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014094:{ /* SetSecondaryAnimationPreset14094 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C1A8F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080140AC:{ /* SetSecondaryAnimationPreset140AC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C2785))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080140C4:{ /* SetSecondaryAnimationPreset140C4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C285C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080140DC:{ /* SetSecondaryAnimationPreset140DC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C279F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080140F4:{ /* SetSecondaryAnimationPreset140F4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C27AB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801410C:{ /* SetSecondaryAnimationPreset1410C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C2891))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014124:{ /* SetSecondaryAnimationPreset14124 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8162))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801413C:{ /* SetSecondaryAnimationPreset1413C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C816E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014154:{ /* SetSecondaryAnimationPreset14154 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C817A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801416C:{ /* SetSecondaryAnimationPreset1416C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8186))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014184:{ /* SetSecondaryAnimationPreset14184 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C819B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801419C:{ /* SetSecondaryAnimationPreset1419C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C81A7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080141B4:{ /* SetSecondaryAnimationPreset141B4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C81B3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080141CC:{ /* SetSecondaryAnimationPreset141CC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C81BF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080141E4:{ /* SetSecondaryAnimationPreset141E4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C81D4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080141FC:{ /* SetSecondaryAnimationPreset141FC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C81E0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014214:{ /* SetSecondaryAnimationPreset14214 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C81E0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801422C:{ /* SetSecondaryAnimationPreset1422C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8201))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014244:{ /* SetSecondaryAnimationPreset14244 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C820D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801425C:{ /* SetSecondaryAnimationPreset1425C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8269))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014274:{ /* SetSecondaryAnimationPreset14274 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C828B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801428C:{ /* SetSecondaryAnimationPreset1428C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C82B2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080142A4:{ /* SetSecondaryAnimationPreset142A4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C82D9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080142BC:{ /* SetSecondaryAnimationPreset142BC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8062))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080142D4:{ /* SetSecondaryAnimationPreset142D4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8312))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080142EC:{ /* SetSecondaryAnimationPreset142EC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8312))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014304:{ /* SetSecondaryAnimationPreset14304 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C831E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801431C:{ /* SetSecondaryAnimationPreset1431C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C831E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014334:{ /* SetSecondaryAnimationPreset14334 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8333))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801434C:{ /* SetSecondaryAnimationPreset1434C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8333))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014364:{ /* SetSecondaryAnimationPreset14364 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C80C1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801437C:{ /* SetSecondaryAnimationPreset1437C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C80CD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014394:{ /* SetSecondaryAnimationAndIndicator14394 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8092))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C00),0x088C8092);
        break;
    }
    case 0x080143B8:{ /* SetSecondaryAnimationPreset143B8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC6F9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080143D0:{ /* SetSecondaryAnimationPreset143D0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC7DB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080143E8:{ /* SetSecondaryAnimationPreset143E8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC79A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014418:{ /* SetSecondaryAnimationPreset14418 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC62A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014430:{ /* SetSecondaryAnimationPreset14430 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC7E7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014448:{ /* SetSecondaryAnimationPreset14448 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC7F3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014460:{ /* SetSecondaryAnimationPreset14460 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC815))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014478:{ /* SetSecondaryAnimationPreset14478 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC84D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014490:{ /* SetSecondaryAnimationPreset14490 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC862))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080144A8:{ /* SetSecondaryAnimationAndInlineIndicator144A8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC86E))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,(uint32_t)0x088BC86E);
        break;
    }
    case 0x080144D8:{ /* ConfigureSecondaryGraphicsAndNextSlot144D8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1412))return HN_SCENE_RESOURCE;
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC71A))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E1B67))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,(uint32_t)0x088BC71A);
        break;
    }
    case 0x0801454C:{ /* SetSecondaryAnimationPreset1454C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD264))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014564:{ /* SetSecondaryAnimationPreset14564 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD206))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801457C:{ /* SetSecondaryAnimationPreset1457C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD279))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014594:{ /* ConfigureSecondaryNextSlot14594 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD35C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088BD368))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080145E4:{ /* SetSecondaryAnimationPreset145E4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD3C3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080145FC:{ /* ConfigureSecondaryNeighborSlots145FC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD3FB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088BD410))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088BD42F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017B18:{ /* SelectActorAnimationForXZDirection17B18 */
        switch((uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03002C00),hn_read8(m,0x03002C0C),false))
        {
        case 3:
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D7EB1))return HN_SCENE_RESOURCE;
        break;
        case 4:
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D7EBD))return HN_SCENE_RESOURCE;
        break;
        case 1:
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D7EA5))return HN_SCENE_RESOURCE;
        break;
        case 2:
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D7E99))return HN_SCENE_RESOURCE;
        break;
        }
        break;
    }
    case 0x08017B8C:{ /* SetRenderSetupAnimationPreset17B8C */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DC075))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017BA4:{ /* SetRenderSetupAnimationPreset17BA4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC075))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017BBC:{ /* SetRenderSetupAnimationPreset17BBC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC097))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017BD4:{ /* ConfigureRenderSetupPreset17BD4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC10E))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088DC11A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088DC12B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017C54:{ /* SetRenderSetupAnimationPreset17C54 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC4C1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017C84:{ /* SetRenderSetupAnimationPreset17C84 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DC53B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017C9C:{ /* SetRenderSetupAnimationPreset17C9C */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DC589))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017CB4:{ /* SetRenderSetupAnimationPreset17CB4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB583))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017CCC:{ /* SetRenderSetupAnimationPreset17CCC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB583))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017CE4:{ /* SetRenderSetupAnimationPreset17CE4 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DB583))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017CFC:{ /* SetRenderSetupAnimationPreset17CFC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB5C5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017D14:{ /* SetRenderSetupAnimationPreset17D14 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB8CA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017D2C:{ /* SetRenderSetupAnimationPreset17D2C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB8CA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017D44:{ /* SetRenderSetupAnimationPreset17D44 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DB8CA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017D5C:{ /* SetRenderSetupAnimationPreset17D5C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB8E1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017D74:{ /* SetRenderSetupAnimationPreset17D74 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB8E1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017D8C:{ /* SetRenderSetupAnimationPreset17D8C */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DB8E1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017DA4:{ /* ConfigureRenderSetupPreset17DA4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB93D))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088DB95A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017DF4:{ /* ConfigureRenderSetupPreset17DF4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB93D))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088DB95A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017E44:{ /* SetRenderSetupAnimationPreset17E44 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB93D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017E5C:{ /* SetRenderSetupAnimationPreset17E5C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB93D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017E74:{ /* SetRenderSetupAnimationPreset17E74 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB97B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017E8C:{ /* SetRenderSetupAnimationPreset17E8C */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DB97B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017EA4:{ /* SetRenderSetupAnimationPreset17EA4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB99E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017EBC:{ /* ConfigureRenderSetupPreset17EBC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB9B3))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088DB9C8))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017F0C:{ /* ConfigureRenderSetupPreset17F0C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB9B3))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088DB9C8))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017F5C:{ /* ConfigureRenderSetupPreset17F5C */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DB9B3))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088DB9C8))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017FAC:{ /* SetRenderSetupAnimationPreset17FAC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB9DB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017FC4:{ /* SetRenderSetupAnimationPreset17FC4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB9DB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017FDC:{ /* SetRenderSetupAnimationPreset17FDC */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DB9DB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017FF4:{ /* SetRenderSetupAnimationPreset17FF4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBA13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801800C:{ /* SetRenderSetupAnimationAndIndicator1800C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB9F6))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DB9F6);
        break;
    }
    case 0x08018030:{ /* SetRenderSetupAnimationPreset18030 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBA28))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018048:{ /* SetRenderSetupAnimationPreset18048 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBA28))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018060:{ /* SetRenderSetupAnimationPreset18060 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBA52))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018078:{ /* SetRenderSetupAnimationPreset18078 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBA52))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018090:{ /* SetRenderSetupAnimationPreset18090 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBA72))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080180A8:{ /* SetRenderSetupAnimationPreset180A8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBA72))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080180C0:{ /* SetRenderSetupAnimationPreset180C0 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBA72))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080180D8:{ /* SetRenderSetupAnimationPreset180D8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBAA0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080180F0:{ /* SetRenderSetupAnimationPreset180F0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBAA0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018108:{ /* SetRenderSetupAnimationPreset18108 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBAA0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018120:{ /* SetRenderSetupAnimationPreset18120 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBAD8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018138:{ /* SetRenderSetupAnimationPreset18138 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBAD8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018150:{ /* SetRenderSetupAnimationPreset18150 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBAE4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018168:{ /* SetRenderSetupAnimationPreset18168 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBAE4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018180:{ /* SetRenderSetupAnimationPreset18180 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBAE4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018198:{ /* SetRenderSetupAnimationPreset18198 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBB99))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080181B0:{ /* SetRenderSetupAnimationPreset181B0 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBB3F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080181C8:{ /* SetRenderSetupAnimationPreset181C8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBB3F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080181E0:{ /* SetRenderSetupAnimationPreset181E0 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBB99))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080181F8:{ /* SetRenderSetupAnimationPreset181F8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBB99))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018210:{ /* SetRenderSetupAnimationPreset18210 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBB99))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018228:{ /* ConfigureRenderSetupPreset18228 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBBAE))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088DBBD9))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DBBAE);
        break;
    }
    case 0x08018284:{ /* SetRenderSetupAnimationPreset18284 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBD36))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801829C:{ /* SetRenderSetupAnimationPreset1829C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBD36))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080182B4:{ /* SetRenderSetupAnimationPreset182B4 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBD36))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080182CC:{ /* SetRenderSetupAnimationPreset182CC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBD60))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080182E4:{ /* SetRenderSetupAnimationPreset182E4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBD87))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018314:{ /* SetRenderSetupAnimationPreset18314 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBDD3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801832C:{ /* SetRenderSetupAnimationPreset1832C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBDD3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018344:{ /* SetRenderSetupAnimationPreset18344 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBDE8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801835C:{ /* SetRenderSetupAnimationPreset1835C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBDE8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018374:{ /* SetRenderSetupAnimationPreset18374 */
        if(!raw_animation(m,hn_read8(m,0x03001F50),0x088DBDE8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801838C:{ /* SetRenderSetupAnimationPreset1838C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBDF4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080183A4:{ /* SetRenderSetupAnimationPreset183A4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088DBE09))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080183BC:{ /* SetRenderSetupAnimationPreset183BC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBE1E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080183D4:{ /* SetRenderSetupAnimationPreset183D4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBE1E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080183EC:{ /* SetRenderSetupAnimationPreset183EC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBE2A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018404:{ /* SetRenderSetupAnimationPreset18404 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBE2A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801841C:{ /* SetRenderSetupAnimationPreset1841C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DC13C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018434:{ /* SetRenderSetupAnimationAndIndicator18434 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC148))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DC148);
        break;
    }
    case 0x08018458:{ /* SetRenderSetupAnimationAndIndicator18458 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DC148))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C00),0x088DC148);
        break;
    }
    case 0x0801847C:{ /* SetRenderSetupAnimationAndIndicator1847C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC205))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DC205);
        break;
    }
    case 0x080184A0:{ /* SetRenderSetupAnimationPreset184A0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088DC258))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080184B8:{ /* SetRenderSetupAnimationPreset184B8 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088DC264))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080184D0:{ /* SetRenderSetupAnimationPreset184D0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088DC270))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080184E8:{ /* SetRenderSetupAnimationAndIndicator184E8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC27C))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DC27C);
        break;
    }
    case 0x0801850C:{ /* SetRenderSetupAnimationPreset1850C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088DC27C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018524:{ /* SetRenderSetupAnimationPreset18524 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088DC288))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801853C:{ /* ConfigureRenderSetupPreset1853C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DBE80))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088DBE8C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088DBE9D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080185BC:{ /* ConfigureRenderSetupPreset185BC */
        if(!animation(m,hn_read8(m,0x03001F50),0x088DC02F))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088DC044))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801860C:{ /* ConfigureRenderSetupPreset1860C */
        if(!hn_graphics_configure_raw(m,5,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC4D6))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E1B11))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DC4D6);
        break;
    }
    case 0x08018684:{ /* SetRenderSetupAnimationAndIndicator18684 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC0AC))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DC0AC);
        break;
    }
    case 0x080186A8:{ /* SetRenderSetupAnimationAndIndicator186A8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC420))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DC420);
        break;
    }
    case 0x080186CC:{ /* ConfigureRenderSetupPreset186CC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1393))return HN_SCENE_RESOURCE;
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DBEC3))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E1FEB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DBEC3);
        break;
    }
    case 0x08018734:{ /* ConfigureRenderSetupPreset18734 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBF1E))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E1FEB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018784:{ /* SelectActorAnimationForXZDirection18784 */
        switch((uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03002C04),hn_read8(m,0x03002C0C),false))
        {
        case 3:
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB5C5))return HN_SCENE_RESOURCE;
        break;
        case 4:
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB5DC))return HN_SCENE_RESOURCE;
        break;
        case 1:
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB59A))return HN_SCENE_RESOURCE;
        break;
        case 2:
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DB583))return HN_SCENE_RESOURCE;
        break;
        }
        break;
    }
    case 0x080187F8:{ /* SelectActorAnimationForXZDirection187F8 */
        switch((uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03002C00),hn_read8(m,0x03002C0C),false))
        {
        case 3:
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB5C5))return HN_SCENE_RESOURCE;
        break;
        case 4:
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB5DC))return HN_SCENE_RESOURCE;
        break;
        case 1:
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB59A))return HN_SCENE_RESOURCE;
        break;
        case 2:
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DB583))return HN_SCENE_RESOURCE;
        break;
        }
        break;
    }
    case 0x0801886C:{ /* SetRenderSetupAnimationPreset1886C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DE8D1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018884:{ /* SetRenderSetupAnimationPreset18884 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DE6FA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801889C:{ /* SetRenderSetupAnimationPreset1889C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DE90A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080188B4:{ /* SetRenderSetupAnimationPreset188B4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DE916))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080188CC:{ /* SetRenderSetupAnimationPreset188CC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DE963))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080188E4:{ /* SetRenderSetupAnimationPreset188E4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DE9CA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080188FC:{ /* ConfigureRenderSetupPreset188FC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DE9DF))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E2254))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E222E))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        if(!priority(m,hn_read8(m,0x03002C04) - 1,0))return HN_SCENE_RESOURCE;
        if(!priority(m,hn_read8(m,0x03002C04) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018998:{ /* SetRenderSetupAnimationPreset18998 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEA3A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080189B0:{ /* SetRenderSetupAnimationPreset189B0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEA61))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080189C8:{ /* SetRenderSetupAnimationPreset189C8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEA79))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080189E0:{ /* SetRenderSetupAnimationPreset189E0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEA85))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080189F8:{ /* SetRenderSetupAnimationPreset189F8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEA9D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018A10:{ /* SetRenderSetupAnimationPreset18A10 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEAA9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018A28:{ /* SetRenderSetupAnimationPreset18A28 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEC87))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018A40:{ /* SetRenderSetupAnimationPreset18A40 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DECA0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018A58:{ /* SetRenderSetupAnimationPreset18A58 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DECB9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018A70:{ /* SetRenderSetupAnimationPreset18A70 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DECDE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018A88:{ /* SetRenderSetupAnimationPreset18A88 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DECF7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018AA0:{ /* SetRenderSetupAnimationPreset18AA0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DED10))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018AB8:{ /* SetRenderSetupAnimationPreset18AB8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEDFD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018AD0:{ /* SetRenderSetupAnimationAndIndicator18AD0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088DEF5F))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C04),0x088DEF5F);
        break;
    }
    case 0x08018AF4:{ /* SetRenderSetupAnimationPreset18AF4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E0F32))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018B0C:{ /* SetRenderSetupAnimationPreset18B0C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E0EFA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018B24:{ /* SetRenderSetupAnimationPreset18B24 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E0F86))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018B3C:{ /* SetRenderSetupAnimationPreset18B3C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E0FA2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018B54:{ /* SetRenderSetupAnimationPreset18B54 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E102D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018B6C:{ /* SetRenderSetupAnimationPreset18B6C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E1049))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018B84:{ /* SetRenderSetupAnimationPreset18B84 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E1065))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018B9C:{ /* SetRenderSetupAnimationPreset18B9C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E1081))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018BB4:{ /* SetRenderSetupAnimationPreset18BB4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E10B6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018BCC:{ /* SetRenderSetupAnimationPreset18BCC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E0F6A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018BE4:{ /* SetRenderSetupAnimationPreset18BE4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E1150))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018BFC:{ /* SetRenderSetupAnimationPreset18BFC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E1167))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018C14:{ /* ConfigureRenderSetupPreset18C14 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E117C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E1188))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018C64:{ /* SetRenderSetupAnimationPreset18C64 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E1199))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018C7C:{ /* ConfigureRenderSetupPreset18C7C */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E11AE))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018CC0:{ /* SetRenderSetupAnimationPreset18CC0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E1209))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018CD8:{ /* SetRenderSetupAnimationPreset18CD8 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E1215))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018CF0:{ /* ConfigureRenderSetupPreset18CF0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E122A))return HN_SCENE_RESOURCE;
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00));
        hn_write32(m,0x03001F7C,(uint32_t)0x088E122A);
        break;
    }
    case 0x08018D20:{ /* SetRenderSetupAnimationPreset18D20 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088E10EB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018D38:{ /* ClearRenderSetupNeighborFields6018D38 */
        if(!field(m,hn_read8(m,0x03002C0C) - 1,0))return HN_SCENE_RESOURCE;
        if(!field(m,hn_read8(m,0x03002C0C) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018D64:{ /* ClearRenderSetupNeighborFields6018D64 */
        if(!field(m,hn_read8(m,0x03002C0C) - 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018D80:{ /* ClearRenderSetupNeighborFields6018D80 */
        if(!field(m,hn_read8(m,0x03002C0C) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018D9C:{ /* ClearRenderSetupNeighborFields6018D9C */
        if(!field(m,hn_read8(m,0x03001F50) - 1,0))return HN_SCENE_RESOURCE;
        if(!field(m,hn_read8(m,0x03001F50) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018DC8:{ /* ClearRenderSetupNeighborFields6018DC8 */
        if(!field(m,hn_read8(m,0x03001F50) - 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018DE4:{ /* ClearRenderSetupNeighborFields6018DE4 */
        if(!field(m,hn_read8(m,0x03001F50) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018E00:{ /* ClearRenderSetupNeighborFields6018E00 */
        if(!field(m,hn_read8(m,0x03002C04) - 1,0))return HN_SCENE_RESOURCE;
        if(!field(m,hn_read8(m,0x03002C04) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018E48:{ /* ClearRenderSetupNeighborFields6018E48 */
        if(!field(m,hn_read8(m,0x03002C04) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018E64:{ /* ClearRenderSetupNeighborFields6018E64 */
        if(!field(m,hn_read8(m,0x03002C00) - 1,0))return HN_SCENE_RESOURCE;
        if(!field(m,hn_read8(m,0x03002C00) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018EAC:{ /* ClearRenderSetupNeighborFields6018EAC */
        if(!field(m,hn_read8(m,0x03002C00) + 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018EC8:{ /* ConfigureRenderSetupPreset18EC8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E38F4))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018F1C:{ /* ConfigureRenderSetupPreset18F1C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088E38FF))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018F70:{ /* ConfigureRenderSetupPreset18F70 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E38F4))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018FC4:{ /* ConfigureRenderSetupPreset18FC4 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E38F4))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019018:{ /* ConfigureRenderSetupPreset19018 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E3995))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801906C:{ /* ConfigureRenderSetupPreset1906C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3995))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080190C0:{ /* ConfigureRenderSetupPreset190C0 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3995))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019114:{ /* ConfigureRenderSetupPreset19114 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,21,1,0xB0,0,0,0,0x088E3995))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,21,2,3,22))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019154:{ /* ConfigureRenderSetupPreset19154 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3920))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080191A8:{ /* ConfigureRenderSetupPreset191A8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E3920))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080191FC:{ /* ConfigureRenderSetupPreset191FC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E3936))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019250:{ /* ConfigureRenderSetupPreset19250 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3920))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080192F8:{ /* ConfigureRenderSetupPreset192F8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,21,1,0xB0,0,0,0,0x088E3920))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,21,2,3,22))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019338:{ /* ConfigureRenderSetupPreset19338 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3943))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801938C:{ /* ConfigureRenderSetupPreset1938C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3943))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080193E0:{ /* ConfigureRenderSetupPreset193E0 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088E3957))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019434:{ /* ConfigureRenderSetupPreset19434 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E3957))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019488:{ /* ConfigureRenderSetupPreset19488 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3957))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080194DC:{ /* ConfigureRenderSetupPreset194DC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3957))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019530:{ /* ConfigureRenderSetupPreset19530 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088E3961))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801962C:{ /* ConfigureRenderSetupPreset1962C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E3961))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019680:{ /* ConfigureRenderSetupPreset19680 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3961))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080196D4:{ /* ConfigureRenderSetupPreset196D4 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E396C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019728:{ /* ConfigureRenderSetupPreset19728 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E396C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801977C:{ /* ConfigureRenderSetupPreset1977C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E396C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019824:{ /* ConfigureRenderSetupPreset19824 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3980))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080198CC:{ /* ConfigureRenderSetupPreset198CC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E398B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019920:{ /* ConfigureRenderSetupPreset19920 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,21,1,0xB0,0,0,0,0x088E398B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,21,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019964:{ /* ConfigureRenderSetupPreset19964 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E399F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080199B8:{ /* ConfigureRenderSetupPreset199B8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1364))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E39A9))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019A60:{ /* ConfigureRenderSetupPreset19A60 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E39F2))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019AB4:{ /* ConfigureRenderSetupPreset19AB4 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088E3A10))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019B08:{ /* ConfigureRenderSetupPreset19B08 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E3A10))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019B5C:{ /* ConfigureRenderSetupPreset19B5C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3A24))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019C04:{ /* ConfigureRenderSetupPreset19C04 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3A2F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019C58:{ /* ConfigureRenderSetupPreset19C58 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3A3A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019C9C:{ /* ConfigureRenderSetupPreset19C9C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3A44))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019CF0:{ /* ConfigureRenderSetupPreset19CF0 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3A44))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019D44:{ /* ConfigureRenderSetupPreset19D44 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3A4E))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019D98:{ /* ConfigureRenderSetupPreset19D98 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3A4E))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019DEC:{ /* ConfigureRenderSetupPreset19DEC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3A63))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019E40:{ /* ConfigureRenderSetupPreset19E40 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1364))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3A8B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019E94:{ /* ConfigureRenderSetupPreset19E94 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1364))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3A8B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019EE8:{ /* ConfigureRenderSetupPreset19EE8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3AAB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019F3C:{ /* ConfigureRenderSetupPreset19F3C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3AAB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019F90:{ /* ConfigureRenderSetupPreset19F90 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3ABF))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A06C:{ /* ConfigureRenderSetupPreset1A06C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3AF1))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A0C0:{ /* ConfigureRenderSetupPreset1A0C0 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3B19))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A114:{ /* ConfigureRenderSetupPreset1A114 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E141A))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088E1C01))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A168:{ /* ConfigureRenderSetupPreset1A168 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088AD9FA))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A1AC:{ /* ConfigureRenderSetupPreset1A1AC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1364))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C0C) - 1,1,0xB0,0,0,0,0x088ADA54))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C0C) - 1,2,3,hn_read8(m,0x03002C0C)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A200:{ /* ConfigureRenderSetupPreset1A200 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088B7069))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) + 1,1,0xB0,0,0,0,0x088B7087))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) + 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A274:{ /* ConfigureRenderSetupPreset1A274 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088B705E))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 2,1,0xB0,0,0,0,0x088B707C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 2,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A2E8:{ /* ConfigureRenderSetupPreset1A2E8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1364))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088ADABC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A33C:{ /* ConfigureRenderSetupPreset1A33C */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088B6C1C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) + 1,1,0xB0,0,0,0,0x088B6C57))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) + 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A404:{ /* ConfigureRenderSetupPreset1A404 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E138B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088C286A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A554:{ /* ConfigureRenderSetupPreset1A554 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088C5D58))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A598:{ /* ConfigureRenderSetupPreset1A598 */
        if(!hn_graphics_configure_raw(m,5,(uint32_t)0x088E1432))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088C5EA1))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A5EC:{ /* ConfigureRenderSetupPreset1A5EC */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3217))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A638:{ /* ConfigureRenderSetupPreset1A638 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088E3267))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A684:{ /* ConfigureRenderSetupPreset1A684 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E13AB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088BA70B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A6D8:{ /* ConfigureRenderSetupPreset1A6D8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E143A))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3168))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A72C:{ /* ConfigureRenderSetupPreset1A72C */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E31CC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A770:{ /* ConfigureRenderSetupPreset1A770 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088C9046))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A7C4:{ /* ConfigureRenderSetupPreset1A7C4 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088C9046))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A818:{ /* ConfigureRenderSetupPreset1A818 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088C8C1C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A86C:{ /* ConfigureRenderSetupPreset1A86C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088C8C1C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A8C0:{ /* ConfigureRenderSetupPreset1A8C0 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088C4A49))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A914:{ /* ConfigureRenderSetupPreset1A914 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088C4A49))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A968:{ /* ConfigureRenderSetupPreset1A968 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CABA2))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A9BC:{ /* ConfigureRenderSetupPreset1A9BC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CABB9))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AA64:{ /* ConfigureRenderSetupPreset1AA64 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088CACBD))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AB0C:{ /* ConfigureRenderSetupPreset1AB0C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088CACD6))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AB60:{ /* ConfigureRenderSetupPreset1AB60 */
        if(!hn_render_configure_raw(m,21,1,0xB0,0,0,0,0x088CA25D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,21,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AB98:{ /* ConfigureRenderSetupPreset1AB98 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088CDC3B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801ABEC:{ /* ConfigureRenderSetupPreset1ABEC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088CDC4C))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AC40:{ /* ConfigureRenderSetupPreset1AC40 */
        if(!hn_render_configure_raw(m,21,1,0xB0,0,0,0,0x088CA29A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,21,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AC78:{ /* ConfigureRenderSetupPreset1AC78 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CA0B7))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801ACBC:{ /* ConfigureRenderSetupPreset1ACBC */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CA270))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AD00:{ /* ConfigureRenderSetupPreset1AD00 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088CA270))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AD44:{ /* ConfigureRenderSetupPreset1AD44 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CA0D3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AD88:{ /* ConfigureRenderSetupPreset1AD88 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CA0C5))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801ADCC:{ /* ConfigureRenderSetupPreset1ADCC */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088CA223))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AE10:{ /* ConfigureRenderSetupPreset1AE10 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CA0E1))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AE54:{ /* ConfigureRenderSetupPreset1AE54 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088CA231))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AE98:{ /* ConfigureRenderSetupPreset1AE98 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088CA215))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AEDC:{ /* ConfigureRenderSetupPreset1AEDC */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CACBD))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CACD6))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AF50:{ /* ConfigureRenderSetupPreset1AF50 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CB10D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AF94:{ /* ConfigureRenderSetupPreset1AF94 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CDBDC))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CDBFB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B008:{ /* ConfigureRenderSetupPreset1B008 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088CDBDC))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088CDBFB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B07C:{ /* ConfigureRenderSetupPreset1B07C */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CBCAE))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B0C0:{ /* ConfigureRenderSetupPreset1B0C0 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088CBCAE))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B104:{ /* ConfigureRenderSetupPreset1B104 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E141A))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E1C01))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B158:{ /* ConfigureRenderSetupPreset1B158 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088D1475))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B1AC:{ /* ConfigureRenderSetupPreset1B1AC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088D1475))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B200:{ /* ConfigureRenderSetupPreset1B200 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088D1533))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B254:{ /* ConfigureRenderSetupPreset1B254 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088D2700))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B298:{ /* ConfigureRenderSetupPreset1B298 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088D272D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B2DC:{ /* ConfigureRenderSetupPreset1B2DC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D2700))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B330:{ /* ConfigureRenderSetupPreset1B330 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D272D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B384:{ /* ConfigureRenderSetupPreset1B384 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D2D02))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B3D8:{ /* ConfigureRenderSetupPreset1B3D8 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D2DE2))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B42C:{ /* ConfigureRenderSetupPreset1B42C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D2DF3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B480:{ /* ConfigureRenderSetupPreset1B480 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1338))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D300D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B4D4:{ /* ConfigureRenderSetupPreset1B4D4 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1338))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D301B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B528:{ /* ConfigureRenderSetupPreset1B528 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E138B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D315B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B57C:{ /* ConfigureRenderSetupPreset1B57C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E138B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D31A3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B5D0:{ /* ConfigureRenderSetupPreset1B5D0 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D3435))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B624:{ /* ConfigureRenderSetupPreset1B624 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D345F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B678:{ /* ConfigureRenderSetupPreset1B678 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D3584))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B6CC:{ /* ConfigureRenderSetupPreset1B6CC */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D3608))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B720:{ /* ConfigureRenderSetupPreset1B720 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088D446D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B764:{ /* ConfigureRenderSetupPreset1B764 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088D447B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B7EC:{ /* ConfigureRenderSetupPreset1B7EC */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088D44C6))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B830:{ /* ConfigureRenderSetupPreset1B830 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088D44D4))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B874:{ /* ConfigureRenderSetupPreset1B874 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D4891))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B8B8:{ /* ConfigureRenderSetupPreset1B8B8 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D5359))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D5378))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B92C:{ /* ConfigureRenderSetupPreset1B92C */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D53AC))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D53CB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B9A0:{ /* ConfigureRenderSetupPreset1B9A0 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D557E))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D558F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BA14:{ /* ConfigureRenderSetupPreset1BA14 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D55AC))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D55BD))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BA88:{ /* ConfigureRenderSetupPreset1BA88 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D55E3))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D5602))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BB70:{ /* ConfigureRenderSetupPreset1BB70 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D8C0B))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BBC4:{ /* ConfigureRenderSetupPreset1BBC4 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D8CE3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BC18:{ /* ConfigureRenderSetupPreset1BC18 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D86C2))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BC5C:{ /* ConfigureRenderSetupPreset1BC5C */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D9104))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BCA0:{ /* ConfigureRenderSetupPreset1BCA0 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D9115))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BCE4:{ /* ConfigureRenderSetupPreset1BCE4 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E137B))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088DB987))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BD38:{ /* ConfigureRenderSetupPreset1BD38 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088DBA7E))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BD7C:{ /* ConfigureRenderSetupPreset1BD7C */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088DC49F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BDD0:{ /* ConfigureRenderSetupPreset1BDD0 */
        if(!hn_graphics_configure_raw(m,4,(uint32_t)0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088DC4B0))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BE24:{ /* ConfigureRenderSetupPreset1BE24 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088DB95A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801467C:{ /* SetExtendedAnimationPreset1467C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD44E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014694:{ /* SetExtendedAnimationPreset14694 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD478))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080146AC:{ /* SetExtendedAnimationPreset146AC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD22A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080146C4:{ /* SetExtendedAnimationPreset146C4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD4F1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080146DC:{ /* SetExtendedAnimationPreset146DC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD4FD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080146F4:{ /* ConfigureExtendedNextSlot146F4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088BD285))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088E1B67))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014744:{ /* SelectExtendedAnimationForXZDirection14744 */
        switch((uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03002C00), hn_read8(m,0x03002C0C),false)) { case 3: if(!raw_animation(m,hn_read8(m,0x03002C00),0x088BD22A))return HN_SCENE_RESOURCE; break; case 4: if(!raw_animation(m,hn_read8(m,0x03002C00),0x088BD21E))return HN_SCENE_RESOURCE; break; }
        break;
    }
    case 0x0801478C:{ /* SetExtendedAnimationAndIndicator1478C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088ABCC8))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088ABCC8);
        break;
    }
    case 0x080147B0:{ /* SetExtendedAnimationPreset147B0 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA52A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080147C8:{ /* SetExtendedAnimationPreset147C8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA536))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080147E0:{ /* SetExtendedAnimationPreset147E0 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA54B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080147F8:{ /* SetExtendedAnimationPreset147F8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA557))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014810:{ /* SetExtendedAnimationPreset14810 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA563))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014828:{ /* SetExtendedAnimationPreset14828 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA5EB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014840:{ /* SetExtendedAnimationPreset14840 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA456))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014858:{ /* SetExtendedAnimationPreset14858 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA462))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014870:{ /* SetExtendedAnimationPreset14870 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA47A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014888:{ /* SetExtendedAnimationPreset14888 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA5F7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080148A0:{ /* SetExtendedAnimationPreset148A0 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA62E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080148B8:{ /* SetExtendedAnimationPreset148B8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA63A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080148D0:{ /* SetExtendedAnimationAndIndicator148D0 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA646))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088BA646);
        break;
    }
    case 0x080148F4:{ /* SetExtendedAnimationAndIndicator148F4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA7C6))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088BA7C6);
        break;
    }
    case 0x08014918:{ /* SetExtendedAnimationAndIndicator14918 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA7AC))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088BA7AC);
        break;
    }
    case 0x0801493C:{ /* SetExtendedAnimationAndIndicator1493C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA72F))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088BA72F);
        break;
    }
    case 0x08014960:{ /* SetExtendedAnimationAndIndicator14960 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088BA82B))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088BA82B);
        break;
    }
    case 0x08014984:{ /* SetExtendedAnimationPreset14984 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5C60))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801499C:{ /* SetExtendedAnimationPreset1499C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5C71))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080149B4:{ /* SetExtendedAnimationPreset149B4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5C8E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080149CC:{ /* SetExtendedAnimationPreset149CC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5CA3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080149E4:{ /* SetExtendedAnimationPreset149E4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5CAF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080149FC:{ /* SetExtendedAnimationPreset149FC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5CE6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014A14:{ /* SetExtendedAnimationPreset14A14 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5D40))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014A2C:{ /* SetExtendedAnimationPreset14A2C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5D4C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014A44:{ /* SetExtendedAnimationAndIndicator14A44 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5D7E))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088C5D7E);
        break;
    }
    case 0x08014A68:{ /* SetExtendedAnimationPreset14A68 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5AAF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014A80:{ /* SetExtendedAnimationPreset14A80 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5DA3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014A98:{ /* SetExtendedAnimationPreset14A98 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5DFD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014AB0:{ /* SetExtendedAnimationPreset14AB0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5E12))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014AC8:{ /* SetExtendedAnimationPreset14AC8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C6073))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014AE0:{ /* SetExtendedAnimationPreset14AE0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C609D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014AF8:{ /* SetExtendedAnimationPreset14AF8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5E1E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014B10:{ /* SetExtendedAnimationPreset14B10 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5F72))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014B28:{ /* SetExtendedAnimationPreset14B28 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5F5B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014B40:{ /* SetExtendedAnimationPreset14B40 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C5EE1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014B58:{ /* SetExtendedAnimationPreset14B58 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C6002))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014B70:{ /* SetExtendedAnimationPreset14B70 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088BC55B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014B88:{ /* SetExtendedAnimationAndIndicator14B88 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088BC570))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088BC570);
        break;
    }
    case 0x08014BAC:{ /* SetExtendedAnimationPreset14BAC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C9019))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014BC4:{ /* SetExtendedAnimationPreset14BC4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C9019))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014BDC:{ /* SetExtendedAnimationPreset14BDC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8BAB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014BF4:{ /* SetExtendedAnimationPreset14BF4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8BAB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014C0C:{ /* SetExtendedAnimationPreset14C0C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8BF9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014C24:{ /* SetExtendedAnimationPreset14C24 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8BF9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014C3C:{ /* SetExtendedAnimationPreset14C3C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C902E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014C54:{ /* SetExtendedAnimationPreset14C54 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C902E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014C6C:{ /* SetExtendedAnimationPreset14C6C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C903A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014C84:{ /* SetExtendedAnimationPreset14C84 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C903A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014C9C:{ /* SetExtendedAnimationPreset14C9C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8C05))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014CB4:{ /* SetExtendedAnimationPreset14CB4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8C05))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014CCC:{ /* SetExtendedAnimationPreset14CCC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8C4E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014CE4:{ /* SetExtendedAnimationPreset14CE4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8C4E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014CFC:{ /* SetExtendedAnimationPreset14CFC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C905C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014D14:{ /* SetExtendedAnimationPreset14D14 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C905C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014D2C:{ /* SetExtendedAnimationPreset14D2C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C9068))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014D44:{ /* SetExtendedAnimationPreset14D44 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C9068))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014D5C:{ /* SetExtendedAnimationPreset14D5C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C9074))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014D74:{ /* SetExtendedAnimationPreset14D74 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C9074))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014D8C:{ /* SetExtendedAnimationPreset14D8C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8C63))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014DA4:{ /* SetExtendedAnimationPreset14DA4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8C63))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014DBC:{ /* SetExtendedAnimationPreset14DBC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8A4A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014DD4:{ /* SetExtendedAnimationPreset14DD4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8A4A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014DEC:{ /* SetExtendedAnimationPreset14DEC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8D38))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014E04:{ /* SetExtendedAnimationPreset14E04 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8D38))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014E1C:{ /* SetExtendedAnimationPreset14E1C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C9080))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014E34:{ /* SetExtendedAnimationPreset14E34 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C9080))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014E4C:{ /* SetExtendedAnimationPreset14E4C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8DDE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014E64:{ /* SetExtendedAnimationPreset14E64 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C910A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014E7C:{ /* SetExtendedAnimationPreset14E7C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C910A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014E94:{ /* SetExtendedAnimationPreset14E94 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8EC1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014EAC:{ /* SetExtendedAnimationPreset14EAC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C8EC1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014EC4:{ /* SetExtendedAnimationPreset14EC4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C8DEA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014EDC:{ /* SetExtendedAnimationPreset14EDC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C9201))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014EF4:{ /* SetExtendedAnimationPreset14EF4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C9201))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014F0C:{ /* SetExtendedAnimationPreset14F0C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C92E2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014F24:{ /* SetExtendedAnimationPreset14F24 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C92E2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014F3C:{ /* SetExtendedAnimationPreset14F3C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C92EE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014F54:{ /* SetExtendedAnimationPreset14F54 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C92EE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014F6C:{ /* SetExtendedAnimationPreset14F6C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C92FA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014F84:{ /* SetExtendedAnimationPreset14F84 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C92FA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014F9C:{ /* ConfigureExtendedAuxiliaryNeighbors14F9C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C930F))return HN_SCENE_RESOURCE; if(!auxiliary(m,hn_read8(m,0x03002C04) - 1,hn_read8(m,0x03002C04),0x088C9324))return HN_SCENE_RESOURCE; if(!auxiliary(m,hn_read8(m,0x03002C04) + 1,hn_read8(m,0x03002C04),0x088C9356))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014FE0:{ /* ConfigureExtendedAuxiliaryNeighbors14FE0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088C930F))return HN_SCENE_RESOURCE; if(!auxiliary(m,hn_read8(m,0x03002C00) - 1,hn_read8(m,0x03002C00),0x088C9324))return HN_SCENE_RESOURCE; if(!auxiliary(m,hn_read8(m,0x03002C00) + 1,hn_read8(m,0x03002C00),0x088C9356))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015024:{ /* SetGraphicsAnimationAndIndicator15024 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C93E2))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088C93E2);
        break;
    }
    case 0x08015048:{ /* SetGraphicsAnimationAndIndicator15048 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088C936F))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088C936F);
        break;
    }
    case 0x0801506C:{ /* SetGraphicsAnimationPreset1506C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C96A6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015084:{ /* SetGraphicsAnimationAndIndicator15084 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C96BB))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088C96BB);
        break;
    }
    case 0x080150A8:{ /* SetGraphicsAnimationPreset150A8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C9673))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080150C0:{ /* SetGraphicsAnimationPreset150C0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088C9785))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080150D8:{ /* SetGraphicsAnimationPreset150D8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BF3EF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080150F0:{ /* SetGraphicsAnimationPreset150F0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BF404))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015108:{ /* SetGraphicsAnimationPreset15108 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BF41C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015120:{ /* SetGraphicsAnimationPreset15120 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BF410))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015138:{ /* SetGraphicsAnimationPreset15138 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BF461))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015150:{ /* SetGraphicsAnimationPreset15150 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BEAF6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015168:{ /* SetGraphicsAnimationPreset15168 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BE722))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015180:{ /* SetGraphicsAnimationPreset15180 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BEB8A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015198:{ /* SetGraphicsAnimationPreset15198 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BEC3E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080151B0:{ /* SetGraphicsAnimationPreset151B0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BEC4A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080151C8:{ /* SetGraphicsAnimationPreset151C8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BEC5F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080151E0:{ /* SetGraphicsAnimationPreset151E0 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BEC80))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080151F8:{ /* SetGraphicsAnimationPreset151F8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BEC8C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015210:{ /* SetGraphicsAnimationPreset15210 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BECD3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015228:{ /* SetGraphicsAnimationPreset15228 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BEC74))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015240:{ /* SetGraphicsAnimationPreset15240 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BED06))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015258:{ /* SetGraphicsAnimationPreset15258 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BED2A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015270:{ /* SetGraphicsAnimationAndIndicator15270 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BE7F3))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088BE7F3);
        break;
    }
    case 0x08015294:{ /* SetGraphicsAnimationAndIndicator15294 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BF46D))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088BF46D);
        break;
    }
    case 0x080152B8:{ /* SetGraphicsAnimationAndIndicator152B8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CA82A))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CA82A);
        break;
    }
    case 0x080152DC:{ /* SetGraphicsAnimationPreset152DC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CA8B0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080152F4:{ /* SetGraphicsAnimationPreset152F4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CA8B0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801530C:{ /* SetGraphicsAnimationPreset1530C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CA8C5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015324:{ /* SetGraphicsAnimationPreset15324 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CA8C5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801533C:{ /* SetGraphicsAnimationPreset1533C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CAAC3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015354:{ /* SetGraphicsAnimationPreset15354 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CAA8F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801536C:{ /* SetGraphicsAnimationPreset1536C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CAB5E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015384:{ /* SetGraphicsAnimationPreset15384 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CABD0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801539C:{ /* SetGraphicsAnimationPreset1539C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CABD0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080153B4:{ /* SetGraphicsAnimationPreset153B4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CAC43))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080153CC:{ /* SetGraphicsAnimationPreset153CC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CAC90))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080153E4:{ /* SetGraphicsAnimationPreset153E4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CAC90))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080153FC:{ /* SetGraphicsAnimationPreset153FC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CAC9C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015414:{ /* SetGraphicsAnimationPreset15414 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CAC9C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801542C:{ /* SetGraphicsAnimationPreset1542C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CACA8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015444:{ /* SetGraphicsAnimationPreset15444 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CACA8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801545C:{ /* SetGraphicsAnimationPreset1545C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CACEF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015474:{ /* SetGraphicsAnimationPreset15474 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CACEF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801548C:{ /* SetGraphicsAnimationPreset1548C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CACFB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080154A4:{ /* SetGraphicsAnimationPreset154A4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CACFB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080154BC:{ /* SetGraphicsAnimationPreset154BC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CAEED))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080154D4:{ /* SetGraphicsAnimationPreset154D4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CAF1E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080154EC:{ /* SetGraphicsAnimationPreset154EC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CAF1E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015504:{ /* SetGraphicsAnimationPreset15504 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CB0AA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801551C:{ /* SetGraphicsAnimationPreset1551C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CB0B6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015534:{ /* SetGraphicsAnimationPreset15534 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB0DD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801554C:{ /* SetGraphicsAnimationPreset1554C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CB0DD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015564:{ /* SetGraphicsAnimationPreset15564 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB0E9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801557C:{ /* SetGraphicsAnimationPreset1557C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CB0E9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015594:{ /* SetGraphicsAnimationPreset15594 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB0F5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080155AC:{ /* SetGraphicsAnimationPreset155AC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CB0F5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080155C4:{ /* SetGraphicsAnimationPreset155C4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB101))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080155DC:{ /* SetGraphicsAnimationPreset155DC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CB101))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080155F4:{ /* SetGraphicsAnimationPreset155F4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB129))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801560C:{ /* SetGraphicsAnimationPreset1560C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB129))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015624:{ /* SetGraphicsAnimationPreset15624 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB177))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801563C:{ /* SetGraphicsAnimationPreset1563C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CB177))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015654:{ /* SetGraphicsAnimationPreset15654 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB1C4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801566C:{ /* SetGraphicsAnimationAndIndicator1566C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB245))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CB245);
        break;
    }
    case 0x08015690:{ /* SetGraphicsAnimationAndIndicator15690 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB26E))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CB26E);
        break;
    }
    case 0x080156B4:{ /* SetGraphicsAnimationPreset156B4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB3F8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080156CC:{ /* SetGraphicsAnimationAndIndicator156CC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB4ED))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CB4ED);
        break;
    }
    case 0x080156F0:{ /* SetGraphicsAnimationAndIndicator156F0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB58B))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088CB58B);
        break;
    }
    case 0x08015714:{ /* SetGraphicsAnimationAndIndicator15714 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB4F0))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CB4F0);
        break;
    }
    case 0x08015738:{ /* SetGraphicsAnimationAndIndicator15738 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB58E))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088CB58E);
        break;
    }
    case 0x0801575C:{ /* SetGraphicsAnimationPreset1575C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB749))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015774:{ /* SetGraphicsAnimationPreset15774 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB76C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801578C:{ /* SetGraphicsAnimationPreset1578C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB3F8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080157A4:{ /* SetGraphicsAnimationPreset157A4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB6BC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080157BC:{ /* SetGraphicsAnimationPreset157BC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB6BC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080157D4:{ /* SetGraphicsAnimationPreset157D4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB749))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080157EC:{ /* SetGraphicsAnimationPreset157EC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CA41A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015804:{ /* SetGraphicsAnimationPreset15804 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CA41A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801581C:{ /* SetGraphicsAnimationPreset1581C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB76C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015834:{ /* SetGraphicsAnimationPreset15834 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB7DE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801584C:{ /* SetGraphicsAnimationPreset1584C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB8BE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015864:{ /* SetGraphicsAnimationPreset15864 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB8BE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801587C:{ /* SetGraphicsAnimationPreset1587C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB98F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015894:{ /* SetGraphicsAnimationPreset15894 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CB98F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080158AC:{ /* SetGraphicsAnimationPreset158AC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CBA99))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080158C4:{ /* SetGraphicsAnimationPreset158C4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CBC61))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080158DC:{ /* SetGraphicsAnimationPreset158DC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CBE2E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080158F4:{ /* SetGraphicsAnimationPreset158F4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CBEEA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801590C:{ /* SetGraphicsAnimationPreset1590C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CBEEA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015924:{ /* SetGraphicsAnimationPreset15924 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CBEFF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801593C:{ /* SetGraphicsAnimationPreset1593C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CC3E4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015954:{ /* SetGraphicsAnimationPreset15954 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CC3E4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801596C:{ /* SetGraphicsAnimationAndIndicator1596C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CC40C))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088CC40C);
        break;
    }
    case 0x0801599C:{ /* SetGraphicsAnimationAndIndicator1599C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CC40C))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00)); hn_write32(m,0x03001F7C,0x088CC40C);
        break;
    }
    case 0x080159CC:{ /* ConfigureGraphicsAnimationSlots159CC */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CC423))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015A10:{ /* ConfigureGraphicsAnimationSlots15A10 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088CC423))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015A54:{ /* SetGraphicsAnimationPreset15A54 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CC471))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015A6C:{ /* SetGraphicsAnimationPreset15A6C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CC776))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015A84:{ /* SetGraphicsAnimationPreset15A84 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CC776))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015A9C:{ /* SetGraphicsAnimationPreset15A9C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CCD52))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015AB4:{ /* SetGraphicsAnimationPreset15AB4 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CCD52))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015ACC:{ /* SetGraphicsAnimationPreset15ACC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CCD7D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015AE4:{ /* SetGraphicsAnimationPreset15AE4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CCE1C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015AFC:{ /* SetGraphicsAnimationPreset15AFC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CCE31))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015B14:{ /* SetGraphicsAnimationPreset15B14 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CCE31))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015B2C:{ /* SetGraphicsAnimationPreset15B2C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CCF02))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015B44:{ /* SetGraphicsAnimationPreset15B44 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CCF49))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015B5C:{ /* SetGraphicsAnimationPreset15B5C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CCF49))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015B74:{ /* ConfigureGraphicsAnimationSlots15B74 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD0A1))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CD126))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; if(!hn_graphics_configure_raw(m,5,0x088E13AB))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088CD0A1);
        break;
    }
    case 0x08015BE8:{ /* ConfigureGraphicsAnimationSlots15BE8 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CD0A1))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088CD126))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE; if(!hn_graphics_configure_raw(m,5,0x088E13AB))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00)); hn_write32(m,0x03001F7C,0x088CD0A1);
        break;
    }
    case 0x08015C5C:{ /* ConfigureGraphicsAnimationSlots15C5C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD213))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CD21F))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CD233))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015CDC:{ /* ConfigureGraphicsAnimationSlots15CDC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CD213))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088CD21F))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088CD233))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015D5C:{ /* ConfigureGraphicsAnimationSlots15D5C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD247))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CD2CC))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; if(!hn_graphics_configure_raw(m,5,0x088E13AB))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088CD247);
        break;
    }
    case 0x08015DD0:{ /* ConfigureGraphicsAnimationSlots15DD0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CD247))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088CD2CC))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE; if(!hn_graphics_configure_raw(m,5,0x088E13AB))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00)); hn_write32(m,0x03001F7C,0x088CD247);
        break;
    }
    case 0x08015E44:{ /* SetGraphicsAnimationPreset15E44 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD357))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015E5C:{ /* SetGraphicsAnimationPreset15E5C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CD357))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015E74:{ /* SetGraphicsAnimationPreset15E74 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD467))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015E8C:{ /* SetGraphicsAnimationPreset15E8C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD485))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015EA4:{ /* SetGraphicsAnimationPreset15EA4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD4AC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015EBC:{ /* SetGraphicsAnimationPreset15EBC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CD4AC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015ED4:{ /* SetGraphicsAnimationPreset15ED4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CD4DE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015EEC:{ /* SetGraphicsAnimationPreset15EEC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CD516))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015F04:{ /* SetGraphicsAnimationPreset15F04 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CD516))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015F1C:{ /* SetGraphicsAnimationPreset15F1C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CD522))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015F34:{ /* SetGraphicsAnimationPreset15F34 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CD522))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015F4C:{ /* SetGraphicsAnimationPreset15F4C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CD54D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015F64:{ /* SetGraphicsAnimationPreset15F64 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CD54D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015F7C:{ /* SetGraphicsAnimationPreset15F7C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CD68A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015F94:{ /* SetGraphicsAnimationPreset15F94 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CD6A2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015FAC:{ /* SetGraphicsAnimationPreset15FAC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDBC7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015FC4:{ /* SetGraphicsAnimationPreset15FC4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CD6AE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08015FDC:{ /* SetGraphicsAnimationAndIndicator15FDC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CD6D1))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CD6D1);
        break;
    }
    case 0x08016000:{ /* SetGraphicsAnimationPreset16000 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CA671))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016018:{ /* SetGraphicsAnimationPreset16018 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDB3B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016030:{ /* SetGraphicsAnimationPreset16030 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CDB3B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016048:{ /* SetGraphicsAnimationPreset16048 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDB50))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016060:{ /* SetGraphicsAnimationPreset16060 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDB50))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016078:{ /* SetGraphicsAnimationPreset16078 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDBC7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016090:{ /* SetGraphicsAnimationPreset16090 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDBC7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080160A8:{ /* SetGraphicsAnimationPreset160A8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDC1A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080160C0:{ /* SetGraphicsAnimationPreset160C0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDC1A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080160D8:{ /* SetGraphicsAnimationPreset160D8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDC2F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080160F0:{ /* SetGraphicsAnimationPreset160F0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDC2F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016108:{ /* SetGraphicsAnimationPreset16108 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDC5D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016120:{ /* SetGraphicsAnimationPreset16120 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDC7D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016138:{ /* SetGraphicsAnimationPreset16138 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDC7D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016150:{ /* SetGraphicsAnimationPreset16150 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDD8A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016168:{ /* SetGraphicsAnimationPreset16168 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDDAE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016180:{ /* SetGraphicsAnimationPreset16180 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDDD2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016198:{ /* SetGraphicsAnimationPreset16198 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CA43E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080161B0:{ /* SetGraphicsAnimationPreset161B0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CB98F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080161C8:{ /* SetGraphicsAnimationPreset161C8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CDE83))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080161E0:{ /* SetGraphicsAnimationPreset161E0 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CDEA3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080161F8:{ /* SetGraphicsAnimationPreset161F8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CDEA3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016210:{ /* SetGraphicsAnimationPreset16210 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CE018))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016228:{ /* SetGraphicsAnimationPreset16228 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE13E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016240:{ /* SetGraphicsAnimationPreset16240 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CE15A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016258:{ /* SetGraphicsAnimationPreset16258 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE0FB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016270:{ /* SetGraphicsAnimationPreset16270 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CE0FB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016288:{ /* SetGraphicsAnimationPreset16288 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CE17D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080162A0:{ /* SetGraphicsAnimationPreset162A0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088CE17D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080162B8:{ /* SetGraphicsAnimationPreset162B8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE201))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080162D0:{ /* SetGraphicsAnimationPreset162D0 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE20D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080162E8:{ /* SetGraphicsAnimationPreset162E8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CE20D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016300:{ /* SetGraphicsAnimationPreset16300 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE44C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016318:{ /* SetGraphicsAnimationPreset16318 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CE44C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016330:{ /* SetGraphicsAnimationPreset16330 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE464))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016348:{ /* SetGraphicsAnimationPreset16348 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE4BA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016360:{ /* SetGraphicsAnimationPreset16360 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE4CF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016378:{ /* SetGraphicsAnimationPreset16378 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE507))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016390:{ /* SetGraphicsAnimationPreset16390 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CE602))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080163A8:{ /* SetGraphicsAnimationPreset163A8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088CE617))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080163C0:{ /* ConfigureGraphicsAnimationSlots163C0 */
        if(!hn_graphics_configure_raw(m,4,0x088E1422))return HN_SCENE_RESOURCE; if(!animation(m,hn_read8(m,0x03002C04),0x088CD142))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E18D8))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088CD142);
        break;
    }
    case 0x08016434:{ /* ConfigureGraphicsAnimationSlots16434 */
        if(!hn_graphics_configure_raw(m,4,0x088E1422))return HN_SCENE_RESOURCE; if(!animation(m,hn_read8(m,0x03002C00),0x088CD142))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E18D8))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00)); hn_write32(m,0x03001F7C,0x088CD142);
        break;
    }
    case 0x080164A8:{ /* SetGraphicsAnimationAndIndicator164A8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CB285))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CB285);
        break;
    }
    case 0x080164CC:{ /* SetGraphicsAnimationAndIndicator164CC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD4B8))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088CD4B8);
        break;
    }
    case 0x080164FC:{ /* ConfigureGraphicsAnimationSlots164FC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CE372))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E1A24))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088CE372);
        break;
    }
    case 0x08016560:{ /* SetGraphicsAnimationAndIndicator16560 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CE3BC))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088CE3BC);
        break;
    }
    case 0x08016590:{ /* SetGraphicsAnimationAndIndicator16590 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CD393))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CD393);
        break;
    }
    case 0x080165B4:{ /* SetGraphicsAnimationAndIndicator165B4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CB1D0))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CB1D0);
        break;
    }
    case 0x080165D8:{ /* ConfigureGraphicsAnimationSlots165D8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CE470))return HN_SCENE_RESOURCE; if(!hn_graphics_configure_raw(m,4,0x088E135C))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E1ADF))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CE470);
        break;
    }
    case 0x08016640:{ /* ConfigureGraphicsAnimationSlots16640 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088CD5B9))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E20C5))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CD5B9);
        break;
    }
    case 0x0801669C:{ /* ConfigureGraphicsAnimationSlots1669C */
        if(!hn_graphics_configure_raw(m,4,0x088E13AB))return HN_SCENE_RESOURCE; if(!animation(m,hn_read8(m,0x03002C04),0x088CE62F))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088E32B2))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088CE62F);
        break;
    }
    case 0x08016704:{ /* InitializeActorGraphicsScript16704 */
        if(!hn_graphics_configure_raw(m,5,0x088CA083))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016718:{ /* ConfigureGraphicsAnimationSlots16718 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CA000))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04) + 1); hn_write32(m,0x03001F7C,0x088CA000);
        break;
    }
    case 0x08016770:{ /* SetGraphicsAnimationPreset16770 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CA432))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016788:{ /* SelectActorAnimationForXZDirection16788 */
        switch((uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03002C04), hn_read8(m,0x03002C0C),false)) { case 3: if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CA432))return HN_SCENE_RESOURCE; break; case 4: if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CA43E))return HN_SCENE_RESOURCE; break; case 1: if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CA426))return HN_SCENE_RESOURCE; break; case 2: if(!raw_animation(m,hn_read8(m,0x03002C04),0x088CA41A))return HN_SCENE_RESOURCE; break; }
        break;
    }
    case 0x080167FC:{ /* SetDirectionalAnimationPreset167FC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D12CF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016814:{ /* SetDirectionalAnimationPreset16814 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D12C3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801682C:{ /* SetDirectionalAnimationPreset1682C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D12AE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016844:{ /* SetDirectionalAnimationPreset16844 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D12EF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801685C:{ /* SetDirectionalAnimationPreset1685C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D12FB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016874:{ /* SetDirectionalAnimationPreset16874 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D1313))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801688C:{ /* SetDirectionalAnimationPreset1688C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D134F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080168A4:{ /* SetDirectionalAnimationPreset168A4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D135B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080168BC:{ /* SetDirectionalAnimationPreset168BC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D1367))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080168D4:{ /* SetDirectionalAnimationPreset168D4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D13AE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080168EC:{ /* SetDirectionalAnimationPreset168EC */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D142E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016904:{ /* SetDirectionalAnimationPreset16904 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D1446))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801691C:{ /* SetDirectionalAnimationPreset1691C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D1469))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016934:{ /* SetDirectionalAnimationPreset16934 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D1509))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801694C:{ /* SetDirectionalAnimationPreset1694C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D151E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016964:{ /* SetDirectionalAnimationPreset16964 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D1552))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801697C:{ /* SetDirectionalAnimationPreset1697C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D1579))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016994:{ /* ConfigureDirectionalAnimationSlots16994 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D13CC))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E18D8))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088D13CC);
        break;
    }
    case 0x080169F8:{ /* ConfigureDirectionalAnimationSlots169F8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D1373))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E1933))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; if(!hn_graphics_configure_raw(m,4,0x088E1340))return HN_SCENE_RESOURCE; hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04)); hn_write32(m,0x03001F7C,0x088D1373);
        break;
    }
    case 0x08016A6C:{ /* SetDirectionalAnimationPreset16A6C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D2FE5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016A84:{ /* SetDirectionalAnimationPreset16A84 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D3134))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016A9C:{ /* SetDirectionalAnimationPreset16A9C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3134))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016AB4:{ /* SetDirectionalAnimationPreset16AB4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3225))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016ACC:{ /* SetDirectionalAnimationPreset16ACC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D323A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016AE4:{ /* SetDirectionalAnimationPreset16AE4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3246))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016AFC:{ /* ActorAnimationNoop16AFC */
        break;
    }
    case 0x08016B00:{ /* SetDirectionalAnimationPreset16B00 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D26EB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016B18:{ /* SetDirectionalAnimationPreset16B18 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D27BE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016B30:{ /* SetDirectionalAnimationPreset16B30 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D280F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016B48:{ /* SetDirectionalAnimationPreset16B48 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D2A47))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016B60:{ /* SetDirectionalAnimationPreset16B60 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D2CED))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016B78:{ /* SetDirectionalAnimationPreset16B78 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D2DD6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016B90:{ /* SetDirectionalAnimationPreset16B90 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D325B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016BA8:{ /* SetDirectionalAnimationPreset16BA8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3267))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016BC0:{ /* SetDirectionalAnimationPreset16BC0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3273))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016BD8:{ /* SetDirectionalAnimationPreset16BD8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D33D4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016BF0:{ /* SetDirectionalAnimationPreset16BF0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D325B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016C08:{ /* SetDirectionalAnimationPreset16C08 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3429))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016C20:{ /* SetDirectionalAnimationPreset16C20 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D347C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016C38:{ /* SetDirectionalAnimationPreset16C38 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D34A6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016C50:{ /* SetDirectionalAnimationPreset16C50 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D34DD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016C68:{ /* SetDirectionalAnimationPreset16C68 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D34DD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016C80:{ /* SetDirectionalAnimationPreset16C80 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D34DD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016C98:{ /* SetDirectionalAnimationPreset16C98 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D34DD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016CB0:{ /* SetDirectionalAnimationPreset16CB0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3501))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016CC8:{ /* SetDirectionalAnimationPreset16CC8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D352B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016CE0:{ /* SetDirectionalAnimationPreset16CE0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3540))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016CF8:{ /* SetDirectionalAnimationPreset16CF8 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D3555))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016D10:{ /* SetDirectionalAnimationPreset16D10 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3555))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016D28:{ /* SetDirectionalAnimationAndIndicator16D28 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D356D))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D356D);
        break;
    }
    case 0x08016D4C:{ /* SetDirectionalAnimationPreset16D4C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3655))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016D64:{ /* SetDirectionalAnimationPreset16D64 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D35FC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016D7C:{ /* SetDirectionalAnimationPreset16D7C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D35FC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016D94:{ /* SetDirectionalAnimationPreset16D94 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3617))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016DAC:{ /* SetDirectionalAnimationAndIndicator16DAC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D3623))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088D3623);
        break;
    }
    case 0x08016DD0:{ /* SetDirectionalAnimationAndIndicator16DD0 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3623))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D3623);
        break;
    }
    case 0x08016DF4:{ /* SetDirectionalAnimationPreset16DF4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D366A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016E0C:{ /* ConfigureDirectionalAnimationSlots16E0C */
        if(!hn_graphics_configure_raw(m,4,0x088E1338))return HN_SCENE_RESOURCE; if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D366A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016E34:{ /* SetDirectionalAnimationAndIndicator16E34 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D36C3))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D36C3);
        break;
    }
    case 0x08016E58:{ /* SetDirectionalAnimationAndIndicator16E58 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D371D))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D371D);
        break;
    }
    case 0x08016E7C:{ /* SetDirectionalAnimationPreset16E7C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3B62))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016E94:{ /* SetDirectionalAnimationPreset16E94 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3B6E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016EAC:{ /* SetDirectionalAnimationPreset16EAC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3BAD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016EC4:{ /* SetDirectionalAnimationPreset16EC4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D2530))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016EDC:{ /* SetDirectionalAnimationAndIndicator16EDC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D31D5))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088D31D5);
        break;
    }
    case 0x08016F00:{ /* ConfigureDirectionalAnimationSlots16F00 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D2D21))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088E1B3C))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016F50:{ /* SetDirectionalAnimationAndIndicator16F50 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D2E04))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D2E04);
        break;
    }
    case 0x08016F74:{ /* SetDirectionalAnimationPreset16F74 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D30BE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016F8C:{ /* SetDirectionalAnimationPreset16F8C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D3BC2))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016FA4:{ /* SetDirectionalAnimationPreset16FA4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D4592))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016FBC:{ /* SetDirectionalAnimationPreset16FBC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D4586))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016FD4:{ /* SetDirectionalAnimationPreset16FD4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D585E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08016FEC:{ /* SetDirectionalAnimationPreset16FEC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D5916))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017004:{ /* SetDirectionalAnimationPreset17004 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D594C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801701C:{ /* SetDirectionalAnimationPreset1701C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D59E9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017034:{ /* SetDirectionalAnimationPreset17034 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D5ABD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801704C:{ /* SetDirectionalAnimationPreset1704C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D5AC9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017064:{ /* SetDirectionalAnimationPreset17064 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D5AD5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801707C:{ /* SetDirectionalAnimationPreset1707C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D5C88))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017094:{ /* SetDirectionalAnimationPreset17094 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D5E06))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080170AC:{ /* SetDirectionalAnimationPreset170AC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D530D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080170C4:{ /* SetDirectionalAnimationPreset170C4 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D5319))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080170DC:{ /* SetDirectionalAnimationPreset170DC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4661))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080170F4:{ /* SetDirectionalAnimationPreset170F4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4885))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801710C:{ /* SetDirectionalAnimationPreset1710C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D48A9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017124:{ /* SetDirectionalAnimationPreset17124 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D48E7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801713C:{ /* SetDirectionalAnimationPreset1713C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4699))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017154:{ /* SetDirectionalAnimationPreset17154 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D47D7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801716C:{ /* SetDirectionalAnimationPreset1716C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D4A23))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017184:{ /* SetDirectionalAnimationPreset17184 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D507E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801719C:{ /* SetDirectionalAnimationPreset1719C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4D18))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080171B4:{ /* SetDirectionalAnimationPreset171B4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4D34))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080171CC:{ /* SetDirectionalAnimationPreset171CC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4D8D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080171E4:{ /* SetDirectionalAnimationPreset171E4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4DA9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080171FC:{ /* SetDirectionalAnimationPreset171FC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4F48))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017214:{ /* SetDirectionalAnimationPreset17214 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D4592))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801722C:{ /* SetDirectionalAnimationPreset1722C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5066))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017244:{ /* SetDirectionalAnimationPreset17244 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D508A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801725C:{ /* SetDirectionalAnimationPreset1725C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D585E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017274:{ /* SetDirectionalAnimationPreset17274 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5873))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801728C:{ /* SetDirectionalAnimationPreset1728C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5888))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080172A4:{ /* SetDirectionalAnimationPreset172A4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D519F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080172BC:{ /* SetDirectionalAnimationPreset172BC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5344))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080172D4:{ /* SetDirectionalAnimationPreset172D4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5397))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080172EC:{ /* SetDirectionalAnimationPreset172EC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D53EA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017304:{ /* SetDirectionalAnimationPreset17304 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D53F6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801731C:{ /* SetDirectionalAnimationPreset1731C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D540E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017334:{ /* SetDirectionalAnimationPreset17334 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5423))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801734C:{ /* SetDirectionalAnimationPreset1734C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5438))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017364:{ /* SetDirectionalAnimationPreset17364 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D544D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801737C:{ /* SetDirectionalAnimationPreset1737C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5466))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017394:{ /* SetDirectionalAnimationPreset17394 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5566))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080173AC:{ /* SetDirectionalAnimationPreset173AC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5572))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080173C4:{ /* SetDirectionalAnimationPreset173C4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D55A0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080173DC:{ /* SetDirectionalAnimationPreset173DC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D55CE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080173F4:{ /* SetDirectionalAnimationPreset173F4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5621))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801740C:{ /* SetDirectionalAnimationPreset1740C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D606A))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017424:{ /* SetDirectionalAnimationPreset17424 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D607F))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801743C:{ /* SetDirectionalAnimationPreset1743C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D608B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017454:{ /* SetDirectionalAnimationPreset17454 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D60AB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801746C:{ /* ConfigureDirectionalAnimationSlots1746C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D4FF9))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088D4FF9); if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088D503A))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; if(!hn_graphics_configure_raw(m,4,0x088E138B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080174D4:{ /* ConfigureDirectionalAnimationSlots174D4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D5486))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E209E))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017524:{ /* ConfigureDirectionalAnimationSlots17524 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D60B7))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088E32FB))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017574:{ /* SetDirectionalAnimationPreset17574 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D83E7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801758C:{ /* SetDirectionalAnimationPreset1758C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8623))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080175A4:{ /* SetDirectionalAnimationPreset175A4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D85AB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080175BC:{ /* SetDirectionalAnimationPreset175BC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D8623))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080175D4:{ /* SetDirectionalAnimationPreset175D4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D86AD))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080175EC:{ /* SetDirectionalAnimationPreset175EC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D86E4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017604:{ /* SetDirectionalAnimationPreset17604 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D8740))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801761C:{ /* SetDirectionalAnimationPreset1761C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D90C5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017634:{ /* SetDirectionalAnimationPreset17634 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D90C5))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801764C:{ /* SetDirectionalAnimationPreset1764C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D90D1))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017664:{ /* SetDirectionalAnimationPreset17664 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D90F8))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801767C:{ /* SetDirectionalAnimationPreset1767C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D7E99))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017694:{ /* SetDirectionalAnimationPreset17694 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D836C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080176AC:{ /* SetDirectionalAnimationPreset176AC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D836C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080176C4:{ /* SetDirectionalAnimationPreset176C4 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D839E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080176DC:{ /* SetDirectionalAnimationPreset176DC */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D839E))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080176F4:{ /* SetDirectionalAnimationPreset176F4 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D83B3))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801770C:{ /* SetDirectionalAnimationPreset1770C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8AA0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017724:{ /* SetDirectionalAnimationPreset17724 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8AAC))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801773C:{ /* SetDirectionalAnimationPreset1773C */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D8BFF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017754:{ /* SetDirectionalAnimationPreset17754 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8C2B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801776C:{ /* SetDirectionalAnimationPreset1776C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8C45))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017784:{ /* SetDirectionalAnimationPreset17784 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8C45))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801779C:{ /* SetDirectionalAnimationPreset1779C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8CCE))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080177B4:{ /* ConfigureDirectionalAnimationSlots177B4 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D8CE3))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080177F8:{ /* SetDirectionalAnimationPreset177F8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8D5C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017810:{ /* SetDirectionalAnimationPreset17810 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8D5C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017828:{ /* SetDirectionalAnimationPreset17828 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8DBF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017840:{ /* SetDirectionalAnimationPreset17840 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8E76))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017858:{ /* SetDirectionalAnimationPreset17858 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8EAF))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017870:{ /* SetDirectionalAnimationPreset17870 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8EBB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017888:{ /* SetDirectionalAnimationPreset17888 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D8EBB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080178A0:{ /* SetDirectionalAnimationPreset178A0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8F2C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080178B8:{ /* SetDirectionalAnimationPreset178B8 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8F67))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080178D0:{ /* SetDirectionalAnimationPreset178D0 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8F67))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080178E8:{ /* SetDirectionalAnimationPreset178E8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D8FC9))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017900:{ /* SetDirectionalAnimationPreset17900 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088D9052))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017918:{ /* SetDirectionalAnimationAndIndicator17918 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8EEE))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C04),0x088D8EEE);
        break;
    }
    case 0x0801793C:{ /* SetDirectionalAnimationAndIndicator1793C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8EEE))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D8EEE);
        break;
    }
    case 0x08017960:{ /* SetDirectionalAnimationPreset17960 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D874C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017978:{ /* SetDirectionalAnimationAndIndicator17978 */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D88AE))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D88AE);
        break;
    }
    case 0x0801799C:{ /* ConfigureDirectionalAnimationSlots1799C */
        if(!animation(m,hn_read8(m,0x03002C04),0x088D8AC1))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D8AC1); if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088D8B13))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088D8B59))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017A2C:{ /* ConfigureDirectionalAnimationSlots17A2C */
        if(!animation(m,hn_read8(m,0x03002C00),0x088D8B6A))return HN_SCENE_RESOURCE; indicator(m,hn_read8(m,0x03002C00),0x088D8B6A); if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D8BB9))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D8BEE))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017AB8:{ /* ConfigureDirectionalAnimationSlots17AB8 */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088D8F73))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D8FB1))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE; if(!hn_graphics_configure_raw(m,4,0x088E138B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013384:{ /* SetActorAnimationPreset13384 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B5325))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080136B8:{ /* SetActorClassAnimationPreset136B8 */
        if(!animation(m,hn_read8(m,0x03001F50),0x088B6FD7))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08013FEC:{ /* SetSecondaryAnimationPreset13FEC */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088C18D6))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014034:{ /* SetSecondaryAnimationPreset14034 */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088C18FB))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08014400:{ /* SetSecondaryAnimationPreset14400 */
        if(!animation(m,hn_read8(m,0x03002C04),0x088BC636))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08017C6C:{ /* SetRenderSetupAnimationPreset17C6C */
        if(!raw_animation(m,hn_read8(m,0x03002C04),0x088DC493))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080182FC:{ /* SetRenderSetupAnimationPreset182FC */
        if(!raw_animation(m,hn_read8(m,0x03002C00),0x088DBD87))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018E2C:{ /* ClearRenderSetupNeighborFields6018E2C */
        if(!field(m,hn_read8(m,0x03002C04) - 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08018E90:{ /* ClearRenderSetupNeighborFields6018E90 */
        if(!field(m,hn_read8(m,0x03002C00) - 1,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080192A4:{ /* ConfigureRenderSetupPreset192A4 */
        if(!hn_graphics_configure_raw(m,4,0x088E135C))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E3920))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019584:{ /* ConfigureRenderSetupPreset19584 */
        if(!hn_graphics_configure_raw(m,4,0x088E1330))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03001F50) - 1,1,0xB0,0,0,0,0x088E3961))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03001F50) - 1,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080195D8:{ /* ConfigureRenderSetupPreset195D8 */
        if(!hn_graphics_configure_raw(m,4,0x088E1330))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3961))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080197D0:{ /* ConfigureRenderSetupPreset197D0 */
        if(!hn_graphics_configure_raw(m,4,0x088E1330))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3980))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019878:{ /* ConfigureRenderSetupPreset19878 */
        if(!hn_graphics_configure_raw(m,4,0x088E137B))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E398B))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019A0C:{ /* ConfigureRenderSetupPreset19A0C */
        if(!hn_graphics_configure_raw(m,4,0x088E1364))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E39A9))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019BB0:{ /* ConfigureRenderSetupPreset19BB0 */
        if(!hn_graphics_configure_raw(m,4,0x088E135C))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3A24))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08019FE4:{ /* ConfigureRenderSetupPreset19FE4 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088E3AC9))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A028:{ /* ConfigureRenderSetupPreset1A028 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088E3AC9))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A3B0:{ /* ConfigureRenderSetupPreset1A3B0 */
        if(!hn_graphics_configure_raw(m,4,0x088E138B))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088C286A))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A458:{ /* ConfigureRenderSetupPreset1A458 */
        if(!hn_graphics_configure_raw(m,4,0x088E1338))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088C5C7D))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A4AC:{ /* ConfigureRenderSetupPreset1A4AC */
        if(!hn_graphics_configure_raw(m,4,0x088E137B))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088C5CC4))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801A500:{ /* ConfigureRenderSetupPreset1A500 */
        if(!hn_graphics_configure_raw(m,4,0x088E137B))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088C5CD5))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AA10:{ /* ConfigureRenderSetupPreset1AA10 */
        if(!hn_graphics_configure_raw(m,4,0x088E1330))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1,1,0xB0,0,0,0,0x088CACBD))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801AAB8:{ /* ConfigureRenderSetupPreset1AAB8 */
        if(!hn_graphics_configure_raw(m,4,0x088E1330))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088CACD6))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801B7A8:{ /* ConfigureRenderSetupPreset1B7A8 */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1,1,0xB0,0,0,0,0x088D449D))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1,2,3,hn_read8(m,0x03002C04)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0801BAFC:{ /* ConfigureRenderSetupPreset1BAFC */
        if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1,1,0xB0,0,0,0,0x088D48C5))return HN_SCENE_RESOURCE; if(!hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1,1,0xB0,0,0,0,0x088D48D6))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE; if(!hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1,2,3,hn_read8(m,0x03002C00)))return HN_SCENE_RESOURCE;
        break;
    }
    default:return base_actor_presets(m,callback);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
