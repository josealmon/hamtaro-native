#include "game/graphics.h"
#include "game/state.h"
#include "game/video.h"
#include "game/render.h"
#include "game/heap.h"
#include "game/raster.h"
#include "gba/timing.h"
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
    hn_graphics_init(&graphics,&memory);hn_write8(&memory,0x02020000,0xB0);
    CHECK(!hn_graphics_update(&graphics,16));CHECK(graphics.fault==HN_GRAPHICS_OPCODE);
}
static void raster_effects(void)
{
    reset();hn_heap_init(&memory);HNGraphics graphics;hn_graphics_init(&graphics,&memory);
    GameAddr state=0x02018000,data=0x02020000;uint32_t running=99;
    const unsigned sizes[]={0,8,8,12,12,260,8,12,8};
    for(unsigned kind=1;kind<=8;kind++){
        hn_write8(&memory,data,(uint8_t)kind);
        CHECK(hn_graphics_command(&graphics,state,0xA7,data,&running));
        GameAddr effect=hn_read32(&memory,HN_RASTER_EFFECT);CHECK(effect!=0&&running==1);
        CHECK(hn_read32(&memory,effect-12)==sizes[kind]);
        for(unsigned i=0;i<sizes[kind];i++)CHECK(hn_read8(&memory,effect+i)==0);
        hn_write8(&memory,effect,0xA5); /* Reallocation must clear reused storage. */
    }
    hn_write8(&memory,data,0);CHECK(!hn_graphics_command(&graphics,state,0xA7,data,&running));
    CHECK(graphics.fault==HN_GRAPHICS_RESOURCE&&hn_read32(&memory,HN_RASTER_EFFECT)==0);
    hn_graphics_init(&graphics,&memory);CHECK(!hn_graphics_command(&graphics,state,0xA2,data,&running));
    CHECK(graphics.fault==HN_GRAPHICS_RESOURCE&&!memory.fault);

    reset();hn_heap_init(&memory);hn_graphics_init(&graphics,&memory);
    const uint8_t sweep[]={0xA7,8,0xA1,1,0x3F,0,0x11,0x20,0x11,1,0xA8,40,0,0xA2,0x80};
    program(data,sweep,sizeof(sweep));CHECK(hn_graphics_configure(&memory,0,data));
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,state)==0xAE);
    CHECK(hn_read16(&memory,HN_RASTER_SCANLINE)==0x50A0);
    CHECK(hn_read32(&memory,0x040000B4)==0x04000042&&hn_read32(&memory,0x040000B8)==0x93400001);
    CHECK((hn_read8(&memory,HN_DISPLAY_SHADOW+1)&0x5F)==0x5F&&hn_read8(&memory,HN_HBLANK_SELECTOR)==0);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read16(&memory,HN_RASTER_SCANLINE)==0x28C8);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read16(&memory,HN_RASTER_SCANLINE)==0x00F0);
    CHECK(hn_read32(&memory,HN_RASTER_EFFECT)==0&&hn_read32(&memory,0x040000B0)==0);
    CHECK(hn_read32(&memory,0x040000B4)==0&&hn_read32(&memory,0x040000B8)==0);
    CHECK(hn_read8(&memory,HN_DISPLAY_SHADOW+1)==0x11&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==1);
    CHECK(hn_graphics_update(&graphics,64));CHECK(hn_read8(&memory,state)==0);

    hn_write8(&memory,data,5);CHECK(hn_graphics_command(&graphics,state,0xA7,data,&running));
    GameAddr effect=hn_read32(&memory,HN_RASTER_EFFECT);hn_write16(&memory,effect+5,2);
    CHECK(hn_raster_profile(&memory));CHECK(hn_read8(&memory,effect+17)==1);
    CHECK(hn_read8(&memory,effect+18)==2&&hn_read8(&memory,effect+19)==2);
    hn_write16(&memory,effect+5,239);hn_write8(&memory,effect+9,2);
    CHECK(hn_raster_profile(&memory)&&hn_read16(&memory,effect+5)==240);
    CHECK(hn_read8(&memory,effect+258)==0); /* Positive-radius clamp preserves the rounded allocation tail. */
    /* A negative increment uses the signed comparison before the profile is generated. */
    hn_write16(&memory,effect+5,10);hn_write8(&memory,effect+9,255);hn_write16(&memory,effect+13,0x0500);
    CHECK(hn_raster_profile(&memory)&&hn_read16(&memory,effect+5)==14);
    hn_write8(&memory,effect,1);hn_video_hblank(&memory,16);
    CHECK(hn_graphics_command(&graphics,state,0xA6,data,&running));
    CHECK(hn_read32(&memory,HN_RASTER_EFFECT)==0&&hn_read8(&memory,HN_HBLANK_SELECTOR)==0);
    CHECK((hn_read8(&memory,HN_DISPLAY_STATUS)&0xD0)==0x40&&!memory.fault);

    hn_write8(&memory,data,5);CHECK(hn_graphics_command(&graphics,state,0xA7,data,&running));
    effect=hn_read32(&memory,HN_RASTER_EFFECT);hn_write8(&memory,effect+3,120);hn_write8(&memory,effect+4,80);
    hn_write16(&memory,effect+5,20);hn_write8(&memory,effect+7,22);hn_write8(&memory,effect+9,1);
    hn_write32(&memory,0x02000040,0x0800F4FD); /* Callback identity metadata. */
    CHECK(hn_graphics_command(&graphics,state,0xA2,data,&running));
    CHECK(hn_read8(&memory,HN_HBLANK_SELECTOR)==16&&hn_read16(&memory,effect+5)==21);
    memory.io[6]=158;CHECK(hn_raster_hblank(&memory));
    CHECK(hn_read16(&memory,HN_RASTER_SCANLINE)==0&&hn_read8(&memory,effect+12)==1);
    memory.io[6]=0;hn_write8(&memory,effect+12,80);CHECK(hn_raster_hblank(&memory));
    CHECK(hn_read16(&memory,HN_RASTER_SCANLINE)==0x638D&&hn_read8(&memory,effect+12)==81);
    memory.io[6]=160;CHECK(hn_raster_hblank(&memory));
    CHECK(hn_read16(&memory,effect+5)==22&&hn_read32(&memory,HN_RASTER_EFFECT)==effect);
    CHECK(hn_raster_hblank(&memory));CHECK(hn_read32(&memory,HN_RASTER_EFFECT)==0);
    CHECK(hn_read8(&memory,HN_HBLANK_SELECTOR)==0&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==1);
    memory.io[4]=0x17;CHECK(hn_raster_hblank(&memory));CHECK(memory.io[4]==7);
    hn_write8(&memory,HN_HBLANK_SELECTOR,255);CHECK(!hn_raster_hblank(&memory));
    hn_write8(&memory,HN_HBLANK_SELECTOR,16);hn_write32(&memory,0x02000040,0x08012345);
    CHECK(!hn_raster_hblank(&memory)&&!memory.fault);
}
typedef struct { HNMemory *memory;unsigned count,lines[8]; } VcountTrace;
static void vcount_irq(void *context,uint16_t pending)
{
    VcountTrace *trace=context;CHECK(pending==4&&trace->count<8);
    trace->lines[trace->count++]=hn_read16(trace->memory,0x04000006);
    hn_write16(trace->memory,0x04000202,4);CHECK(hn_raster_vcount(trace->memory));
}
static void vcount_timing(void)
{
    reset();uint32_t consumed=99;HNClock clock;VcountTrace trace={&memory,0,{0}};
    hn_write32(&memory,HN_VCOUNT_HANDLERS+4,0x080CD4E1);hn_write32(&memory,HN_VCOUNT_HANDLERS+8,0x080CF239);
    hn_write32(&memory,HN_VIDEO_STATE,2);hn_write8(&memory,HN_VCOUNT_PHASE,0);
    CHECK(hn_raster_callback(&memory,0x080CD33D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read32(&memory,HN_VIDEO_STATE)==2&&hn_read8(&memory,HN_VCOUNT_PHASE)==0);
    CHECK(hn_read8(&memory,HN_VCOUNT_SELECTOR)==1&&hn_read16(&memory,HN_DISPLAY_STATUS)==0x6868);
    hn_video_commit(&memory);CHECK(hn_read16(&memory,0x04000050)==0x1342&&hn_read16(&memory,0x04000052)==0x0E03);
    CHECK(hn_raster_callback(&memory,0x080CD581,0,&consumed)==HN_SCENE_OK);
    hn_write16(&memory,0x04000208,1);hn_clock_init(&clock,&memory,NULL,vcount_irq,&trace);
    hn_clock_advance(&clock,&memory,104*HN_CYCLES_PER_LINE-1);CHECK(trace.count==0);
    hn_clock_advance(&clock,&memory,1);CHECK(trace.count==1&&trace.lines[0]==104);
    CHECK(hn_read16(&memory,0x04000052)==0x0A06&&hn_read8(&memory,HN_VCOUNT_PHASE)==1);
    hn_clock_advance(&clock,&memory,124*HN_CYCLES_PER_LINE);CHECK(trace.count==1&&clock.line==0);
    hn_clock_advance(&clock,&memory,104*HN_CYCLES_PER_LINE);CHECK(trace.count==2&&trace.lines[1]==104);
    CHECK(hn_read16(&memory,0x04000052)==0x040C&&hn_read8(&memory,HN_VCOUNT_PHASE)==0);
    hn_clock_advance(&clock,&memory,124*HN_CYCLES_PER_LINE);CHECK(trace.count==3&&trace.lines[2]==0);
    CHECK(hn_read16(&memory,0x04000052)==0x0A06&&hn_read8(&memory,HN_VCOUNT_PHASE)==1);
    hn_clock_advance(&clock,&memory,HN_CYCLES_PER_FRAME);
    CHECK(trace.count==5&&trace.lines[3]==104&&trace.lines[4]==0&&clock.frames==3);
    CHECK(hn_read16(&memory,0x04000050)==0x1D42&&!(hn_read16(&memory,0x04000202)&4));
    /* Rearm through the original disable/enable path; changing an already
       enabled compare value alone does not set the video pending bits. */
    CHECK(hn_raster_callback(&memory,0x080CF219,0,&consumed)==HN_SCENE_OK);hn_video_commit(&memory);
    CHECK((hn_read16(&memory,0x04000004)>>8)==104);
    hn_video_vcount(&memory,0,0);hn_video_commit(&memory);
    trace.count=0;CHECK(hn_raster_callback(&memory,0x080CF219,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_VCOUNT_PHASE)==1);hn_video_commit(&memory);
    hn_write16(&memory,0x0201DC92,65535);hn_clock_advance(&clock,&memory,HN_CYCLES_PER_FRAME);
    CHECK(trace.count==1&&trace.lines[0]==1&&hn_read16(&memory,0x0201DC92)==0);
    CHECK(hn_raster_callback(&memory,0x080CF249,0,&consumed)==HN_SCENE_OK);
    hn_clock_advance(&clock,&memory,HN_CYCLES_PER_FRAME);
    CHECK(trace.count==1&&(hn_read16(&memory,0x04000202)&4)&&hn_read16(&memory,0x0201DC92)==0);
    CHECK(hn_raster_callback(&memory,0x080CD581,0,&consumed)==HN_SCENE_OK);
    hn_clock_advance(&clock,&memory,1);CHECK(trace.count==2&&trace.lines[1]==0&&hn_read16(&memory,0x0201DC92)==1);
    hn_video_vcount(&memory,0,159);CHECK(hn_read8(&memory,HN_VCOUNT_PHASE)==1);
    CHECK(hn_raster_vcount(&memory)&&!(hn_read16(&memory,0x04000004)&32));
    hn_write8(&memory,HN_VCOUNT_SELECTOR,3);CHECK(!hn_raster_vcount(&memory));
    hn_write8(&memory,HN_VCOUNT_SELECTOR,1);hn_write32(&memory,HN_VCOUNT_HANDLERS+4,0x08012345);
    CHECK(!hn_raster_vcount(&memory)&&!memory.fault);
    hn_write32(&memory,HN_VCOUNT_HANDLERS+4,0x080005B9);CHECK(hn_raster_vcount(&memory));
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
static void scanline_windows(void)
{
    reset();uint32_t consumed=99;const GameAddr args=0x02020000;
    hn_write32(&memory,0x02000014,0x080CE0E1);hn_write8(&memory,HN_HBLANK_SELECTOR,5);
    hn_write8(&memory,HN_SCANLINE_TRANSITION,50);hn_write16(&memory,0x0400001C,123);
    CHECK(hn_raster_hblank(&memory)&&hn_read16(&memory,HN_SCANLINE_WINDOWS)==190&&hn_read8(&memory,HN_SCANLINE_PHASE)==1);
    CHECK(hn_raster_hblank(&memory)&&hn_read16(&memory,HN_SCANLINE_WINDOWS)==0x32F0&&!hn_read8(&memory,HN_SCANLINE_PHASE));
    CHECK(hn_read16(&memory,0x0400001C)==123);
    memory.io[6]=160;hn_write8(&memory,HN_SCANLINE_TRANSITION,250);hn_write8(&memory,HN_SCANLINE_TRANSITION+1,20);
    CHECK(hn_raster_hblank(&memory)&&hn_read8(&memory,HN_SCANLINE_TRANSITION)==14&&hn_read8(&memory,HN_SCANLINE_TRANSITION+1)==20);
    hn_write8(&memory,HN_SCANLINE_TRANSITION,250);hn_write8(&memory,HN_SCANLINE_TRANSITION+1,1);
    CHECK(hn_raster_hblank(&memory)&&hn_read8(&memory,HN_SCANLINE_TRANSITION)==240&&!hn_read8(&memory,HN_SCANLINE_TRANSITION+1));
    hn_write32(&memory,0x02000024,0x080CE33D);hn_write8(&memory,HN_HBLANK_SELECTOR,9);memory.io[6]=0;
    hn_write8(&memory,HN_SCANLINE_PHASE,20);hn_write8(&memory,HN_SCANLINE_PHASE+1,0);hn_write8(&memory,HN_SCANLINE_PHASE+2,5);
    hn_write16(&memory,HN_SCANLINE_WINDOWS+2,160);
    CHECK(hn_raster_hblank(&memory)&&hn_read16(&memory,HN_SCANLINE_WINDOWS)==0x8CF0&&hn_read8(&memory,HN_SCANLINE_PHASE)==25);
    memory.io[6]=160;hn_write16(&memory,HN_SCANLINE_WINDOWS+2,200);hn_write8(&memory,HN_SCANLINE_TRANSITION+1,100);
    CHECK(hn_raster_hblank(&memory)&&hn_read16(&memory,HN_SCANLINE_WINDOWS+2)==300&&hn_read16(&memory,HN_SCANLINE_WINDOWS)==240);
    CHECK(!hn_read8(&memory,HN_SCANLINE_TRANSITION+1)&&!hn_read8(&memory,HN_SCANLINE_PHASE));
    reset();hn_write32(&memory,HN_SCANLINE_PROFILE,0x02018000);hn_write8(&memory,HN_SCANLINE_TRANSITION+2,1);
    CHECK(hn_raster_scanline_profile(&memory,0xA5)&&hn_read8(&memory,0x02018000)==0xA5&&!hn_read8(&memory,0x02018001));
    hn_heap_init(&memory);hn_write8(&memory,args,5);hn_write8(&memory,HN_SCENE_CONTROLS+0x61,6);
    CHECK(hn_raster_callback(&memory,0x080CE72D,args,&consumed)==HN_SCENE_OK&&consumed==2);
    GameAddr allocated=hn_read32(&memory,HN_SCANLINE_PROFILE);
    CHECK(allocated&&allocated!=0x02018000&&!hn_read8(&memory,0x02018000)&&hn_read8(&memory,HN_SCANLINE_TRANSITION+2)==119);
    CHECK(hn_read8(&memory,HN_HBLANK_SELECTOR)==11&&hn_read32(&memory,0x040000B0)==HN_SCANLINE_WINDOWS&&hn_read32(&memory,0x040000B4)==0x04000040);
    CHECK(hn_read16(&memory,0x040000BA)==0xA340&&!hn_read8(&memory,allocated+118));
    CHECK(hn_raster_callback(&memory,0x080CE9F5,args,&consumed)==HN_SCENE_OK&&!consumed);
    CHECK(hn_read32(&memory,HN_SCANLINE_PROFILE)==allocated&&!hn_read32(&memory,0x040000B8)&&!hn_read8(&memory,HN_HBLANK_SELECTOR));
    hn_write8(&memory,HN_SCENE_CONTROLS+0x61,9);
    CHECK(hn_raster_callback(&memory,0x080CE72D,args,&consumed)==HN_SCENE_RESOURCE&&!memory.fault);
    CHECK(hn_raster_callback(&memory,0x080CE721,args,&consumed)==HN_SCENE_UNKNOWN);
    hn_write8(&memory,HN_SCANLINE_PHASE,255);hn_raster_scanline_phase_reset(&memory);CHECK(!hn_read8(&memory,HN_SCANLINE_PHASE));
    reset();hn_heap_init(&memory);hn_write8(&memory,args,5);hn_write8(&memory,HN_SCENE_CONTROLS+0x61,8);
    CHECK(hn_raster_callback(&memory,0x080CE72D,args,&consumed)==HN_SCENE_OK&&!memory.fault);
}
int main(void)
{
    rom=calloc(1,0x600000);CHECK(rom!=NULL);
    for(unsigned i=1;i<8;i++)rom[HN_PALETTE_GROUP_SIZES-HN_ROM_BASE+i]=(uint8_t)(1u<<i);
    palettes();flows();transitions();raster_effects();vcount_timing();scanline_windows();free(rom);
    printf("%u native graphics checks passed.\n",checks);return 0;
}
