#include "game/bank6.h"
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
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
/* Source-shaped wrappers; no original resource contents are embedded. */
HNSceneResult hn_bank6_presets(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x0809FE40:{ /* ConfigureMapBank6Preset9FE40 */
        REQUIRE(field(m,9, 0));
        REQUIRE(field(m,11, 0));
        break;
    }
    case 0x0809FE58:{ /* ConfigureMapBank6Preset9FE58 */
        REQUIRE(field(m,12, 0));
        REQUIRE(field(m,14, 0));
        break;
    }
    case 0x0809FE70:{ /* ConfigureMapBank6Preset9FE70 */
        REQUIRE(field(m,15, 0));
        REQUIRE(field(m,17, 0));
        break;
    }
    case 0x0809FE88:{ /* ConfigureMapBank6Preset9FE88 */
        REQUIRE(field(m,18, 0));
        REQUIRE(field(m,20, 0));
        break;
    }
    case 0x0809FEA0:{ /* ConfigureMapBank6Preset9FEA0 */
        REQUIRE(field(m,28, 0));
        REQUIRE(field(m,30, 0));
        break;
    }
    case 0x0809FEB8:{ /* ConfigureMapBank6Preset9FEB8 */
        REQUIRE(hn_bank6_presets(m,0x0809FE40)==HN_SCENE_OK);
        REQUIRE(hn_bank6_presets(m,0x0809FE58)==HN_SCENE_OK);
        REQUIRE(animation(m,10, 0x088A5D5B));
        REQUIRE(animation(m,13, 0x088A9F2E));
        indicator(m,10, 0x088A5D5B);
        break;
    }
    case 0x0809FEEC:{ /* ConfigureMapBank6Preset9FEEC */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088D5C94));
        break;
    }
    case 0x0809FF08:{ /* ConfigureMapBank6Preset9FF08 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088D457A));
        break;
    }
    case 0x0809FF24:{ /* ConfigureMapBank6Preset9FF24 */
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088D66FF));
        break;
    }
    case 0x0809FF48:{ /* ConfigureMapBank6Preset9FF48 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088D5888));
        break;
    }
    case 0x0809FF64:{ /* ConfigureMapBank6Preset9FF64 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088D489D));
        break;
    }
    case 0x0809FF80:{ /* ConfigureMapBank6Preset9FF80 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088D5CA9));
        hn_write8(m,0x03001FAC,hn_read8(m,0x03002C04));
        hn_write32(m,0x03001F7C,0x088D5CA9);
        break;
    }
    case 0x0809FFB4:{ /* ConfigureMapBank6Preset9FFB4 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E3A24));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x0809FFF8:{ /* ConfigureMapBank6Preset9FFF8 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1330));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E3A63));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080A003C:{ /* ConfigureMapBank6PresetA003C */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1364));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E3A81));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080A0080:{ /* ConfigureMapBank6PresetA0080 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E137B));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E3A6D));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080A00C4:{ /* ConfigureMapBank6PresetA00C4 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        break;
    }
    case 0x080A00D0:{ /* ConfigureMapBank6PresetA00D0 */
        REQUIRE(hn_bank6_presets(m,0x0809FE40)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,HN_RENDER_CLASS_LEADER), 0x088AD32A));
        break;
    }
    case 0x080A00EC:{ /* ConfigureMapBank6PresetA00EC */
        REQUIRE(hn_bank6_presets(m,0x0809FE58)==HN_SCENE_OK);
        break;
    }
    case 0x080A00F8:{ /* ConfigureMapBank6PresetA00F8 */
        REQUIRE(hn_bank6_presets(m,0x0809FE58)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER), 0x088B5319));
        break;
    }
    case 0x080A0114:{ /* ConfigureMapBank6PresetA0114 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E1364));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088D589D));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080A0158:{ /* ConfigureMapBank6PresetA0158 */
        REQUIRE(field(m,9, 0));
        REQUIRE(field(m,11, 0));
        break;
    }
    case 0x080A0170:{ /* ConfigureMapBank6PresetA0170 */
        REQUIRE(field(m,12, 0));
        REQUIRE(field(m,14, 0));
        break;
    }
    case 0x080A0188:{ /* ConfigureMapBank6PresetA0188 */
        REQUIRE(field(m,15, 0));
        REQUIRE(field(m,17, 0));
        break;
    }
    case 0x080A01A0:{ /* ConfigureMapBank6PresetA01A0 */
        REQUIRE(field(m,18, 0));
        REQUIRE(field(m,20, 0));
        break;
    }
    case 0x080A01B8:{ /* ConfigureMapBank6PresetA01B8 */
        REQUIRE(field(m,28, 0));
        REQUIRE(field(m,30, 0));
        break;
    }
    case 0x080A01D0:{ /* ConfigureMapBank6PresetA01D0 */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088D3429));
        break;
    }
    case 0x080A01EC:{ /* ConfigureMapBank6PresetA01EC */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF2DC));
        break;
    }
    case 0x080A0208:{ /* ConfigureMapBank6PresetA0208 */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DFA04));
        break;
    }
    case 0x080A0224:{ /* ConfigureMapBank6PresetA0224 */
        REQUIRE(field(m,15, 0));
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088DFC24));
        break;
    }
    case 0x080A0250:{ /* ConfigureMapBank6PresetA0250 */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF739));
        break;
    }
    case 0x080A026C:{ /* ConfigureMapBank6PresetA026C */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF495));
        break;
    }
    case 0x080A0288:{ /* ConfigureMapBank6PresetA0288 */
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088DFC3C));
        break;
    }
    case 0x080A02AC:{ /* ConfigureMapBank6PresetA02AC */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DFB94));
        break;
    }
    case 0x080A02C8:{ /* ConfigureMapBank6PresetA02C8 */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF65D));
        break;
    }
    case 0x080A02E4:{ /* ConfigureMapBank6PresetA02E4 */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF8D9));
        break;
    }
    case 0x080A0300:{ /* ConfigureMapBank6PresetA0300 */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF940));
        break;
    }
    case 0x080A031C:{ /* ConfigureMapBank6PresetA031C */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF6B6));
        break;
    }
    case 0x080A0338:{ /* ConfigureMapBank6PresetA0338 */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF6D2));
        break;
    }
    case 0x080A0354:{ /* ConfigureMapBank6PresetA0354 */
        REQUIRE(hn_bank6_presets(m,0x080A0188)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF7A0));
        break;
    }
    case 0x080A0370:{ /* ConfigureMapBank6PresetA0370 */
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088D2500));
        break;
    }
    case 0x080A0394:{ /* ConfigureMapBank6PresetA0394 */
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088D2518));
        break;
    }
    case 0x080A03B8:{ /* ConfigureMapBank6PresetA03B8 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E3A24));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080A03F8:{ /* ConfigureMapBank6PresetA03F8 */
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E438D));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088DF65D));
        hn_write8(m,0x03001FAC,15);
        hn_write32(m,0x03001F7C,0x088E438D);
        break;
    }
    case 0x080A0450:{ /* ConfigureMapBank6PresetA0450 */

        break;
    }
    case 0x080A0454:{ /* ConfigureMapBank6PresetA0454 */
        REQUIRE(field(m,9, 0));
        REQUIRE(field(m,11, 0));
        break;
    }
    case 0x080A046C:{ /* ConfigureMapBank6PresetA046C */
        REQUIRE(field(m,12, 0));
        REQUIRE(field(m,14, 0));
        break;
    }
    case 0x080A0484:{ /* ConfigureMapBank6PresetA0484 */
        REQUIRE(field(m,15, 0));
        REQUIRE(field(m,17, 0));
        break;
    }
    case 0x080A049C:{ /* ConfigureMapBank6PresetA049C */
        REQUIRE(field(m,18, 0));
        REQUIRE(field(m,20, 0));
        break;
    }
    case 0x080A04B4:{ /* ConfigureMapBank6PresetA04B4 */
        REQUIRE(field(m,28, 0));
        REQUIRE(field(m,30, 0));
        break;
    }
    case 0x080A04CC:{ /* ConfigureMapBank6PresetA04CC */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CA41A));
        break;
    }
    case 0x080A04E8:{ /* ConfigureMapBank6PresetA04E8 */
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088CFBEA));
        break;
    }
    case 0x080A050C:{ /* ConfigureMapBank6PresetA050C */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CA43E));
        break;
    }
    case 0x080A0528:{ /* ConfigureMapBank6PresetA0528 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CDC1A));
        break;
    }
    case 0x080A0544:{ /* ConfigureMapBank6PresetA0544 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CDBA2));
        break;
    }
    case 0x080A0560:{ /* ConfigureMapBank6PresetA0560 */

        break;
    }
    case 0x080A0564:{ /* ConfigureMapBank6PresetA0564 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CACEF));
        break;
    }
    case 0x080A0580:{ /* ConfigureMapBank6PresetA0580 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CE464));
        break;
    }
    case 0x080A059C:{ /* ConfigureMapBank6PresetA059C */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CDABE));
        break;
    }
    case 0x080A05B8:{ /* ConfigureMapBank6PresetA05B8 */
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088D24E8));
        break;
    }
    case 0x080A05DC:{ /* ConfigureMapBank6PresetA05DC */
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088D3273));
        break;
    }
    case 0x080A05F4:{ /* ConfigureMapBank6PresetA05F4 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088D2E30));
        break;
    }
    case 0x080A0610:{ /* ConfigureMapBank6PresetA0610 */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088D3429));
        break;
    }
    case 0x080A062C:{ /* ConfigureMapBank6PresetA062C */
        REQUIRE(hn_bank6_presets(m,0x0809FE70)==HN_SCENE_OK);
        REQUIRE(animation(m,hn_read8(m,0x03002C00), 0x088D3655));
        break;
    }
    case 0x080A0648:{ /* ConfigureMapBank6PresetA0648 */

        break;
    }
    case 0x080A064C:{ /* ConfigureMapBank6PresetA064C */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088E3A24));
        REQUIRE(hn_render_modes_raw(m,15, 2, 3, 16));
        break;
    }
    case 0x080A068C:{ /* ConfigureMapBank6PresetA068C */

        break;
    }
    case 0x080A0690:{ /* ConfigureMapBank6PresetA0690 */

        break;
    }
    case 0x080A0694:{ /* ConfigureMapBank6PresetA0694 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,18, 1, 176, 0, 0, 0, 0x088E3A24));
        REQUIRE(hn_render_modes_raw(m,18, 2, 3, 19));
        break;
    }
    case 0x080A06D4:{ /* ConfigureMapBank6PresetA06D4 */
        REQUIRE(hn_render_configure_raw(m,28, 1, 0, 0, 0, 0, 0x088CFC32));
        break;
    }
    case 0x080A06F8:{ /* ConfigureMapBank6PresetA06F8 */

        break;
    }
    case 0x080A06FC:{ /* ConfigureMapBank6PresetA06FC */
        REQUIRE(field(m,21, 0));
        REQUIRE(animation(m,22, 0x088F2154));
        break;
    }
    case 0x080A0718:{ /* ConfigureMapBank6PresetA0718 */
        REQUIRE(animation(m,22, 0x088F217E));
        break;
    }
    case 0x080A072C:{ /* ConfigureMapBank6PresetA072C */
        REQUIRE(animation(m,22, 0x088F215C));
        break;
    }
    case 0x080A0740:{ /* ConfigureMapBank6PresetA0740 */
        REQUIRE(animation(m,22, 0x088F2176));
        break;
    }
    case 0x080A0754:{ /* ConfigureMapBank6PresetA0754 */
        REQUIRE(animation(m,22, 0x088F2169));
        break;
    }
    case 0x080A0768:{ /* ConfigureMapBank6PresetA0768 */
        REQUIRE(animation(m,22, 0x088F218E));
        break;
    }
    case 0x080A077C:{ /* ConfigureMapBank6PresetA077C */
        REQUIRE(animation(m,22, 0x088F2196));
        break;
    }
    case 0x080A0790:{ /* ConfigureMapBank6PresetA0790 */
        REQUIRE(animation(m,22, 0x088F21A6));
        break;
    }
    case 0x080A07A4:{ /* ConfigureMapBank6PresetA07A4 */
        REQUIRE(animation(m,22, 0x088F21AE));
        break;
    }
    case 0x080A07B8:{ /* ConfigureMapBank6PresetA07B8 */
        REQUIRE(animation(m,22, 0x088F21BB));
        break;
    }
    case 0x080A07CC:{ /* ConfigureMapBank6PresetA07CC */
        REQUIRE(animation(m,22, 0x088F219E));
        break;
    }
    case 0x080A07E0:{ /* ConfigureMapBank6PresetA07E0 */
        REQUIRE(animation(m,22, 0x088F21C8));
        break;
    }
    case 0x080A07F4:{ /* ConfigureMapBank6PresetA07F4 */
        REQUIRE(animation(m,22, 0x088F2186));
        break;
    }
    case 0x080A0808:{ /* ConfigureMapBank6PresetA0808 */
        REQUIRE(animation(m,22, 0x088F21D5));
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088E48B9));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, 22));
        indicator(m,21, 0x088E48B9);
        break;
    }
    case 0x080A08D8:{ /* ConfigureMapBank6PresetA08D8 */

        break;
    }
    case 0x080A08DC:{ /* ConfigureMapBank6PresetA08DC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CF2FF));
        break;
    }
    case 0x080A08F4:{ /* ConfigureMapBank6PresetA08F4 */
        REQUIRE(hn_render_configure_raw(m,15, 1, 114, 88, 0, 24, 0x088F2337));
        REQUIRE(hn_render_configure_raw(m,17, 1, 114, 88, 0, 24, 0x088F234E));
        break;
    }
    case 0x080A0934:{ /* ConfigureMapBank6PresetA0934 */

        break;
    }
    case 0x080A0938:{ /* ConfigureMapBank6PresetA0938 */
        REQUIRE(hn_render_configure_raw(m,18, 1, 114, 160, 0, 24, 0x088F2318));
        REQUIRE(hn_render_configure_raw(m,20, 1, 114, 160, 0, 24, 0x088F232F));
        break;
    }
    case 0x080A0978:{ /* ConfigureMapBank6PresetA0978 */
        REQUIRE(hn_render_configure_raw(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 1, 114, 0, 0, 0, 0x088F235E));
        indicator(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 0x088F235E);
        break;
    }
    case 0x080A09B8:{ /* ConfigureMapBank6PresetA09B8 */
        REQUIRE(field(m,21, 0));
        REQUIRE(hn_actor_presets_apply(m,0x08012DC4)==HN_SCENE_OK);
        break;
    }
    case 0x080A09CC:{ /* ConfigureMapBank6PresetA09CC */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088CF320));
        indicator(m,hn_read8(m,0x03002C04), 0x088CF320);
        break;
    }
    case 0x080A0DA8:{ /* ConfigureMapBank6PresetA0DA8 */
        REQUIRE(hn_graphics_initialize_raw(m,3, 1, 0x088E13AB));
        if (hn_read8(m,HN_SCENE_CONTROLS+0x13) != 3) {
            REQUIRE(animation(m,13, 0x088B5926));
            REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088F24AD));
            REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
            REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088F24D4));
            REQUIRE(hn_render_modes_raw(m,15, 2, 3, 13));
        } else {
            REQUIRE(animation(m,13, 0x088DBB13));
            REQUIRE(hn_render_configure_raw(m,14, 1, 176, 0, 0, 0, 0x088F24A3));
            REQUIRE(hn_render_modes_raw(m,14, 2, 3, 13));
            REQUIRE(hn_render_configure_raw(m,15, 1, 176, 0, 0, 0, 0x088F24CA));
            REQUIRE(hn_render_modes_raw(m,15, 2, 3, 13));
        }
        break;
    }
    case 0x080A1074:{ /* ConfigureMapBank6PresetA1074 */
        indicator(m,hn_read8(m,0x03002C00), 0x088D85AB);
        break;
    }
    case 0x080A1090:{ /* ConfigureMapBank6PresetA1090 */
        uint32_t direction = (uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03002C04), hn_read8(m,HN_RENDER_CLASS_LEADER),false);
        switch (direction) {
            case 3: REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088CE458));break;
            case 4: REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088CE464));break;
            case 1: REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088CA671));break;
        }
        break;
    }
    case 0x080A10F0:{ /* ConfigureMapBank6PresetA10F0 */
        uint32_t direction = (uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03002C00), hn_read8(m,HN_RENDER_CLASS_LEADER),false);
        switch (direction) {
            case 3: REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088D8866));break;
            case 4: REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088D83DB));break;
            case 2: REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088D8D5C));break;
        }
        break;
    }
    case 0x080A1150:{ /* ConfigureMapBank6PresetA1150 */
        REQUIRE(hn_actor_presets_apply(m,0x08012FAC)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x080138E0)==HN_SCENE_OK);
        break;
    }
    case 0x080A1160:{ /* ConfigureMapBank6PresetA1160 */
        REQUIRE(hn_actor_presets_apply(m,0x0801545C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08016D10)==HN_SCENE_OK);
        break;
    }
    case 0x080A1170:{ /* ConfigureMapBank6PresetA1170 */
        REQUIRE(hn_actor_presets_apply(m,0x08016330)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08016E7C)==HN_SCENE_OK);
        break;
    }
    case 0x080A1180:{ /* ConfigureMapBank6PresetA1180 */
        REQUIRE(hn_actor_presets_apply(m,0x080165D8)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08016F8C)==HN_SCENE_OK);
        break;
    }
    case 0x080A1190:{ /* ConfigureMapBank6PresetA1190 */
        REQUIRE(hn_actor_presets_apply(m,0x08016348)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08016EAC)==HN_SCENE_OK);
        break;
    }
    case 0x080A11A0:{ /* ConfigureMapBank6PresetA11A0 */
        REQUIRE(raw_animation(m,26, 0x088F2DE4));
        REQUIRE(raw_animation(m,28, 0x088F2DE4));
        REQUIRE(raw_animation(m,30, 0x088F2DE4));
        REQUIRE(raw_animation(m,32, 0x088F2DE4));
        REQUIRE(raw_animation(m,34, 0x088F2DE4));
        REQUIRE(raw_animation(m,36, 0x088F2DE4));
        break;
    }
    case 0x080A11E0:{ /* ConfigureMapBank6PresetA11E0 */
        REQUIRE(raw_animation(m,26, 0x088F2DE4));
        REQUIRE(hn_render_configure_raw(m,27, 1, 176, 0, 0, 0, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,27, 2, 3, 26));
        REQUIRE(raw_animation(m,28, 0x088F2DE4));
        REQUIRE(hn_render_configure_raw(m,29, 1, 176, 0, 0, 0, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,29, 2, 3, 28));
        REQUIRE(raw_animation(m,30, 0x088F2DE4));
        REQUIRE(hn_render_configure_raw(m,31, 1, 176, 0, 0, 0, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,31, 2, 3, 30));
        REQUIRE(raw_animation(m,32, 0x088F2DE4));
        REQUIRE(hn_render_configure_raw(m,33, 1, 176, 0, 0, 0, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,33, 2, 3, 32));
        REQUIRE(raw_animation(m,34, 0x088F2DE4));
        REQUIRE(hn_render_configure_raw(m,35, 1, 176, 0, 0, 0, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,35, 2, 3, 34));
        REQUIRE(raw_animation(m,36, 0x088F2DE4));
        REQUIRE(hn_render_configure_raw(m,37, 1, 176, 0, 0, 0, 0x088E3995));
        REQUIRE(hn_render_modes_raw(m,37, 2, 3, 36));
        break;
    }
    case 0x080A12E0:{ /* ConfigureMapBank6PresetA12E0 */
        REQUIRE(raw_animation(m,26, 0x088F2E98));
        REQUIRE(raw_animation(m,28, 0x088F2E98));
        REQUIRE(raw_animation(m,30, 0x088F2E98));
        REQUIRE(raw_animation(m,32, 0x088F2E98));
        REQUIRE(raw_animation(m,34, 0x088F2E98));
        REQUIRE(raw_animation(m,36, 0x088F2E98));
        indicator(m,26, 0x088F2E98);
        break;
    }
    case 0x080A1390:{ /* ConfigureMapBank6PresetA1390 */
        REQUIRE(raw_animation(m,26, 0x088F2E38));
        REQUIRE(raw_animation(m,28, 0x088F2E38));
        REQUIRE(raw_animation(m,30, 0x088F2E38));
        REQUIRE(raw_animation(m,32, 0x088F2E38));
        REQUIRE(raw_animation(m,34, 0x088F2E38));
        REQUIRE(raw_animation(m,36, 0x088F2E38));
        indicator(m,26, 0x088F2E38);
        break;
    }
    case 0x080A13D8:{ /* ConfigureMapBank6PresetA13D8 */
        REQUIRE(raw_animation(m,26, 0x088F2E47));
        REQUIRE(raw_animation(m,28, 0x088F2E47));
        REQUIRE(raw_animation(m,30, 0x088F2E47));
        REQUIRE(raw_animation(m,32, 0x088F2E47));
        REQUIRE(raw_animation(m,34, 0x088F2E47));
        REQUIRE(raw_animation(m,36, 0x088F2E47));
        break;
    }
    case 0x080A1418:{ /* ConfigureMapBank6PresetA1418 */
        if (!hn_read8(m,HN_SCENE_CONTROLS+0x1A8))
            REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088F2F52));
        else
            REQUIRE(raw_animation(m,38, 0x088F2F52));
        break;
    }
    case 0x080A1450:{ /* ConfigureMapBank6PresetA1450 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088F2F52));
        REQUIRE(hn_actor_presets_apply(m,0x08018F70)==HN_SCENE_OK);
        break;
    }
    case 0x080A146C:{ /* ConfigureMapBank6PresetA146C */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088F2F52));
        REQUIRE(hn_actor_presets_apply(m,0x08019878)==HN_SCENE_OK);
        break;
    }
    case 0x080A1488:{ /* ConfigureMapBank6PresetA1488 */
        REQUIRE(raw_animation(m,38, 0x088F2F5A));
        break;
    }
    case 0x080A149C:{ /* ConfigureMapBank6PresetA149C */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088F2F6A));
        break;
    }
    case 0x080A14B4:{ /* ConfigureMapBank6PresetA14B4 */
        REQUIRE(hn_actor_presets_apply(m,0x08016120)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801906C)==HN_SCENE_OK);
        break;
    }
    case 0x080A14C4:{ /* ConfigureMapBank6PresetA14C4 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088F2DF4));
        break;
    }
    case 0x080A14DC:{ /* ConfigureMapBank6PresetA14DC */

        break;
    }
    case 0x080A14E0:{ /* ConfigureMapBank6PresetA14E0 */
        REQUIRE(raw_animation(m,26, 0x088F2DEC));
        break;
    }
    case 0x080A14F4:{ /* ConfigureMapBank6PresetA14F4 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088F2D69));
        break;
    }
    case 0x080A150C:{ /* ConfigureMapBank6PresetA150C */
        REQUIRE(hn_render_configure_raw(m,45, 1, 0, 0, 0, 0, 0x088CFC1A));
        break;
    }
    case 0x080A1530:{ /* ConfigureMapBank6PresetA1530 */

        break;
    }
    case 0x080A1534:{ /* ConfigureMapBank6PresetA1534 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088F2D9A));
        break;
    }
    case 0x080A154C:{ /* ConfigureMapBank6PresetA154C */
        REQUIRE(raw_animation(m,28, 0x088F2DEC));
        break;
    }
    case 0x080A1560:{ /* ConfigureMapBank6PresetA1560 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088F2DCA));
        break;
    }
    case 0x080A1578:{ /* ConfigureMapBank6PresetA1578 */
        REQUIRE(hn_render_configure_raw(m,45, 1, 0, 0, 0, 0, 0x088CFC1A));
        break;
    }
    case 0x080A159C:{ /* ConfigureMapBank6PresetA159C */
        REQUIRE(hn_render_configure_raw(m,45, 1, 0, 0, 0, 0, 0x088CFC1A));
        break;
    }
    case 0x080A15C0:{ /* ConfigureMapBank6PresetA15C0 */
        REQUIRE(raw_animation(m,30, 0x088F2DEC));
        break;
    }
    case 0x080A15D4:{ /* ConfigureMapBank6PresetA15D4 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088F2DCA));
        break;
    }
    case 0x080A15EC:{ /* ConfigureMapBank6PresetA15EC */
        REQUIRE(hn_actor_presets_apply(m,0x08016270)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08019680)==HN_SCENE_OK);
        break;
    }
    case 0x080A15FC:{ /* ConfigureMapBank6PresetA15FC */
        if (!hn_read8(m,HN_SCENE_CONTROLS+0x1A8))
            REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088F2D69));
        else
            REQUIRE(raw_animation(m,32, 0x088F2D69));
        break;
    }
    case 0x080A1634:{ /* ConfigureMapBank6PresetA1634 */
        REQUIRE(raw_animation(m,32, 0x088F2D71));
        break;
    }
    case 0x080A1648:{ /* ConfigureMapBank6PresetA1648 */
        REQUIRE(hn_render_configure_raw(m,45, 1, 0, 0, 0, 0, 0x088CFC1A));
        break;
    }
    case 0x080A166C:{ /* ConfigureMapBank6PresetA166C */
        REQUIRE(raw_animation(m,32, 0x088F2DEC));
        break;
    }
    case 0x080A1680:{ /* ConfigureMapBank6PresetA1680 */

        break;
    }
    case 0x080A1684:{ /* ConfigureMapBank6PresetA1684 */
        REQUIRE(hn_actor_presets_apply(m,0x0801599C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08015A10)==HN_SCENE_OK);
        break;
    }
    case 0x080A1694:{ /* ConfigureMapBank6PresetA1694 */
        REQUIRE(hn_render_configure_raw(m,45, 1, 0, 0, 0, 0, 0x088CFC02));
        break;
    }
    case 0x080A16B8:{ /* ConfigureMapBank6PresetA16B8 */
        REQUIRE(raw_animation(m,34, 0x088F2DEC));
        break;
    }
    case 0x080A16CC:{ /* ConfigureMapBank6PresetA16CC */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088F2FD0));
        break;
    }
    case 0x080A16E4:{ /* ConfigureMapBank6PresetA16E4 */
        REQUIRE(raw_animation(m,36, 0x088F2DEC));
        break;
    }
    case 0x080A16F8:{ /* ConfigureMapBank6PresetA16F8 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088F2D61));
        break;
    }
    case 0x080A1710:{ /* ConfigureMapBank6PresetA1710 */
        REQUIRE(hn_actor_presets_apply(m,0x08016B90)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B5D0)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B624)==HN_SCENE_OK);
        break;
    }
    case 0x080A1724:{ /* ConfigureMapBank6PresetA1724 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088F300E));
        REQUIRE(hn_actor_presets_apply(m,0x0801A028)==HN_SCENE_OK);
        break;
    }
    case 0x080A1740:{ /* ConfigureMapBank6PresetA1740 */
        REQUIRE(raw_animation(m,hn_read8(m,0x03002C00), 0x088D280F));
        indicator(m,hn_read8(m,0x03002C00), 0x088D280F);
        REQUIRE(hn_actor_presets_apply(m,0x0801A06C)==HN_SCENE_OK);
        break;
    }
    case 0x080A1768:{ /* ConfigureMapBank6PresetA1768 */
        REQUIRE(hn_actor_presets_apply(m,0x08016C08)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08019F3C)==HN_SCENE_OK);
        break;
    }
    case 0x080A1778:{ /* ConfigureMapBank6PresetA1778 */
        REQUIRE(hn_actor_presets_apply(m,0x08016C08)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B5D0)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B624)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 0x088E135C));
        REQUIRE(hn_render_configure_raw(m,21, 1, 176, 0, 0, 0, 0x088E3AAB));
        REQUIRE(hn_render_modes_raw(m,21, 2, 3, hn_read8(m,0x03002C00)));
        hn_write8(m,hn_render_slot(m,41)+0x60,0);
        break;
    }
    case 0x080A17DC:{ /* ConfigureMapBank6PresetA17DC */
        REQUIRE(hn_actor_presets_apply(m,0x08016EAC)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801A0C0)==HN_SCENE_OK);
        break;
    }
    case 0x080A17EC:{ /* ConfigureMapBank6PresetA17EC */
        REQUIRE(raw_animation(m,42, 0x088F2FC0));
        break;
    }
    case 0x080A1800:{ /* ConfigureMapBank6PresetA1800 */
        REQUIRE(raw_animation(m,42, 0x088F2FC0));
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 143528827));
        REQUIRE(hn_render_configure_raw(m,41, 1, 176, 0, 0, 0, 0x088E3AE7));
        REQUIRE(hn_render_modes_raw(m,41, 2, 3, 42));
        break;
    }
    case 0x080A184C:{ /* ConfigureMapBank6PresetA184C */

        break;
    }
    case 0x080A1A5C:{ /* ConfigureMapBank6PresetA1A5C */
        REQUIRE(raw_animation(m,42, 0x088F2FD0));
        break;
    }
    case 0x080A1A70:{ /* ConfigureMapBank6PresetA1A70 */
        REQUIRE(hn_bank6_presets(m,0x080A1A5C)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 143528875));
        REQUIRE(hn_render_configure_raw(m,43, 1, 176, 0, 0, 0, 0x088E3AAB));
        REQUIRE(hn_render_modes_raw(m,43, 2, 3, 42));
        break;
    }
    case 0x080A1AB4:{ /* ConfigureMapBank6PresetA1AB4 */
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 143528827));
        REQUIRE(raw_animation(m,42, 0x088F2FD0));
        REQUIRE(hn_render_configure_raw(m,41, 1, 176, 0, 0, 0, 0x088E3AE7));
        REQUIRE(hn_render_modes_raw(m,41, 2, 3, 42));
        break;
    }
    case 0x080A1B00:{ /* ConfigureMapBank6PresetA1B00 */
        REQUIRE(raw_animation(m,42, 0x088F2FE5));
        break;
    }
    case 0x080A1B14:{ /* ConfigureMapBank6PresetA1B14 */
        REQUIRE(hn_bank6_presets(m,0x080A1B00)==HN_SCENE_OK);
        REQUIRE(hn_graphics_initialize_raw(m,4, 1, 143528827));
        REQUIRE(hn_render_configure_raw(m,41, 1, 176, 0, 0, 0, 0x088E3AE7));
        REQUIRE(hn_render_modes_raw(m,41, 2, 3, 42));
        break;
    }
    case 0x080A1B58:{ /* ConfigureMapBank6PresetA1B58 */
        if (!hn_read8(m,HN_SCENE_CONTROLS+0x1A8))
            REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088F2FDD));
        else
            REQUIRE(raw_animation(m,42, 0x088F2FDD));
        break;
    }
    case 0x080A1B90:{ /* ConfigureMapBank6PresetA1B90 */
        REQUIRE(hn_actor_presets_apply(m,0x08012D7C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x080134EC)==HN_SCENE_OK);
        break;
    }
    case 0x080A1C58:{ /* ConfigureMapBank6PresetA1C58 */
        REQUIRE(hn_actor_presets_apply(m,0x08012EF8)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08013610)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801A1AC)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801A2E8)==HN_SCENE_OK);
        break;
    }
    case 0x080A1C70:{ /* ConfigureMapBank6PresetA1C70 */
        REQUIRE(hn_actor_presets_apply(m,0x08018D38)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018D9C)==HN_SCENE_OK);
        break;
    }
    case 0x080A1C80:{ /* ConfigureMapBank6PresetA1C80 */
        REQUIRE(hn_actor_presets_apply(m,0x08012F58)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08013220)==HN_SCENE_OK);
        break;
    }
    case 0x080A1D84:{ /* ConfigureMapBank6PresetA1D84 */
        REQUIRE(hn_actor_presets_apply(m,0x08012EF8)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801342C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801A1AC)==HN_SCENE_OK);
        break;
    }
    case 0x080A1D98:{ /* ConfigureMapBank6PresetA1D98 */
        REQUIRE(raw_animation(m,hn_read8(m,HN_RENDER_CLASS_LEADER), 0x088AF645));
        REQUIRE(raw_animation(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER), 0x088B6AF9));
        break;
    }
    case 0x080A1DC4:{ /* ConfigureMapBank6PresetA1DC4 */
        REQUIRE(hn_actor_presets_apply(m,0x080134EC)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08019584)==HN_SCENE_OK);
        break;
    }
    case 0x080A1DD4:{ /* ConfigureMapBank6PresetA1DD4 */

        break;
    }
    case 0x080A1DD8:{ /* ConfigureMapBank6PresetA1DD8 */
        REQUIRE(animation(m,22, 0x088F3110));
        break;
    }
    case 0x080A1DEC:{ /* ConfigureMapBank6PresetA1DEC */
        if ((uint8_t)hn_render_direction_raw(m,hn_read8(m,0x03002C04), hn_read8(m,HN_RENDER_CLASS_LEADER),false) == 4)
            REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088DBE09));
        else
            REQUIRE(raw_animation(m,hn_read8(m,0x03002C04), 0x088DBA28));
        break;
    }
    case 0x080A1E2C:{ /* ConfigureMapBank6PresetA1E2C */
        REQUIRE(animation(m,22, 0x088F3128));
        break;
    }
    case 0x080A1F0C:{ /* ConfigureMapBank6PresetA1F0C */
        REQUIRE(hn_audio_song(m,77,HN_SONG_CHANGE));
        break;
    }
    case 0x080A20C4:{ /* ConfigureMapBank6PresetA20C4 */
        REQUIRE(hn_actor_presets_apply(m,0x0801889C)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018E2C)==HN_SCENE_OK);
        break;
    }
    case 0x080A20D4:{ /* ConfigureMapBank6PresetA20D4 */
        REQUIRE(hn_actor_presets_apply(m,0x080188B4)==HN_SCENE_OK);
        REQUIRE(auxiliary(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), hn_read8(m,0x03002C04), 0x088DE93D));
        break;
    }
    case 0x080A20F8:{ /* ConfigureMapBank6PresetA20F8 */
        REQUIRE(hn_actor_presets_apply(m,0x080189F8)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018E2C)==HN_SCENE_OK);
        break;
    }
    case 0x080A2108:{ /* ConfigureMapBank6PresetA2108 */
        REQUIRE(hn_actor_presets_apply(m,0x08018A58)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08019EE8)==HN_SCENE_OK);
        break;
    }
    case 0x080A2118:{ /* ConfigureMapBank6PresetA2118 */
        REQUIRE(hn_actor_presets_apply(m,0x08018A70)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801A818)==HN_SCENE_OK);
        break;
    }
    case 0x080A2128:{ /* ConfigureMapBank6PresetA2128 */
        REQUIRE(hn_actor_presets_apply(m,0x08018AA0)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801B254)==HN_SCENE_OK);
        break;
    }
    case 0x080A2138:{ /* ConfigureMapBank6PresetA2138 */
        REQUIRE(hn_actor_presets_apply(m,0x08018E2C)==HN_SCENE_OK);
        REQUIRE(animation(m,10, 0x088A6860));
        REQUIRE(animation(m,13, 0x088AADD7));
        indicator(m,10, 0x088A6860);
        break;
    }
    case 0x080A2168:{ /* ConfigureMapBank6PresetA2168 */
        REQUIRE(animation(m,16, 0x088F3128));
        break;
    }
    case 0x080A217C:{ /* ConfigureMapBank6PresetA217C */
        REQUIRE(animation(m,16, 0x088F3110));
        break;
    }
    case 0x080A2190:{ /* ConfigureMapBank6PresetA2190 */
        REQUIRE(animation(m,hn_read8(m,0x03002C04), 0x088F3130));
        indicator(m,hn_read8(m,0x03002C04), 0x088F3130);
        break;
    }
    case 0x080A21B4:{ /* ConfigureMapBank6PresetA21B4 */
        REQUIRE(animation(m,16, 0x088F3179));
        indicator(m,16, 0x088F3179);
        break;
    }
    case 0x080A2B2C:{ /* ConfigureMapBank6PresetA2B2C */
        REQUIRE(field(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 0));
        REQUIRE(field(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 0));
        REQUIRE(hn_actor_presets_apply(m,0x08015024)==HN_SCENE_OK);
        break;
    }
    case 0x080A2B5C:{ /* ConfigureMapBank6PresetA2B5C */
        REQUIRE(field(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 0));
        REQUIRE(field(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 0));
        REQUIRE(hn_actor_presets_apply(m,0x08014F0C)==HN_SCENE_OK);
        break;
    }
    case 0x080A2B8C:{ /* ConfigureMapBank6PresetA2B8C */
        REQUIRE(hn_actor_presets_apply(m,0x08013610)==HN_SCENE_OK);
        REQUIRE(auxiliary(m,12, 13, 0x088E3976));
        break;
    }
    case 0x080A2BA4:{ /* ConfigureMapBank6PresetA2BA4 */
        REQUIRE(hn_actor_presets_apply(m,0x08017CE4)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018EC8)==HN_SCENE_OK);
        break;
    }
    case 0x080A2BB4:{ /* ConfigureMapBank6PresetA2BB4 */
        REQUIRE(hn_actor_presets_apply(m,0x08017D44)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x080196D4)==HN_SCENE_OK);
        break;
    }
    case 0x080A2BC4:{ /* ConfigureMapBank6PresetA2BC4 */
        REQUIRE(hn_actor_presets_apply(m,0x080131D8)==HN_SCENE_OK);
        REQUIRE(auxiliary(m,12, 13, 0x088E34DB));
        break;
    }
    case 0x080A2BDC:{ /* ConfigureMapBank6PresetA2BDC */
        REQUIRE(hn_actor_presets_apply(m,0x08012E50)==HN_SCENE_OK);
        REQUIRE(auxiliary(m,9, 10, 0x088E34AD));
        break;
    }
    case 0x080A2BF4:{ /* ConfigureMapBank6PresetA2BF4 */
        REQUIRE(hn_actor_presets_apply(m,0x08017D8C)==HN_SCENE_OK);
        REQUIRE(auxiliary(m,12, 13, 0x088DB922));
        break;
    }
    case 0x080A3918:{ /* ConfigureMapBank6PresetA3918 */
        REQUIRE(hn_actor_presets_apply(m,0x080187F8)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08019824)==HN_SCENE_OK);
        break;
    }
    case 0x080A3928:{ /* ConfigureMapBank6PresetA3928 */
        REQUIRE(hn_actor_presets_apply(m,0x08015138)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08019728)==HN_SCENE_OK);
        break;
    }
    case 0x080A3938:{ /* ConfigureMapBank6PresetA3938 */
        REQUIRE(hn_actor_presets_apply(m,0x08017D74)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018E90)==HN_SCENE_OK);
        break;
    }
    case 0x080A3948:{ /* ConfigureMapBank6PresetA3948 */
        REQUIRE(hn_actor_presets_apply(m,0x080187F8)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x0801977C)==HN_SCENE_OK);
        break;
    }
    case 0x080A3958:{ /* ConfigureMapBank6PresetA3958 */
        REQUIRE(hn_actor_presets_apply(m,0x08015138)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x080197D0)==HN_SCENE_OK);
        break;
    }
    case 0x080A3968:{ /* ConfigureMapBank6PresetA3968 */
        REQUIRE(hn_actor_presets_apply(m,0x08012F58)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08013220)==HN_SCENE_OK);
        break;
    }
    case 0x080A3978:{ /* MapBank6ReturnZero */

        break;
    }
    case 0x080A397C:{ /* ConfigureMapBank6Slot22Animation */
        REQUIRE(animation(m,22, 0x088F4197));
        break;
    }
    case 0x080A3990:{ /* StartMapBank6Song79 */
        REQUIRE(hn_audio_song(m,79,HN_SONG_CHANGE));
        break;
    }
    case 0x080A399C:{ /* ConfigureMapBank6Slot18Indicator */
        REQUIRE(hn_render_configure_raw(m,18, 1, 114, 0, 0, 0, 0x088F42A1));
        indicator(m,18, 0x088F42A1);
        break;
    }
    case 0x080A3BDC:{ /* ConfigureMapBank6PresetA3BDC */
        REQUIRE(hn_actor_presets_apply(m,0x080150D8)==HN_SCENE_OK);
        REQUIRE(field(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 0));
        break;
    }
    case 0x080A3BFC:{ /* ConfigureMapBank6PresetA3BFC */
        REQUIRE(hn_actor_presets_apply(m,0x08015138)==HN_SCENE_OK);
        REQUIRE(auxiliary(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), hn_read8(m,0x03002C04), 0x088E3915));
        break;
    }
    case 0x080A3C20:{ /* ConfigureMapBank6PresetA3C20 */
        REQUIRE(hn_actor_presets_apply(m,0x080151E0)==HN_SCENE_OK);
        REQUIRE(field(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), 0));
        REQUIRE(field(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), 0));
        break;
    }
    case 0x080A3C50:{ /* ConfigureMapBank6PresetA3C50 */
        REQUIRE(hn_actor_presets_apply(m,0x080151F8)==HN_SCENE_OK);
        REQUIRE(auxiliary(m,(uint8_t)(hn_read8(m,0x03002C04) - 1), hn_read8(m,0x03002C04), 0x088BECA1));
        REQUIRE(auxiliary(m,(uint8_t)(hn_read8(m,0x03002C04) + 1), hn_read8(m,0x03002C04), 0x088BECBA));
        break;
    }
    case 0x080A3C8C:{ /* ConfigureMapBank6PresetA3C8C */
        REQUIRE(hn_actor_presets_apply(m,0x08015210)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018E00)==HN_SCENE_OK);
        break;
    }
    case 0x080A3C9C:{ /* ConfigureMapBank6PresetA3C9C */
        REQUIRE(hn_actor_presets_apply(m,0x08015240)==HN_SCENE_OK);
        REQUIRE(hn_actor_presets_apply(m,0x08018E00)==HN_SCENE_OK);
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank6_preset_callback(HNMemory *m,GameAddr callback,uint32_t *consumed)
{
    switch(callback&~1u){
    case 0x0809FEB8:
    case 0x080A0DA8:
    case 0x080A1074:
    case 0x080A3978:
        *consumed=0;return hn_bank6_presets(m,callback);
    default:return HN_SCENE_UNKNOWN;
    }
}
