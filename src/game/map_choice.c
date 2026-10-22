#include "game/map_choice.h"
#include "game/state.h"
#include "game/render.h"
#include "game/presets.h"
#include "game/graphics.h"
#include "game/audio.h"
#include "game/motion.h"
#include "game/collision.h"
#include "game/input.h"
#include "game/particles.h"
#include "gba/bios.h"

static bool valid(HNMemory *m,unsigned index)
{return hn_read32(m,HN_RENDER_SLOTS)&&index<hn_read8(m,HN_RENDER_INFO+16);}
static bool select_animation(HNMemory *m,uint8_t index,GameAddr resource,bool gate)
{
    if(!valid(m,index))return false;
    if(gate&&index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,resource);return !m->fault;
}
static bool field(HNMemory *m,uint8_t index,uint8_t value)
{if(!valid(m,index))return false;hn_write8(m,hn_render_slot(m,index)+0x60,value);return !m->fault;}
static bool field_animation(HNMemory *m,uint8_t index,uint8_t value,GameAddr resource)
{
    if(!valid(m,index))return false;
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    return field(m,index,value)&&select_animation(m,index,resource,false);
}
static bool position(HNMemory *m,uint8_t index,int32_t x,int32_t y,int32_t z)
{
    if(!valid(m,index))return false;GameAddr slot=hn_render_slot(m,index);
    hn_write32(m,slot+0x18,((uint32_t)(uint16_t)x<<16)+0x8000);
    hn_write32(m,slot+0x1C,((uint32_t)(uint16_t)y<<16)+0x8000);
    hn_write32(m,slot+0x20,((uint32_t)(uint16_t)z<<16)+0x8000);return !m->fault;
}
static bool priority(HNMemory *m,uint8_t index,uint8_t value)
{if(!valid(m,index))return false;hn_render_flags(m,index,12,(uint8_t)(value<<2));return !m->fault;}
static void indicator(HNMemory *m,uint8_t index,GameAddr resource)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,resource);}
static bool flag(HNMemory *m,unsigned id)
{return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void set_flag(HNMemory *m,unsigned id,bool value)
{
    GameAddr p=0x03002180+(id>>3);unsigned mask=1u<<(id&7),old=hn_read8(m,p);
    hn_write8(m,p,(uint8_t)(value?old|mask:old&~mask));
}
static bool apply(HNMemory *m,GameAddr callback)
{uint32_t ignored;return hn_map_choice_apply(m,callback,&ignored)==HN_SCENE_OK;}
static bool preset(HNMemory *m,GameAddr callback)
{return hn_preset_apply(m,callback)==HN_SCENE_OK;}

/* Ordered map-choice presets adapted from the reference source. Only resource
   identifiers are retained; their contents stay in the player-supplied ROM. */
static HNSceneResult simple_apply(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x0802F5DC:{ /* ConfigureMapChoiceSlots13And14F5DC */
        if(!select_animation(m,13,0x088B6FD7,true))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,0,0,0x088B6FE3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F664:{ /* SetMapChoiceSlot13Animation6FBBAndFace */
        if(!select_animation(m,13,0x088B6FBB,true))return HN_SCENE_RESOURCE;
        if(!apply(m,0x0802F67C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F688:{ /* ConfigureMapChoiceSlots12And13F688 */
        if(!select_animation(m,13,0x088D836C,true))return HN_SCENE_RESOURCE;
        if(!field_animation(m,12,1,0x088D776D))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F6A8:{ /* SetMapChoiceSlot13IndicatorF6A8 */
        GameAddr animation=0x088D8546;
        if(!select_animation(m,13,animation,true))return HN_SCENE_RESOURCE;
        indicator(m,13,animation);
        break;
    }
    case 0x0802F6C8:{ /* SetMapChoiceSlot13IndicatorF6C8 */
        GameAddr animation=0x088D85AB;
        if(!select_animation(m,13,animation,true))return HN_SCENE_RESOURCE;
        indicator(m,13,animation);
        break;
    }
    case 0x0802F6E8:{ /* SetMapChoiceSlots10And13F6E8 */
        if(!select_animation(m,10,0x088AC0C6,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088D8623,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F708:{ /* ConfigureMapChoiceSlot14IndicatorF708 */
        GameAddr animation=0x088D86AD;
        if(!field_animation(m,13,2,animation))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,animation,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,0,0,animation = 0x088D86C2))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        indicator(m,14,animation);
        break;
    }
    case 0x0802F75C:{ /* ConfigureMapChoiceSlot14F75C */
        if(!select_animation(m,13,0x088D83E7,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,0,0,0x088B6FE3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F798:{ /* SetMapChoiceSlot13IndicatorF798 */
        GameAddr animation;
        hn_write16(m,0x0201DC40+(44)*2,16);
        animation=0x088D7050;
        if(!select_animation(m,13,animation,true))return HN_SCENE_RESOURCE;
        indicator(m,13,animation);
        break;
    }
    case 0x0802F7C4:{ /* SetMapChoiceSlot13IndicatorF7C4 */
        GameAddr animation;
        hn_write16(m,0x0201DC40+(44)*2,16);
        hn_write16(m,0x0201DC40+(43)*2,16);
        hn_write16(m,0x0201DC40+(42)*2,16);
        animation=0x088D7064;
        if(!select_animation(m,13,animation,true))return HN_SCENE_RESOURCE;
        indicator(m,13,animation);
        break;
    }
    case 0x0802F7F8:{ /* SetMapChoiceSlot13IndicatorF7F8 */
        GameAddr animation;
        hn_write16(m,0x0201DC40+(44)*2,16);
        hn_write16(m,0x0201DC40+(43)*2,16);
        hn_write16(m,0x0201DC40+(42)*2,16);
        animation=0x088D7078;
        if(!select_animation(m,13,animation,true))return HN_SCENE_RESOURCE;
        indicator(m,13,animation);
        break;
    }
    case 0x0802F82C:{ /* SetMapChoiceSlot13IndicatorF82C */
        GameAddr animation;
        hn_write16(m,0x0201DC40+(44)*2,16);
        hn_write16(m,0x0201DC40+(43)*2,16);
        hn_write16(m,0x0201DC40+(42)*2,16);
        animation=0x088D708C;
        if(!select_animation(m,13,animation,true))return HN_SCENE_RESOURCE;
        indicator(m,13,animation);
        break;
    }
    case 0x0802F860:{ /* ConfigureMapChoiceSlot14F860 */
        if(!hn_render_configure(m,14,1,176,0,0,0,0x088D8658))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F890:{ /* ConfigureMapChoiceLinkedSlots14And15F890 */
        if(!apply(m,0x0802F67C))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088D86F9,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,0,0,0x088D870E))return HN_SCENE_RESOURCE;
        if(!priority(m,14,1))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088D8727))return HN_SCENE_RESOURCE;
        if(!priority(m,15,1))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F908:{ /* SetMapChoiceSlot13AnimationF908 */
        if(!apply(m,0x0802F67C))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088D86E4,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F920:{ /* EmptyMapChoiceCallbackF920 */
        break;
    }
    case 0x0802F924:{ /* EmptyMapChoiceCallbackF924 */
        break;
    }
    case 0x0802F928:{ /* EmptyMapChoiceCallbackF928 */
        break;
    }
    case 0x0802F92C:{ /* ConfigureMapChoiceSlots15And16F92C */
        if(!select_animation(m,16,0x088D836C,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088D8381))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F968:{ /* SetMapChoiceSlot19AnimationF968 */
        if(!select_animation(m,19,0x088D4645,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802F97C:{ /* SetMapChoiceSlot16InlineIndicatorF97C */
        GameAddr animation=0x088D8332;
        if(!select_animation(m,16,animation,true))return HN_SCENE_RESOURCE;
        hn_write8(m,50339756,16);
        hn_write32(m,50339708,(uint32_t)animation);
        break;
    }
    case 0x0802F9A4:{ /* SetMapChoiceSlot19InlineIndicatorF9A4 */
        GameAddr animation=0x088D4816;
        if(!select_animation(m,19,animation,true))return HN_SCENE_RESOURCE;
        hn_write8(m,50339756,19);
        hn_write32(m,50339708,(uint32_t)animation);
        break;
    }
    case 0x0802F9CC:{ /* SetMapChoiceSlots16And19InlineIndicatorF9CC */
        GameAddr animation=0x088D88E6;
        if(!select_animation(m,16,animation,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088D4938,true))return HN_SCENE_RESOURCE;
        hn_write8(m,50339756,16);
        hn_write32(m,50339708,(uint32_t)animation);
        break;
    }
    case 0x0802FA00:{ /* SetMapChoiceLeaderAnimationFA00 */
        if(!select_animation(m,hn_read8(m,0x03002C0C),0x088AD641,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FA18:{ /* ConfigureMapChoiceFollowerLinkedSlotFA18 */
        if(!select_animation(m,hn_read8(m,0x03001F50),0x088B6FD7,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,0,0,0x088B6FE3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FA68:{ /* SetMapChoiceFollowerAnimationFA68 */
        if(!select_animation(m,hn_read8(m,0x03001F50),0x088B6ADD,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FAE0:{ /* SetMapChoiceSlot16IndicatorFAE0 */
        GameAddr animation=0x088CAB0B;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x0802FB00:{ /* ConfigureMapChoiceLinkedSlot15FB00 */
        GameAddr animation=0x088CA971;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1338))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088E20EC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x0802FB58:{ /* ConfigureMapChoiceLinkedSlots15And17FB58 */
        GameAddr animation=0x088CAA8F;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1338))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088CAAD3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,0,0,0,0x088CAAEF))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x0802FBD4:{ /* ConfigureMapChoiceOrderedSlots15And17FBD4 */
        if(!select_animation(m,16,0x088CAB5E,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1338))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088CABA2))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!hn_render_reorder(m,15,16,0))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,0,0,0,0x088CABB9))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        if(!hn_render_reorder(m,17,16,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FC58:{ /* ClearMapChoiceSlots15And17Field60 */
        if(!field(m,15,0))return HN_SCENE_RESOURCE;
        if(!field(m,17,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FD34:{ /* ConfigureMapChoiceFollowerOffsetFD34 */
        if(!select_animation(m,hn_read8(m,0x03001F50),0x088B5073,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,-18,-20,0,0x088E3EE8))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FD8C:{ /* ConfigureMapChoiceFollowerOffsetFD8C */
        if(!select_animation(m,hn_read8(m,0x03001F50),0x088B508F,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,-24,0,0x088E4084))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,hn_read8(m,0x03001F50)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FDE4:{ /* ConfigureMapChoiceSlot17AndClear29FDE4 */
        if(!select_animation(m,16,0x088C0EBD,false))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,0,0,0,0x088C0F2A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        if(!field(m,29,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FE28:{ /* ConfigureMapChoiceRenderGroupFE28 */
        GameAddr animation=0x088C1177;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1340))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088C11E6))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,0,0,0,0x088C1201))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        if(!hn_render_reorder(m,15,16,1))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,23,1,113,0,0,0,0x088E6FCA))return HN_SCENE_RESOURCE;
        if(!field_animation(m,29,1,0x088C11E0))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x0802FED4:{ /* SetMapChoiceSlot16AnimationFED4 */
        if(!select_animation(m,16,0x088C121C,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FEE8:{ /* SetMapChoiceSlot16IndicatorFEE8 */
        GameAddr animation=0x088C1238;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x0802FF08:{ /* SetMapChoiceSlots16And29FF08 */
        if(!select_animation(m,16,0x088C14C2,false))return HN_SCENE_RESOURCE;
        if(!field_animation(m,29,1,0x088C1515))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FF28:{ /* SetMapChoiceSlot16AnimationFF28 */
        if(!select_animation(m,16,0x088C151B,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FF3C:{ /* SetMapChoiceSlot16IndicatorFF3C */
        GameAddr animation=0x088C153F;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x0802FF5C:{ /* ConfigureMapChoiceOffsetSlot17FF5C */
        if(!select_animation(m,16,0x088C151B,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,-20,-18,0,0x088E3EE8))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0802FFAC:{ /* SetMapChoiceClassAnimationsFFAC */
        GameAddr animation;
        if(!select_animation(m,hn_read8(m,0x03002C0C),animation = 0x088A6326,false))return HN_SCENE_RESOURCE;
        if(!select_animation(m,hn_read8(m,0x03001F50),0x088AA7CE,false))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),animation);
        break;
    }
    case 0x0802FFE4:{ /* ApplyMapChoiceActorPresetsFFE4 */
        if(!preset(m,0x08014034))return HN_SCENE_RESOURCE;
        if(!preset(m,0x0801A3B0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030140:{ /* SetMapChoiceSlot16Animation30140 */
        if(!select_animation(m,16,0x088E0EFA,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030154:{ /* SetMapChoiceSlot16Animation30154 */
        if(!select_animation(m,16,0x088E0F16,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030168:{ /* SetMapChoiceSlot16Animation30168 */
        if(!select_animation(m,16,0x088E0F32,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803017C:{ /* SetMapChoiceSlot16Animation3017C */
        if(!select_animation(m,16,0x088E0F4E,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030190:{ /* SetMapChoiceSlot16Animation30190 */
        if(!select_animation(m,16,0x088E0F6A,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080301A4:{ /* SetMapChoiceSlot16Animation301A4 */
        if(!select_animation(m,16,0x088E0F86,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080301B8:{ /* SetMapChoiceSlot16Animation301B8 */
        if(!select_animation(m,16,0x088E0FA2,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080301CC:{ /* SetMapChoiceSlot16Animation301CC */
        if(!select_animation(m,16,0x088E102D,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080301E0:{ /* SetMapChoiceSlot16Animation301E0 */
        if(!select_animation(m,16,0x088E1049,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080301F4:{ /* SetMapChoiceSlot16Animation301F4 */
        if(!select_animation(m,16,0x088E1065,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030208:{ /* SetMapChoiceSlot16Animation30208 */
        if(!select_animation(m,16,0x088E1081,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803021C:{ /* SetMapChoiceSlot16Animation3021C */
        if(!select_animation(m,16,0x088E10B6,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030230:{ /* SetMapChoiceSlot16Animation30230 */
        if(!select_animation(m,16,0x088E0CC4,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030244:{ /* StartMapChoiceSong23 */
        if(!hn_audio_song(m,23,HN_SONG_START))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803028C:{ /* SetMapChoiceSlot16Animation3028C */
        if(!select_animation(m,16,0x088DF42B,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080302DC:{ /* SetMapChoiceSlot16Animation302DC */
        if(!select_animation(m,16,0x088DF495,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803032C:{ /* SetMapChoiceSlot16Animation3032C */
        if(!select_animation(m,16,0x088DF2DC,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030340:{ /* ClearMapChoiceSlot15AndSet16Animation30340 */
        if(!field(m,15,0))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088DF2DC,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803035C:{ /* SetMapChoiceSlot16InlineIndicator3035C */
        GameAddr animation=0x088DF65D;
        if(!select_animation(m,16,animation,true))return HN_SCENE_RESOURCE;
        hn_write8(m,50339756,16);
        hn_write32(m,50339708,(uint32_t)animation);
        break;
    }
    case 0x08030384:{ /* SetMapChoiceSlot16Animation30384 */
        if(!select_animation(m,16,0x088DF495,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030398:{ /* SetMapChoiceSlot16Animation30398 */
        if(!select_animation(m,16,0x088DF6D2,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080303AC:{ /* SetMapChoiceSlot16Animation303AC */
        if(!select_animation(m,16,0x088DF739,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080303C0:{ /* SetMapChoiceSlot16Animation303C0 */
        if(!select_animation(m,16,0x088DF6B6,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080303D4:{ /* SetMapChoiceSlot16Animation303D4 */
        if(!select_animation(m,16,0x088DF8D9,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080303E8:{ /* SetMapChoiceSlot16Animation303E8 */
        if(!select_animation(m,16,0x088DF940,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080303FC:{ /* SetMapChoiceSlot16Animation303FC */
        if(!select_animation(m,16,0x088DF7A0,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030410:{ /* ConfigureMapChoiceLinkedSlots8And9 */
        if(!select_animation(m,10,0x088B0CA0,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,8,1,176,0,0,0,0x088B0CBC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,8,2,3,10))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,9,1,176,0,0,0,0x088B0CDB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9,2,3,10))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030474:{ /* SetMapChoiceSlot8InlineIndicator30474 */
        GameAddr animation=0x088E72AE;
        if(!select_animation(m,8,animation,true))return HN_SCENE_RESOURCE;
        hn_write8(m,50339756,8);
        hn_write32(m,50339708,(uint32_t)animation);
        break;
    }
    case 0x0803049C:{ /* ConfigureMapChoiceLinkedSlot15BeforeAnimation3049C */
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088E438D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088DF65D,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080304D8:{ /* ConfigureMapChoiceLinkedSlot15AfterAnimation304D8 */
        if(!select_animation(m,16,0x088DFA04,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088DFA10))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030580:{ /* SetMapChoiceSlot13Animation30580 */
        if(!select_animation(m,13,0x088D7837,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030594:{ /* SetMapChoiceSlot13InlineIndicator30594 */
        GameAddr animation;
        if(!apply(m,0x0802F67C))return HN_SCENE_RESOURCE;
        animation=0x088D78B3;
        if(!select_animation(m,13,animation,true))return HN_SCENE_RESOURCE;
        hn_write8(m,50339756,13);
        hn_write32(m,50339708,(uint32_t)animation);
        break;
    }
    case 0x080305C0:{ /* SetMapChoiceSlot13Animation305C0 */
        if(!select_animation(m,13,0x088D78F5,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080305D4:{ /* ConfigureMapChoiceLinkedSlot12AfterAnimation305D4 */
        if(!select_animation(m,13,0x088D74AC,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12,1,176,0,0,0,0x088D776D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030794:{ /* SetMapChoicePairIndicator30794 */
        GameAddr animation=0x088A644F;
        if(!select_animation(m,10,animation,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088AA989,true))return HN_SCENE_RESOURCE;
        indicator(m,10,animation);
        break;
    }
    case 0x080307C0:{ /* SetMapChoicePairIndicator307C0 */
        GameAddr animation=0x088A65CB;
        if(!select_animation(m,10,animation,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088AABEF,true))return HN_SCENE_RESOURCE;
        indicator(m,10,animation);
        break;
    }
    case 0x080307EC:{ /* SetMapChoiceSlot16Animation307EC */
        if(!select_animation(m,16,0x088C2EE2,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030800:{ /* SetMapChoiceSlot16Animation30800 */
        if(!select_animation(m,16,0x088C2F27,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030814:{ /* SetMapChoiceSlot16Animation30814 */
        if(!select_animation(m,16,0x088C31FF,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030828:{ /* SetMapChoiceSlot16Animation30828 */
        if(!select_animation(m,16,0x088C4433,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803083C:{ /* ConfigureMapChoiceLinkedSlot15AfterAnimation3083C */
        if(!select_animation(m,16,0x088C3B4D,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088C3B69))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030878:{ /* SetMapChoiceSlot16Animation30878 */
        if(!select_animation(m,16,0x088C4481,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803088C:{ /* SetMapChoiceSlot16Animation3088C */
        if(!select_animation(m,16,0x088C44B6,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080308A0:{ /* SetMapChoiceSlot16Animation308A0 */
        if(!select_animation(m,16,0x088C452A,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080308B4:{ /* SetMapChoiceSlot16Animation308B4 */
        if(!select_animation(m,16,0x088C4591,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080308C8:{ /* SetMapChoiceSlot16Animation308C8 */
        if(!select_animation(m,16,0x088C4689,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080308DC:{ /* SetMapChoiceSlot16Animation308DC */
        if(!select_animation(m,16,0x088C46BE,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080308F0:{ /* ConfigureMapChoiceLinkedSlot15Priority0 */
        if(!select_animation(m,16,0x088C46F1,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088C4780))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!priority(m,15,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030934:{ /* ClearMapChoiceSlot15AndSet16Animation30934 */
        if(!field(m,15,0))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088C47AC,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030950:{ /* SetMapChoiceSlot16Animation30950 */
        if(!select_animation(m,16,0x088C480C,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030964:{ /* SetMapChoiceSlot16Animation30964 */
        if(!select_animation(m,16,0x088C2F12,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030978:{ /* SetMapChoiceSlot19Animation30978 */
        if(!select_animation(m,19,0x088CC6E3,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803098C:{ /* SetMapChoiceSlot19Indicator3098C */
        GameAddr animation=0x088CC8BA;
        if(!select_animation(m,19,animation,true))return HN_SCENE_RESOURCE;
        indicator(m,19,animation);
        break;
    }
    case 0x080309AC:{ /* SetMapChoiceSlot16Animation309AC */
        if(!select_animation(m,16,0x088BB84F,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080309C0:{ /* SetMapChoiceSlot16Animation309C0 */
        if(!select_animation(m,16,0x088BC0DF,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080309D4:{ /* ConfigureMapChoiceLinkedSlot15Indicator309D4 */
        GameAddr animation=0x088BC0FB;
        if(!select_animation(m,16,animation,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088BC16A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08030A1C:{ /* ConfigureMapChoiceLinkedSlot15Indicator30A1C */
        GameAddr animation=0x088BC19C;
        if(!select_animation(m,16,animation,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088E1DD3))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08030A64:{ /* ConfigureMapChoiceLinkedSlot15AfterAnimation30A64 */
        if(!select_animation(m,16,0x088BC28B,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088E390A))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030AA0:{ /* SetMapChoiceSlot16Animation30AA0 */
        if(!select_animation(m,16,0x088BC2DB,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030AB4:{ /* SetMapChoiceSlot16Animation30AB4 */
        if(!select_animation(m,16,0x088BC331,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030AC8:{ /* SetMapChoiceSlot16Animation30AC8 */
        if(!select_animation(m,16,0x088BC366,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030ADC:{ /* SetMapChoiceSlot16Animation30ADC */
        if(!select_animation(m,16,0x088BC382,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030AF0:{ /* SetMapChoiceSlot16Animation30AF0 */
        if(!select_animation(m,16,0x088BC3E9,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030B04:{ /* SetMapChoiceSlot16Indicator30B04 */
        GameAddr animation=0x088BC41E;
        if(!select_animation(m,16,animation,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08030B24:{ /* SetMapChoicePairAnimations30B24 */
        if(!select_animation(m,10,0x088AC134,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088B408C,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030B44:{ /* ConfigureMapChoiceLinkedRenderGroup30B44 */
        GameAddr animation=0x088A5FBF;
        if(!select_animation(m,10,animation,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088AA177,true))return HN_SCENE_RESOURCE;
        indicator(m,10,animation);
        if(!hn_render_configure(m,9,1,176,0,0,0,0x088E201E))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11,1,176,0,0,0,0x088E204D))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12,1,176,0,0,0,0x088E205E))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,0,0,0x088E208D))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,9,2,3,10))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,11,2,3,10))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12,2,3,13))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030C04:{ /* SetMapChoiceLeaderAnimationAndFixedIndicator30C04 */
        GameAddr animation;
        if(!select_animation(m,hn_read8(m,0x03002C0C),animation = 0x088A4E50,false))return HN_SCENE_RESOURCE;
        indicator(m,10,animation);
        break;
    }
    case 0x08030C28:{ /* ConfigureMapChoiceSlot14Offset30C28 */
        if(!select_animation(m,13,0x088B6FD7,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,-18,-20,0,0x088E3EE8))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030C78:{ /* SetMapChoiceSlot13Animation30C78 */
        if(!select_animation(m,13,0x088B6ADD,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030C8C:{ /* ConfigureMapChoiceSlot14WithSong70 */
        GameAddr animation=0x088B739D;
        if(!select_animation(m,13,animation,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E13AB))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,-18,-20,0,0x088E33CF))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        if(!hn_audio_song(m,70,HN_SONG_START))return HN_SCENE_RESOURCE;
        indicator(m,13,animation);
        break;
    }
    case 0x08030CF0:{ /* SetMapChoiceSlot13Indicator30CF0 */
        GameAddr animation=0x088A8B25;
        if(!select_animation(m,13,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,13,animation);
        break;
    }
    case 0x08030D10:{ /* SetMapChoiceSlot13Animation30D10 */
        if(!select_animation(m,13,0x088B4FE1,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030D24:{ /* SetMapChoiceSlot13Animation30D24 */
        if(!select_animation(m,13,0x088B6A9F,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030D38:{ /* SetMapChoiceSlot13Animation30D38 */
        if(!select_animation(m,13,0x088B4002,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030D4C:{ /* ConfigureMapChoiceLinkedSlots12And14_30D4C */
        if(!select_animation(m,13,0x088B4002,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1338))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,12,1,176,0,0,0,0x088B4039))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,0,0,0x088B4047))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,12,2,3,13))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030DBC:{ /* SetMapChoiceSlot16Animation30DBC */
        if(!select_animation(m,16,0x088CABD0,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030DD0:{ /* ConfigureMapChoiceLinkedSlot17_30DD0 */
        if(!select_animation(m,16,0x088CAC0E,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1364))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,0,0,0,0x088CAC58))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030E1C:{ /* SetMapChoiceSlot16Animation30E1C */
        if(!select_animation(m,16,0x088CAC90,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030E30:{ /* ConfigureMapChoiceSlot17Offset30E30 */
        if(!select_animation(m,16,0x088CAC9C,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E135C))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,-18,-20,0,0x088E3EE8))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030E80:{ /* ConfigureMapChoiceLinkedSlots15And17_30E80 */
        if(!select_animation(m,16,0x088CACA8,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1338))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088CACBD))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,0,0,0,0x088CACD6))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030EF0:{ /* SetMapChoiceSlot16Animation30EF0 */
        if(!select_animation(m,16,0x088CACEF,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030F04:{ /* SetMapChoiceSlot16Indicator30F04 */
        GameAddr animation=0x088CB4ED;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08030F24:{ /* ConfigureMapChoiceLinkedSlot17_30F24 */
        if(!select_animation(m,16,0x088CACFB,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,0,0,0,0x088CAD07))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030F70:{ /* SetMapChoiceSlot16Indicator30F70 */
        GameAddr animation=0x088CAD15;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08030F90:{ /* SetMapChoiceSlot16Animation30F90 */
        if(!select_animation(m,16,0x088CA5C1,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030FA4:{ /* ConfigureMapChoiceSlot32Animation30FA4 */
        if(!hn_render_configure(m,32,1,0,0,0,0,0x088CFBEA))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08030FC8:{ /* SetMapChoiceSlot16Indicator30FC8 */
        GameAddr animation=0x088CAD87;
        if(!select_animation(m,16,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08030FE8:{ /* ConfigureMapChoiceSlot17Offset30FE8 */
        if(!hn_graphics_configure(m,4,0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,176,0,-24,0,0x088E4084))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031108:{ /* ClearMapChoiceSlots15And17Field6031108 */
        if(!field(m,15,0))return HN_SCENE_RESOURCE;
        if(!field(m,17,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803178C:{ /* SetMapChoiceSlot19Animation3178C */
        if(!select_animation(m,19,0x088D4F99,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080317A0:{ /* SetMapChoiceSlot19Animation317A0 */
        if(!select_animation(m,19,0x088D4C93,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080317B4:{ /* SetMapChoiceSlot19Animation317B4 */
        if(!select_animation(m,19,0x088D4D34,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080317C8:{ /* SetMapChoiceSlot19Animation317C8 */
        if(!select_animation(m,19,0x088D4F48,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080317DC:{ /* SetMapChoiceSlot19Animation317DC */
        if(!select_animation(m,19,0x088D4D8D,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080317F0:{ /* ConfigureMapChoiceSlot20Offset317F0 */
        if(!select_animation(m,19,0x088D4D18,false))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1330))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,20,1,176,0,-28,0,0x088E40AB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,20,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803189C:{ /* SetMapChoiceSlot19Animation3189C */
        if(!select_animation(m,19,0x088D48E7,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080318B0:{ /* SetMapChoiceSlot19Animation318B0 */
        if(!select_animation(m,19,0x088D4629,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080318C4:{ /* SetMapChoiceSlot19Indicator318C4 */
        GameAddr animation=0x088D4D50;
        if(!select_animation(m,19,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,19,animation);
        break;
    }
    case 0x080318E4:{ /* SetMapChoiceLeaderIndicator318E4 */
        uint32_t index=hn_read8(m,0x03002C0C);
        GameAddr animation=0x088A680C;
        if(!select_animation(m,(uint8_t)index,animation,false))return HN_SCENE_RESOURCE;
        indicator(m,hn_read8(m,0x03002C0C),animation);
        break;
    }
    case 0x08031908:{ /* SetMapChoiceSlot19Animation31908 */
        if(!select_animation(m,19,0x088D4F7D,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803191C:{ /* SetMapChoiceSlot19Animation3191C */
        if(!select_animation(m,19,0x088D4DC5,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031930:{ /* SetMapChoiceSlot19Animation31930 */
        if(!select_animation(m,19,0x088D48A9,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031944:{ /* SetMapChoiceSlot19Animation31944 */
        if(!select_animation(m,19,0x088D4DA9,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031958:{ /* SetMapChoiceSlot19Animation31958 */
        if(!select_animation(m,19,0x088D4DE1,false))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031E60:{ /* ClearMapChoiceSlots15And17Field6031E60 */
        if(!field(m,15,0))return HN_SCENE_RESOURCE;
        if(!field(m,17,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031E78:{ /* ClearMapChoiceSlots18And20Field60 */
        if(!field(m,18,0))return HN_SCENE_RESOURCE;
        if(!field(m,20,0))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031E90:{ /* ClearMapChoiceFourLinkedSlotFields */
        if(!apply(m,0x08031E60))return HN_SCENE_RESOURCE;
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031EEC:{ /* SetMapChoicePairIndicator31EEC */
        GameAddr animation=0x088A6C66;
        if(!select_animation(m,10,animation,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088AB299,true))return HN_SCENE_RESOURCE;
        indicator(m,10,animation);
        break;
    }
    case 0x08031F18:{ /* ConfigureMapChoiceLinkedPair31F18 */
        GameAddr animation;
        GameAddr auxiliary;
        if(!hn_graphics_configure(m,4,0x088E1330))return HN_SCENE_RESOURCE;
        animation=0x088A651D;
        if(!select_animation(m,10,animation,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,13,0x088AAA54,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,11,1,176,0,0,0,auxiliary = 0x088E2209))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,11,2,3,10))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,14,1,176,0,0,0,auxiliary))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,14,2,3,13))return HN_SCENE_RESOURCE;
        indicator(m,10,animation);
        break;
    }
    case 0x08031F9C:{ /* EmptyMapChoiceCallback31F9C */
        break;
    }
    case 0x08031FA0:{ /* ConfigureMapChoiceSlot15_31FA0 */
        if(!apply(m,0x08031E60))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CBE12,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,0x088CB755))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08031FE4:{ /* SetMapChoiceSlots16And19_31FE4 */
        GameAddr animation;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        animation=0x088CDB3B;
        if(!select_animation(m,16,animation,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,animation,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032008:{ /* ApplyMapChoicePreset32008 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CBE12,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,0x088CB755))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB749,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB755))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803207C:{ /* ApplyMapChoicePreset3207C */
        GameAddr animation;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CB4ED,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB58B,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x080320AC:{ /* ApplyMapChoicePreset320AC */
        if(!apply(m,0x08031E60))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB76C,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080320C4:{ /* ApplyMapChoicePreset320C4 */
        GameAddr animation;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CBA99,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CBAD9,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x080320F4:{ /* ApplyMapChoicePreset320F4 */
        if(!apply(m,0x08031E60))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CBEEA,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803210C:{ /* ApplyMapChoicePreset3210C */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCF02,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CCF17))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032150:{ /* ApplyMapChoicePreset32150 */
        GameAddr animation;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CE71E,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CD516,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08032180:{ /* ApplyMapChoicePreset32180 */
        GameAddr animation;
        GameAddr auxiliary;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!hn_graphics_configure(m,4,0x088E1340))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CBB19,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,auxiliary = 0x088E16F9))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CBB88,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,auxiliary))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08032208:{ /* ApplyMapChoicePreset32208 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB41F,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CA8B0,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803222C:{ /* ApplyMapChoicePreset3222C */
        if(!apply(m,0x08031E60))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CDB3B,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032244:{ /* ApplyMapChoicePreset32244 */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB76C,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803225C:{ /* ApplyMapChoicePreset3225C */
        GameAddr animation;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CBEFF,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB78E,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x0803228C:{ /* ApplyMapChoicePreset3228C */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CBDE0,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,0x088CBDFC))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB2E1,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB309))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032300:{ /* ApplyMapChoicePreset32300 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB749,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,0x088CB755))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB749,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB755))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032370:{ /* ApplyMapChoicePreset32370 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CBF4F,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB896,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032394:{ /* ApplyMapChoicePreset32394 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB8BE,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB8BE,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080323B8:{ /* ApplyMapChoicePreset323B8 */
        GameAddr animation;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!field(m,22,0))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CBF77,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB8D3,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x080323F0:{ /* ApplyMapChoicePreset323F0 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB98F,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,0x088CB9A4))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,114,0,0,0,0x088CB9CE))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CA6B9,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032464:{ /* ApplyMapChoicePreset32464 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB9F8,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CBA42))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,20,1,114,0,0,0,0x088CBA6F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,20,2,3,19))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CA69D,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080324D8:{ /* ApplyMapChoicePreset324D8 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB749,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCE00,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB755))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032528:{ /* ApplyMapChoicePreset32528 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CCE1C,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCE31,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803254C:{ /* ApplyMapChoicePreset3254C */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CBEEA,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCE46,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032570:{ /* ApplyMapChoicePreset32570 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CA5C1,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCF02,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CCF17))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080325C0:{ /* ApplyMapChoicePreset325C0 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CCE62,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCEB2,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080325E4:{ /* ApplyMapChoicePreset325E4 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB749,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,0x088CB755))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCE00,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB755))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032658:{ /* ApplyMapChoicePreset32658 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CCF49,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCF55,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803267C:{ /* ApplyMapChoicePreset3267C */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB9F8,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB9A4))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,20,1,114,0,0,0,0x088CB9CE))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,20,2,3,19))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CA69D,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080326F0:{ /* ApplyMapChoicePreset326F0 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB98F,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,0x088CBA42))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,17,1,114,0,0,0,0x088CBA6F))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,17,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CA6B9,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032764:{ /* ApplyMapChoicePreset32764 */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CAC43,true))return HN_SCENE_RESOURCE;
        if(!preset(m,0x08019A0C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032780:{ /* ApplyMapChoicePreset32780 */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CC776,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032798:{ /* ApplyMapChoicePreset32798 */
        GameAddr animation;
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,animation = 0x088CE700,true))return HN_SCENE_RESOURCE;
        indicator(m,19,animation);
        break;
    }
    case 0x080327BC:{ /* ApplyMapChoicePreset327BC */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB6D1,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB706))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,20,1,114,0,0,0,0x088CB733))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,20,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032824:{ /* ApplyMapChoicePreset32824 */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CC776,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803283C:{ /* ApplyMapChoicePreset3283C */
        if(!select_animation(m,19,0x088CCD92,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CCDE9))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x0803287C:{ /* ApplyMapChoicePreset3287C */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB8BE,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB896,true))return HN_SCENE_RESOURCE;
        if(!position(m,19,256,0,152))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080328B0:{ /* ApplyMapChoicePreset328B0 */
        GameAddr animation;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CBE2E,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,animation = 0x088CB352,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB3DB))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        indicator(m,19,animation);
        break;
    }
    case 0x0803290C:{ /* ApplyMapChoicePreset3290C */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CB41F,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB6D1,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CB706))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,20,1,114,0,0,0,0x088CB733))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,20,2,3,19))return HN_SCENE_RESOURCE;
        if(!position(m,19,256,0,152))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032990:{ /* ApplyMapChoicePreset32990 */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CA8B0,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080329A8:{ /* ApplyMapChoicePreset329A8 */
        GameAddr animation;
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CCB01,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CCBD6,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x080329D8:{ /* ApplyMapChoicePreset329D8 */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CBF4F,true))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CAB86,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,18,1,114,0,0,0,0x088CABA2))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,18,2,3,19))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,20,1,114,0,0,0,0x088CABB9))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,20,2,3,19))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032A4C:{ /* ApplyMapChoicePreset32A4C */
        if(!apply(m,0x08031E90))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,0x088CCD52,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,114,0,0,0,0x088CCD67))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CA655,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032A9C:{ /* ApplyMapChoicePreset32A9C */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CBEEA,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032AB4:{ /* ApplyMapChoicePreset32AB4 */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CB8BE,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032ACC:{ /* ApplyMapChoicePreset32ACC */
        GameAddr animation;
        if(!apply(m,0x08031E60))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CB4F0,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08032AF0:{ /* ApplyMapChoicePreset32AF0 */
        GameAddr animation;
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,animation = 0x088CBD26,true))return HN_SCENE_RESOURCE;
        indicator(m,19,animation);
        break;
    }
    case 0x08032B14:{ /* ApplyMapChoicePreset32B14 */
        GameAddr animation;
        if(!apply(m,0x08031E60))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CBCD6,true))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08032B38:{ /* ApplyMapChoicePreset32B38 */
        GameAddr animation;
        if(!apply(m,0x08031E60))return HN_SCENE_RESOURCE;
        if(!select_animation(m,16,animation = 0x088CC0AD,true))return HN_SCENE_RESOURCE;
        if(!hn_render_configure(m,15,1,176,0,0,0,0x088E2209))return HN_SCENE_RESOURCE;
        if(!hn_render_modes(m,15,2,3,16))return HN_SCENE_RESOURCE;
        indicator(m,16,animation);
        break;
    }
    case 0x08032B88:{ /* ApplyMapChoicePreset32B88 */
        break;
    }
    case 0x08032B8C:{ /* ApplyMapChoicePreset32B8C */
        if(!apply(m,0x08031E78))return HN_SCENE_RESOURCE;
        if(!select_animation(m,19,0x088CDB3B,true))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032D3C:{ /* ApplyMapChoicePresets32D3C */
        if(!preset(m,0x08017694))return HN_SCENE_RESOURCE;
        if(!preset(m,0x08019FE4))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032D4C:{ /* ApplyMapChoicePresets32D4C */
        if(!preset(m,0x080176C4))return HN_SCENE_RESOURCE;
        if(!preset(m,0x08018E2C))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032D5C:{ /* ApplyMapChoicePresets32D5C */
        if(!preset(m,0x080176AC))return HN_SCENE_RESOURCE;
        if(!preset(m,0x0801A028))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x08032D6C:{ /* ApplyMapChoicePresets32D6C */
        if(!preset(m,0x080178D0))return HN_SCENE_RESOURCE;
        if(!preset(m,0x08018E90))return HN_SCENE_RESOURCE;
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}

static HNSceneResult simple_main(HNMemory *m,GameAddr callback,uint32_t *consumed)
{
    switch(callback&~1u){
    case 0x0802F5DC:
    case 0x08031EEC:
    case 0x08031F18:
    case 0x08031FA0:
    case 0x08031FE4:
    case 0x08032008:
    case 0x0803207C:
    case 0x080320AC:
    case 0x080320C4:
    case 0x080320F4:
    case 0x0803210C:
    case 0x08032150:
    case 0x08032180:
    case 0x08032208:
    case 0x0803222C:
    case 0x08032244:
    case 0x0803225C:
    case 0x0803228C:
    case 0x08032300:
    case 0x08032370:
    case 0x08032394:
    case 0x080323B8:
    case 0x080323F0:
    case 0x08032464:
    case 0x080324D8:
    case 0x08032528:
    case 0x0803254C:
    case 0x08032570:
    case 0x080325C0:
    case 0x080325E4:
    case 0x08032658:
    case 0x0803267C:
    case 0x080326F0:
    case 0x08032764:
    case 0x08032780:
    case 0x08032798:
    case 0x080327BC:
    case 0x08032824:
    case 0x0803283C:
    case 0x0803287C:
    case 0x080328B0:
    case 0x0803290C:
    case 0x08032990:
    case 0x080329A8:
    case 0x080329D8:
    case 0x08032A4C:
    case 0x08032A9C:
    case 0x08032AB4:
    case 0x08032ACC:
    case 0x08032AF0:
    case 0x08032B14:
    case 0x08032B38:
    case 0x08032B88:
    case 0x08032B8C:
        *consumed=0;return simple_apply(m,callback);
    default:return HN_SCENE_UNKNOWN;
    }
}
static GameAddr class_slot(HNMemory *m,GameAddr selector)
{unsigned index=hn_read8(m,selector);return valid(m,index)?hn_render_slot(m,index):0;}
static bool facing(HNMemory *m)
{
    unsigned direction=hn_render_direction_between(m,10,13,false);if(!direction)return false;
    return select_animation(m,10,hn_read32(m,HN_MAP_CHOICE_FACING+(direction-1)*4),true);
}
static bool motion_animations(HNMemory *m,unsigned mode)
{
    if(!valid(m,29))return false;
    GameAddr auxiliary=mode==1?0x088E7888:0x088E7880;
    if(hn_read32(m,hn_render_slot(m,29)+4)!=auxiliary&&!select_animation(m,29,auxiliary,false))return false;
    unsigned index=hn_read8(m,HN_RENDER_CLASS_LEADER);if(!valid(m,index))return false;
    GameAddr resource=mode==0?0x088ACB03:mode==1?0x088ACB38:0x088ACD94;
    if(hn_read32(m,hn_render_slot(m,index)+4)!=resource){
        if(!select_animation(m,(uint8_t)index,resource,false))return false;
        if(flag(m,16)){
            resource=mode==0?0x088B4FED:mode==1?0x088B5022:0x088B511A;
            if(!select_animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),resource,false)||!field(m,23,(uint8_t)(mode==0))||
               !position(m,13,-20,mode==2?-1:0,mode==2?1:0))return false;
        }
    }
    return apply(m,mode==2?0x0803171C:0x080316F4);
}
HNSceneResult hn_map_choice_apply(HNMemory *m,GameAddr callback,uint32_t *value)
{
    *value=0;bool ok=true;
    switch(callback&~1u){
    case 0x0802F5B4:case 0x0802F67C:ok=facing(m);break;
    case 0x0802F628:{
        unsigned direction=hn_render_direction_between(m,13,10,false);if(!direction)return HN_SCENE_RESOURCE;
        GameAddr table=hn_read32(m,HN_MAP_CHOICE_SECONDARY+hn_read8(m,HN_SCENE_CONTROLS+0x6E)*4);
        ok=field_animation(m,13,(uint8_t)direction,hn_read32(m,table+(direction-1)*4));break;
    }
    case 0x0802FA80:
        ok=hn_render_configure(m,16,1,114,192,0,20,0x088CA8D1)&&select_animation(m,hn_read8(m,HN_RENDER_CLASS_LEADER),0x088B0AAC,false);
        if(ok){indicator(m,16,0x088CA8D1);if(flag(m,16))ok=select_animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),0x088B7241,false);}break;
    case 0x0802FC70:{
        ok=hn_graphics_configure(m,4,0x088E1338)&&select_animation(m,hn_read8(m,HN_RENDER_CLASS_LEADER),0x088A61FC,false)&&
           hn_render_configure(m,(uint8_t)(hn_read8(m,HN_RENDER_CLASS_LEADER)+1),1,176,0,-1,1,0x088E214E)&&
           hn_render_modes(m,(uint8_t)(hn_read8(m,HN_RENDER_CLASS_LEADER)+1),2,3,hn_read8(m,HN_RENDER_CLASS_LEADER));
        if(ok&&flag(m,16))ok=select_animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),0x088AA63F,false)&&
            hn_render_configure(m,(uint8_t)(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)+1),1,176,0,-1,1,0x088E214E)&&
            hn_render_modes(m,(uint8_t)(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)+1),2,3,hn_read8(m,HN_RENDER_CLASS_FOLLOWER));
        if(ok)indicator(m,hn_read8(m,HN_RENDER_CLASS_LEADER),0x088A61FC);break;
    }
    case 0x08030250:case 0x080302A0:case 0x080302F0:{
        if(!valid(m,10))return HN_SCENE_RESOURCE;bool upper=(int32_t)hn_read32(m,hn_render_slot(m,10)+0x20)>0x57FFFF;
        GameAddr resource=(callback&~1u)==0x08030250?(upper?0x088DF42B:0x088DF460):
                         (callback&~1u)==0x080302A0?(upper?0x088DF495:0x088DF4CA):(upper?0x088DF2DC:0x088DF314);
        ok=select_animation(m,16,resource,true);break;
    }
    case 0x08030514:{
        unsigned direction=hn_render_direction_between(m,10,13,false);if(!direction)return HN_SCENE_RESOURCE;
        if(direction==1)ok=field_animation(m,13,2,0x088D70A0);else if(direction==2)ok=field_animation(m,13,1,0x088D70D5);
        else if(direction==3)ok=field_animation(m,13,4,0x088D713F);else ok=field_animation(m,13,3,0x088D710A);break;
    }
    case 0x0803102C:case 0x08031840:{
        unsigned index=hn_read8(m,HN_RENDER_CLASS_LEADER);if(!valid(m,index))return HN_SCENE_RESOURCE;
        uint32_t offset=(uint32_t)(hn_read8(m,hn_render_slot(m,index)+0x60)-1u)*4;
        ok=select_animation(m,(uint8_t)index,hn_read32(m,HN_MAP_CHOICE_FIELD60+offset),false);if(!ok)break;
        index=hn_read8(m,HN_RENDER_CLASS_LEADER);if(!valid(m,index))return HN_SCENE_RESOURCE;
        offset=(uint32_t)(hn_read8(m,hn_render_slot(m,index)+0x60)-1u)*4;
        indicator(m,(uint8_t)index,hn_read32(m,HN_MAP_CHOICE_FIELD60+offset));break;
    }
    case 0x08031088:case 0x080310C8:{
        bool first=(callback&~1u)==0x08031088;GameAddr resource=first?0x088A62A9:0x088A6761;
        ok=select_animation(m,hn_read8(m,HN_RENDER_CLASS_LEADER),resource,false);
        if(ok&&flag(m,16))ok=select_animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER),first?0x088AA75A:0x088AACAF,false);
        if(ok)indicator(m,hn_read8(m,HN_RENDER_CLASS_LEADER),resource);break;
    }
    case 0x08031524:{
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;int sign=1;
        if((int32_t)hn_read32(m,slot+0x24)<0){hn_write32(m,slot+0x24,0u-hn_read32(m,slot+0x24));sign=-1;}
        hn_write32(m,slot+0x24,hn_read32(m,slot+0x24)-0x800);
        if((int32_t)hn_read32(m,slot+0x24)<0)hn_write32(m,slot+0x24,0);
        hn_write32(m,slot+0x24,hn_read32(m,slot+0x24)*(uint32_t)sign);break;
    }
    case 0x08031568:ok=motion_animations(m,0);break;
    case 0x080315EC:ok=motion_animations(m,1);break;
    case 0x08031670:ok=motion_animations(m,2);break;
    case 0x080316F4:
        if(!hn_read8(m,HN_MAP_CHOICE_GRAPHICS_TOGGLE)){ok=hn_graphics_configure(m,12,0x088E77FB);if(ok)hn_write8(m,HN_MAP_CHOICE_GRAPHICS_TOGGLE,1);}break;
    case 0x0803171C:
        if(hn_read8(m,HN_MAP_CHOICE_GRAPHICS_TOGGLE)){ok=hn_graphics_configure(m,12,0x088E7786);if(ok)hn_write8(m,HN_MAP_CHOICE_GRAPHICS_TOGGLE,0);}break;
    case 0x08031744:{
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;
        *value=hn_read32(m,slot+0x18)-0x688000u<=0x600000u&&hn_read32(m,slot+0x20)-0x778000u<=0x90000u;break;
    }
    case 0x08031AE4:{
        GameAddr leader=class_slot(m,HN_RENDER_CLASS_LEADER),follower=class_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!leader||!follower)return HN_SCENE_RESOURCE;
        int32_t distance=(int32_t)((hn_read32(m,leader+0x18)&0xFFFF0000u)-(hn_read32(m,follower+0x18)&0xFFFF0000u));unsigned index=0;
        while(index<16&&distance){bool positive=distance>0;hn_write8(m,HN_ROUTE_POINTS+index,(uint8_t)(positive?4:3));
            distance=(int32_t)((uint32_t)distance+(positive?0xFFF80000u:0x80000u));index++;}
        hn_write8(m,HN_ROUTE_END,(uint8_t)index);while(index<16)hn_write8(m,HN_ROUTE_POINTS+index++,0);break;
    }
    case 0x08031B90:case 0x08031BDC:{
        GameAddr leader=class_slot(m,HN_RENDER_CLASS_LEADER),follower=class_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!leader||!follower)return HN_SCENE_RESOURCE;
        uint32_t a=hn_read32(m,leader+0x18),b=hn_read32(m,follower+0x18);bool result;
        if((callback&~1u)==0x08031B90)result=a==b;
        else{int32_t distance=(int32_t)(a-b);if(distance<0)distance=(int32_t)(0u-(uint32_t)distance);result=(distance>>16)>31;}
        hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)result);break;
    }
    case 0x08031E3C:{GameAddr slot=class_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!slot)return HN_SCENE_RESOURCE;hn_write32(m,slot+0x20,0x988000);break;}
    case 0x08031EA0:
        ok=select_animation(m,10,0x088A4CC7,true)&&select_animation(m,13,0x088A8945,true);
        if(ok){hn_write8(m,HN_SCENE_CONTROLS+0x60,13);hn_write8(m,HN_SCENE_CONTROLS+0x61,10);hn_write8(m,HN_SCENE_CONTROLS+0x62,32);
            ok=hn_motion_relative_velocity(m);if(ok)indicator(m,13,0x088A8945);}break;
    /* These Main callbacks ignore their argument register; F4 may ignore their
       defined return too. Argument-reading callbacks have no task binding. */
    case 0x0802F03C:case 0x0802F18C:case 0x0802F590:case 0x08030688:case 0x08030714:
    case 0x08031250:case 0x08031358:case 0x080313D0:case 0x080314EC:case 0x0803196C:
    case 0x080319B8:case 0x08031A20:case 0x08031A8C:case 0x08031B70:case 0x08031C34:
    case 0x08031CA4:case 0x08031D48:case 0x08032BA4:case 0x08032BD4:case 0x08032D0C:
        return hn_map_choice_callback(m,callback,0,value);
    default:return simple_apply(m,callback);
    }
    return ok&&!m->fault?HN_SCENE_OK:HN_SCENE_RESOURCE;
}
HNSceneResult hn_map_choice_slot(HNMemory *m,GameAddr callback,GameAddr slot,uint32_t *value)
{
    *value=0;
    switch(callback&~1u){
    case 0x08030078:if(!hn_motion_map_height(m,slot))return HN_SCENE_RESOURCE;break;
    case 0x080300FC:{uint32_t y=hn_read32(m,slot+0x1C);hn_write32(m,slot+0x1C,(int32_t)y<=-0x88000?(uint32_t)-0x88000:y-0x20000);break;}
    case 0x08030120:{uint32_t y=hn_read32(m,slot+0x1C);hn_write32(m,slot+0x1C,(int32_t)y>0x7FFF?0x8000:y+0x20000);break;}
    case 0x08032C80:{
        int32_t vx=(int32_t)hn_read32(m,slot+0x24),vz=(int32_t)hn_read32(m,slot+0x2C);
        uint32_t x=hn_read32(m,slot+0x18)+(uint32_t)vx;int32_t z=(int32_t)(hn_read32(m,slot+0x20)+(uint32_t)vz);
        if(x-0x480000u<=0x700000u&&z>0xCFFFFF)*value=(x-0x480001u<=0x37FFFEu&&vx>0)||(x-0x800001u<=0x37FFFEu&&vx<0)||(z>0xD00000&&vz>0);
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_map_choice_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield)
{
    *consumed=0;bool ok=true;
    switch(callback&~1u){
    case 0x0802F50C:{
        unsigned mode=hn_read8(m,arguments),destination=hn_read8(m,arguments+1),axis=mode<=2?0x18:0x20;
        GameAddr leader=class_slot(m,HN_RENDER_CLASS_LEADER);if(!leader)return HN_SCENE_RESOURCE;
        int32_t distance=(int32_t)(hn_read32(m,slot+axis)-hn_read32(m,leader+axis))>>16;
        *consumed=distance>0?(mode<=2?17:7):distance<0?(mode<=2?22:12):2;
        if(distance<0)distance=-distance;if(hn_read8(m,HN_SCENE_CONTROLS+0x60))distance>>=1;
        hn_write16(m,0x0201DC40+destination*2,(uint16_t)distance);break;
    }
    case 0x0802FFF4:
        hn_write32(m,slot+0x24,(uint32_t)((int32_t)(0x00E08000u-hn_read32(m,slot+0x18))/48));
        hn_write32(m,slot+0x2C,(uint32_t)((int32_t)(0x00B88000u-hn_read32(m,slot+0x20))/48));*yield=0;break;
    case 0x0803002C:{
        GameAddr leader=class_slot(m,HN_RENDER_CLASS_LEADER);if(!leader||!hn_motion_map_height(m,leader))return HN_SCENE_RESOURCE;
        if(flag(m,16)){GameAddr follower=class_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!follower||!hn_motion_map_height(m,follower))return HN_SCENE_RESOURCE;}
        *yield=1;break;
    }
    case 0x08031120:{
        unsigned phase=hn_read8(m,HN_MAP_CHOICE_PHASE);uint32_t inside=0;
        if(phase==0){
            if(!valid(m,13))return HN_SCENE_RESOURCE;GameAddr resource=hn_read32(m,hn_render_slot(m,13)+4);
            if(resource!=0x088B503E&&resource!=0x088B7318)ok=select_animation(m,13,0x088B503E,false);
            if(ok&&hn_map_choice_apply(m,0x08031744,&inside)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
            if(ok&&inside){ok=select_animation(m,13,0x088B6FD7,false)&&hn_graphics_configure(m,4,0x088E135C)&&
                hn_render_configure(m,33,1,176,-18,-20,0,0x088E3EE8)&&hn_render_modes(m,33,2,3,13);if(ok)hn_write8(m,HN_MAP_CHOICE_PHASE,1);}
        }else if(phase==1){
            if(hn_map_choice_apply(m,0x08031744,&inside)!=HN_SCENE_OK)return HN_SCENE_RESOURCE;
            if(!inside){ok=select_animation(m,13,0x088B7318,false);if(ok)hn_write8(m,HN_MAP_CHOICE_PHASE,0);}
        }
        *yield=1;break;
    }
    case 0x08032CE8:{GameAddr target=class_slot(m,0x03002C00);if(!target)return HN_SCENE_RESOURCE;
        uint32_t z=hn_read32(m,target+0x20);hn_write32(m,target+0x20,hn_read32(m,target+0x1C));hn_write32(m,target+0x1C,z);break;}
    default:return HN_SCENE_UNKNOWN;
    }
    return ok&&!m->fault?HN_SCENE_OK:HN_SCENE_RESOURCE;
}
static bool constrain(HNMemory *m,unsigned mode)
{
    GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return false;
    int32_t x=(int32_t)(hn_read32(m,slot+0x18)+hn_read32(m,slot+0x24));
    int32_t z=(int32_t)(hn_read32(m,slot+0x20)+hn_read32(m,slot+0x2C));
    if(z<=0x787FFF){hn_write32(m,slot+0x20,0x788000);hn_write32(m,slot+0x2C,0);}
    else if(z>0x988000){hn_write32(m,slot+0x20,0x988000);hn_write32(m,slot+0x2C,0);}
    if(mode==0&&x<=(z<=0x837FFF?0x467FFF:0x3C7FFF)){hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x2C,0);}
    if(mode==2&&x>0x957FFF){hn_write32(m,slot+0x18,0x958000);hn_write32(m,slot+0x24,0);return apply(m,0x08031670);}
    return (int32_t)hn_read32(m,slot+0x24)<=0||apply(m,0x08031568);
}
static uint32_t approach(uint32_t current,uint32_t target)
{
    int32_t distance=(int32_t)(target-current),sign=1;
    if(distance<0){distance=(int32_t)(0u-(uint32_t)distance);sign=-1;}
    if(distance>0x8000)distance=0x8000;return current+(uint32_t)distance*(uint32_t)sign;
}
HNSceneResult hn_map_choice_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;bool ok=true;
    switch(callback&~1u){
    case 0x0802F03C:{
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;
        int32_t x=(int16_t)hn_read16(m,slot+0x1A),z=(int16_t)hn_read16(m,slot+0x22);unsigned direction=0,classification=0;
        for(;direction<4;direction++){GameAddr sample=hn_collision_ahead(m,hn_read32(m,HN_COLLISION_MAP),x,z,direction);
            classification=(hn_read16(m,sample)&0x3C0)>>6;if(classification)break;}
        hn_write16(m,0x03001FBC,(uint16_t)classification);hn_write16(m,0x0201DC84,(uint16_t)classification);hn_write16(m,0x0201DC86,(uint16_t)(direction+1));break;
    }
    case 0x0802F0B0:{
        unsigned width=hn_read8(m,arguments),left=hn_read16(m,arguments+1),right=hn_read16(m,arguments+3);
        unsigned a=width?hn_read16(m,0x0201DC40+left*2):hn_read8(m,HN_SCENE_CONTROLS+left);
        unsigned b=width?hn_read16(m,0x0201DC40+right*2):hn_read8(m,HN_SCENE_CONTROLS+right);*consumed=a==b?5:10;break;
    }
    case 0x0802F110:{
        unsigned previous=hn_read8(m,HN_SCENE_CONTROLS+0x1A0),choice=hn_read8(m,arguments),shift=choice>1?(choice-1)*4:0;
        uint32_t offset=(previous-1u)*2,value=hn_read16(m,HN_MAP_CHOICE_NEXT+offset);
        value=shift<32?(value>>shift)&15:0;hn_write8(m,HN_SCENE_CONTROLS+0x1A0,(uint8_t)value);
        value=hn_read16(m,HN_MAP_CHOICE_FLAGS+offset);value=shift<32?(value>>shift)&15:0;
        if(value)set_flag(m,0x2B1,true);set_flag(m,0x2B0,false);*consumed=1;break;
    }
    case 0x0802F18C:{unsigned id=0x1CB+hn_read8(m,HN_SCENE_CONTROLS+0x1A0);if(flag(m,id))*consumed=5;else set_flag(m,id,true);break;}
    case 0x0802F1C8:{
        int32_t item=(int32_t)hn_read8(m,arguments)-1;unsigned z_word;
        if(item==7){item=2;z_word=hn_read16(m,HN_MAP_CHOICE_Z+4);}
        else{if(item>1){if(!valid(m,10))return HN_SCENE_RESOURCE;z_word=hn_read16(m,hn_render_slot(m,10)+0x22);}
            else z_word=hn_read16(m,HN_MAP_CHOICE_Z+(uint32_t)item*2);hn_write16(m,0x0201DC94,40);}
        unsigned x_word=hn_read16(m,HN_MAP_CHOICE_X+(uint32_t)item*2),variant=0;GameAddr resource;
        if(hn_read16(m,0x03001FBC)){resource=hn_read32(m,HN_MAP_CHOICE_PRIMARY+((uint32_t)item+4)*4);
            hn_write16(m,0x0201DC94,hn_read16(m,0x0201DC94)>>1);variant=1;}
        else resource=hn_read32(m,HN_MAP_CHOICE_PRIMARY+(uint32_t)item*4);
        if(hn_read8(m,HN_SCENE_CONTROLS+0x28)>22){x_word=104;z_word=item==0?288:((z_word<<16)+0xFFF80000u)>>16;}
        int32_t x=(int16_t)x_word,z=(int16_t)z_word;
        ok=hn_render_configure(m,10,hn_read8(m,arguments),51,x,0,z,resource);if(!ok)break;
        if(item){GameAddr table=hn_read32(m,HN_MAP_CHOICE_SECONDARY+hn_read8(m,HN_SCENE_CONTROLS+0x6E)*4);
            resource=hn_read32(m,table+(uint32_t)item*4);ok=hn_render_configure(m,13,hn_read8(m,arguments),113,x,0,z,resource);if(ok)hn_write16(m,0x0201DC96,0);}
        else{resource=hn_read32(m,HN_MAP_CHOICE_VARIANTS+variant*16+hn_read8(m,HN_SCENE_CONTROLS+0x6E)*48);
            ok=hn_render_configure(m,13,hn_read8(m,arguments),113,x,0,(int16_t)(z+24),resource);if(ok)hn_write16(m,0x0201DC96,hn_read16(m,0x0201DC94));}
        *consumed=1;break;
    }
    case 0x0802F344:{
        unsigned mode=hn_read8(m,arguments),destination=hn_read8(m,arguments+1),selector=hn_read8(m,HN_SCENE_CONTROLS+hn_read8(m,arguments+2));
        if(!valid(m,13))return HN_SCENE_RESOURCE;GameAddr choice=hn_render_slot(m,13),base=hn_render_slot(m,10);int32_t distance,direction;
        if(mode<=2){distance=(int32_t)(hn_read32(m,choice+0x18)-hn_read32(m,base+0x18))>>16;
            if(distance>0)direction=2;else if(distance<0){direction=3;distance=-distance;}
            else{uint32_t z=hn_read32(m,choice+0x20);direction=(int32_t)mode-1;
                if(z+0xFFEF7FFFu<=0x77FFFEu){distance=(int32_t)(z-hn_read32(m,base+0x20))>>16;if(distance<0)distance=-distance;}else distance=0;}}
        else{distance=(int32_t)(hn_read32(m,choice+0x20)-hn_read32(m,base+0x20))>>16;
            if(distance>0)direction=0;else if(distance<0){direction=1;distance=-distance;}
            else{direction=(int32_t)mode-1;distance=(int32_t)(hn_read32(m,base+0x18)-hn_read32(m,choice+0x18))>>16;if(distance<0)distance=-distance;}}
        unsigned kind=0;if(!distance)kind=2;else if(hn_read8(m,HN_SCENE_CONTROLS+0x60)){distance>>=1;kind=1;}
        GameAddr resource=hn_read32(m,HN_MAP_CHOICE_MOTION+selector*48+kind*16+(uint32_t)direction*4);
        hn_write16(m,0x0201DC40+destination*2,(uint16_t)distance);ok=select_animation(m,13,resource,true);
        if(ok&&mode>=1&&mode<=4){if(!valid(m,10))return HN_SCENE_RESOURCE;GameAddr target=hn_render_slot(m,10);
            uint32_t speed=hn_read8(m,HN_SCENE_CONTROLS+0x60)?0x20000u:0x10000u;
            hn_write32(m,target+0x24,mode==3?0u-speed:mode==4?speed:0);hn_write32(m,target+0x28,0);
            hn_write32(m,target+0x2C,mode==1?0u-speed:mode==2?speed:0);}
        *consumed=3;break;
    }
    case 0x0802F590:
        hn_write8(m,HN_SCENE_SHARED_STATE,1);hn_write8(m,HN_SCENE_SHARED_STATE+1,14);hn_write32(m,HN_SCENE_SHARED_STATE+4,HN_PARTICLE_ALTERNATE);
        ok=hn_particles_initialize(m);break;
    case 0x08030610:case 0x08030650:{
        unsigned index=hn_read8(m,arguments),variable=hn_read8(m,arguments+1);if(!valid(m,index))return HN_SCENE_RESOURCE;GameAddr slot=hn_render_slot(m,index);
        for(unsigned axis=0;axis<3;axis++){GameAddr cell=0x03002BB0+(variable+axis)*4,coordinate=slot+0x18+axis*4;
            if((callback&~1u)==0x08030610)hn_write32(m,cell,hn_read32(m,coordinate));else hn_write32(m,coordinate,hn_read32(m,cell));}
        *consumed=2;break;
    }
    case 0x08030688:{GameAddr slot=class_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!slot)return HN_SCENE_RESOURCE;
        *consumed=(int32_t)hn_read32(m,slot+0x18)>0x688000&&(int32_t)hn_read32(m,slot+0x20)<=0x707FFF?5:0;break;}
    case 0x080306C4:{
        unsigned target=hn_read8(m,arguments),direction=hn_render_direction_between(m,10,(uint8_t)target,true);if(!direction)return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)direction);hn_write8(m,HN_SCENE_CONTROLS+0x62,1);uint32_t ignored;
        ok=hn_scene_callback(m,0x0801C9B4,0x02001524,&ignored)==HN_SCENE_OK;if(!ok)break;
        direction=hn_render_direction_between(m,13,hn_read8(m,arguments),true);if(!direction)return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS+0x64,(uint8_t)direction);hn_write8(m,HN_SCENE_CONTROLS+0x66,1);
        ok=hn_scene_callback(m,0x0801C9B4,0x02001527,&ignored)==HN_SCENE_OK;*consumed=1;break;
    }
    case 0x08030714:ok=field_animation(m,9,1,0x088E741B);if(ok)indicator(m,9,0x088E741B);break;
    case 0x08030738:{
        unsigned index=hn_read8(m,arguments);uint32_t resource=hn_read32(m,arguments+1),target=hn_read32(m,arguments+5);
        if(!valid(m,index))return HN_SCENE_RESOURCE;GameAddr slot=hn_render_slot(m,index);
        if(hn_read8(m,slot+0x60)&&hn_read32(m,slot+4)==resource){hn_write32(m,0x03002C60,target);*consumed=UINT32_MAX-3;}else *consumed=9;break;
    }
    case 0x080311E4:{
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;
        hn_write8(m,slot+0x60,hn_read8(m,HN_SCENE_CONTROLS+0x11));bool alternate=hn_read8(m,arguments)!=0;
        hn_write32(m,slot+0x18,((uint32_t)hn_read16(m,alternate?0x0201DC60:0x0201DC84)<<16)|0x8000);
        hn_write32(m,slot+0x20,((uint32_t)hn_read16(m,alternate?0x0201DC84:0x0201DC62)<<16)|0x8000);*consumed=1;break;
    }
    case 0x08031250:{
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;unsigned keys=hn_input_query(m,0xF0,0);
        hn_write32(m,slot+0x2C,!(keys&0xC0)?0:(keys&0x80)?0x8000:(uint32_t)-0x8000);
        if(!(keys&0x30))ok=apply(m,0x08031524);
        else if(keys&0x20){uint32_t vx=hn_read32(m,slot+0x24)-0x800;hn_write32(m,slot+0x24,(int32_t)vx< -0x10000?(uint32_t)-0x10000:vx);}
        else if(keys&0x10){uint32_t vx=hn_read32(m,slot+0x24)+0x800;hn_write32(m,slot+0x24,(int32_t)vx>0x18000?0x18000:vx);}
        if(!ok)break;
        if((int32_t)hn_read32(m,slot+0x24)>0){
            if(!hn_read8(m,HN_SCENE_CONTROLS+0x2A)){uint32_t pitch=(uint32_t)(int16_t)(46*((int32_t)hn_read32(m,slot+0x24)>>11));
                hn_audio_track_parameter(m,0x03004E90,3,pitch,1);hn_audio_track_parameter(m,0x03004E90,3,255,0);ok=hn_audio_song(m,27,HN_SONG_CHANGE);}
            else hn_audio_track_parameter(m,0x03004E90,3,0,1);
        }else{ok=hn_audio_song(m,27,HN_SONG_STOP);if(ok)ok=apply(m,(int32_t)hn_read32(m,slot+0x24)<0?0x080315EC:0x08031670);}break;
    }
    case 0x08031358:ok=constrain(m,0);break;
    case 0x0803196C:ok=constrain(m,1);break;
    case 0x08031A20:ok=constrain(m,2);break;
    case 0x080313D0:{
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;hn_write16(m,0x03001FBC,0);
        int32_t z=(int32_t)hn_read32(m,slot+0x20),x=(int32_t)hn_read32(m,slot+0x18);
        if(z<=0x788000){if((uint32_t)x-0xD08000u<=0x280000u)hn_write16(m,0x03001FBC,1);}
        else if(z>0x987FFF&&(uint32_t)x-0x3C8000u<=0x140000u)hn_write16(m,0x03001FBC,2);
        if(x>0x1087FFF)hn_write16(m,0x03001FBC,3);break;
    }
    case 0x080319B8:case 0x08031A8C:{
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;hn_write16(m,0x03001FBC,0);
        bool lower=(callback&~1u)==0x08031A8C;int32_t z=(int32_t)hn_read32(m,slot+0x20);
        if((lower?z<=0x788000:z>0x987FFF)&&hn_read32(m,slot+0x18)-(lower?0x588000u:0x408000u)<=(lower?0x380000u:0x300000u))hn_write16(m,0x03001FBC,1);
        if((int32_t)hn_read32(m,slot+0x18)<=0x208000)hn_write16(m,0x03001FBC,2);
        else if(!lower&&(int32_t)hn_read32(m,slot+0x18)>0x1087FFF)hn_write16(m,0x03001FBC,3);break;
    }
    case 0x08031450:{
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;
        uint32_t target_x=((uint32_t)hn_read16(m,arguments)<<16)|0x8000,target_z=((uint32_t)hn_read16(m,arguments+2)<<16)|0x8000;
        uint32_t z=hn_read32(m,slot+0x20),x;
        if(z==target_z){x=hn_read32(m,slot+0x18);if(x==target_x){hn_write16(m,0x03001FBC,1);*consumed=4;break;}}
        else{hn_write32(m,slot+0x20,approach(z,target_z));x=hn_read32(m,slot+0x18);}
        if(x!=target_x)hn_write32(m,slot+0x18,approach(x,target_x));hn_write16(m,0x03001FBC,0);*consumed=4;break;
    }
    case 0x080314EC:{GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_SHARED_STATE,(uint8_t)((hn_read32(m,slot+0x24)|hn_read32(m,slot+0x2C))!=0));break;}
    case 0x08031B70:{GameAddr slot=class_slot(m,HN_RENDER_CLASS_FOLLOWER);if(!slot)return HN_SCENE_RESOURCE;hn_write32(m,slot+0x24,0);break;}
    case 0x08031C34:case 0x08031CA4:{
        unsigned mode=hn_read8(m,HN_SCENE_CONTROLS+0x61),lane=hn_read8(m,HN_SCENE_CONTROLS+0x1A9);
        GameAddr slot=class_slot(m,HN_RENDER_CLASS_LEADER);if(!slot)return HN_SCENE_RESOURCE;int32_t x=(int16_t)hn_read16(m,slot+0x1A);bool reached;
        if((callback&~1u)==0x08031C34)reached=mode?x>191:x>=80+(int32_t)(lane<<3);
        else reached=mode?x<=(hn_read8(m,HN_SCENE_CONTROLS+0x7B)==1?88:120):x<=200-(int32_t)(lane<<3);
        if(reached)hn_write8(m,HN_SCENE_CONTROLS+0x60,1);break;
    }
    case 0x08031D48:{
        unsigned lane=hn_read8(m,HN_SCENE_CONTROLS+0x1A9);
        if(lane){unsigned index=hn_read8(m,HN_RENDER_CLASS_LEADER);if(!valid(m,index))return HN_SCENE_RESOURCE;GameAddr base=hn_read32(m,HN_RENDER_SLOTS);
            hn_write32(m,base+index*104+0x1C,0);index=hn_read8(m,HN_RENDER_CLASS_LEADER);hn_write32(m,base+index*104+0x20,0x808000u-((lane-1)*3<<19));}
        if(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)){lane=hn_read8(m,HN_SCENE_CONTROLS+0x1AA);unsigned index=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
            if(!valid(m,index))return HN_SCENE_RESOURCE;GameAddr base=hn_read32(m,HN_RENDER_SLOTS);hn_write32(m,base+index*104+0x1C,0);
            index=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);hn_write32(m,base+index*104+0x20,lane?0x808000u-((lane-1)*3<<19):0x988000);}
        break;
    }
    case 0x08031E00:{
        unsigned index=hn_read8(m,arguments),raised=hn_read8(m,arguments+1);if(!valid(m,index))return HN_SCENE_RESOURCE;GameAddr slot=hn_render_slot(m,index);
        hn_write32(m,slot+0x1C,raised?0x108000:0);hn_write32(m,slot+0x20,hn_read32(m,slot+0x20)+(raised?0xFFEF8000u:0x108000u));*consumed=2;break;
    }
    case 0x08032BA4:{
        unsigned index=hn_read8(m,HN_SCENE_CONTROLS+0x12),direction=hn_render_direction_between(m,(uint8_t)index,10,false);
        if(!direction)return HN_SCENE_RESOURCE;ok=select_animation(m,(uint8_t)index,hn_read32(m,HN_MAP_CHOICE_SLOTS+(direction-1)*4),true);break;
    }
    case 0x08032BD4:{
        uint16_t second_z=hn_read16(m,0x0201DC8A),second_x=hn_read16(m,0x0201DC88);
        if((uint16_t)((int16_t)second_x-9)<=13&&(int16_t)second_z>25)second_z=25;
        int32_t x=(int16_t)hn_read16(m,0x0201DC84),z=(int16_t)hn_read16(m,0x0201DC86);unsigned selector=hn_read16(m,0x0201DC8C);
        ok=hn_render_configure(m,16,(uint8_t)selector,114,(int16_t)((uint32_t)x<<3),0,(int16_t)((uint32_t)z<<3),hn_read32(m,HN_MAP_CHOICE_SLOTS+(selector-1u)*4));
        if(ok){selector=hn_read16(m,0x0201DC8E);ok=hn_render_configure(m,19,(uint8_t)selector,114,(int16_t)(second_x<<3),0,(int16_t)(second_z<<3),hn_read32(m,HN_MAP_CHOICE_SLOTS+(selector-1u)*4));}break;
    }
    case 0x08032D0C:
        hn_write16(m,0x0201DC84,hn_read16(m,0x03002BB2));hn_write16(m,0x0201DC86,hn_read16(m,0x03002BBA));
        hn_write16(m,0x0201DC88,hn_read16(m,0x03002BBE));hn_write16(m,0x0201DC8A,hn_read16(m,0x03002BC6));break;
    /* No-argument u32 helpers retain their actual return, including the region
       predicate; void presets never enter Main through this path. */
    case 0x0802F5B4:case 0x0802F628:case 0x08031744:case 0x08031AE4:case 0x08031B90:case 0x08031BDC:case 0x08031E3C:case 0x08031EA0:
        return hn_map_choice_apply(m,callback,consumed);
    default:return simple_main(m,callback,consumed);
    }
    return ok&&!m->fault?HN_SCENE_OK:HN_SCENE_RESOURCE;
}
