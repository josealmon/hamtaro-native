#include "game/bank5.h"
#include "game/state.h"
#include "game/render.h"
#include "game/actor_presets.h"
#include "game/graphics.h"
#include "game/audio.h"

static bool raw_animation(HNMemory *m,uint32_t index,GameAddr image)
{hn_render_set_animation(m,index,image);return !m->fault;}
static bool animation(HNMemory *m,uint8_t index,GameAddr image)
{if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);return raw_animation(m,index,image);}
static bool field(HNMemory *m,uint8_t index,uint8_t value)
{hn_write8(m,hn_render_slot(m,index)+0x60,value);return !m->fault;}
static void indicator(HNMemory *m,uint8_t index,GameAddr image)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,image);}
static bool auxiliary(HNMemory *m,uint8_t index,uint8_t parent,GameAddr image)
{return hn_render_configure_raw(m,index,1,176,0,0,0,image)&&hn_render_modes_raw(m,index,2,3,parent);}
static void cell_position(HNMemory *m,uint32_t index,int32_t x,int32_t y,int32_t z)
{GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x18,((uint32_t)(uint16_t)x<<16)+0x8000);hn_write32(m,slot+0x1C,((uint32_t)(uint16_t)y<<16)+0x8000);hn_write32(m,slot+0x20,((uint32_t)(uint16_t)z<<16)+0x8000);}
static void velocity(HNMemory *m,uint8_t index,int16_t x,int16_t y,int16_t z)
{GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x24,(uint32_t)(int32_t)x<<16);hn_write32(m,slot+0x28,(uint32_t)(int32_t)y<<16);hn_write32(m,slot+0x2C,(uint32_t)(int32_t)z<<16);}
static void set_flag(HNMemory *m,uint32_t id,uint32_t value)
{GameAddr address=0x03002180+(id>>3);uint8_t bit=(uint8_t)(1u<<(id&7));hn_write8(m,address,(uint8_t)((hn_read8(m,address)&~bit)|(value?bit:0)));}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
/* Resource pointers remain addresses in the player's private ROM. */
HNSceneResult hn_bank5_presets(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x08084DC0:{ /* ConfigureMapBank5Preset84DC0 */
        REQUIRE(animation(m,16, 0x088DFA04));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DFA10));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x08084DFC:{ /* ConfigureMapBank5Preset84DFC */
        REQUIRE(animation(m,16, 0x088DFA25));
        break;
    }
    case 0x08084E10:{ /* ConfigureMapBank5Preset84E10 */
        REQUIRE(field(m,15, 0));
        REQUIRE(animation(m,16, 0x088DFA31));
        break;
    }
    case 0x08084E2C:{ /* ConfigureMapBank5Preset84E2C */
        REQUIRE(animation(m,16, 0x088DFA3D));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DF581));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088DF5AE));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 16));
        break;
    }
    case 0x08084E90:{ /* ConfigureMapBank5Preset84E90 */
        REQUIRE(animation(m,16, 0x088DF739));
        break;
    }
    case 0x08084EA4:{ /* ConfigureMapBank5Preset84EA4 */
        REQUIRE(animation(m,16, 0x088DFA49));
        break;
    }
    case 0x08084EB8:{ /* ConfigureMapBank5Preset84EB8 */
        REQUIRE(animation(m,16, 0x088DFA55));
        break;
    }
    case 0x08084ECC:{ /* ConfigureMapBank5Preset84ECC */
        REQUIRE(animation(m,16, 0x088DFA61));
        break;
    }
    case 0x08084EE0:{ /* ConfigureMapBank5Preset84EE0 */
        REQUIRE(field(m,15, 0));
        REQUIRE(animation(m,16, 0x088DF2DC));
        break;
    }
    case 0x08084EFC:{ /* ConfigureMapBank5Preset84EFC */
        REQUIRE(animation(m,16, 0x088DF65D));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088DF65D);
        break;
    }
    case 0x08084F24:{ /* ConfigureMapBank5Preset84F24 */
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E438D));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(animation(m,16, 0x088DF65D));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088DF65D);
        break;
    }
    case 0x08084F78:{ /* ConfigureMapBank5Preset84F78 */
        REQUIRE(field(m,15, 0));
        REQUIRE(animation(m,16, 0x088DFA76));
        break;
    }
    case 0x08084F94:{ /* ConfigureMapBank5Preset84F94 */
        REQUIRE(animation(m,16, 0x088DF6D2));
        break;
    }
    case 0x08084FA8:{ /* ConfigureMapBank5Preset84FA8 */
        REQUIRE(animation(m,16, 0x088DF95C));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088DF978));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x08084FE4:{ /* ConfigureMapBank5Preset84FE4 */
        REQUIRE(animation(m,16, 0x088DFA82));
        break;
    }
    case 0x08084FF8:{ /* ConfigureMapBank5Preset84FF8 */
        REQUIRE(animation(m,16, 0x088DF6B6));
        break;
    }
    case 0x0808500C:{ /* ConfigureMapBank5Preset8500C */
        REQUIRE(animation(m,16, 0x088DF495));
        break;
    }
    case 0x08085020:{ /* ConfigureMapBank5Preset85020 */
        REQUIRE(animation(m,16, 0x088DF940));
        break;
    }
    case 0x08085034:{ /* ConfigureMapBank5Preset85034 */
        REQUIRE(animation(m,16, 0x088DF8D9));
        break;
    }
    case 0x08085048:{ /* ConfigureMapBank5Preset85048 */
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088EE73A));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(animation(m,16, 0x088DF65D));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088DF65D);
        break;
    }
    case 0x080850B8:{ /* ConfigureMapBank5Preset850B8 */
        REQUIRE(animation(m,16, 0x088C4C1C));
        break;
    }
    case 0x080850CC:{ /* ConfigureMapBank5Preset850CC */
        REQUIRE(animation(m,16, 0x088C46F1));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088C4780));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x08085108:{ /* ConfigureMapBank5Preset85108 */
        REQUIRE(field(m,15, 0));
        REQUIRE(animation(m,16, 0x088C4C28));
        hn_write8(m,0x03001FAC,16);
        hn_write32(m,0x03001F7C,0x088C4C28);
        break;
    }
    case 0x08085138:{ /* ConfigureMapBank5Preset85138 */
        REQUIRE(animation(m,19, 0x088CB101));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088CB10D));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08085174:{ /* ConfigureMapBank5Preset85174 */
        REQUIRE(animation(m,19, 0x088CB129));
        REQUIRE(field(m,18, 0));
        hn_write8(m,0x03001FAC,19);
        hn_write32(m,0x03001F7C,0x088CB129);
        break;
    }
    case 0x080851A4:{ /* ConfigureMapBank5Preset851A4 */
        REQUIRE(animation(m,19, 0x088CC40C));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088CC423));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        hn_write8(m,0x03001FAC,19);
        hn_write32(m,0x03001F7C,0x088CC40C);
        break;
    }
    case 0x080851F8:{ /* ConfigureMapBank5Preset851F8 */
        REQUIRE(field(m,18, 0));
        REQUIRE(animation(m,19, 0x088CA41A));
        break;
    }
    case 0x08085214:{ /* ConfigureMapBank5Preset85214 */
        REQUIRE(animation(m,19, 0x088CB58B));
        hn_write8(m,0x03001FAC,19);
        hn_write32(m,0x03001F7C,0x088CB58B);
        break;
    }
    case 0x0808523C:{ /* ConfigureMapBank5Preset8523C */
        REQUIRE(animation(m,19, 0x088CACFB));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088CAD07));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x08085278:{ /* ConfigureMapBank5Preset85278 */
        REQUIRE(animation(m,19, 0x088CAC43));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088CAC58));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x080852B4:{ /* ConfigureMapBank5Preset852B4 */
        REQUIRE(raw_animation(m,10, 0x088A54EC));
        REQUIRE(hn_actor_presets_apply(m,0x080138F8)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801A114)==HN_SCENE_OK);
        indicator(m,10, 0x088A54EC);
        break;
    }
    case 0x080852DC:{ /* ConfigureMapBank5Preset852DC */
        REQUIRE(hn_actor_presets_apply(m,0x08016258)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x080195D8)==HN_SCENE_OK);
        break;
    }
    case 0x080852EC:{ /* ConfigureMapBank5Preset852EC */
        REQUIRE(hn_actor_presets_apply(m,0x080165B4)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08016F74)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B104)==HN_SCENE_OK);
        break;
    }
    case 0x08085300:{ /* ConfigureMapBank5Preset85300 */
        REQUIRE(hn_actor_presets_apply(m,0x08016A9C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B528)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B57C)==HN_SCENE_OK);
        break;
    }
    case 0x08085314:{ /* ConfigureMapBank5Preset85314 */
        REQUIRE(hn_actor_presets_apply(m,0x08016D4C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018E64)==HN_SCENE_OK);
        break;
    }
    case 0x08085324:{ /* ConfigureMapBank5Preset85324 */
        if (hn_read8(m,HN_SCENE_CONTROLS+0x1AA) == 2)
            REQUIRE(hn_actor_presets_apply(m,0x0801886C)==HN_SCENE_OK);
        else
            REQUIRE(hn_actor_presets_apply(m,0x080188E4)==HN_SCENE_OK);
        break;
    }
    case 0x08085348:{ /* ConfigureMapBank5Preset85348 */
        REQUIRE(hn_actor_presets_apply(m,0x080188FC)==HN_SCENE_OK);
        indicator(m,hn_read8(m,0x03002C04), 0x088DE9DF);
        break;
    }
    case 0x08085364:{ /* ConfigureMapBank5Preset85364 */
        REQUIRE(animation(m,10, 0x088A68D4));
        REQUIRE(hn_render_configure_raw(m,9, 1, 176, 0, 0, 0, 0x088E2254));
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088E222E));
        REQUIRE(hn_render_modes_raw(m,9, 2, 3, 10));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        REQUIRE(animation(m,13, 0x088AAE48));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088E2254));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088E2241));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        break;
    }
    case 0x080855F8:{ /* ApplyMapBank5Preset855F8 */
        REQUIRE(animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER), 0x088B6A9F));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) + 1, 1, 176, 0, 0, 0, 0x088E3980));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) + 1, 2, 3, hn_read8(m,HN_RENDER_CLASS_FOLLOWER)));
        break;
    }
    case 0x0808564C:{ /* ApplyMapBank5Preset8564C */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EECDF));
        break;
    }
    case 0x08085684:{ /* ApplyMapBank5Preset85684 */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EED7C));
        break;
    }
    case 0x080856BC:{ /* ApplyMapBank5Preset856BC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EED8A));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1, 1, 176, 0, 0, 0, 0x088E351C));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x08085710:{ /* ApplyMapBank5Preset85710 */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EED57));
        break;
    }
    case 0x08085748:{ /* ApplyMapBank5Preset85748 */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EED98));
        break;
    }
    case 0x08085780:{ /* ApplyMapBank5Preset85780 */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EECED));
        break;
    }
    case 0x080857B8:{ /* ApplyMapBank5Preset857B8 */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EEDA6));
        break;
    }
    case 0x080857F0:{ /* ApplyMapBank5Preset857F0 */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EED49));
        break;
    }
    case 0x08085828:{ /* ApplyMapBank5Preset85828 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EECFB));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1, 1, 176, 0, 0, 0, 0x088D1475));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0808587C:{ /* ApplyMapBank5Preset8587C */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EEDB4));
        break;
    }
    case 0x080858B4:{ /* ApplyMapBank5Preset858B4 */
        uint32_t index = hn_read8(m,0x03002C04);
        GameAddr slot=hn_render_slot(m,index);
        if (hn_read8(m,slot+104+0x60))
            hn_write8(m,slot+104+0x60,0);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EECC9));
        break;
    }
    case 0x08085B7C:{ /* ConfigureMapBank5Preset85B7C */

        break;
    }
    case 0x08085BC0:{ /* ConfigureMapBank5Preset85BC0 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1364));
        REQUIRE(hn_actor_presets_apply(m,0x08013610)==HN_SCENE_OK);
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 1, 176, 0, 0, 0, 0x088ADA54));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 2, 3, hn_read8(m,HN_RENDER_CLASS_FOLLOWER)));
        break;
    }
    case 0x08085C18:{ /* ConfigureMapBank5Preset85C18 */

        break;
    }
    case 0x08085C1C:{ /* ConfigureMapBank5Preset85C1C */
        cell_position(m,hn_read8(m,0x03002C04), 168, 0, 64);
        velocity(m,hn_read8(m,0x03002C04), 0, 0, 0);
        REQUIRE(field(m,15, 0));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00), 1, 114, 208, 0, 64, 0x088BD206));
        REQUIRE(field(m,18, 0));
        break;
    }
    case 0x08085C78:{ /* ConfigureMapBank5Preset85C78 */
        cell_position(m,hn_read8(m,0x03002C00), 208, 0, 64);
        velocity(m,hn_read8(m,0x03002C00), 0, 0, 0);
        REQUIRE(field(m,18, 0));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04), 1, 114, 168, 0, 64, 0x088BC61E));
        REQUIRE(field(m,15, 0));
        break;
    }
    case 0x08085CD4:{ /* ConfigureMapBank5Preset85CD4 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF463));
        break;
    }
    case 0x08085CEC:{ /* ConfigureMapBank5Preset85CEC */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF463));
        break;
    }
    case 0x08085D04:{ /* ConfigureMapBank5Preset85D04 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF471));
        break;
    }
    case 0x08085D1C:{ /* ConfigureMapBank5Preset85D1C */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF471));
        break;
    }
    case 0x08085D34:{ /* ConfigureMapBank5Preset85D34 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF47F));
        break;
    }
    case 0x08085D4C:{ /* ConfigureMapBank5Preset85D4C */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF4EB));
        break;
    }
    case 0x08085D64:{ /* ConfigureMapBank5Preset85D64 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF4F9));
        break;
    }
    case 0x08085D7C:{ /* ConfigureMapBank5Preset85D7C */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF507));
        break;
    }
    case 0x08085D94:{ /* ConfigureMapBank5Preset85D94 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF507));
        break;
    }
    case 0x08085DAC:{ /* ConfigureMapBank5Preset85DAC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF515));
        break;
    }
    case 0x08085DC4:{ /* ConfigureMapBank5Preset85DC4 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF515));
        break;
    }
    case 0x08085DDC:{ /* ConfigureMapBank5Preset85DDC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF523));
        break;
    }
    case 0x08085DF4:{ /* ConfigureMapBank5Preset85DF4 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF523));
        break;
    }
    case 0x08085E0C:{ /* ConfigureMapBank5Preset85E0C */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF56A));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1, 1, 176, 0, 0, 0, 0x088DEAE5));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x08085E5C:{ /* ConfigureMapBank5Preset85E5C */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF56A));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1, 1, 176, 0, 0, 0, 0x088DEAE5));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1, 2, 3, hn_read8(m,0x03002C00)));
        break;
    }
    case 0x08085EAC:{ /* ConfigureMapBank5Preset85EAC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF422));
        break;
    }
    case 0x08085EC4:{ /* ConfigureMapBank5Preset85EC4 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF422));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1, 1, 176, 0, 0, 0, 0x088E39A9));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1, 2, 3, hn_read8(m,0x03002C00)));
        break;
    }
    case 0x08085F14:{ /* ConfigureMapBank5Preset85F14 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF422));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1, 1, 176, 0, 0, 0, 0x088EF447));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1, 2, 3, hn_read8(m,0x03002C00)));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1, 1, 176, 0, 0, 0, 0x088EF455));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1, 2, 3, hn_read8(m,0x03002C00)));
        break;
    }
    case 0x08085F94:{ /* ConfigureMapBank5Preset85F94 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF58F));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1, 1, 176, 0, 0, 0, 0x088E1E37));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1, 2, 3, hn_read8(m,0x03002C04)));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1, 1, 176, 0, 0, 0, 0x088E1EFC));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1, 2, 3, hn_read8(m,0x03002C04)));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,0x088EF58F);
        break;
    }
    case 0x08086028:{ /* ConfigureMapBank5Preset86028 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF58F));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1, 1, 176, 0, 0, 0, 0x088E1E37));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1, 2, 3, hn_read8(m,0x03002C00)));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1, 1, 176, 0, 0, 0, 0x088E1EFC));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1, 2, 3, hn_read8(m,0x03002C00)));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00));
        hn_write32(m,0x03001F7C,0x088EF58F);
        break;
    }
    case 0x080860BC:{ /* ConfigureMapBank5Preset860BC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF606));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,0x088EF606);
        break;
    }
    case 0x080860EC:{ /* ConfigureMapBank5Preset860EC */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E13AB));
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088EF6CD));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1, 1, 176, 0, 0, 0, 0x088E1C3C));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1, 2, 3, hn_read8(m,0x03002C00)));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C00));
        hn_write32(m,0x03001F7C,0x088EF6CD);
        break;
    }
    case 0x08086204:{ /* ConfigureMapBank5Preset86204 */

        break;
    }
    case 0x08086208:{ /* ConfigureMapBank5Preset86208 */
        REQUIRE(hn_render_configure_raw(m,17, 1, 176, 0, 0, 0, 0x088BC821));
        REQUIRE(hn_render_modes_raw(m,17, 2, 3, 16));
        break;
    }
    case 0x08086238:{ /* ConfigureMapBank5Preset86238 */
        REQUIRE(hn_render_configure_raw(m,20, 1, 176, 0, 0, 0, 0x088BD3CF));
        REQUIRE(hn_render_modes_raw(m,20, 2, 3, 19));
        break;
    }
    case 0x08086268:{ /* ConfigureMapBank5Preset86268 */
        REQUIRE(hn_render_configure_raw(m,11, 1, 176, 0, 0, 0, 0x088AD273));
        REQUIRE(hn_render_modes_raw(m,11, 2, 3, 10));
        break;
    }
    case 0x08086298:{ /* ConfigureMapBank5Preset86298 */
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088B88BA));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        break;
    }
    case 0x080862C8:{ /* ConfigureMapBank5Preset862C8 */
        hn_write8(m,0x03001FAC,13);
        hn_write32(m,0x03001F7C,0x088B8840);
        break;
    }
    case 0x080862E4:{ /* ConfigureMapBank5Preset862E4 */
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 1, 176, 0, 0, 0, 0x088B4039));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 2, 3, hn_read8(m,HN_RENDER_CLASS_FOLLOWER)));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) + 1, 1, 176, 0, 0, 0, 0x088B4047));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) + 1, 2, 3, hn_read8(m,HN_RENDER_CLASS_FOLLOWER)));
        break;
    }
    case 0x08086358:{ /* ConfigureMapBank5Preset86358 */
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 1, 176, 0, 0, 0, 0x088B53AC));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 2, 3, hn_read8(m,HN_RENDER_CLASS_FOLLOWER)));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) + 1, 1, 176, 0, 0, 0, 0x088B53BA));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) + 1, 2, 3, hn_read8(m,HN_RENDER_CLASS_FOLLOWER)));
        break;
    }
    case 0x080863CC:{ /* ConfigureMapBank5Preset863CC */
        REQUIRE(hn_actor_presets_apply(m,0x08016048)==HN_SCENE_OK);
        REQUIRE(auxiliary(m,hn_read8(m,0x03002C04) - 1, hn_read8(m,0x03002C04), 0x088CDB6B));
        break;
    }
    case 0x080863F0:{ /* ConfigureMapBank5Preset863F0 */
        REQUIRE(hn_audio_song(m,47,HN_SONG_CHANGE));
        break;
    }
    case 0x080863FC:{ /* ConfigureMapBank5Preset863FC */
        REQUIRE(hn_audio_song(m,48,HN_SONG_CHANGE));
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EF9E2));
        break;
    }
    case 0x08086494:{ /* ConfigureMapBank5Preset86494 */
        REQUIRE(hn_actor_presets_apply(m,0x08017D74)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08019BB0)==HN_SCENE_OK);
        break;
    }
    case 0x080864A4:{ /* ConfigureMapBank5Preset864A4 */
        REQUIRE(hn_actor_presets_apply(m,0x08017CFC)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018E90)==HN_SCENE_OK);
        break;
    }
    case 0x08086AB4:{ /* ApplyMapBank5Preset86AB4 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EFD56));
        indicator(m,hn_read8(m,0x03002C04), 0x088EFD56);
        break;
    }
    case 0x08086ADC:{ /* ApplyMapBank5Preset86ADC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EFCDE));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1, 1, 176, 0, -1, 1, 0x088E18D8));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1, 2, 3, hn_read8(m,0x03002C04)));
        hn_render_flags(m,hn_read8(m,0x03002C04) + 1,3, 1);
        REQUIRE(hn_render_reorder(m,(hn_read8(m,0x03002C04) + 1), hn_read8(m,0x03002C04), 0));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,0x088EFCDE);
        break;
    }
    case 0x08086B60:{ /* ApplyMapBank5Preset86B60 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EFD40));
        break;
    }
    case 0x08086B7C:{ /* ApplyMapBank5Preset86B7C */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088EECED));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1, 1, 176, 0, 0, 0, 0x088EFF2A));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1, 2, 3, hn_read8(m,0x03002C04)));
        REQUIRE(hn_render_reorder(m,(hn_read8(m,0x03002C04) + 1), hn_read8(m,0x03002C04), 0));
        break;
    }
    case 0x08086DD4:{ /* ConfigureMapBank5Preset86DD4 */

        break;
    }
    case 0x08086DD8:{ /* ConfigureMapBank5Preset86DD8 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F031C));
        break;
    }
    case 0x08086DF0:{ /* ConfigureMapBank5Preset86DF0 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F02E2));
        break;
    }
    case 0x08086E08:{ /* ConfigureMapBank5Preset86E08 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F02CA));
        break;
    }
    case 0x08086E20:{ /* ConfigureMapBank5Preset86E20 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F0306));
        break;
    }
    case 0x08086E38:{ /* ConfigureMapBank5Preset86E38 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F02F0));
        break;
    }
    case 0x08086E50:{ /* ConfigureMapBank5Preset86E50 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F02D4));
        break;
    }
    case 0x08086E68:{ /* ConfigureMapBank5Preset86E68 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F0338));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1, 1, 176, 0, 0, 0, 0x088E1FD6));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1, 2, 3, hn_read8(m,0x03002C04)));
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0524));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) - 1, 1, 176, 0, 0, 0, 0x088E1FAD));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) - 1, 2, 3, hn_read8(m,0x03002C00)));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,0x088F0338);
        break;
    }
    case 0x08086F18:{ /* ConfigureMapBank5Preset86F18 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F0246));
        break;
    }
    case 0x08086F30:{ /* ConfigureMapBank5Preset86F30 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F04A2));
        break;
    }
    case 0x08086F48:{ /* ConfigureMapBank5Preset86F48 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F04B0));
        break;
    }
    case 0x08086F60:{ /* ConfigureMapBank5Preset86F60 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F05B2));
        break;
    }
    case 0x08086F78:{ /* ConfigureMapBank5Preset86F78 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F04BE));
        break;
    }
    case 0x08086F90:{ /* ConfigureMapBank5Preset86F90 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F04CC));
        break;
    }
    case 0x08086FA8:{ /* ConfigureMapBank5Preset86FA8 */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F04F1));
        break;
    }
    case 0x08086FC0:{ /* ConfigureMapBank5Preset86FC0 */
        REQUIRE(animation(m,(hn_read8(m,0x03002C04) - 1), 0x088F0314));
        break;
    }
    case 0x08086FE0:{ /* ConfigureMapBank5Preset86FE0 */
        REQUIRE(animation(m,(hn_read8(m,0x03002C04) - 1), 0x088F02FE));
        break;
    }
    case 0x08087088:{ /* ConfigureMapBank5Preset87088 */
        REQUIRE(animation(m,10, 0x088A5E7C));
        REQUIRE(animation(m,13, 0x088AA043));
        indicator(m,10, 0x088A5E7C);
        break;
    }
    case 0x080870B4:{ /* ConfigureMapBank5Preset870B4 */
        REQUIRE(hn_actor_presets_apply(m,0x08013778)==HN_SCENE_OK);
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 1, 176, 0, 0, 0, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 2, 3, hn_read8(m,HN_RENDER_CLASS_FOLLOWER)));
        break;
    }
    case 0x080870FC:{ /* ConfigureMapBank5Preset870FC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088D3638));
        indicator(m,hn_read8(m,0x03002C04), 0x088D3638);
        break;
    }
    case 0x08087120:{ /* ConfigureMapBank5Preset87120 */
        REQUIRE(hn_actor_presets_apply(m,0x08013610)==HN_SCENE_OK);
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 1, 176, 0, 0, 0, 0x088ADA54));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER) - 1, 2, 3, hn_read8(m,HN_RENDER_CLASS_FOLLOWER)));
        break;
    }
    case 0x08087168:{ /* ConfigureMapBank5Preset87168 */
        REQUIRE(field(m,9, 0));
        REQUIRE(field(m,11, 0));
        break;
    }
    case 0x08087180:{ /* ConfigureMapBank5Preset87180 */
        REQUIRE(field(m,12, 0));
        REQUIRE(field(m,14, 0));
        break;
    }
    case 0x08087198:{ /* ConfigureMapBank5Preset87198 */
        REQUIRE(field(m,15, 0));
        REQUIRE(field(m,17, 0));
        break;
    }
    case 0x080871B0:{ /* ConfigureMapBank5Preset871B0 */
        REQUIRE(field(m,18, 0));
        REQUIRE(field(m,20, 0));
        break;
    }
    case 0x080871C8:{ /* ConfigureMapBank5Preset871C8 */
        REQUIRE(field(m,28, 0));
        REQUIRE(field(m,30, 0));
        break;
    }
    case 0x080871E0:{ /* ConfigureMapBank5Preset871E0 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0985));
        break;
    }
    case 0x080871FC:{ /* ConfigureMapBank5Preset871FC */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0996));
        break;
    }
    case 0x08087218:{ /* ConfigureMapBank5Preset87218 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F09A7));
        break;
    }
    case 0x08087234:{ /* ConfigureMapBank5Preset87234 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F09B8));
        break;
    }
    case 0x08087250:{ /* ConfigureMapBank5Preset87250 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,19, 0x088F09DD));
        break;
    }
    case 0x08087268:{ /* ConfigureMapBank5Preset87268 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0A15));
        break;
    }
    case 0x08087284:{ /* ConfigureMapBank5Preset87284 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0A26));
        break;
    }
    case 0x080872A0:{ /* ConfigureMapBank5Preset872A0 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0A37));
        break;
    }
    case 0x080872BC:{ /* ConfigureMapBank5Preset872BC */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0A37));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1, 1, 176, 0, -8, 8, 0x088D1475));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C00) + 1, 2, 3, hn_read8(m,0x03002C00)));
        break;
    }
    case 0x08087324:{ /* ConfigureMapBank5Preset87324 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0A5E));
        break;
    }
    case 0x08087340:{ /* ConfigureMapBank5Preset87340 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0A4D));
        break;
    }
    case 0x0808735C:{ /* ConfigureMapBank5Preset8735C */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0A15));
        break;
    }
    case 0x08087378:{ /* ConfigureMapBank5Preset87378 */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0985));
        break;
    }
    case 0x08087394:{ /* ConfigureMapBank5Preset87394 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F0E8A));
        break;
    }
    case 0x080873B0:{ /* ConfigureMapBank5Preset873B0 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F0E82));
        break;
    }
    case 0x080873CC:{ /* ConfigureMapBank5Preset873CC */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0A6F));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C00) + 1, 1, 114, 0, 0, 0, 0x088F0A94));
        break;
    }
    case 0x0808741C:{ /* ConfigureMapBank5Preset8741C */
        REQUIRE(hn_bank5_presets(m,0x080871B0)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088F0ABF));
        break;
    }
    case 0x08087438:{ /* ConfigureMapBank5Preset87438 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F101D));
        break;
    }
    case 0x08087454:{ /* ConfigureMapBank5Preset87454 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F10F9));
        break;
    }
    case 0x08087470:{ /* ConfigureMapBank5Preset87470 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1150));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1, 1, 114, 0, 0, 0, 0x088F1185));
        break;
    }
    case 0x080874C0:{ /* ConfigureMapBank5Preset874C0 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088C4A01));
        break;
    }
    case 0x080874DC:{ /* ConfigureMapBank5Preset874DC */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F0F9C));
        break;
    }
    case 0x080874F8:{ /* ConfigureMapBank5Preset874F8 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F11A9));
        break;
    }
    case 0x08087514:{ /* ConfigureMapBank5Preset87514 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F11EC));
        break;
    }
    case 0x08087530:{ /* ConfigureMapBank5Preset87530 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,13, 0x088B70BE));
        break;
    }
    case 0x08087548:{ /* ConfigureMapBank5Preset87548 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,13, 0x088B6A9F));
        break;
    }
    case 0x08087560:{ /* ConfigureMapBank5Preset87560 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,13, 0x088B6ADD));
        break;
    }
    case 0x08087578:{ /* ConfigureMapBank5Preset87578 */
        REQUIRE(hn_bank5_presets(m,0x08087198)==HN_SCENE_OK);
        REQUIRE(animation(m,13, 0x088F186A));
        hn_write8(m,0x03001FAC,13);
        hn_write32(m,0x03001F7C,0x088F186A);
        break;
    }
    case 0x080876CC:{ /* ConfigureMapBank5Preset876CC */

        break;
    }
    case 0x080876D0:{ /* ConfigureMapBank5Preset876D0 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1ABC));
        break;
    }
    case 0x080876E8:{ /* ConfigureMapBank5Preset876E8 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1AE3));
        break;
    }
    case 0x08087700:{ /* ConfigureMapBank5Preset87700 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1963));
        break;
    }
    case 0x08087718:{ /* ConfigureMapBank5Preset87718 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F19EF));
        break;
    }
    case 0x08087730:{ /* ConfigureMapBank5Preset87730 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1A84));
        break;
    }
    case 0x08087748:{ /* ConfigureMapBank5Preset87748 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1A68));
        break;
    }
    case 0x08087760:{ /* ConfigureMapBank5Preset87760 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1A5A));
        break;
    }
    case 0x08087778:{ /* ConfigureMapBank5Preset87778 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1955));
        break;
    }
    case 0x08087790:{ /* ConfigureMapBank5Preset87790 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F19FD));
        break;
    }
    case 0x080877A8:{ /* ConfigureMapBank5Preset877A8 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F1A76));
        break;
    }
    case 0x080877C0:{ /* ConfigureMapBank5Preset877C0 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F19CA));
        break;
    }
    case 0x080877D8:{ /* ConfigureMapBank5Preset877D8 */
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) + 1, 1, 176, 0, 0, 0, 0x088D8C0B));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) + 1, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0808784C:{ /* ConfigureMapBank5Preset8784C */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1338));
        REQUIRE(animation(m,10, 0x088A4F2D));
        REQUIRE(animation(m,13, 0x088A8CF9));
        REQUIRE(hn_render_configure_raw(m,12, 1, 176, 0, 0, 0, 0x088E1994));
        REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088E19AE));
        REQUIRE(hn_render_modes_raw(m,12, 2, 3, 13));
        REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
        indicator(m,10, 0x088A4F2D);
        break;
    }
    case 0x080878D4:{ /* ConfigureMapBank5Preset878D4 */
        REQUIRE(hn_actor_presets_apply(m,0x08016D7C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801977C)==HN_SCENE_OK);
        break;
    }
    case 0x080878E4:{ /* ConfigureMapBank5Preset878E4 */
        REQUIRE(hn_actor_presets_apply(m,0x08016D28)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B678)==HN_SCENE_OK);
        break;
    }
    case 0x080878F4:{ /* ConfigureMapBank5Preset878F4 */
        REQUIRE(hn_actor_presets_apply(m,0x0801581C)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1, 1, 176, 0, 0, 0, 0x088CB778));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x0808794C:{ /* ConfigureMapBank5Preset8794C */
        REQUIRE(hn_actor_presets_apply(m,0x0801536C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801A968)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801A9BC)==HN_SCENE_OK);
        break;
    }
    case 0x08087960:{ /* ConfigureMapBank5Preset87960 */
        REQUIRE(hn_actor_presets_apply(m,0x080153B4)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x080199B8)==HN_SCENE_OK);
        break;
    }
    case 0x08087970:{ /* ConfigureMapBank5Preset87970 */

        break;
    }
    case 0x08087974:{ /* ConfigureMapBank5Preset87974 */
        set_flag(m,23, 1);
        REQUIRE(hn_actor_presets_apply(m,0x0801533C)==HN_SCENE_OK);
        break;
    }
    case 0x08087988:{ /* ConfigureMapBank5Preset87988 */
        REQUIRE(hn_actor_presets_apply(m,0x08015B2C)==HN_SCENE_OK);
        REQUIRE(hn_render_configure_raw(m,hn_read8(m,0x03002C04) - 1, 1, 176, 0, 0, 0, 0x088F1E86));
        REQUIRE(hn_render_modes_raw(m,hn_read8(m,0x03002C04) - 1, 2, 3, hn_read8(m,0x03002C04)));
        break;
    }
    case 0x080879D0:{ /* ConfigureMapBank5Preset879D0 */
        REQUIRE(hn_actor_presets_apply(m,0x08015384)==HN_SCENE_OK);
        indicator(m,hn_read8(m,0x03002C04), 0x088CABD0);
        break;
    }
    case 0x080879EC:{ /* ConfigureMapBank5Preset879EC */
        REQUIRE(hn_actor_presets_apply(m,0x080136B8)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x080192A4)==HN_SCENE_OK);
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank5_preset_callback(HNMemory *m,GameAddr callback,uint32_t *consumed)
{
    switch(callback&~1u){
    case 0x080855F8:
    case 0x0808564C:
    case 0x08085684:
    case 0x080856BC:
    case 0x08085710:
    case 0x08085748:
    case 0x08085780:
    case 0x080857B8:
    case 0x080857F0:
    case 0x08085828:
    case 0x0808587C:
    case 0x080858B4:
    case 0x08086AB4:
    case 0x08086ADC:
    case 0x08086B60:
    case 0x08086B7C:
    case 0x0808784C:
        *consumed=0;return hn_bank5_presets(m,callback);
    default:return HN_SCENE_UNKNOWN;
    }
}
