#include "game/graphics.h"
#include "game/state.h"
#include "game/video.h"
#include "game/render.h"
#include <stdio.h>
#include <stdlib.h>
static HNMemory memory;
static uint8_t *rom;
static unsigned checks;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c);exit(1);}}while(0)
static void reset(void)
{
    hn_memory_init(&memory,rom,0x600000);hn_video_state_init(&memory);hn_mosaic_init(&memory);
    hn_write32(&memory,HN_PALETTE_OUTPUT,0x05000000);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02018000);hn_write8(&memory,HN_GRAPHICS_COUNT,2);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,4);
}
static void program(GameAddr address,const uint8_t *data,unsigned size)
{for(unsigned i=0;i<size;i++)hn_write8(&memory,address+i,data[i]);}
static void palettes(void)
{
    reset();hn_write16(&memory,HN_PALETTE_SOURCE,31);hn_write16(&memory,0x05000000,31<<10);
    CHECK(hn_palette_prepare(&memory,1,0,1));
    CHECK(hn_read16(&memory,HN_PALETTE_TRANSITIONS)==1);
    CHECK((int16_t)hn_read16(&memory,HN_PALETTE_TRANSITIONS+6)==-3968);
    CHECK(hn_palette_update(&memory,0,1));CHECK(hn_read16(&memory,0x05000000)==(15|(15<<10)));
    CHECK(hn_palette_update(&memory,0,1));CHECK(hn_read16(&memory,0x05000000)==31);
    CHECK(hn_palette_prepare(&memory,1,0,1));CHECK(hn_read16(&memory,HN_PALETTE_TRANSITIONS)==0);
    /* Source wraps at 256; output and transition entries keep the full index. */
    hn_write16(&memory,0x05000200,0);CHECK(hn_palette_prepare(&memory,0,256,1));
    CHECK(hn_palette_update(&memory,256,1));CHECK(hn_read16(&memory,0x05000200)==31);
    CHECK(!hn_palette_prepare(&memory,1,0,0));CHECK(!hn_palette_update(&memory,512,1));
    CHECK(!hn_palette_update(&memory,511,2));CHECK(!memory.fault);
}
static void flows(void)
{
    reset();HNGraphics graphics;hn_graphics_init(&graphics,&memory);
    CHECK(!hn_graphics_configure(&memory,2,0x02020000));
    const uint8_t repeat[]={0x8A,2,0x83,1,1,0,0x50,2,2,0x55,0x8B,0x80};
    program(0x02020000,repeat,sizeof(repeat));CHECK(hn_graphics_configure(&memory,0,0x02020000));
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,0x02025000)==0x55);
    CHECK(hn_read8(&memory,0x02018003)==2);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,0x02018003)==1);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,0x02018000)==0);
    const uint8_t call[]={0xAF,0,1,2,2,0x80};
    const uint8_t subroutine[]={0x83,0,1,0,0x50,2,2,0x66,0x80};
    program(0x02020000,call,sizeof(call));program(0x02020100,subroutine,sizeof(subroutine));
    CHECK(hn_graphics_configure(&memory,0,0x02020000));CHECK(hn_graphics_update(&graphics,64));
    CHECK(hn_read8(&memory,0x02025000)==0x66&&hn_read8(&memory,0x02018000)==0);
    /* Type FF is paused; zero delay does not imply that the state is active. */
    hn_write8(&memory,0x02018000,255);hn_write32(&memory,0x02018018,0x01000000);
    CHECK(hn_graphics_update(&graphics,64));CHECK(!memory.fault);
    CHECK(hn_graphics_configure(&memory,0,0x02020000));
    hn_write8(&memory,0x02020000,0x82);hn_write32(&memory,0x02020001,0x02020000);
    CHECK(!hn_graphics_update(&graphics,16));CHECK(graphics.fault==HN_GRAPHICS_BUDGET);
    hn_graphics_init(&graphics,&memory);hn_write8(&memory,0x02020000,0xA2);
    CHECK(!hn_graphics_update(&graphics,16));CHECK(graphics.fault==HN_GRAPHICS_OPCODE);
}
static void transitions(void)
{
    reset();HNGraphics graphics;hn_graphics_init(&graphics,&memory);
    const uint8_t alpha[]={0x96,0,1,2,2,16,0,0,16,0x80};
    program(0x02020000,alpha,sizeof(alpha));CHECK(hn_graphics_configure(&memory,0,0x02020000));
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,HN_BLEND+2)==8&&hn_read8(&memory,HN_BLEND+3)==8);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,HN_BLEND+2)==0&&hn_read8(&memory,HN_BLEND+3)==16);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,0x02018000)==0);
    const uint8_t mosaic[]={0x9E,0x9F,0,2,2,12,8,0,0,0x80};
    program(0x02020000,mosaic,sizeof(mosaic));CHECK(hn_graphics_configure(&memory,0,0x02020000));
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read16(&memory,HN_MOSAIC)==0);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read16(&memory,HN_MOSAIC)==0x4646);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read16(&memory,HN_MOSAIC)==0);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,0x02018000)==0);
    hn_mosaic_sizes(&memory,2,32,64);CHECK(hn_read32(&memory,HN_MOSAIC+4)==0x40204020);
    /* Raw palette frames decode to the private bus, then yield for their delay. */
    const uint8_t palette_frame[]={2,0,0,0,0x20,2,2,0x80};
    const uint8_t compressed[]={0x10,4,0,0,0,31,0,0xE0,3};
    program(0x02020000,palette_frame,sizeof(palette_frame));program(0x02022000,compressed,sizeof(compressed));
    CHECK(hn_graphics_configure(&memory,0,0x02020000));CHECK(hn_graphics_update(&graphics,64));
    CHECK(hn_read16(&memory,0x05000000)==31&&hn_read16(&memory,0x05000002)==0x03E0);
    CHECK(hn_read16(&memory,0x0201801C)==2);CHECK(hn_read32(&memory,0x02018018)==0x02020007);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read16(&memory,0x0201801C)==1);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_graphics_update(&graphics,64));
    CHECK(hn_read8(&memory,0x02018000)==0&& !memory.fault);
}
int main(void)
{
    rom=calloc(1,0x600000);CHECK(rom!=NULL);
    for(unsigned i=1;i<8;i++)rom[HN_PALETTE_GROUP_SIZES-HN_ROM_BASE+i]=(uint8_t)(1u<<i);
    palettes();flows();transitions();free(rom);
    printf("%u native graphics checks passed.\n",checks);return 0;
}
