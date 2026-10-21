#include "gba/memory.h"
#include "gba/bios.h"
#include "gba/video.h"
#include "gba/timing.h"
#include "game/input.h"
#include "game/random.h"
#include "game/heap.h"
#include "game/script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static int checks=0;
#define CHECK(condition) do { checks++; if(!(condition)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#condition); exit(1); } } while(0)
static HNMemory memory;
static uint8_t rom[512];
static uint32_t frame[HN_WIDTH*HN_HEIGHT];
static void reset(void) { hn_memory_init(&memory,rom,sizeof(rom)); }
static void memory_contract(void)
{
    reset();
    CHECK(sizeof(GameAddr)==4);
    hn_write32(&memory,0x02000000,0x87654321);
    CHECK(hn_read32(&memory,0x02040000)==0x87654321);
    hn_write16(&memory,0x03000004,0xFEDC);
    CHECK(hn_read16(&memory,0x03008004)==0xFEDC);
    hn_write8(&memory,0x05000003,0x42);
    CHECK(hn_read16(&memory,0x05000002)==0x4242);
    hn_write8(&memory,0x07000000,0xFF);
    CHECK(hn_read16(&memory,0x07000000)==0);
    hn_write8(&memory,0x06010000,0xFF);
    CHECK(hn_read16(&memory,0x06010000)==0);
    CHECK(!memory.sram_dirty);
    hn_write8(&memory,0x0E008001,0x71);
    CHECK(memory.sram_dirty && memory.sram[1]==0x71);
    CHECK(hn_read8(&memory,0x08000200)==0 && memory.fault);
    CHECK(memory.fault_address==0x08000200);
    reset();hn_write8(&memory,0x08000000,0xFF);
    CHECK(memory.fault && rom[0]==0);
    reset();memory.io[0x202]=0xFF;hn_write16(&memory,0x04000202,3);
    CHECK(memory.io[0x202]==0xFC);
    CHECK(!hn_memory_copy(&memory,0xFFFFFFFE,0x02000000,4));
}
static void dma_and_bios(void)
{
    reset();
    hn_write32(&memory,0x02000000,0xAABBCCDD);
    hn_write32(&memory,0x040000D4,0x02000000);
    hn_write32(&memory,0x040000D8,0x03000000);
    hn_write32(&memory,0x040000DC,0x85000004);
    for(unsigned i=0;i<4;i++) CHECK(hn_read32(&memory,0x03000000+i*4)==0xAABBCCDD);
    CHECK(!(hn_read32(&memory,0x040000DC)&0x80000000u));
    hn_write32(&memory,0x040000D4,0x02000000);
    hn_write32(&memory,0x040000D8,0x03000100);
    hn_write32(&memory,0x040000DC,0xD5000001);
    CHECK(hn_read32(&memory,0x03000100)==0);
    hn_dma_trigger(&memory,1);
    CHECK(hn_read32(&memory,0x03000100)==0xAABBCCDD);
    CHECK(hn_read16(&memory,0x04000202)&0x800);
    CHECK(hn_bios_cpu_set(&memory,0x02000000,0x03000200,0x05000003));
    CHECK(hn_read32(&memory,0x03000208)==0xAABBCCDD);
    CHECK(hn_bios_cpu_fast_set(&memory,0x02000000,0x03000300,0x01000001));
    CHECK(hn_read32(&memory,0x0300031C)==0xAABBCCDD);
    CHECK(hn_runtime_divide(-7,3)==-2);
    CHECK(hn_runtime_remainder(-7,3)==-1);
    CHECK(hn_runtime_divide(INT_MIN,-1)==INT_MIN);
    CHECK(hn_runtime_remainder(INT_MIN,-1)==0);
    CHECK(hn_runtime_divide(9,0)==0);
    /* A B C + a six-byte overlapping copy -> ABCABCABC. */
    static const uint8_t lz[]={0x10,9,0,0,0x10,'A','B','C',0x30,2};
    memcpy(rom,lz,sizeof(lz));reset();
    CHECK(hn_bios_lz77(&memory,0x08000000,0x02000000,9));
    CHECK(memcmp(memory.ewram,"ABCABCABC",9)==0);
    reset();CHECK(hn_bios_lz77(&memory,0x08000000,0x06000000,9));
    CHECK(memcmp(memory.vram,"ABCABCABC",9)==0);
    reset();CHECK(!hn_bios_lz77(&memory,0x08000000,0x02000000,8));
    rom[4]=0x80;reset();
    CHECK(!hn_bios_lz77(&memory,0x08000000,0x02000000,9));
    memset(rom,0,sizeof(rom));
}
static void video_contract(void)
{
    reset();hn_write16(&memory,0x04000000,0x80);hn_video_render(&memory,frame);
    CHECK(frame[0]==0xFFFFFFFFu && frame[HN_WIDTH*HN_HEIGHT-1]==0xFFFFFFFFu);
    /* Mode 3 identity affine transform; actual RGB15 conversion. */
    reset();hn_write16(&memory,0x04000000,0x0403);
    hn_write16(&memory,0x04000020,256);hn_write16(&memory,0x04000026,256);
    hn_write16(&memory,0x06000000,31);hn_write16(&memory,0x06000002,31<<10);
    hn_video_render(&memory,frame);
    CHECK(frame[0]==0xFF0000FFu);CHECK(frame[1]==0xFFFF0000u);
    /* Text BG0: map at screen block 1, tile 1, palette slot 1. */
    reset();hn_write16(&memory,0x04000000,0x0100);
    hn_write16(&memory,0x04000008,0x0100);
    hn_write16(&memory,0x06000800,1);
    hn_write16(&memory,0x06000020,0x1111);
    hn_write16(&memory,0x05000002,31<<5);
    hn_video_render(&memory,frame);
    CHECK(frame[0]==0xFF00FF00u && frame[3]==0xFF00FF00u);
    /* Fade toward white with coefficient 8. */
    hn_write16(&memory,0x04000050,0x0081);hn_write16(&memory,0x04000054,8);
    hn_video_render(&memory,frame);CHECK(frame[0]==0xFF7BFF7Bu);
    hn_write16(&memory,0x04000050,0x00C1);
    hn_video_render(&memory,frame);CHECK(frame[0]==0xFF008400u);
    /* Window 0 suppresses BG0 inside a non-wrapping rectangle. */
    hn_write16(&memory,0x04000050,0);
    hn_write16(&memory,0x04000000,0x2100);
    hn_write16(&memory,0x04000040,0x0002);hn_write16(&memory,0x04000044,0x0002);
    hn_write16(&memory,0x04000048,0);hn_write16(&memory,0x0400004A,0x3F);
    hn_video_render(&memory,frame);
    CHECK(frame[0]==0xFF000000u && frame[2]==0xFF00FF00u);
    /* OBJ at (0,0), all other objects disabled. */
    reset();
    for(unsigned n=0;n<128;n++)hn_write16(&memory,0x07000000+n*8,0x200);
    hn_write16(&memory,0x04000000,0x1040);
    hn_write16(&memory,0x07000000,0);
    hn_write16(&memory,0x06010000,0x1111);
    hn_write16(&memory,0x05000202,31);
    hn_video_render(&memory,frame);
    CHECK(frame[0]==0xFF0000FFu && frame[4]==0xFF000000u);
    /* Mode 4 index zero reveals a lower-priority OBJ; index one covers it. */
    reset();hn_write16(&memory,0x04000000,0x0404);
    hn_write16(&memory,0x04000020,256);hn_write16(&memory,0x04000026,256);
    hn_write16(&memory,0x05000000,31<<10);
    hn_video_render(&memory,frame);CHECK(frame[0]==0xFFFF0000u);
    for(unsigned n=0;n<128;n++)hn_write16(&memory,0x07000000+n*8,0x200);
    hn_write16(&memory,0x04000000,0x1444);hn_write16(&memory,0x07000000,0);
    hn_write16(&memory,0x07000004,512|(3<<10));hn_write16(&memory,0x06014000,0x1111);
    hn_write16(&memory,0x05000202,31);hn_write16(&memory,0x05000002,31<<5);
    hn_write16(&memory,0x06000000,1);hn_video_render(&memory,frame);
    CHECK(frame[0]==0xFF00FF00u&&frame[1]==0xFF0000FFu);
}
static void game_contract(void)
{
    HNKeys keys;hn_keys_init(&keys);
    hn_keys_step(&keys,HN_A|HN_RIGHT);
    CHECK(keys.pressed==(HN_A|HN_RIGHT) && keys.repeat==keys.pressed);
    for(int i=0;i<88;i++)hn_keys_step(&keys,HN_A|HN_RIGHT);
    CHECK(keys.repeat==0);
    hn_keys_step(&keys,HN_A|HN_RIGHT);CHECK(keys.repeat==(HN_A|HN_RIGHT));
    hn_keys_step(&keys,0);CHECK(keys.released==(HN_A|HN_RIGHT) && keys.held==0);
    HNRandom rng;hn_random_init(&rng,0);uint32_t value=hn_random_next(&rng);
    CHECK(rng.seed[0]==(0x3AC2u*171u)%30269u);
    CHECK(rng.seed[1]==(0x4DF3u*172u)%30307u && rng.seed[2]==176);
    CHECK(value==rng.seed[0]*70946u+rng.seed[1]*70857u+176u*70820u);
}
typedef struct { HNMemory *memory; unsigned vblank,hblank,vcount; } IRQCounts;
static void irq(void *context,uint16_t mask)
{
    IRQCounts *counts=context;
    if(mask&1)counts->vblank++;
    if(mask&2)counts->hblank++;
    if(mask&4)counts->vcount++;
    hn_write16(counts->memory,0x04000202,mask);
}
static void timing_contract(void)
{
    reset();HNClock clock;IRQCounts counts={&memory,0,0,0};
    hn_write16(&memory,0x04000004,0x0338);
    hn_write16(&memory,0x04000200,7);hn_write16(&memory,0x04000208,1);
    hn_clock_init(&clock,&memory,NULL,irq,&counts);
    hn_clock_advance(&clock,&memory,1007);CHECK(counts.hblank==0 && clock.line==0);
    hn_clock_advance(&clock,&memory,1);CHECK(counts.hblank==1 && (hn_read16(&memory,0x04000004)&2));
    hn_clock_advance(&clock,&memory,224);CHECK(clock.line==1 && clock.line_cycle==0);
    CHECK(hn_read16(&memory,0x04000006)==1);
    hn_write16(&memory,0x04000006,99);CHECK(hn_read16(&memory,0x04000006)==1);
    hn_write32(&memory,0x02000000,0x11223344);
    hn_write32(&memory,0x040000D4,0x02000000);hn_write32(&memory,0x040000D8,0x03000000);
    hn_write32(&memory,0x040000DC,0x95000001);
    hn_clock_advance(&clock,&memory,159*HN_CYCLES_PER_LINE);
    CHECK(clock.line==160 && counts.hblank==160 && counts.vblank==1 && counts.vcount==1);
    CHECK(hn_read16(&memory,0x04000004)&1);
    CHECK(hn_read32(&memory,0x03000000)==0x11223344);
    hn_clock_advance(&clock,&memory,67*HN_CYCLES_PER_LINE);
    CHECK(clock.line==227 && !(hn_read16(&memory,0x04000004)&1));
    hn_clock_advance(&clock,&memory,HN_CYCLES_PER_LINE);
    CHECK(clock.frames==1 && clock.cycles==HN_CYCLES_PER_FRAME && clock.line==0);
    CHECK(counts.hblank==228 && counts.vblank==1 && counts.vcount==1);
}
static void heap_contract(void)
{
    reset();hn_heap_init(&memory);
    GameAddr first=hn_heap_allocate(&memory,13,true),second=hn_heap_allocate(&memory,8,false);
    CHECK(first==0x0202280C && second==0x02022828);
    CHECK(hn_read32(&memory,first-12)==16 && hn_read32(&memory,second-12)==8);
    hn_write32(&memory,second,0xDEADBEEF);
    CHECK(hn_heap_free(&memory,first)==16);
    CHECK(hn_heap_free(&memory,first)==0);
    CHECK(hn_heap_free(&memory,second)==8);
    hn_heap_coalesce(&memory);
    CHECK(hn_read32(&memory,0x02022000)==0x1D7FC);
    CHECK(hn_read32(&memory,0x02022004)==0x02022800);
    CHECK(hn_heap_allocate(&memory,0xFFFFFFFF,false)==0);
    GameAddr large=hn_heap_allocate(&memory,0x1D7F0,true);
    CHECK(large==first);
    CHECK(hn_heap_allocate(&memory,1,false)==0);
    CHECK(!memory.fault);
}
static uint32_t test_callback(HNMemory *m,GameAddr args)
{hn_write8(m,0x02000100,hn_read8(m,args));return 1;}
static void script_contract(void)
{
    reset();HNScript script;hn_script_init(&script,&memory,NULL,0);
    hn_write8(&memory,0x02000000,0x2B);hn_write16(&memory,0x02000001,0x1234);
    hn_write8(&memory,0x02000003,2);hn_write16(&memory,0x02000004,2);
    hn_write8(&memory,0x02000006,1);hn_write32(&memory,0x03002C60,0x02000000);
    CHECK(hn_main_frame(&script,32));CHECK(hn_read16(&memory,0x03001FBC)==0x1234);
    CHECK(hn_read16(&memory,0x0300001A)==1);
    CHECK(hn_main_frame(&script,32));CHECK(hn_read16(&memory,0x0300001A)==0);
    CHECK(hn_main_frame(&script,32));CHECK(hn_read8(&memory,0x03000010)==1);
    HNMainCallback callback={0x08001001,test_callback};
    hn_script_init(&script,&memory,&callback,1);
    hn_write32(&memory,0x02000000,0x08001001);hn_write8(&memory,0x02000004,0x5A);
    CHECK(hn_main_command(&script,0x1C,0x02000000));
    CHECK(hn_read8(&memory,0x02000100)==0x5A && hn_read32(&memory,0x03000014)==5);
    hn_script_init(&script,&memory,NULL,0);
    CHECK(!hn_main_command(&script,0x1C,0x02000000));CHECK(script.fault==HN_SCRIPT_CALLBACK);
    hn_script_init(&script,&memory,NULL,0);
    CHECK(!hn_main_command(&script,0xFF,0x02000000));CHECK(script.fault==HN_SCRIPT_OPCODE);
    hn_script_init(&script,&memory,NULL,0);
    CHECK(!hn_main_command(&script,0x1F,0x02000000));CHECK(script.fault==HN_SCRIPT_STACK);
    hn_script_init(&script,&memory,NULL,0);hn_write8(&memory,0x03000010,4);
    hn_write8(&memory,0x02000006,4);
    hn_write32(&memory,0x03002C60,0x02000006);
    CHECK(!hn_main_frame(&script,3));CHECK(script.fault==HN_SCRIPT_BUDGET);
}
int main(void)
{
    memory_contract();dma_and_bios();video_contract();game_contract();timing_contract();heap_contract();script_contract();
    printf("%d native-core checks passed.\n",checks);return 0;
}
