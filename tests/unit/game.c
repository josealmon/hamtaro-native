#include "game/world.h"
#include "game/video.h"
#include "game/transfer.h"
#include "game/expression.h"
#include "game/script.h"
#include "game/state.h"
#include "game/heap.h"
#include "game/save.h"
#include "game/input.h"
#include "game/scene.h"
#include "game/presets.h"
#include "game/map_choice.h"
#include "game/particles.h"
#include "game/random.h"
#include "game/text.h"
#include "game/render.h"
#include "game/actor.h"
#include "game/motion.h"
#include "game/task.h"
#include "game/choice.h"
#include "game/collision.h"
#include "game/runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static HNMemory memory;
static unsigned checks;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c);exit(1);}}while(0)
static void reset(void){hn_memory_init(&memory,NULL,0);}
static void dma_contract(void)
{
    reset();
    for(unsigned i=0;i<8;i++)hn_write32(&memory,0x02000000+i*4,0x11000000+i);
    hn_write32(&memory,0x040000D4,0x02000000);hn_write32(&memory,0x040000D8,0x03001000);
    /* Repeating VBlank DMA, two words, destination reload. */
    hn_write32(&memory,0x040000DC,0x96600002);
    CHECK(hn_read32(&memory,0x03001000)==0);
    hn_dma_trigger(&memory,1);
    CHECK(hn_read32(&memory,0x03001000)==0x11000000);
    CHECK(hn_read32(&memory,0x03001004)==0x11000001);
    hn_dma_trigger(&memory,1);
    CHECK(hn_read32(&memory,0x03001000)==0x11000002);
    CHECK(hn_read32(&memory,0x03001004)==0x11000003);
    CHECK(hn_read32(&memory,0x040000D4)==0x02000000);
    CHECK(hn_read32(&memory,0x040000D8)==0x03001000);
    CHECK(hn_read32(&memory,0x040000DC)&0x80000000u);
    hn_write16(&memory,0x040000DE,0);
    hn_dma_trigger(&memory,1);
    CHECK(hn_read32(&memory,0x03001000)==0x11000002);
    /* Byte programming the enable register must also start immediate DMA. */
    hn_write32(&memory,0x040000D4,0x02000000);hn_write32(&memory,0x040000D8,0x03001100);
    hn_write16(&memory,0x040000DC,2);hn_write8(&memory,0x040000DF,0x84);
    CHECK(hn_read32(&memory,0x03001104)==0x11000001);
    CHECK(!(hn_read32(&memory,0x040000DC)&0x80000000u));
    memory.io[4]=7;memory.io[6]=160;memory.io[0x202]=255;
    hn_write8(&memory,0x04000004,8);hn_write8(&memory,0x04000006,0);
    hn_write8(&memory,0x04000202,0x81);hn_write16(&memory,0x04000000,15);
    CHECK(hn_read8(&memory,0x04000004)==15);
    CHECK(hn_read8(&memory,0x04000006)==160);
    CHECK(hn_read8(&memory,0x04000202)==0x7E);
    CHECK(hn_read16(&memory,0x04000000)==7);
}
static void transfer_contract(void)
{
    reset();memset(memory.iwram+0x840,0xA5,0x880);hn_transfer_reset(&memory);
    CHECK(hn_read32(&memory,HN_TRANSFER_A)==0);
    CHECK(hn_read32(&memory,HN_TRANSFER_A+255*4)==0xA5A5A5A5);
    CHECK(hn_read32(&memory,HN_TRANSFER_B+255*4)==0xA5A5A5A5);
    hn_transfer_queue(&memory,0x02000000,0x05000000,0);
    CHECK(hn_read8(&memory,HN_TRANSFER_WRITE)==0);
    for(unsigned i=0;i<86;i++)hn_transfer_queue(&memory,0x02000000,0x05000000,4);
    CHECK(hn_read8(&memory,HN_TRANSFER_WRITE)==3);
    hn_transfer_reset(&memory);
    hn_write16(&memory,0x02000000,0x1234);hn_write16(&memory,0x02000002,0x5678);
    hn_transfer_queue(&memory,0x02000000,0x05000000,4);
    hn_transfer_queue_fill(&memory,0xABCD9876,0x06004000,16);
    memory.io[6]=220;hn_transfer_flush(&memory);
    CHECK(hn_read16(&memory,0x05000000)==0);
    CHECK(hn_read8(&memory,HN_TRANSFER_READ)==0);
    memory.io[6]=160;hn_transfer_flush(&memory);
    CHECK(hn_read16(&memory,0x05000000)==0x1234);
    CHECK(hn_read16(&memory,0x05000002)==0x5678);
    CHECK(hn_read32(&memory,0x06004000)==0xABCD9876);
    CHECK(hn_read32(&memory,0x0600400C)==0xABCD9876);
    CHECK(hn_read8(&memory,HN_TRANSFER_READ)==hn_read8(&memory,HN_TRANSFER_WRITE));
    CHECK(!memory.fault);
}
static void video_contract(void)
{
    reset();hn_video_state_init(&memory);hn_mosaic_init(&memory);
    hn_video_blend_targets(&memory,63,0);hn_video_fade_phase(&memory,0);
    hn_video_commit(&memory);
    CHECK(hn_read16(&memory,0x04000000)==0x1180);
    CHECK(hn_read16(&memory,0x04000004)==8);
    CHECK(hn_read16(&memory,0x04000050)==0xFF);
    CHECK(hn_read16(&memory,0x04000054)==16);
    CHECK((hn_read32(&memory,HN_VIDEO_STATE)&63)==0);
    hn_video_forced_blank(&memory,0);
    CHECK(hn_read16(&memory,0x04000000)==0x1100);
    hn_video_set_mode(&memory,4);hn_video_set_layers(&memory,0,1);hn_video_set_layers(&memory,1,4);
    hn_video_commit(&memory);CHECK(hn_read16(&memory,0x04000000)==0x1404);
    hn_video_fade_phase(&memory,128);hn_video_commit(&memory);
    CHECK((hn_read16(&memory,0x04000050)&192)==0);
    CHECK(hn_read16(&memory,0x04000054)==0);
    hn_write8(&memory,HN_MOSAIC+3,31);hn_write8(&memory,HN_MOSAIC+4,0xA0);
    hn_mosaic_update(&memory);CHECK(hn_read16(&memory,HN_MOSAIC)==0xAAAA);
    hn_write8(&memory,HN_MOSAIC+2,1);hn_write8(&memory,HN_MOSAIC+4,0);
    hn_mosaic_update(&memory);CHECK(hn_read16(&memory,HN_MOSAIC)==0xAAAA);
}
static void put32(uint8_t *p,uint32_t v)
{for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static void scene_contract(void)
{
    uint8_t *rom=calloc(1,0x700000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x700000);
    for(unsigned i=0;i<16;i++){rom[HN_KEY_DIRECTION_LOW-HN_ROM_BASE+i]=(uint8_t)i;
        rom[HN_KEY_DIRECTION_HIGH-HN_ROM_BASE+i]=(uint8_t)(16-i);}
    hn_write16(&memory,HN_KEY_STATE,0xFACE);hn_write16(&memory,HN_KEY_STATE+10,0xCAFE);
    hn_write16(&memory,HN_KEY_STATE+22,0xBEEF);hn_input_initialize(&memory);
    CHECK(hn_read16(&memory,HN_KEY_STATE)==0xFACE&&hn_read16(&memory,HN_KEY_STATE+12)==90);
    CHECK(hn_input_update(&memory,0xF090));CHECK(hn_input_query(&memory,0x3FF,0)==0x90);
    CHECK(hn_input_query(&memory,0x3FF,1)==0x90&&hn_read16(&memory,HN_KEY_STATE+8)==0x0709);
    CHECK(hn_read16(&memory,0x04000130)==0x36F);CHECK(hn_input_update(&memory,0));
    CHECK(hn_input_query(&memory,0xFFFF,3)==0x90&&hn_read16(&memory,HN_KEY_STATE+8)==0x0709);
    hn_input_repeat(&memory,16,8);CHECK(hn_input_update(&memory,0x90));
    for(unsigned i=0;i<15;i++)CHECK(hn_input_update(&memory,0x90));
    CHECK(hn_input_query(&memory,0x3FF,2)==0x90&&hn_read16(&memory,HN_KEY_STATE+16)==8);
    hn_write16(&memory,HN_KEY_STATE+18,0xFFFF);CHECK(hn_input_update(&memory,0x90));
    CHECK(hn_read16(&memory,HN_KEY_STATE+18)==0&&hn_input_query(&memory,0x3FF,4)==0);
    CHECK(hn_read16(&memory,HN_KEY_STATE+10)==0xCAFE&&hn_read16(&memory,HN_KEY_STATE+22)==0xBEEF);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+0x10,32);
    hn_write32(&memory,HN_RENDER_STATES,0x02018000);hn_write8(&memory,HN_BACKGROUND_COUNT,9);
    for(unsigned i=0;i<15;i++)put32(rom+HN_SCENE_RECORD_MENU-HN_ROM_BASE+i*4,HN_ROM_BASE+0x100+i*4);
    for(unsigned i=0;i<5;i++)put32(rom+HN_SCENE_RECORD_LABELS-HN_ROM_BASE+i*4,HN_ROM_BASE+0x200+i*4);
    for(unsigned i=0;i<10;i++)put32(rom+HN_SCENE_RECORD_MODES-HN_ROM_BASE+i*4,HN_ROM_BASE+0x300+i*4);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1E,2);hn_write8(&memory,HN_SCENE_CONTROLS+0xEC,0);
    uint32_t consumed=99;CHECK(hn_scene_callback(&memory,0x080C019D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read32(&memory,0x02018000+2*80+0x14)==HN_ROM_BASE+0x11C);
    CHECK(hn_read32(&memory,0x02018000+5*80+0x14)==HN_ROM_BASE+0x208);
    CHECK(hn_read32(&memory,0x02018000+6*80+0x14)==HN_ROM_BASE+0x31C);
    GameAddr cursor=0x02010000+27*104;CHECK(hn_read32(&memory,cursor+0x1C)==0x588000);
    CHECK(hn_read32(&memory,cursor+4)==HN_SCENE_RECORD_CURSOR&&(hn_read8(&memory,cursor+0x61)&12)==0);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1E,5);CHECK(hn_scene_callback(&memory,0x080C019D,0,&consumed)==HN_SCENE_RESOURCE);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1E,0);hn_write16(&memory,HN_KEY_STATE+2,0x40);
    CHECK(hn_scene_callback(&memory,0x080C0261,0,&consumed)==HN_SCENE_OK);
    hn_write16(&memory,HN_KEY_STATE+2,0x80);CHECK(hn_scene_callback(&memory,0x080C0261,0,&consumed)==HN_SCENE_RESOURCE);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x1E)==0); /* Missing driver does not partially move selection. */
    hn_write16(&memory,0x03001F64,0x3FF);hn_write8(&memory,HN_SCENE_CONTROLS+0x1A1,0);
    hn_write16(&memory,HN_KEY_STATE+2,0);hn_write16(&memory,HN_KEY_STATE+4,0x40);
    hn_write16(&memory,0x03002180,0x0035);CHECK(hn_scene_callback(&memory,0x080C0335,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x1A1)==2&&hn_read16(&memory,0x03001FBC)==1);
    CHECK(hn_read16(&memory,0x0201DC40)==0x40&&(hn_read16(&memory,0x03002180)&63)==0x35);
    hn_write16(&memory,HN_KEY_STATE+2,3);CHECK(hn_scene_callback(&memory,0x080C0335,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,0x03001FBC)==4&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A1)==2);
    hn_write16(&memory,HN_KEY_STATE+2,2);CHECK(hn_scene_callback(&memory,0x080C0335,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,0x03001FBC)==5&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A1)==3);
    /* Ordinals count enabled buttons from L through A, independently of mode. */
    hn_write16(&memory,0x02000000,0x8221);hn_write16(&memory,HN_KEY_STATE,0x201);
    hn_write16(&memory,HN_KEY_STATE+2,0x21);hn_write16(&memory,HN_KEY_STATE+4,1);
    const unsigned ordinal_callbacks[]={0x0801C865,0x0801C889,0x0801C8AD,0x0801C8D1};
    const unsigned ordinal_results[]={1,2,3,3};
    for(unsigned i=0;i<4;i++){
        CHECK(hn_scene_callback(&memory,ordinal_callbacks[i],0x02000000,&consumed)==HN_SCENE_OK);
        CHECK(consumed==2&&hn_read16(&memory,0x03001FBC)==ordinal_results[i]);
    }
    hn_write16(&memory,HN_KEY_STATE,0x80);hn_write16(&memory,HN_KEY_STATE+2,1);
    hn_write16(&memory,0x02000000,0x81);
    CHECK(hn_scene_callback(&memory,0x0801C8D1,0x02000000,&consumed)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,0x03001FBC)==1); /* Held direction precedes pressed A. */
    hn_write16(&memory,0x02000000,0x8000);
    CHECK(hn_scene_callback(&memory,0x0801C889,0x02000000,&consumed)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,0x03001FBC)==0);
    memset(memory.iwram+(HN_BACKGROUND_BOUNDS-0x03000000),0xA5,48);
    CHECK(hn_scene_callback(&memory,0x080CB151,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read16(&memory,HN_BACKGROUND_BOUNDS+26)==1024&&hn_read16(&memory,HN_BACKGROUND_BOUNDS+30)==1024);
    CHECK(hn_read16(&memory,HN_BACKGROUND_BOUNDS+24)==0xA5A5&&hn_read16(&memory,HN_BACKGROUND_BOUNDS+28)==0xA5A5);
    CHECK(hn_read8(&memory,HN_BACKGROUND_BOUNDS+32)==0xA5&&hn_read8(&memory,HN_BACKGROUND_BOUNDS+38)==0xA5);
    hn_write8(&memory,0x02000100,1);hn_write32(&memory,0x02000101,0x02000200);
    hn_write8(&memory,0x02000200,255);hn_write8(&memory,0x03002182,0xE0);
    CHECK(hn_scene_callback(&memory,0x0801C2D5,0x02000100,&consumed)==HN_SCENE_OK);
    CHECK(consumed==5&&hn_read8(&memory,0x02000200)==0&&hn_read8(&memory,0x03002182)==0xE6);
    hn_write8(&memory,0x02000200,5);CHECK(hn_scene_callback(&memory,0x0801C345,0x02000100,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,0x02000200)==4&&hn_read8(&memory,0x03002182)==0xE4); /* Original byte-subtract flag rule. */
    hn_write16(&memory,0x02000100,65535);hn_write32(&memory,0x02000102,0x0300113A);hn_write32(&memory,0x03001134,99);
    CHECK(hn_scene_callback(&memory,0x0801C3B5,0x02000100,&consumed)==HN_SCENE_OK);
    CHECK(consumed==6&&hn_read16(&memory,0x0300113A)==65534&&hn_read32(&memory,0x03001134)==99);
    CHECK(hn_read8(&memory,0x03002182)==0xE0); /* Step and target alias: compare the updated step. */
    hn_write8(&memory,0x02000100,2);hn_write32(&memory,0x02000101,0x03002182);hn_write8(&memory,0x03002182,254);
    CHECK(hn_scene_callback(&memory,0x0801C2D5,0x02000100,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,0x03002182)==2); /* The zero flag changes the value read by the carry test. */
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,0);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,1);
    hn_write8(&memory,hn_render_slot(&memory,0)+0x60,0xA3);hn_write8(&memory,hn_render_slot(&memory,1)+0x60,0xB4);
    CHECK(hn_scene_callback(&memory,0x0801C4A5,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS)==3&&hn_read8(&memory,HN_SCENE_CONTROLS+1)==4);
    hn_write8(&memory,0x02000100,255);hn_write8(&memory,0x02000101,5);
    CHECK(hn_scene_callback(&memory,0x0801C475,0x02000100,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_read8(&memory,HN_RENDER_CLASS_LEADER)==0&&hn_read8(&memory,HN_RENDER_CLASS_FOLLOWER)==5);
    CHECK(hn_scene_callback(&memory,0x08001001,0,&consumed)==HN_SCENE_UNKNOWN);CHECK(!memory.fault);free(rom);
}
static unsigned text_pixel(GameAddr shadow,unsigned width,unsigned x,unsigned y)
{
    GameAddr byte=shadow+(y/8*width+x/8)*32+(y%8)*4+(x%8)/2;
    return (hn_read8(&memory,byte)>>((x&1)*4))&15;
}
static void text_contract(void)
{
    reset();hn_heap_init(&memory);GameAddr glyph=0x02001000,shadow=0x02010000;
    for(unsigned y=0;y<16;y++)for(unsigned group=0;group<4;group++){
        unsigned packed=0;for(unsigned x=0;x<4;x++)packed|=((x+y)&3)<<(6-x*2);
        hn_write8(&memory,glyph+y*4+group,(uint8_t)packed);
    }
    const unsigned offsets[]={0,1,2,7,8,54,55,56,255,256,257};
    const unsigned widths[]={0,1,7,256,257};
    for(unsigned w=0;w<sizeof(widths)/sizeof(*widths);w++)for(unsigned o=0;o<sizeof(offsets)/sizeof(*offsets);o++){
        memset(memory.ewram+0x10000,0x55,2048);
        CHECK(hn_text_blit_glyph(&memory,glyph,shadow,widths[w],offsets[o],6));
        unsigned width=widths[w]&255,origin=offsets[o]&255;
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<width*8;x++){
            unsigned expected=5;
            if(x>=origin&&x<origin+16-(origin&1))expected=8+((x-origin+y)&3);
            CHECK(text_pixel(shadow,width,x,y)==expected);
        }
        CHECK(hn_read32(&memory,shadow+width*64)==0x55555555);
    }
    CHECK(!hn_text_blit_glyph(&memory,glyph,0,7,0,0));CHECK(!memory.fault);
    reset();hn_heap_init(&memory);CHECK(hn_heap_allocate(&memory,0x1D7F0,false)!=0);
    CHECK(!hn_text_blit_glyph(&memory,glyph,shadow,7,0,0)&&!memory.fault);

    /* Generated fixtures only: defaults, font and widths are not game resources. */
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x900000);hn_heap_init(&memory);
    for(unsigned i=0;i<7;i++){
        rom[HN_TEXT_DEFAULT_FIRST-HN_ROM_BASE+i]=(uint8_t)(i+2);
        rom[HN_TEXT_DEFAULT_SECOND-HN_ROM_BASE+i]=(uint8_t)(i+10);
        rom[HN_TEXT_DEFAULT_SPANISH-HN_ROM_BASE+i]=(uint8_t)(i+20);
    }
    memset(rom+HN_TEXT_GLYPHS-HN_ROM_BASE,0x1B,255*64);
    memset(rom+HN_TEXT_ADVANCES-HN_ROM_BASE,1,255);
    uint32_t consumed=99;hn_write8(&memory,HN_SCENE_CONTROLS+0xEB,4);memset(memory.vram,0x55,sizeof(memory.vram));
    CHECK(hn_scene_callback(&memory,0x080C0C5D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(!memcmp(memory.iwram+(HN_TEXT_FIRST-0x03000000),rom+HN_TEXT_DEFAULT_FIRST-HN_ROM_BASE,9));
    CHECK(!memcmp(memory.iwram+(HN_TEXT_SECOND-0x03000000),rom+HN_TEXT_DEFAULT_SPANISH-HN_ROM_BASE,9));
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x1A2)==0);
    CHECK(hn_read8(&memory,0x06004020)==0xDC&&hn_read8(&memory,0x060041E0)==0xDC);
    CHECK(hn_read8(&memory,0x0600401F)==0x55&&hn_read8(&memory,0x060043A0)==0x55);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A2,1);
    CHECK(hn_scene_text_character(&memory,0,42)==42&&hn_scene_text_character(&memory,0,255)==42);
    CHECK(hn_read8(&memory,HN_TEXT_SECOND)==20);
    CHECK(hn_scene_callback(&memory,0x080C0C91,1,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_TEXT_FIRST)==2&&hn_read8(&memory,HN_TEXT_SECOND)==20);
    memset(memory.iwram+(HN_TEXT_FIRST-0x03000000),1,7);
    CHECK(hn_scene_callback(&memory,0x080C0955,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_TEXT_FIRST)==2&&hn_read16(&memory,0x03001FBC)==1);
    memset(memory.iwram+(HN_TEXT_FIRST-0x03000000),1,7);hn_write8(&memory,HN_TEXT_FIRST+1,42);
    CHECK(hn_scene_callback(&memory,0x080C0955,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_TEXT_FIRST)==42&&hn_read8(&memory,HN_TEXT_FIRST+6)==1);
    CHECK(hn_scene_callback(&memory,0x080C0CF1,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_TEXT_FIRST+1)==0&&hn_read8(&memory,HN_TEXT_FIRST+6)==0);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A3,77);hn_write8(&memory,HN_SCENE_CONTROLS+0x1A5,0);
    hn_write8(&memory,HN_RENDER_INFO+0x10,36);hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);
    CHECK(hn_scene_callback(&memory,0x080C09F1,0,&consumed)==HN_SCENE_RESOURCE);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A3,0);hn_write8(&memory,HN_SCENE_CONTROLS+0x1A5,3);
    CHECK(hn_scene_callback(&memory,0x080C09F1,0,&consumed)==HN_SCENE_RESOURCE);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A5,0);hn_write16(&memory,0x0201DC40,0x10);
    memset(rom+HN_TEXT_GRID-HN_ROM_BASE,255,77);
    CHECK(hn_scene_callback(&memory,0x080C0561,0,&consumed)==HN_SCENE_RESOURCE); /* No endless search in a corrupt grid. */
    hn_heap_coalesce(&memory);CHECK(hn_heap_allocate(&memory,0x1D7F0,false)!=0);CHECK(!memory.fault);
    hn_memory_init(&memory,rom,0x900000);hn_heap_init(&memory);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A2,1);hn_write8(&memory,HN_TEXT_FIRST,1);
    CHECK(hn_heap_allocate(&memory,0x1D7FC-12-(0x280+12)-64,false)!=0);
    CHECK(!hn_scene_text_render(&memory)&&!memory.fault);
    CHECK(hn_heap_allocate(&memory,0x280,false)!=0); /* Failed glyph allocation releases the name shadow. */
    free(rom);
}
static void panel_contract(void)
{
    uint8_t *rom=calloc(1,0x700000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x700000);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+0x10,3);
    rom[HN_PANEL_SHIFTS-HN_ROM_BASE]=3;
    rom[HN_PANEL_SHIFTS-HN_ROM_BASE+1]=2;rom[HN_PANEL_SHIFTS-HN_ROM_BASE+2]=255;
    rom[HN_PANEL_PIXELS-HN_ROM_BASE+1]=10;rom[HN_PANEL_PIXELS-HN_ROM_BASE+2]=20;
    put32(rom+HN_PANEL_ANIMATIONS-HN_ROM_BASE,HN_ROM_BASE+0x100);
    uint32_t consumed=99,yield=99;
    CHECK(hn_scene_callback(&memory,0x080C0E3D,0x02000000,&consumed)==HN_SCENE_OK&&consumed==1);
    GameAddr first=hn_render_slot(&memory,0),second=hn_render_slot(&memory,1);
    CHECK(hn_read32(&memory,first+0x18)==0x1A8000&&hn_read32(&memory,second+0x18)==0x0C8000);
    CHECK(hn_read32(&memory,first+0x24)==0xFFF80000&&hn_read32(&memory,second+0x24)==0x80000);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x1A6)==2&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A7)==1);
    for(unsigned i=0;i<3;i++)CHECK(hn_scene_callback(&memory,0x080C10B1,0x02000000,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x1A6)==255&&hn_read32(&memory,first+0x24)==0);
    CHECK(hn_scene_callback(&memory,0x080C0FD1,0x02000000,&consumed)==HN_SCENE_OK);
    for(unsigned i=0;i<8&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A6)!=255;i++)
        CHECK(hn_scene_callback(&memory,0x080C1151,0x02000000,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,first+0x60)==0&&hn_read32(&memory,first+4)==0);
    CHECK(hn_read8(&memory,second+0x60)==0&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A6)==255);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1B7,1);rom[HN_PANEL_ORDER-HN_ROM_BASE]=1;
    rom[HN_PANEL_ORDER-HN_ROM_BASE+1]=1;hn_write32(&memory,first+0x20,0x49000);
    CHECK(hn_scene_animation_callback(&memory,0x080C12E5,0,0,&consumed,&yield)==HN_SCENE_OK);
    CHECK(consumed==0&&yield==1&&hn_read32(&memory,second+0x20)==0x40000);
    CHECK(hn_read32(&memory,second+0x2C)==0xFFFF8000&&hn_read32(&memory,second+0x38)==0x1000);
    CHECK(hn_read32(&memory,first+0x20)==0x48000&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1B7)==127);
    hn_write8(&memory,0x02000000,4);
    CHECK(hn_scene_callback(&memory,0x080C0E3D,0x02000000,&consumed)==HN_SCENE_RESOURCE);
    hn_write8(&memory,0x02000000,0);hn_write8(&memory,HN_SCENE_CONTROLS+0xEB,5);
    CHECK(hn_scene_callback(&memory,0x080C0E3D,0x02000000,&consumed)==HN_SCENE_RESOURCE);
    hn_write8(&memory,HN_SCENE_CONTROLS+0xEB,0);rom[HN_PANEL_SHIFTS-HN_ROM_BASE]=4;
    CHECK(hn_scene_callback(&memory,0x080C0E3D,0x02000000,&consumed)==HN_SCENE_RESOURCE);
    CHECK(!memory.fault);free(rom);
}
static void actor_contract(void)
{
    reset();memset(memory.iwram+0x50,0xA5,17*84);
    for(unsigned i=0;i<17;i++)hn_write32(&memory,HN_ACTOR_CONTEXTS+i*84,0);
    CHECK(hn_actor_configure(&memory,0,0x02020000));CHECK(hn_read8(&memory,HN_ACTOR_CONTEXTS+4)==0xA5);
    CHECK(!hn_actor_configure(&memory,17,0x02020000));
    CHECK(hn_actor_configure(&memory,1,0x02020000)&&hn_actor_configure(&memory,16,0x02020100));
    const uint8_t first[]={0x2B,0xEF,0xBE,0x2D,0,1,0x5A,2,2,0,0x27};
    const uint8_t last[]={0x2D,1,1,0xCA,0x27};
    memcpy(memory.ewram+0x20000,first,sizeof(first));memcpy(memory.ewram+0x20100,last,sizeof(last));
    memset(memory.iwram+0x10,0x77,16);memset(memory.iwram+0x2C10,0xA6,64);
    hn_write32(&memory,0x03002C60,0x02021000);hn_write8(&memory,0x03002BF0,15);
    hn_write16(&memory,0x03001FBC,0xFACE);
    uint8_t main_state[16],returns[64];memcpy(main_state,memory.iwram+0x10,16);memcpy(returns,memory.iwram+0x2C10,64);
    HNScript script;hn_script_init(&script,&memory,NULL,0);
    CHECK(hn_actor_frames(&script,64));
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x100)==0x5A&&hn_read8(&memory,HN_SCENE_CONTROLS+0x101)==0xCA);
    CHECK(hn_read16(&memory,HN_ACTOR_CONTEXTS+84+0x4A)==0xBEEF);
    CHECK(hn_read16(&memory,HN_ACTOR_CONTEXTS+84+0x48)==1&&hn_read32(&memory,HN_ACTOR_CONTEXTS+16*84)==0);
    CHECK(hn_read8(&memory,HN_ACTOR_CONTEXTS+84+0x51)==0xA5&&hn_read8(&memory,HN_ACTOR_CONTEXTS+0x51)==0xA5);
    for(unsigned i=0;i<3;i++){
        CHECK(!memcmp(main_state,memory.iwram+0x10,16)&&!memcmp(returns,memory.iwram+0x2C10,64));
        CHECK(hn_read32(&memory,0x03002C60)==0x02021000&&hn_read8(&memory,0x03002BF0)==15);
        CHECK(hn_read16(&memory,0x03001FBC)==0xFACE);
        CHECK(hn_actor_frames(&script,64));
    }
    CHECK(hn_read32(&memory,HN_ACTOR_CONTEXTS+84)==0);
    /* An actor started by an earlier actor can run later in the same frame. */
    hn_write8(&memory,0x02020000,0x26);hn_write8(&memory,0x02020001,16);
    hn_write32(&memory,0x02020002,0x02020100);hn_write8(&memory,0x02020006,0x27);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x101,0);CHECK(hn_actor_configure(&memory,1,0x02020000));
    CHECK(hn_actor_frames(&script,64)&&hn_read8(&memory,HN_SCENE_CONTROLS+0x101)==0xCA);
    CHECK(hn_read16(&memory,0x03001FBC)==0xFACE);
    /* Resource faults and command budgets restore Main and retain the actor's diagnostic state. */
    hn_write8(&memory,0x02020000,0x0C);hn_write32(&memory,0x02020001,0x02020000);
    CHECK(hn_actor_configure(&memory,1,0x02020000));
    CHECK(!hn_actor_frames(&script,16)&&script.fault==HN_SCRIPT_BUDGET);
    CHECK(hn_read32(&memory,0x03002C60)==0x02021000&&hn_read16(&memory,0x03001FBC)==0xFACE);
    hn_script_init(&script,&memory,NULL,0);hn_write8(&memory,0x02020000,0xFF);
    CHECK(hn_actor_configure(&memory,1,0x02020000));
    CHECK(!hn_actor_frames(&script,16)&&script.fault==HN_SCRIPT_OPCODE&&script.fault_opcode==0xFF);
    CHECK(hn_read32(&memory,0x03002C60)==0x02021000&&hn_read8(&memory,0x03002BF0)==15);
    hn_write8(&memory,0x02020000,1);hn_write32(&memory,0x02020001,UINT32_MAX);
    hn_script_init(&script,&memory,NULL,0);CHECK(hn_main_command(&script,0x26,0x02020000));
    CHECK(hn_read32(&memory,HN_ACTOR_CONTEXTS+84)==0&&hn_read8(&memory,0x0300001D)==1);
    CHECK(!memory.fault);
}
static void counter_contract(void)
{
    uint8_t *rom=calloc(1,0x600000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x600000);
    for(unsigned bank=0;bank<3;bank++){
        put32(rom+HN_COUNTER_TEMPLATES-HN_ROM_BASE+bank*4,HN_ROM_BASE+0x1000+bank*32);
        for(unsigned i=0;i<16;i++){rom[0x1000+bank*32+i*2]=(uint8_t)i;rom[0x1001+bank*32+i*2]=(uint8_t)(0x10+bank);}
        for(unsigned i=0;i<3;i++)rom[HN_COUNTER_PALETTES-HN_ROM_BASE+bank*6+i*2]=(uint8_t)(bank*3+i+1);
    }
    hn_write16(&memory,0x0201DC52,123);hn_write16(&memory,0x0201DC56,987);hn_write16(&memory,0x0201DC54,456);
    CHECK(hn_text_counter(&memory,123,0));CHECK(hn_read16(&memory,0x0201DC4E)==123);
    CHECK(hn_read16(&memory,HN_COUNTER_BUFFER)==0x1000&&hn_read16(&memory,HN_COUNTER_BUFFER+8)==0x1004);
    CHECK(hn_read16(&memory,HN_COUNTER_BUFFER+6)==11&&hn_read16(&memory,HN_COUNTER_BUFFER+12)==12);
    CHECK(hn_read16(&memory,HN_COUNTER_BUFFER+18)==13&&hn_read16(&memory,0x0500020A)==1);
    CHECK(hn_read8(&memory,HN_TRANSFER_WRITE)==3&&hn_read32(&memory,HN_TRANSFER_A)==HN_COUNTER_PALETTES);
    CHECK(hn_text_counter(&memory,987,1));CHECK(hn_read16(&memory,0x0201DC4E)==987);
    CHECK(hn_read16(&memory,HN_COUNTER_BUFFER+6)==46&&hn_read16(&memory,0x0500020A)==4);
    CHECK(hn_text_counter(&memory,456,2));CHECK(hn_read16(&memory,0x0201DC4E)==456);
    CHECK(hn_read16(&memory,HN_COUNTER_BUFFER+6)==14&&hn_read16(&memory,HN_COUNTER_BUFFER+12)==42);
    CHECK(hn_text_counter_digits(&memory,-123));CHECK(hn_read16(&memory,HN_COUNTER_BUFFER+6)==9);
    CHECK(hn_read16(&memory,HN_COUNTER_BUFFER+12)==8&&hn_read16(&memory,HN_COUNTER_BUFFER+18)==7);
    uint32_t consumed=99,yield=0xABCD1234;
    CHECK(hn_scene_animation_callback(&memory,0x0801E3E9,0,0,&consumed,&yield)==HN_SCENE_OK);
    CHECK(consumed==0&&yield==0xABCD1201&&hn_read32(&memory,HN_COUNTER_BANK)==0);
    CHECK(!hn_text_counter(&memory,1,3)&&hn_read32(&memory,HN_COUNTER_BANK)==0);
    hn_write32(&memory,HN_COUNTER_BANK,3);CHECK(!hn_text_counter_digits(&memory,1)&&!memory.fault);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write16(&memory,0x0201DC52,995);hn_write16(&memory,0x0201DC4C,10);
    CHECK(hn_scene_callback(&memory,0x0801DC9D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read16(&memory,0x0201DC52)==999&&hn_read16(&memory,0x0201DC4C)==4);
    CHECK(hn_read32(&memory,HN_COUNTER_STEPS)==4&&hn_read32(&memory,HN_COUNTER_DELTA)==1&&hn_read32(&memory,HN_COUNTER_DELAY)==8);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,2)+4)==HN_COUNTER_ANIMATION&&!(hn_read8(&memory,hn_render_slot(&memory,2)+0x61)&12));
    hn_write16(&memory,0x0201DC4C,1000);
    CHECK(hn_scene_callback(&memory,0x0801DD5D,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC52)==999);
    CHECK(!hn_read16(&memory,0x0201DC4C)&&hn_read32(&memory,HN_COUNTER_STEPS)==32&&!hn_read32(&memory,HN_COUNTER_DELTA));
    yield=0xABCD1234;
    CHECK(hn_scene_animation_callback(&memory,0x0801E349,0,0,&consumed,&yield)==HN_SCENE_OK&&yield==0xABCD1201);
    CHECK(hn_read32(&memory,HN_COUNTER_STEPS)==31&&hn_read32(&memory,HN_COUNTER_COUNTDOWN)==1);
    hn_write32(&memory,HN_COUNTER_COUNTDOWN,0x80000000);hn_write32(&memory,HN_COUNTER_STEPS,0);
    CHECK(hn_text_counter_update(&memory,&yield)&&hn_read32(&memory,HN_COUNTER_COUNTDOWN)==0x7FFFFFFF);
    hn_write32(&memory,HN_COUNTER_COUNTDOWN,0);
    CHECK(hn_text_counter_update(&memory,&yield)&&!hn_read32(&memory,HN_COUNTER_COUNTDOWN));
    hn_write16(&memory,0x0201DC54,49);hn_write16(&memory,0x0201DC4C,31);
    CHECK(hn_scene_callback(&memory,0x0801E289,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC54)==50);
    CHECK(hn_read32(&memory,HN_COUNTER_DELAY)==31&&hn_read32(&memory,HN_COUNTER_BANK)==2);
    hn_write16(&memory,0x0201DC4C,32);
    CHECK(hn_scene_callback(&memory,0x0801E1D1,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC54)==18);
    CHECK(hn_read32(&memory,HN_COUNTER_DELTA)==UINT32_MAX&&hn_read32(&memory,HN_COUNTER_DELAY)==1);
    CHECK(!hn_text_counter_change(&memory,1,false)&&!memory.fault);free(rom);
}
static void motion_contract(void)
{
    reset();hn_write16(&memory,HN_COLLISION_DIMENSIONS,32);hn_write16(&memory,HN_COLLISION_DIMENSIONS+2,32);
    hn_write32(&memory,HN_COLLISION_MAP,0x02010000);uint32_t obstacle=99;
    hn_write16(&memory,0x02010000,0xF000);hn_write16(&memory,0x02010000+(32+4)*2,0xF000);
    CHECK(hn_motion_route_obstacle(&memory,0,0,0,0,&obstacle)&&obstacle==0); /* Top and right edges are excluded. */
    hn_write16(&memory,0x02010000+(32+3)*2,0x0FFF);
    CHECK(hn_motion_route_obstacle(&memory,0,0,0,0,&obstacle)&&obstacle==0);
    hn_write16(&memory,0x02010000+(32+3)*2,0x1000);
    CHECK(hn_motion_route_obstacle(&memory,0,0,0,0,&obstacle)&&obstacle==1);
    CHECK(hn_motion_route_obstacle(&memory,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,&obstacle)&&obstacle==0);
    hn_write16(&memory,HN_COLLISION_DIMENSIONS,65535);hn_write16(&memory,HN_COLLISION_DIMENSIONS+2,65535);
    CHECK(!hn_motion_route_obstacle(&memory,0,0,0,0,&obstacle)&&!memory.fault);
    hn_write16(&memory,HN_COLLISION_DIMENSIONS,32);hn_write16(&memory,HN_COLLISION_DIMENSIONS+2,32);
    memset(memory.ewram+0x10000,0,2048);hn_write32(&memory,HN_RENDER_SLOTS,0x02017000);
    hn_write8(&memory,HN_RENDER_INFO+0x10,32);hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);GameAddr leader=hn_render_slot(&memory,10),follower=hn_render_slot(&memory,13);
    hn_write32(&memory,leader+0x18,80*65536u+0x8000);hn_write32(&memory,leader+0x20,64*65536u+0x8000);
    hn_write32(&memory,follower+0x18,40*65536u+0x8000);hn_write32(&memory,follower+0x20,40*65536u+0x8000);
    CHECK(hn_motion_rebuild_route(&memory));CHECK(hn_read8(&memory,HN_ROUTE_END)==1);
    CHECK(hn_read32(&memory,HN_ROUTE_POINTS)==(40u|(40u<<16)));
    CHECK(hn_read16(&memory,HN_ROUTE_POINTS+4)==40&&hn_read16(&memory,HN_ROUTE_POINTS+6)==0);
    CHECK(hn_read32(&memory,HN_ROUTE_POINTS+8)==(80u|(40u<<16))&&hn_read16(&memory,HN_ROUTE_POINTS+14)==24);
    hn_write16(&memory,0x02010000+(7*32+13)*2,0xF000);CHECK(hn_motion_rebuild_route(&memory));
    CHECK(hn_read16(&memory,HN_ROUTE_POINTS+4)==0&&hn_read16(&memory,HN_ROUTE_POINTS+6)==24);
    CHECK(hn_read32(&memory,HN_ROUTE_POINTS+8)==(40u|(64u<<16))&&hn_read16(&memory,HN_ROUTE_POINTS+12)==40);
    hn_write32(&memory,leader+0x18,hn_read32(&memory,follower+0x18));hn_write32(&memory,HN_ROUTE_POINTS+8,0xDEADBEEF);
    CHECK(hn_motion_rebuild_route(&memory)&&hn_read8(&memory,HN_ROUTE_END)==0);
    CHECK(hn_read32(&memory,HN_ROUTE_POINTS+8)==0xDEADBEEF);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,32);CHECK(!hn_motion_rebuild_route(&memory)&&!memory.fault);

    memset(memory.iwram+(HN_BACKGROUND_BOUNDS-0x03000000),0xA5,48);
    for(unsigned i=0;i<0x1200;i+=2)hn_write16(&memory,0x02020000+i,0x2340);
    for(unsigned i=0;i<128;i++)hn_write8(&memory,0x02022000+i,(uint8_t)i);
    hn_write32(&memory,HN_COLLISION_DEFAULT+0x1200,0xDEADBEEF);
    CHECK(hn_motion_configure(&memory,0x1000A,0x1000D,0x101,0x102,2,0x10040,0x10080,0x02020000,0x02022000));
    CHECK(hn_read8(&memory,HN_RENDER_CLASS_LEADER)==10&&hn_read8(&memory,HN_RENDER_CLASS_FOLLOWER)==13);
    CHECK(hn_read16(&memory,HN_BACKGROUND_BOUNDS+2)==64&&hn_read16(&memory,HN_BACKGROUND_BOUNDS+6)==128);
    CHECK(hn_read16(&memory,HN_BACKGROUND_BOUNDS+14)==0xA5A5&&hn_read16(&memory,HN_BACKGROUND_BOUNDS+26)==64);
    CHECK(hn_read8(&memory,HN_BACKGROUND_BOUNDS+32)==1&&hn_read8(&memory,HN_BACKGROUND_BOUNDS+44)==0xA5);
    CHECK(hn_read32(&memory,HN_COLLISION_DEFAULT)==0x23402340&&hn_read32(&memory,HN_COLLISION_DEFAULT+0x1200)==0xDEADBEEF);
    CHECK(hn_read8(&memory,0x0300226F)==127&&hn_read8(&memory,0x03002170)==0);
    const uint8_t compressed[]={0x11,16,0,0,0,'A','B','C','D','E','F','G','H',0,'I','J','K','L','M','N','O','P'};
    memcpy(memory.ewram+0x20000,compressed,sizeof(compressed));
    CHECK(hn_motion_configure(&memory,10,13,0,0,0,256,256,0x02020000,0));
    CHECK(!memcmp(memory.ewram+(HN_COLLISION_DEFAULT-0x02000000),"ABCDEFGHIJKLMNOP",16));
    CHECK(hn_read32(&memory,HN_COLLISION_DEFAULT+16)==0x23402340&&hn_read32(&memory,0x030021F0)==0);
    HNScript script;hn_script_init(&script,&memory,NULL,0);
    const uint8_t config[]={10,13,0,0,0xC0,0,1,0,2,0,0,0,0,1,0,0,0};
    memcpy(memory.ewram+0x20000,config,sizeof(config));hn_write32(&memory,0x03002BB0,HN_COLLISION_DEFAULT);
    hn_write32(&memory,0x03002BB4,0x02022000);hn_write8(&memory,0x030021E0,0xA5);hn_write8(&memory,HN_ROUTE_READ,5);
    CHECK(hn_main_command(&script,0x20,0x02020000));CHECK(hn_read32(&memory,0x03000014)==17);
    CHECK(hn_read8(&memory,HN_ROUTE_READ)==5&&hn_read8(&memory,0x030021E0)==0xA5);
    hn_write8(&memory,0x02020004,0);CHECK(hn_main_command(&script,0x20,0x02020000));
    CHECK(hn_read8(&memory,HN_ROUTE_READ)==0&&hn_read8(&memory,0x030021E0)==0&&!memory.fault);
    reset();hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,4);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,1);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,2);
    leader=hn_render_slot(&memory,1);follower=hn_render_slot(&memory,2);
    hn_write8(&memory,leader+0x60,0xA5);hn_write32(&memory,leader+0x18,0xFFFF1234);hn_write32(&memory,leader+0x20,0x8000ABCD);
    uint32_t consumed=99;
    CHECK(hn_scene_callback(&memory,0x0801D415,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x11)==5&&hn_read16(&memory,0x0201DC60)==65535&&hn_read16(&memory,0x0201DC62)==32768);
    hn_write16(&memory,0x0201DC84,0x1234);hn_write8(&memory,0x02020000,1);
    CHECK(hn_scene_callback(&memory,0x0801D455,0x02020000,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read32(&memory,leader+0x18)==0xFFFF8000&&hn_read32(&memory,leader+0x20)==0x12348000&&hn_read8(&memory,leader+0x60)==5);
    CHECK(hn_motion_transition(&memory,false,false));
    CHECK(hn_read32(&memory,leader+0x18)==0x12348000&&hn_read32(&memory,leader+0x20)==0x80008000);
    hn_write16(&memory,0x0201DC86,65534);hn_write16(&memory,0x0201DC88,3);
    CHECK(hn_scene_callback(&memory,0x0801D5E5,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read32(&memory,follower+0x18)==0x12328000&&hn_read32(&memory,follower+0x20)==0x80038000&&hn_read8(&memory,follower+0x60)==5);
    hn_write16(&memory,0x0201DC60,10);hn_write8(&memory,0x02020000,255);
    CHECK(hn_scene_callback(&memory,0x0801D581,0x02020000,&consumed)==HN_SCENE_OK&&consumed==1&&hn_read16(&memory,0x0201DC60)==10);
    hn_write16(&memory,0x0201DC8E,2);hn_write16(&memory,0x0201DC92,1);
    CHECK(hn_motion_interpolate(&memory,1)&&hn_read16(&memory,0x0201DC60)==8);
    hn_write16(&memory,0x0201DC8E,65535);hn_write16(&memory,0x0201DC60,8);
    CHECK(hn_motion_interpolate(&memory,1)&&hn_read16(&memory,0x0201DC60)==65528);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,4);
    CHECK(!hn_motion_transition(&memory,false,false)&&!memory.fault);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,1);hn_write8(&memory,0x02020000,0);
    const uint8_t remap[]={128,32,224,64,192,0};memcpy(memory.ewram+0x4580,remap,sizeof(remap));
    hn_write32(&memory,leader+0x18,0x0040C000);
    CHECK(hn_scene_callback(&memory,0x0802D191,0x02020000,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read16(&memory,0x0201DC84)==64&&hn_read32(&memory,leader+0x18)==0x0040C000&&hn_read32(&memory,leader+0x24)==43008);
    hn_write8(&memory,0x02020000,1);
    CHECK(hn_motion_scene_actor(&memory,0x02020000)&&hn_read32(&memory,leader+0x18)==0x00550000);
    CHECK((int32_t)hn_read32(&memory,leader+0x24)==-43008);
    CHECK(hn_scene_callback(&memory,0x0802D2ED,0,&consumed)==HN_SCENE_OK&&consumed==0&&hn_read32(&memory,leader+0x18)==0x00408000);
    hn_write32(&memory,leader+0x18,0x00C8C000);
    CHECK(hn_motion_scene_actor(&memory,0x02020000)&&hn_read32(&memory,leader+0x18)==0x00B00000&&hn_read32(&memory,leader+0x24)==49152);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x65,2);hn_write32(&memory,follower+0x18,0x00E08000);
    CHECK(hn_motion_scene_actor(&memory,0x02020000)&&hn_read16(&memory,0x0201DC86)==224);
    CHECK(hn_read32(&memory,follower+0x18)==0x00C00000&&hn_read32(&memory,follower+0x24)==26214);
    CHECK(hn_motion_restore_actors(&memory)&&hn_read32(&memory,follower+0x18)==0x00C00000);
    hn_write8(&memory,0x03002182,1);
    CHECK(hn_motion_restore_actors(&memory)&&hn_read32(&memory,follower+0x18)==0x00E08000&&hn_read32(&memory,leader+0x18)==0x00C88000);
    /* Position writes can change the live controls used by the velocity division. */
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,0);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,0);
    hn_write32(&memory,HN_RENDER_SLOTS,HN_SCENE_CONTROLS+0x64-0x18);
    const uint8_t aliases[]={128,3,200,32,10,1,10,0};memcpy(memory.ewram+0x4580,aliases,sizeof(aliases));
    CHECK(hn_motion_scene_actor(&memory,0x02020000)&&!hn_read8(&memory,HN_SCENE_CONTROLS+0x65));
    CHECK((int32_t)hn_read32(&memory,HN_SCENE_CONTROLS+0x64+12)==-55296);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,4);hn_write8(&memory,HN_SCENE_CONTROLS+0x65,0);
    CHECK(!hn_motion_scene_actor(&memory,0x02020000)&&!memory.fault);
    reset();hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,4);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,1);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,2);
    leader=hn_render_slot(&memory,1);follower=hn_render_slot(&memory,2);uint32_t yield=0xABCD;
    hn_write32(&memory,leader+0x18,0x00E08000-48);hn_write32(&memory,leader+0x20,0x00B88000+96);hn_write32(&memory,leader+0x28,0xDEADBEEF);
    CHECK(hn_scene_animation_callback(&memory,0x0802FFF5,leader,0,&consumed,&yield)==HN_SCENE_OK&&consumed==0&&yield==0);
    CHECK(hn_read32(&memory,leader+0x24)==1&&hn_read32(&memory,leader+0x2C)==(uint32_t)-2&&hn_read32(&memory,leader+0x28)==0xDEADBEEF);
    hn_write32(&memory,leader+0x18,0x007C8000);hn_write32(&memory,leader+0x20,0x00878000);hn_write32(&memory,leader+0x1C,(uint32_t)-0x80000);
    CHECK(hn_motion_map_height(&memory,leader)&&(int32_t)hn_read32(&memory,leader+0x1C)==-0xA0000);
    CHECK(hn_motion_map_height(&memory,leader)&&(int32_t)hn_read32(&memory,leader+0x1C)==-0x88000);
    hn_write8(&memory,0x03002194,2);hn_write32(&memory,leader+0x1C,0);
    CHECK(hn_motion_map_height(&memory,leader)&&hn_read32(&memory,leader+0x1C)==0x20000);
    hn_write8(&memory,0x030021A2,2);
    CHECK(hn_motion_map_height(&memory,leader)&&!hn_read32(&memory,leader+0x1C));
    hn_write8(&memory,0x030021A2,0);hn_write32(&memory,leader+0x18,0x00408000);hn_write32(&memory,leader+0x20,0x00408000);
    hn_write32(&memory,leader+0x1C,0x8000);hn_write8(&memory,0x03002182,1);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,1);
    CHECK(hn_scene_animation_callback(&memory,0x0803002D,0,0,&consumed,&yield)==HN_SCENE_OK&&yield==1&&consumed==0);
    CHECK((int32_t)hn_read32(&memory,leader+0x1C)==-0x38000); /* Aliased classes are both updated. */
    hn_write32(&memory,leader+0x18,0);hn_write32(&memory,leader+0x20,0);hn_write32(&memory,leader+0x1C,0x7FFF);
    CHECK(hn_motion_map_height(&memory,leader)&&hn_read32(&memory,leader+0x1C)==0x27FFF);
    CHECK(hn_motion_map_height(&memory,leader)&&hn_read32(&memory,leader+0x1C)==0x8000);
    CHECK(!memory.fault);
    reset();hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,1);hn_write8(&memory,HN_SCENE_CONTROLS+0xE4,1);
    leader=hn_render_slot(&memory,1);GameAddr source=hn_render_slot(&memory,2),actor=hn_render_slot(&memory,3);
    hn_write16(&memory,HN_BACKGROUND_BOUNDS+2,384);hn_write16(&memory,HN_BACKGROUND_BOUNDS+6,384);
    hn_write8(&memory,HN_SCENE_CONTROLS+0xE7,0xA5);hn_write8(&memory,HN_SCENE_CONTROLS+0xE8,0xB9);
    hn_write32(&memory,source+0x18,0x00508000);hn_write32(&memory,source+0x1C,0x8000);hn_write32(&memory,source+0x20,0x012C8000);
    hn_write32(&memory,leader+0x18,0x00C88000);hn_write32(&memory,leader+0x1C,0x8000);hn_write32(&memory,leader+0x20,0x00648000);
    hn_write16(&memory,HN_SCENE_DURATION,16);hn_write8(&memory,0x02020000,3);hn_write8(&memory,0x02020001,2);
    CHECK(hn_scene_callback(&memory,0x08011C8D,0x02020000,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_read32(&memory,actor+0x18)==0x00680000&&hn_read32(&memory,actor+0x1C)==0x8000&&hn_read32(&memory,actor+0x20)==0x01200000);
    CHECK(hn_read32(&memory,actor+0x24)==395264&&(int32_t)hn_read32(&memory,actor+0x2C)==-765952&&!hn_read32(&memory,actor+0x28));
    CHECK(hn_read8(&memory,source+0x62)==0x75&&hn_read8(&memory,actor+0x62)==0x39&&hn_read32(&memory,actor+4)==HN_APPROACH_ANIMATION);
    CHECK(hn_scene_callback(&memory,0x08011DC1,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,actor+0x62)==0x75&&hn_read8(&memory,leader+0x62)==0x39);
    yield=0xABCD5A;
    CHECK(hn_scene_animation_callback(&memory,0x08011DC1,0,0,&consumed,&yield)==HN_SCENE_OK&&consumed==0&&yield==0xABCD5A);
    hn_write16(&memory,HN_SCENE_DURATION,0);
    CHECK(!hn_motion_approach_clamped(&memory,0x02020000)&&!memory.fault);
    hn_render_set_animation(&memory,3,0);CHECK(hn_scene_callback(&memory,0x08011DC1,0,&consumed)==HN_SCENE_RESOURCE);
    hn_write32(&memory,0x03002BB0,0x08000010);hn_write32(&memory,0x03002BB4,0x08000020);
    CHECK(hn_scene_callback(&memory,0x08011E3D,0,&consumed)==HN_SCENE_OK);
    GameAddr linked=hn_render_slot(&memory,9);
    CHECK(hn_read32(&memory,linked+4)==0x08000010&&hn_read32(&memory,linked+0x18)==0x00148000&&hn_read32(&memory,linked+0x1C)==0xFFF88000);
    CHECK(hn_read8(&memory,linked+0x62)==0xB0&&hn_read8(&memory,linked+0x65)==1);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,9);hn_write8(&memory,0x03002170,0xA5);
    CHECK(hn_scene_callback(&memory,0x08011E7D,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,linked+4)==0x08000020&&!hn_read8(&memory,0x03002170));
    CHECK(hn_render_modes(&memory,9,4,255,0xA5)&&hn_read8(&memory,linked+0x62)==0x30&&hn_read8(&memory,linked+0x65)==0xA5);
    CHECK(hn_render_modes(&memory,9,1,3,0xFE)&&hn_read8(&memory,linked+0x62)==0x7E&&hn_read8(&memory,linked+0x65)==0xA5);
    CHECK(!hn_render_modes(&memory,40,0,0,0)&&!memory.fault);
    hn_write8(&memory,0x02020000,1);hn_write8(&memory,0x02020001,0x70);hn_write8(&memory,HN_SCENE_CONTROLS+0x70,2);
    CHECK(hn_scene_callback(&memory,0x08011EA9,0x02020000,&consumed)==HN_SCENE_OK&&consumed==2&&hn_read8(&memory,HN_SCENE_CONTROLS+0x70)==2);
    hn_write32(&memory,0x03002BF0,UINT32_MAX);
    CHECK(hn_scene_callback(&memory,0x08011EF5,0,&consumed)==HN_SCENE_OK&&consumed==0&&hn_read8(&memory,HN_SAVED_CALL_DEPTH)==255&&!memory.fault);
}
static void map_contract(void)
{
    const size_t size=0x600000;uint8_t *rom=calloc(1,size);CHECK(rom!=NULL);
    put32(rom+(HN_MAP_BANKS-HN_ROM_BASE),HN_ROM_BASE+0x40);
    put32(rom+0x40,HN_ROM_BASE+0x200);put32(rom+0x44,HN_ROM_BASE+0x100);
    rom[0x48]=10;rom[0x49]=9;rom[0x4A]=1;rom[0x4B]=1;rom[0x4C]=7;
    put32(rom+0x50,HN_ROM_BASE+0x300);
    rom[0x58]=16;rom[0x59]=15;rom[0x5A]=2;rom[0x5B]=2;
    put32(rom+0x100,2);put32(rom+0x104,0x12345678);put32(rom+0x108,0x87654321);
    hn_memory_init(&memory,rom,size);hn_heap_init(&memory);
    CHECK(hn_map_initialize(&memory,0,0));
    GameAddr slots=hn_read32(&memory,HN_RENDER_SLOTS);
    CHECK(hn_read8(&memory,HN_RENDER_INFO+16)==10);
    CHECK(hn_read8(&memory,HN_RENDER_INFO+17)==8&&hn_read8(&memory,HN_RENDER_INFO+18)==9);
    CHECK(hn_read32(&memory,0x03002C60)==HN_ROM_BASE+0x200);
    CHECK(hn_read32(&memory,HN_MAP_WORDS)==0x12345678);
    CHECK(hn_read32(&memory,HN_MAP_WORDS+4)==0x87654321);
    CHECK(hn_read8(&memory,0x03002C0C)==7);
    CHECK(hn_read16(&memory,slots+0x3C)==256);
    CHECK(hn_read16(&memory,slots+0x4E)==240);
    CHECK(hn_read8(&memory,hn_read32(&memory,HN_RENDER_ORDER)+9)==9);
    hn_write32(&memory,slots+0x18,0x31415926);hn_write32(&memory,slots+0x20,0x12345678);
    hn_write16(&memory,slots+0x48,0xABCD);hn_render_slot_reset(&memory,0);
    CHECK(hn_read32(&memory,slots+0x18)==0x31415926);
    CHECK(hn_read32(&memory,slots+0x20)==0x12345678);
    CHECK(hn_read16(&memory,slots+0x48)==0xABCD);
    hn_write8(&memory,slots+0x62,128);hn_write8(&memory,slots+0x65,1);
    hn_write32(&memory,slots+0x68+0x20,0x00005678);
    CHECK(hn_render_key(&memory,0)==0x12345678);
    CHECK(hn_map_initialize(&memory,0,1));
    CHECK(hn_read8(&memory,HN_RENDER_INFO+16)==16);
    CHECK(hn_read32(&memory,0x03002C60)==HN_ROM_BASE+0x300);
    CHECK(hn_read8(&memory,HN_BACKGROUND_COUNT)==2);
    CHECK(hn_read8(&memory,HN_GRAPHICS_COUNT)==2);
    CHECK(hn_read8(&memory,0x0200452E)==1);
    CHECK(hn_read16(&memory,HN_LAYER_OFFSETS+0x22)==256);
    CHECK(hn_read16(&memory,HN_LAYER_OFFSETS+0x24)==0);
    hn_background_layers_reset(&memory,1);
    CHECK(hn_read16(&memory,HN_LAYER_OFFSETS+0x24)==256);
    CHECK(!memory.fault);free(rom);reset();
}
static void expression_contract(void)
{
    reset();uint32_t result=0;
    /* Control byte += 300, returning the untruncated arithmetic result. */
    hn_write8(&memory,0x02004525,250);
    hn_write16(&memory,0x02000000,0x7605);hn_write16(&memory,0x02000002,0xDF00);
    hn_write16(&memory,0x02000004,300);
    CHECK(hn_expression_evaluate(&memory,0x02000000,true,&result));
    CHECK(result==550&&hn_read8(&memory,0x02004525)==38);
    CHECK(hn_read32(&memory,0x0300079C)==0x02000006);
    /* Boolean AND with false left side does not consume the right operand. */
    hn_write16(&memory,0x02000000,0xA400);hn_write16(&memory,0x02000002,0xBE01);
    CHECK(hn_expression_evaluate(&memory,0x02000000,false,&result));
    CHECK(result==0&&hn_read32(&memory,0x03000798)==0x02000002);
    /* Bitwise updates evaluate both operands. */
    CHECK(hn_expression_evaluate(&memory,0x02000000,true,&result));
    CHECK(result==0&&hn_read32(&memory,0x0300079C)==0x02000004);
    hn_write16(&memory,0x02000000,0x3E0D);
    CHECK(hn_expression_evaluate(&memory,0x02000000,true,&result));
    CHECK(result==1&&(hn_read8(&memory,0x03002181)&32));
    hn_write16(&memory,0x02000000,0x5E0D);
    CHECK(hn_expression_evaluate(&memory,0x02000000,true,&result));
    CHECK(result==0&&!(hn_read8(&memory,0x03002181)&32));
    memset(memory.ewram,0,1024);
    CHECK(!hn_expression_evaluate(&memory,0x02000000,false,&result));
    CHECK(!memory.fault);
    HNScript script;hn_script_init(&script,&memory,NULL,0);
    hn_write32(&memory,0x02001000,0x02000000);
    CHECK(!hn_main_command(&script,0x23,0x02001000));
    CHECK(script.fault==HN_SCRIPT_RESOURCE);
    hn_script_init(&script,&memory,NULL,0);
    hn_write32(&memory,0x02001000,0x0801C709);hn_write16(&memory,0x02001004,0xF5A5);
    CHECK(hn_main_command(&script,0x1C,0x02001000));
    CHECK(hn_read16(&memory,0x03001138)==0xF5A5&&hn_read16(&memory,0x03001F64)==0x1A5);
    hn_script_init(&script,&memory,NULL,0);
    hn_write16(&memory,0x02001008,65535);hn_write16(&memory,0x0200100A,65535);
    CHECK(!hn_main_command(&script,0x35,0x02001000));
    CHECK(script.fault==HN_SCRIPT_RESOURCE&&!memory.fault);
    reset();
    CHECK(!hn_expression_evaluate(&memory,0x01000000,false,&result));
    CHECK(memory.fault&&memory.fault_address==0x01000000);
}
static void save_contract(void)
{
    reset();hn_heap_init(&memory);memset(memory.sram,0,sizeof(memory.sram));
    hn_write8(&memory,0x02000000,1);hn_write8(&memory,0x02000001,2);hn_write8(&memory,0x02000002,3);
    CHECK(hn_save_checksum(&memory,0x02000000,0)==0xFFFF);
    CHECK(hn_save_checksum(&memory,0x02000000,3)==0xFDFB);
    hn_write16(&memory,0x04000204,0x45B4);
    CHECK(hn_sram_read(&memory,0x0E000000,0x02001000,0));
    CHECK(hn_read16(&memory,0x04000204)==0x45B7);
    CHECK(!hn_sram_write(&memory,0x02000000,0x0E000000,HN_SRAM_SIZE+1));
    CHECK(hn_sram_write(&memory,0x02000000,0x0E000100,3));
    CHECK(hn_sram_verify(&memory,0x02000000,0x0E000100,3)==0);
    hn_write8(&memory,0x0E000102,4);
    CHECK(hn_sram_verify(&memory,0x02000000,0x0E000100,3)==0x0E000102);
    hn_write8(&memory,0x03002180,1);uint32_t size;
    CHECK(hn_save_produce(&memory,1,0x02010000,&size));CHECK(size==0x395);
    CHECK(hn_read8(&memory,0x020102F0)==9&&hn_read8(&memory,0x03002180)==1);
    CHECK(hn_save_produce(&memory,2,0x02010000,&size));CHECK(size==0x6EA);
    CHECK(hn_save_produce(&memory,3,0x02010000,&size));CHECK(size==0xFB);
    hn_write8(&memory,0x02004527,0x5A);hn_write16(&memory,0x0201DC90,0xABCD);
    hn_write8(&memory,0x03003D00,3);
    CHECK(hn_save_write_groups(&memory));CHECK(memory.sram_dirty);
    const unsigned offsets[]={0xEE,0x6B8,0xDC2},lengths[]={0x5CA,0x70A,0x66A};
    for(unsigned i=0;i<3;i++){
        CHECK(hn_save_checksum(&memory,0x0E000000+offsets[i],lengths[i])==0);
        CHECK(hn_save_checksum(&memory,0x0E004000+offsets[i],lengths[i])==0);
    }
    hn_write8(&memory,0x02004527,0);hn_write16(&memory,0x0201DC90,0);hn_write8(&memory,0x03003D00,0);
    CHECK(hn_save_load_groups(&memory));CHECK(hn_read8(&memory,0x03002169)==1);
    CHECK(hn_read8(&memory,0x02004527)==0x5A);CHECK(hn_read16(&memory,0x0201DC90)==0xABCD);
    CHECK(hn_read8(&memory,0x03003D00)==3);
    /* A primary failure reloads all three groups from backup together. */
    hn_write8(&memory,0x0E0006C0,hn_read8(&memory,0x0E0006C0)^1);
    hn_write8(&memory,0x02004527,0);
    CHECK(hn_save_load_groups(&memory));CHECK(hn_read8(&memory,0x03002169)==2);
    CHECK(hn_read8(&memory,0x0300216B)==2&&hn_read8(&memory,0x02004527)==0x5A);
    hn_write8(&memory,0x0E004DC2,hn_read8(&memory,0x0E004DC2)^1);
    CHECK(hn_save_load_groups(&memory));CHECK(hn_read8(&memory,0x03002169)==0);
    CHECK(hn_save_write_record(&memory,2,0));
    CHECK(hn_save_load_group(&memory,2));CHECK(hn_read8(&memory,0x0300216A)==1);
    HNScript script;hn_script_init(&script,&memory,NULL,0);hn_write8(&memory,0x02000000,2);
    CHECK(hn_main_command(&script,0x43,0x02000000));CHECK(hn_read32(&memory,0x03000014)==1);
    /* A failed triplet load resumes the script without a map reload. */
    hn_write8(&memory,0x0E0000EE,hn_read8(&memory,0x0E0000EE)^1);
    hn_write8(&memory,0x0E0040EE,hn_read8(&memory,0x0E0040EE)^1);
    hn_write32(&memory,0x03002BF0,3);hn_script_init(&script,&memory,NULL,0);
    CHECK(hn_main_command(&script,0x60,0x02000000));CHECK(hn_read32(&memory,0x03002BF0)==0);
    CHECK(hn_read32(&memory,0x03000014)==0&&hn_read8(&memory,0x03000010)==0);
    hn_write32(&memory,0x02000000,0x080C0129);hn_write8(&memory,0x02000004,3);
    hn_write8(&memory,0x0300216B,2);hn_script_init(&script,&memory,NULL,0);
    CHECK(hn_main_command(&script,0x1C,0x02000000));CHECK(hn_read8(&memory,0x02004528)==2);
    CHECK(hn_read32(&memory,0x03000014)==5);
    CHECK(!hn_save_write_record(&memory,0,0));CHECK(!hn_save_write_record(&memory,4,51));
    CHECK(!hn_save_load_group(&memory,5));CHECK(!hn_save_consume(&memory,5,0x02000000));
    CHECK(!memory.fault);
}
static void task_contract(void)
{
    /* Generated font and metadata: these tests need no game dump. */
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);
    memset(rom+HN_TEXT_GLYPHS-HN_ROM_BASE,0x55,64);
    memset(rom+HN_TEXT_GLYPHS-HN_ROM_BASE+64,0xAA,64);
    rom[HN_TEXT_ADVANCES-HN_ROM_BASE]=3;rom[HN_TEXT_ADVANCES-HN_ROM_BASE+1]=5;
    hn_memory_init(&memory,rom,0x900000);hn_heap_init(&memory);hn_task_contexts_init(&memory);
    GameAddr a=hn_task_allocate(&memory),b=hn_task_allocate(&memory),c=hn_task_allocate(&memory);
    CHECK(a&&b&&c&&a!=b&&b!=c);CHECK(hn_read32(&memory,HN_TASK_HEAD)==a);
    CHECK(hn_read32(&memory,b+0x28)==a+0x2C);CHECK(hn_task_remove(&memory,b));
    CHECK(hn_read32(&memory,a+0x2C)==c&&hn_read32(&memory,c+0x28)==a+0x2C);
    CHECK(hn_task_remove(&memory,a));CHECK(hn_read32(&memory,HN_TASK_HEAD)==c);
    CHECK(hn_task_remove(&memory,c));CHECK(hn_read32(&memory,HN_TASK_HEAD)==HN_TASK_SENTINEL);
    CHECK(!hn_task_remove(&memory,c));CHECK(!hn_task_remove(&memory,HN_TASK_SENTINEL));
    a=hn_task_allocate(&memory);CHECK(a!=0);
    const GameAddr script=0x02010000,shadow=0x0200C720,map=0x02011000;
    hn_write32(&memory,0x03007C00,0x12345678);
    hn_write16(&memory,map,0xAC00);hn_write16(&memory,map+64,0xBC00);
    CHECK(hn_task_graphics(&memory,a,258,258,256,0,2,1,256,0x06008000,map));
    CHECK(hn_read8(&memory,a+1)==2&&hn_read8(&memory,a+2)==2);
    CHECK(hn_read32(&memory,0x03007C00)==0x12345678);
    CHECK(hn_read32(&memory,shadow)==0x44444444);
    CHECK(hn_read16(&memory,map)==0xAC00);
    CHECK(hn_read16(&memory,map+64)==0xBC02);
    CHECK(hn_read32(&memory,0x040000D8)==0x06008000);
    CHECK(!hn_task_graphics(&memory,a,0,2,0,0,0,0,0,0x06008000,map));
    hn_write8(&memory,script,1);hn_write8(&memory,script+1,2);hn_write8(&memory,script+2,0xE1);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02014000);hn_write8(&memory,HN_RENDER_INFO+0x10,32);
    b=hn_task_allocate(&memory);CHECK(b!=0);
    CHECK(hn_task_graphics(&memory,b,2,2,0,0,0,1,0,0x06008800,map+512));
    hn_write8(&memory,script+64,2);hn_write8(&memory,script+65,0xE1);
    CHECK(hn_task_set_script(&memory,b,script+64));
    CHECK(hn_task_set_script(&memory,a,script));HNTasks tasks;hn_tasks_init(&tasks,&memory);
    hn_input_initialize(&memory);CHECK(hn_tasks_update(&tasks,32));
    CHECK(hn_read8(&memory,b)==1&&hn_read16(&memory,b+0x1C)==5);
    CHECK(text_pixel(hn_read32(&memory,b+0x18),2,0,0)==6);
    CHECK(hn_read8(&memory,a)==4&&hn_read8(&memory,a+6)==2);
    CHECK(hn_read16(&memory,a+0x1C)==3&&hn_read32(&memory,a+0x10)==script+1);
    CHECK(text_pixel(shadow,2,0,0)==5&&text_pixel(shadow,2,0,15)==5);
    CHECK(hn_tasks_update(&tasks,32));CHECK(hn_read8(&memory,a+6)==1);
    CHECK(hn_tasks_update(&tasks,32));CHECK(hn_read8(&memory,a)==3);
    CHECK(hn_tasks_update(&tasks,32));CHECK(hn_read16(&memory,a+0x1C)==8);
    CHECK(hn_input_update(&memory,2));CHECK(hn_tasks_update(&tasks,32));
    CHECK(hn_read8(&memory,a)==1&&hn_read32(&memory,a+0x10)==script+2);
    CHECK(!hn_read8(&memory,HN_TASK_DIRTY));
    /* Return to a caller after an automatically terminated name substitution. */
    hn_write8(&memory,HN_TEXT_FIRST,1);hn_write8(&memory,HN_TEXT_FIRST+1,0);
    hn_write8(&memory,script,0xE7);hn_write8(&memory,script+1,2);hn_write8(&memory,a+5,0);
    hn_write16(&memory,a+0x1C,0);CHECK(hn_task_set_script(&memory,a,script));
    CHECK(hn_tasks_update(&tasks,32));CHECK(hn_read8(&memory,a)==1);
    CHECK(hn_read16(&memory,a+0x1C)==8&&hn_read32(&memory,a+0x14)==0);
    /* A scrolling window keeps palette attributes and wraps tile rows. */
    hn_write8(&memory,a+2,4);hn_write16(&memory,a+0x1E,3);hn_write8(&memory,a+7,2);
    for(unsigned row=0;row<4;row++)for(unsigned col=0;col<2;col++)hn_write16(&memory,map+row*64+col*2,(uint16_t)(0xA800+row*2+col));
    CHECK(hn_task_scroll_map(&memory,a,false));CHECK(hn_read16(&memory,map)==0xA802);
    CHECK(hn_read16(&memory,map+192)==0xA805&&hn_read8(&memory,HN_TASK_GATE)==1);
    CHECK(hn_read32(&memory,0x06008080)==0x88888888);
    CHECK(hn_task_scroll_map(&memory,a,true));CHECK(hn_read16(&memory,map+192)==0xA807);
    CHECK(hn_read8(&memory,a)==3&&hn_read8(&memory,HN_TASK_GATE)==0);
    hn_task_text_number(&memory,0,65535);CHECK(hn_read8(&memory,0x03000648)==8&&hn_read8(&memory,0x0300064C)==7);
    hn_task_text_number(&memory,0x84,12);CHECK(hn_read32(&memory,0x03000648)==0x04030101);
    hn_task_text_number(&memory,2,1234);CHECK(hn_read16(&memory,0x03000648)==0x0605);
    /* Unknown callbacks and recursive text stop explicitly instead of executing guest words. */
    hn_write32(&memory,a+0x10,script);hn_write8(&memory,script,7);hn_write32(&memory,0x03001FC0+28,0x08001235);
    uint32_t keep=0;hn_tasks_init(&tasks,&memory);CHECK(!hn_task_command(&tasks,a,0xF4,&keep));
    CHECK(tasks.fault==HN_TASK_CALLBACK&&tasks.fault_address==0x08001235);
    hn_write8(&memory,script,0xF9);hn_write8(&memory,script+1,0);hn_write32(&memory,0x03002BB0,script+32);
    hn_write32(&memory,script+32,script);hn_write8(&memory,HN_TASK_VARIANT,0);
    hn_write8(&memory,a,3);hn_write32(&memory,a+0x10,script);hn_tasks_init(&tasks,&memory);
    CHECK(!hn_tasks_update(&tasks,8)&&tasks.fault==HN_TASK_BUDGET);
    hn_write8(&memory,a,1);hn_write32(&memory,a+0x2C,a);hn_tasks_init(&tasks,&memory);
    CHECK(!hn_tasks_update(&tasks,8)&&tasks.fault==HN_TASK_RESOURCE);
    CHECK(!memory.fault);hn_write32(&memory,HN_TASK_HEAD,0xDEADBEEF);hn_tasks_init(&tasks,&memory);
    CHECK(!hn_tasks_update(&tasks,8)&&tasks.fault==HN_TASK_MEMORY&&tasks.fault_address==0xDEADBEEF);
    free(rom);reset();
}
static void choice_contract(void)
{
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x900000);
    hn_heap_init(&memory);hn_task_contexts_init(&memory);
    const GameAddr descriptor=0x02018000,family=0x02018100,states=0x02019000,slots=0x0201A000;
    unsigned offset=HN_CHOICE_FAMILIES-HN_ROM_BASE;
    for(unsigned byte=0;byte<4;byte++)rom[offset+byte]=(uint8_t)(family>>(byte*8));
    for(unsigned i=0;i<9;i++)hn_write32(&memory,family+i*4,descriptor);
    hn_write32(&memory,HN_RENDER_STATES,states);hn_write8(&memory,HN_BACKGROUND_COUNT,1);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,states+256);hn_write8(&memory,HN_GRAPHICS_COUNT,1);
    hn_write32(&memory,HN_RENDER_SLOTS,slots);hn_write8(&memory,HN_RENDER_INFO+16,4);
    hn_write8(&memory,descriptor+16,3);hn_write8(&memory,descriptor+17,3);hn_write8(&memory,descriptor+18,10);
    hn_write8(&memory,descriptor+20,24);hn_write8(&memory,descriptor+21,48);hn_write32(&memory,descriptor+12,0x0201C000);
    for(unsigned i=0;i<9;i++){
        hn_write32(&memory,HN_CHOICE_RECORDS+i*12,i+1);
        hn_write32(&memory,HN_CHOICE_RECORDS+i*12+8,hn_task_allocate(&memory));
    }
    hn_write8(&memory,HN_CHOICE_COUNT,9);hn_write32(&memory,HN_CHOICE_INITIAL,8);
    CHECK(hn_choice_initial_selection(&memory,descriptor));
    CHECK(hn_read8(&memory,HN_CHOICE_ROW)==2&&hn_read8(&memory,HN_CHOICE_COLUMN)==1);
    hn_write16(&memory,HN_CHOICE_TILES,0xA08D);hn_write16(&memory,HN_CHOICE_TILES+2,0xB0A0);
    hn_write16(&memory,HN_CHOICE_TILES+4,0xC0A1);hn_write16(&memory,HN_CHOICE_TILES+6,0xF000);
    CHECK(hn_choice_display(&memory,descriptor));
    CHECK(hn_read32(&memory,slots+0x18)==96u*65536&&hn_read32(&memory,slots+0x1C)==80u*65536);
    CHECK(hn_read32(&memory,slots+104+4)==HN_CHOICE_CURSOR_10);
    CHECK(hn_read16(&memory,HN_CHOICE_TILES)==0x108D&&hn_read16(&memory,HN_CHOICE_TILES+2)==0x10A0);
    CHECK(hn_read16(&memory,HN_CHOICE_TILES+4)==0xA1&&hn_read16(&memory,HN_CHOICE_TILES+6)==0);
    CHECK(hn_read32(&memory,states+0x14)==0x0201C000&&hn_read8(&memory,states+0x38)==8);
    hn_write32(&memory,HN_CHOICE_RECORDS+7*12+4,1);
    CHECK(hn_choice_select_identifier(&memory,descriptor,8));
    CHECK(!hn_read8(&memory,HN_CHOICE_ROW)&&!hn_read8(&memory,HN_CHOICE_COLUMN));
    hn_write32(&memory,HN_CHOICE_INITIAL,99);CHECK(hn_choice_initial_selection(&memory,descriptor));
    CHECK(!hn_read8(&memory,HN_CHOICE_ROW)&&!hn_read8(&memory,HN_CHOICE_COLUMN));
    hn_write8(&memory,descriptor+16,0);CHECK(!hn_choice_display(&memory,descriptor));hn_write8(&memory,descriptor+16,3);
    HNScript script;hn_script_init(&script,&memory,NULL,0);const GameAddr arguments=0x0201D000;
    hn_write8(&memory,arguments,2);hn_write8(&memory,arguments+1,9);hn_write8(&memory,arguments+2,7);
    hn_write16(&memory,arguments+3,512);hn_write8(&memory,HN_SCENE_CONTROLS,5);
    CHECK(hn_main_command(&script,0x45,arguments));
    CHECK(hn_read8(&memory,HN_CHOICE_FAMILY)==2&&hn_read8(&memory,HN_CHOICE_FLAGS)==9);
    CHECK(hn_read32(&memory,HN_CHOICE_INITIAL)==6&&hn_read32(&memory,HN_CHOICE_PENDING)==7);
    CHECK(hn_read32(&memory,0x03000014)==5&&hn_read8(&memory,0x03000010)==0);
    hn_choice_request(&memory,8);CHECK(hn_read8(&memory,HN_CHOICE_FLAGS)==25&&hn_read32(&memory,HN_CHOICE_PENDING)==8);
    hn_write16(&memory,HN_CHOICE_STATE,1);CHECK(hn_choice_reset(&memory));
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x69)==0&&hn_read16(&memory,HN_CHOICE_STATE)==1);
    hn_input_repeat(&memory,77,9);hn_write16(&memory,HN_CHOICE_STATE,2);uint32_t consumed=99;
    CHECK(hn_scene_callback(&memory,0x08009319,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read16(&memory,HN_CHOICE_STATE)==0&&hn_read32(&memory,HN_TASK_HEAD)==HN_TASK_SENTINEL);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x69)==1&&hn_read16(&memory,HN_KEY_STATE+12)==77);
    CHECK(hn_read32(&memory,states+0x14)==HN_CHOICE_RESET_BACKGROUND);
    for(unsigned i=0;i<9;i++)CHECK(!hn_read32(&memory,HN_CHOICE_RECORDS+i*12));
    /* Closing preserves the script result until the background releases the dialog. */
    hn_write8(&memory,HN_CHOICE_FAMILY,0);hn_write16(&memory,HN_CHOICE_STATE,3);hn_write16(&memory,0x03001FBC,4);
    hn_write8(&memory,states+0x38,1);hn_write8(&memory,arguments,3);hn_write32(&memory,arguments+1,0x0201D100);hn_write32(&memory,arguments+5,0x0201D200);
    CHECK(hn_main_command(&script,0x24,arguments));CHECK(hn_read16(&memory,HN_CHOICE_STATE)==3&&hn_read16(&memory,0x03001FBC)==4);
    hn_write8(&memory,states+0x38,0);CHECK(hn_main_command(&script,0x24,arguments));
    CHECK(hn_read16(&memory,HN_CHOICE_STATE)==0&&hn_read16(&memory,0x03001FBC)==4&&hn_read32(&memory,0x03000014)==9);
    /* Candidate lists may exceed the nine displayed cells. Reject only an
       overflowing filtered result, without writing past the records. */
    hn_write8(&memory,HN_CHOICE_FLAGS,8);hn_write32(&memory,HN_CHOICE_RECORDS+9*12,0xDEADBEEF);
    for(unsigned i=0;i<10;i++)hn_write16(&memory,0x0201D200+i*2,0xBE01);
    uint32_t selected;
    CHECK(!hn_choice_update(&memory,10,0x0201D100,0x0201D200,&selected)&&!memory.fault);
    CHECK(hn_read8(&memory,HN_CHOICE_COUNT)==9&&hn_read32(&memory,HN_CHOICE_RECORDS)==1&&hn_read32(&memory,HN_CHOICE_RECORDS+8*12)==9);
    CHECK(hn_read32(&memory,HN_CHOICE_RECORDS+9*12)==0xDEADBEEF);
    hn_heap_init(&memory);hn_task_contexts_init(&memory);
    for(unsigned i=0;i<255;i++){hn_write8(&memory,0x0201D100+i,0);hn_write16(&memory,0x0201D200+i*2,i>=252?0xBE01:0xBE00);}
    put32(rom+HN_CHOICE_TEXT_PAIRS-HN_ROM_BASE,HN_TASK_DEFAULT_SCRIPT);hn_write16(&memory,HN_CHOICE_OPTIONS,65535);
    /* This fixture has no audio player; filtering/task setup precedes that failure. */
    CHECK(!hn_choice_update(&memory,255,0x0201D100,0x0201D200,&selected)&&!memory.fault);
    CHECK(hn_read8(&memory,HN_CHOICE_COUNT)==3&&hn_read32(&memory,HN_CHOICE_RECORDS)==253&&hn_read32(&memory,HN_CHOICE_RECORDS+24)==255);
    CHECK(!hn_read32(&memory,HN_CHOICE_RECORDS+4)&&!hn_read32(&memory,HN_CHOICE_RECORDS+28));
    CHECK(hn_read32(&memory,0x03000798)==0x0201D3FE);
    CHECK(!memory.fault);free(rom);reset();
}
static void following_contract(void)
{
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x900000);
    for(unsigned bank=0;bank<4;bank++){
        const GameAddr tables[2]={0x0858BF68,0x0858BFB8};
        for(unsigned type=0;type<2;type++){
            GameAddr family=0x02001000+bank*0x100+type*0x400;
            for(unsigned byte=0;byte<4;byte++)rom[tables[type]-HN_ROM_BASE+bank*4+byte]=(uint8_t)(family>>(byte*8));
            for(unsigned index=0;index<16;index++)hn_write32(&memory,family+index*4,0x02008000+type*0x1000+bank*0x100+index*4);
        }
    }
    const GameAddr slots=0x02003000,a=slots+10*104,b=slots+13*104;
    hn_write32(&memory,HN_RENDER_SLOTS,slots);hn_write8(&memory,HN_RENDER_INFO+16,32);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);
    hn_write16(&memory,HN_COLLISION_DIMENSIONS,64);hn_write16(&memory,HN_COLLISION_DIMENSIONS+2,64);
    hn_write32(&memory,HN_COLLISION_MAP,0x02010000);hn_write8(&memory,b+0x60,4);
    hn_write32(&memory,a+0x18,100u*65536+32768);hn_write32(&memory,a+0x20,100u*65536+32768);
    hn_write32(&memory,b+0x18,20u*65536+32768);hn_write32(&memory,b+0x20,100u*65536+32768);
    hn_write16(&memory,HN_ROUTE_POINTS,20);hn_write16(&memory,HN_ROUTE_POINTS+2,100);hn_write16(&memory,HN_ROUTE_POINTS+4,80);
    hn_write8(&memory,HN_MOTION_MODE,1);CHECK(hn_motion_follower(&memory));
    CHECK(hn_read32(&memory,b+0x24)==65536&&!hn_read32(&memory,b+0x2C));
    CHECK(hn_read32(&memory,b+4)==0x02008218&&hn_read8(&memory,b+0x60)==4);
    hn_write8(&memory,HN_MOTION_MODE,2);CHECK(hn_motion_follower(&memory));CHECK(hn_read32(&memory,b+0x24)==131072);
    CHECK(hn_read32(&memory,b+4)==0x0200821C);
    hn_write8(&memory,0x03002170,0);CHECK(hn_motion_update_following(&memory));CHECK(hn_read32(&memory,b+0x18)==20u*65536+32768);
    hn_write8(&memory,0x03002170,1);CHECK(hn_motion_update_following(&memory));
    CHECK(hn_read32(&memory,b+0x18)==21u*65536+32768&&!hn_read32(&memory,b+0x24));
    hn_write8(&memory,0x03002183,1);CHECK(hn_motion_update_following(&memory));
    CHECK(hn_read32(&memory,b+0x18)==23u*65536+32768&&hn_read8(&memory,HN_MOTION_MODE)==2);
    /* Idle animation waits for three updates; route rebuilding keeps fractional positions. */
    hn_write32(&memory,b+0x18,100u*65536);hn_write32(&memory,HN_FOLLOWER_IDLE_DELAY,0);
    CHECK(hn_motion_follower(&memory));CHECK(hn_read32(&memory,HN_FOLLOWER_IDLE_DELAY)==1&&hn_read32(&memory,b+4)==0x0200821C);
    CHECK(hn_motion_follower(&memory));CHECK(hn_read32(&memory,HN_FOLLOWER_IDLE_DELAY)==2);
    CHECK(hn_motion_follower(&memory));CHECK(hn_read32(&memory,HN_FOLLOWER_IDLE_DELAY)==0&&hn_read32(&memory,b+4)==0x0200830C);
    CHECK(!hn_read32(&memory,b+0x24)&&!hn_read32(&memory,b+0x2C));
    /* The native quantizer maps exactly -2 to -1, and preserves the original class. */
    hn_write32(&memory,b+0x24,0xFFFE0000);hn_write32(&memory,b+0x2C,0xFFFD0000);
    uint16_t x,z;uint32_t mode;CHECK(hn_motion_steps(&memory,&x,&z,&mode));CHECK(x==65535&&z==65534&&mode==1);
    hn_write32(&memory,b+0x24,0x80000000);hn_write32(&memory,b+0x2C,0);CHECK(hn_motion_direction(&memory,b)==2);
    /* A bend advances the circular tail and retains the prior segment's endpoint. */
    hn_write8(&memory,HN_ROUTE_END,31);GameAddr point=HN_ROUTE_POINTS+31*8;
    hn_write16(&memory,point,5);hn_write16(&memory,point+2,(uint16_t)-7);hn_write16(&memory,point+4,8);hn_write16(&memory,point+6,4);
    hn_write32(&memory,a+0x18,9u<<16);hn_write32(&memory,a+0x20,(uint32_t)-4<<16);
    CHECK(hn_motion_extend_route(&memory));CHECK(hn_read8(&memory,HN_ROUTE_END)==0);
    CHECK(hn_read16(&memory,HN_ROUTE_POINTS)==13&&hn_read16(&memory,HN_ROUTE_POINTS+2)==(uint16_t)-3);
    CHECK(hn_read16(&memory,HN_ROUTE_POINTS+4)==(uint16_t)-4&&hn_read16(&memory,HN_ROUTE_POINTS+6)==(uint16_t)-1);
    hn_write8(&memory,HN_RENDER_INFO+16,0);CHECK(!hn_motion_follower(&memory));CHECK(!hn_motion_extend_route(&memory));
    CHECK(!memory.fault);free(rom);reset();
}
static void collision_contract(void)
{
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x900000);
    const GameAddr slots=0x02003000,other=slots+104,record=0x02004000,map=0x02010000;
    hn_write32(&memory,HN_RENDER_SLOTS,slots);hn_write8(&memory,HN_RENDER_INFO+16,2);hn_write8(&memory,HN_RENDER_CLASS_LEADER,0);
    hn_write16(&memory,HN_COLLISION_DIMENSIONS,4);hn_write16(&memory,HN_COLLISION_DIMENSIONS+2,4);hn_write32(&memory,HN_COLLISION_MAP,map);
    /* Synthetic offset/mask tables exercise signed wrapping and a single edge. */
    rom[0x58ED50]=255;rom[0x58ED51]=255;rom[0x58ED38]=255;rom[0x58ED39]=255;rom[0x58ED48]=1;
    for(unsigned i=0;i<4;i++)rom[0x58ED31+i*2]=(uint8_t)(0x10u<<i);
    CHECK(hn_collision_sample(&memory,map,0,0,0,255)==map+6);
    CHECK(hn_collision_sample(&memory,map,-9,0,0,0)==map+4);CHECK(hn_collision_ahead(&memory,map,0,0,0)==map+4);
    CHECK(hn_collision_response_direction(1,0)==4&&hn_collision_response_direction(56,2)==1&&hn_collision_response_direction(63,0)==0);
    hn_write16(&memory,map,0x1000);CHECK(hn_collision_mask(&memory,map,0,0)==0x1000);
    hn_write8(&memory,slots+0x60,1);CHECK(hn_collision_flags(&memory,3,0));
    CHECK(hn_read8(&memory,0x03001FA8)==4&&hn_read8(&memory,0x03001F8C)==0);
    hn_write8(&memory,0x03000794,99);hn_collision_stage(&memory,12);
    CHECK(hn_read16(&memory,0x03001FBC)==12&&hn_read8(&memory,0x03001FA8)==6&&!hn_read8(&memory,0x03000794));
    hn_write16(&memory,map+6,0x0407);CHECK(hn_collision_scripts(&memory,0,0));CHECK(hn_read16(&memory,0x03001FBC)==7);
    hn_write16(&memory,record,1);hn_write16(&memory,record+2,42);hn_write16(&memory,record+4,1);hn_write16(&memory,record+6,2);
    hn_write16(&memory,record+12,24);hn_write16(&memory,record+14,24);hn_write8(&memory,other+0x60,1);
    hn_write32(&memory,other+0x18,100u*65536+32768);hn_write32(&memory,other+0x20,100u*65536+32768);
    uint32_t result;CHECK(hn_collision_contains(&memory,record,124,132,&result)&&result==1);
    hn_write32(&memory,slots+0x18,124u*65536);hn_write32(&memory,slots+0x20,132u*65536);
    CHECK(hn_collision_query(&memory,0,record,&result)&&result==0);
    hn_write32(&memory,slots+0x18,100u*65536+32768);hn_write32(&memory,slots+0x20,100u*65536+32768);
    CHECK(hn_collision_query(&memory,0,record,&result)&&result==42);CHECK(hn_collision_response(&memory,0,record));
    CHECK(hn_read32(&memory,slots+0x24)==24u*65536&&hn_read8(&memory,0x03001F8C)==4&&hn_read8(&memory,0x03002174)==1);
    hn_write32(&memory,slots+0x24,0);hn_write32(&memory,slots+0x2C,0);
    /* A generated cosine sample makes the circle's axial response exact. */
    rom[HN_RENDER_SINE_TABLE-HN_ROM_BASE+64*2+1]=1;hn_write16(&memory,record+4,0);hn_write16(&memory,record+12,10);
    hn_write32(&memory,slots+0x18,84u*65536+32768);hn_write32(&memory,slots+0x20,76u*65536+32768);
    CHECK(hn_collision_contains(&memory,record,110,100,&result)&&result==1);
    CHECK(hn_collision_query(&memory,0,record,&result)&&result==42);CHECK(hn_collision_response(&memory,0,record));
    CHECK(hn_read32(&memory,slots+0x24)==10u*65536&&!hn_read32(&memory,slots+0x2C)&&hn_read8(&memory,0x03001F8C)==3);
    hn_write32(&memory,0x030021F0,record);CHECK(hn_collision_has_record(&memory,100,100,&result)&&result==1);
    hn_write8(&memory,other+0x60,0);CHECK(hn_collision_has_record(&memory,100,100,&result)&&result==0);
    hn_write16(&memory,HN_KEY_STATE+2,2);hn_write8(&memory,0x030021E0,2);hn_motion_control(&memory);
    CHECK(hn_read8(&memory,0x03000785)==8&&hn_read8(&memory,0x03002C50)==1);
    hn_write16(&memory,HN_KEY_STATE+2,1);hn_write8(&memory,0x030021E0,8);hn_write8(&memory,0x03002C50,7);hn_motion_control(&memory);
    CHECK(!hn_read8(&memory,0x03000785)&&hn_read8(&memory,0x03002C50)==7);
    hn_write8(&memory,0x030021E0,4);hn_write32(&memory,HN_CHOICE_PENDING,0x12345678);
    hn_write8(&memory,0x03000018,1);hn_write8(&memory,0x03000010,0x21);hn_write8(&memory,HN_SCENE_CONTROLS+18,9);
    HNScript script;hn_script_init(&script,&memory,NULL,0);CHECK(hn_main_command(&script,0x21,0));
    CHECK(hn_read8(&memory,0x03001FA8)==16&&hn_read16(&memory,0x03001FBC)==0x5678&&!hn_read32(&memory,HN_CHOICE_PENDING));
    CHECK(!hn_read8(&memory,0x03000010)&&!hn_read32(&memory,0x03000014)&&!hn_read8(&memory,HN_SCENE_CONTROLS+18));
    CHECK(hn_read8(&memory,0x03000018)==1&&!(hn_read8(&memory,0x030021E0)&4));
    CHECK(!memory.fault);free(rom);reset();
}
static void indicator_contract(void)
{
    uint8_t *rom=calloc(1,0x700000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x700000);
    for(unsigned i=0;i<9;i++){
        rom[HN_SCENE_INDICATOR_POSITIONS-HN_ROM_BASE+i*4]=(uint8_t)(i*10);
        rom[HN_SCENE_INDICATOR_POSITIONS-HN_ROM_BASE+i*4+2]=(uint8_t)(i*20);
    }
    for(unsigned i=0;i<32;i++)hn_write8(&memory,0x03002180+i,255);
    for(unsigned i=1;i<=7;i++)CHECK(hn_scene_indicator_complete(&memory,i));
    hn_write8(&memory,0x03002180+0x9B/8,(uint8_t)(255&~(1u<<(0x9B&7))));
    CHECK(!hn_scene_indicator_complete(&memory,0xA5000001)&&hn_scene_indicator_complete(&memory,2));
    CHECK(!hn_scene_indicator_complete(&memory,0)&&!hn_scene_indicator_complete(&memory,8));
    CHECK(hn_scene_indicator_visible(&memory,255));rom[HN_SCENE_INDICATOR_MASKS-HN_ROM_BASE]=5;
    hn_write8(&memory,HN_SCENE_CONTROLS+0x16,1);
    CHECK(hn_scene_indicator_visible(&memory,0xA5000001)&&hn_scene_indicator_visible(&memory,3));
    CHECK(!hn_scene_indicator_visible(&memory,0)&&!hn_scene_indicator_visible(&memory,2)&&!hn_scene_indicator_visible(&memory,33));
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    CHECK(hn_scene_indicator_configure(&memory,1,1));GameAddr slot=hn_render_slot(&memory,15);
    CHECK(hn_read32(&memory,slot+4)==0x088F6FAC&&hn_read32(&memory,slot+0x18)==0x000A8000);
    CHECK(hn_read32(&memory,slot+0x1C)==0x00148000&&(hn_read8(&memory,slot+0x61)&12)==4);
    CHECK(hn_scene_indicator_configure(&memory,1,2)&&hn_read32(&memory,slot+4)==0x088F6FB4);
    CHECK(hn_scene_indicator_configure(&memory,1,0xA5000003));slot=hn_render_slot(&memory,18);
    CHECK(hn_read32(&memory,slot+4)==0x088F6FD6&&hn_read8(&memory,slot+0x60)==1);
    CHECK(hn_scene_indicator_configure(&memory,1,0)&&!hn_read8(&memory,slot+0x60)&&hn_read32(&memory,slot+4)==0x088F6FD6);
    CHECK(hn_scene_indicator_configure(&memory,1,255)&&hn_read32(&memory,slot+4)==0x088F6FD6);
    hn_write8(&memory,HN_RENDER_INFO+16,15);CHECK(!hn_scene_indicator_configure(&memory,1,1)&&!memory.fault);
    hn_write8(&memory,HN_RENDER_INFO+16,40);hn_write32(&memory,HN_RENDER_STATES,0x02018000);hn_write8(&memory,HN_BACKGROUND_COUNT,5);
    hn_write32(&memory,HN_SCENE_INDICATOR_BACKGROUNDS+4,0x08001000);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x6C,1);hn_write8(&memory,HN_SCENE_CONTROLS+0xE5,1);
    CHECK(hn_scene_indicator_update(&memory,1));
    CHECK(hn_read32(&memory,0x02018000+4*80+0x14)==0x08001000);
    CHECK(hn_read32(&memory,slot+4)==0x088F6FBC&&hn_read32(&memory,hn_render_slot(&memory,26)+4)==0x088F6FE8);
    uint32_t consumed=99;hn_write8(&memory,HN_SCENE_INDICATOR_CHANGED,255);
    CHECK(hn_scene_callback(&memory,0x080C1459,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,HN_SCENE_INDICATOR_PREVIOUS)==1&&!hn_read8(&memory,HN_SCENE_INDICATOR_CHANGED));
    hn_write8(&memory,0x03002188,0);hn_write8(&memory,0x030021B0,0x8A);
    CHECK(hn_scene_callback(&memory,0x080C1AC5,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,0x030021B0)==0x88&&hn_read8(&memory,0x03002188)==2);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,19)+4)==0x088F6FC4);
    CHECK(hn_scene_callback(&memory,0x080C1AC5,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,0x030021B0)==0x80);
    CHECK(hn_scene_callback(&memory,0x080C1AC5,0,&consumed)==HN_SCENE_OK&&!hn_read8(&memory,0x030021B0));
    CHECK(hn_read8(&memory,0x03002188)==0x8A&&hn_read32(&memory,hn_render_slot(&memory,25)+4)==0x088F6FC4);
    hn_write8(&memory,HN_SCENE_CONTROLS+0xE6,7);
    CHECK(hn_scene_callback(&memory,0x080C1B35,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0xE5)==7);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,26)+4)==0x088F6FF0);
    hn_write8(&memory,HN_SCENE_INDICATOR_PREVIOUS,99);hn_write8(&memory,HN_SCENE_INDICATOR_CHANGED,0);
    CHECK(hn_scene_callback(&memory,0x080C1491,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_INDICATOR_PREVIOUS)==99);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x16,0);hn_write8(&memory,0x03002188,3);
    hn_write32(&memory,HN_SCENE_INDICATOR_NAVIGATION,0x02022000);hn_write8(&memory,0x02022000+4+1,2);
    hn_write16(&memory,HN_KEY_STATE+2,HN_RIGHT|HN_LEFT);
    CHECK(hn_scene_callback(&memory,0x080C19FD,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x6C)==1&&!hn_read8(&memory,HN_SCENE_INDICATOR_CHANGED));
    CHECK(!memory.fault);free(rom);reset();
}
static void book_contract(void)
{
    reset();uint32_t consumed=99;GameAddr controls=HN_SCENE_CONTROLS;
    hn_write8(&memory,controls+0x76,255);
    CHECK(hn_scene_callback(&memory,0x080C2721,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,HN_BOOK_COUNT)==10&&!hn_read8(&memory,controls+0x76));
    CHECK(hn_read8(&memory,HN_BOOK_WORDS)==1&&hn_read8(&memory,HN_BOOK_WORDS+3)==4&&hn_read8(&memory,HN_BOOK_WORDS+9)==63);
    CHECK((hn_read8(&memory,0x03002188)&128)&&(hn_read8(&memory,0x03002190)&32));
    hn_write8(&memory,controls+0x76,1);hn_write8(&memory,controls+0x70,0xA5);
    CHECK(hn_scene_callback(&memory,0x080C2721,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_BOOK_COUNT)==10&&hn_read8(&memory,controls+0x76)==1&&hn_read8(&memory,controls+0x70)==0xA5);
    hn_write8(&memory,controls+0x76,86);
    CHECK(hn_scene_callback(&memory,0x080C2721,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_BOOK_COUNT)==11&&hn_read8(&memory,HN_BOOK_WORDS+10)==86&&!hn_read8(&memory,controls+0x76));
    CHECK(!hn_read8(&memory,controls+0x70)&&!hn_read8(&memory,controls+0x71)&&hn_read8(&memory,controls+0x72)==1);
    hn_write8(&memory,controls+0x22,86);
    CHECK(hn_scene_callback(&memory,0x080C2385,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x72)==1);
    hn_write8(&memory,controls+0x22,87);
    CHECK(hn_scene_callback(&memory,0x080C2385,0,&consumed)==HN_SCENE_RESOURCE&&!memory.fault);
    hn_write8(&memory,controls+0x72,0);
    CHECK(hn_scene_callback(&memory,0x080C2385,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x72)==1);
    uint8_t *rom=calloc(1,0x700000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x700000);
    for(unsigned i=0;i<11;i++)rom[HN_BOOK_CATEGORIES-HN_ROM_BASE+i]=(uint8_t)(i*8);
    hn_write8(&memory,0x0300219F,1);hn_write8(&memory,HN_BOOK_ORDER,34);hn_write8(&memory,controls+0x76,0x81);
    CHECK(hn_scene_callback(&memory,0x080C2721,0,&consumed)==HN_SCENE_OK);
    CHECK(!hn_read8(&memory,HN_BOOK_COUNT)&&hn_read8(&memory,controls+0x70)==4&&hn_read8(&memory,controls+0x71)==8&&hn_read8(&memory,controls+0x72)==3);
    hn_write8(&memory,0x03002188,128);
    CHECK(hn_scene_callback(&memory,0x080C28DD,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x22)==1);
    hn_write8(&memory,0x0300219F,0);hn_write8(&memory,HN_BOOK_COUNT,255);
    CHECK(hn_scene_callback(&memory,0x080C21E9,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x60)==64);
    hn_write8(&memory,0x0300219F,1);hn_write8(&memory,controls+0xEB,5);hn_write8(&memory,controls+0x60,0xA5);
    CHECK(hn_scene_callback(&memory,0x080C21E9,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x60)==0xA5);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02018000);hn_write8(&memory,HN_GRAPHICS_COUNT,8);
    for(unsigned i=0;i<10;i++)for(unsigned j=0;j<4;j++)rom[HN_BOOK_DIGITS-HN_ROM_BASE+i*4+j]=(uint8_t)((0x08000100u+i*8)>>(j*8));
    hn_write8(&memory,controls+0x60,42);hn_write8(&memory,0x02020000,10);hn_write8(&memory,0x02020001,20);hn_write8(&memory,0x02020002,32);
    CHECK(hn_scene_callback(&memory,0x080C22CD,0x02020000,&consumed)==HN_SCENE_OK&&consumed==3);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,10)+4)==0x08000120&&hn_read32(&memory,hn_render_slot(&memory,11)+4)==0x08000110);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,11)+0x18)==0x00198000);
    hn_write8(&memory,controls+0x72,15);
    CHECK(hn_scene_callback(&memory,0x080C2559,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,29)+0x1C)==0x8000);
    for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++){
        GameAddr table=i<4?HN_BOOK_GRAPHICS_FIRST:HN_BOOK_GRAPHICS_SECOND;
        rom[table-HN_ROM_BASE+(i&3)*4+j]=(uint8_t)((0x06004020u+i*0x400)>>(j*8));
    }
    hn_heap_init(&memory);hn_task_contexts_init(&memory);hn_write8(&memory,0x0300219F,0);hn_write8(&memory,HN_BOOK_COUNT,0);
    CHECK(hn_scene_callback(&memory,0x080C1D79,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,HN_BOOK_CONTEXTS)==hn_read32(&memory,HN_TASK_HEAD));
    CHECK(hn_scene_callback(&memory,0x080C2091,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,HN_TASK_HEAD)==HN_TASK_SENTINEL);
    CHECK(hn_scene_callback(&memory,0x080C1DC5,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_scene_callback(&memory,0x080C1F29,0,&consumed)==HN_SCENE_OK);
    GameAddr task=hn_read32(&memory,HN_TASK_HEAD);
    for(unsigned i=0;i<8;i++){
        CHECK(task==hn_read32(&memory,HN_BOOK_CONTEXTS+i*4)&&hn_read8(&memory,task)==16);
        task=hn_read32(&memory,task+0x2C);
    }
    CHECK(task==HN_TASK_SENTINEL);
    CHECK(hn_scene_callback(&memory,0x080C20A5,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,HN_TASK_HEAD)==HN_TASK_SENTINEL);
    CHECK(hn_scene_callback(&memory,0x080C20A5,0,&consumed)==HN_SCENE_RESOURCE&&!memory.fault);
    for(unsigned i=0;i<10;i++)hn_write32(&memory,HN_NUMBER_ANIMATIONS+i*4,0x08000300+i*16);
    CHECK(hn_render_number(&memory,5,0x12340068,0xFFFF0000u+65532,0));
    CHECK(hn_read32(&memory,hn_render_slot(&memory,5)+4)==0x08000310&&hn_read32(&memory,hn_render_slot(&memory,6)+4)==0x08000300);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,7)+4)==0x08000340&&hn_read32(&memory,hn_render_slot(&memory,7)+0x18)==0x000C8000);
    CHECK(hn_render_number(&memory,5,10,65532,0)&&hn_render_number(&memory,5,0,65532,0));
    CHECK(hn_read32(&memory,hn_render_slot(&memory,5)+4)==0x08000300&&hn_read32(&memory,hn_render_slot(&memory,5)+0x18)==0x000C8000);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,7)+4)==0x08000340); /* Unused trailing slots are retained. */
    CHECK(!hn_render_number(&memory,40,0,0,0)&&!memory.fault);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,1);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,2);hn_write8(&memory,controls+0x12,8);
    hn_write16(&memory,hn_render_slot(&memory,1)+0x50,70);hn_write16(&memory,hn_render_slot(&memory,8)+0x50,80);
    CHECK(hn_scene_callback(&memory,0x0801CDA5,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x60)==1);
    hn_write16(&memory,hn_render_slot(&memory,1)+0x50,63);
    CHECK(hn_scene_callback(&memory,0x0801CDA5,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x60)==2);
    hn_write8(&memory,controls+0x60,0xA5);
    CHECK(hn_scene_callback(&memory,0x0801CE29,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x03001FBC)==1&&hn_read8(&memory,controls+0x60)==0xA5);
    hn_write16(&memory,hn_render_slot(&memory,8)+0x50,71);
    CHECK(hn_scene_callback(&memory,0x0801CE29,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x03001FBC)==2);
    hn_write8(&memory,0x02020001,0x90);hn_write16(&memory,HN_BACKGROUND_BOUNDS+6,143);
    hn_write16(&memory,hn_render_slot(&memory,10)+0x22,70);
    CHECK(hn_scene_callback(&memory,0x0801C975,0x02020000,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x90)==2&&consumed==2);
    hn_write16(&memory,hn_render_slot(&memory,10)+0x22,71);
    CHECK(hn_scene_callback(&memory,0x0801C975,0x02020000,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x90)==1);
    hn_write16(&memory,HN_BACKGROUND_BOUNDS+6,65535);hn_write16(&memory,hn_render_slot(&memory,10)+0x22,65534);
    CHECK(hn_scene_callback(&memory,0x0801C975,0x02020000,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x90)==2);
    hn_write8(&memory,0x02020000,0x90);hn_write8(&memory,0x02020001,0x80);hn_write8(&memory,hn_render_slot(&memory,1)+0x60,1);
    hn_write32(&memory,controls+0x90,0xE7A5CD05);
    CHECK(hn_scene_callback(&memory,0x0801C9B5,0x02020000,&consumed)==HN_SCENE_OK&&hn_read32(&memory,controls+0x90)==0x00A50004);
    hn_write8(&memory,controls+0x90,0);
    CHECK(hn_scene_callback(&memory,0x0801C9B5,0x02020000,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x90)==255&&hn_read8(&memory,controls+0x92)==2);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,0);hn_write8(&memory,0x02020001,0x81);hn_write32(&memory,controls+0x90,0xE7A5CDAB);
    CHECK(hn_scene_callback(&memory,0x0801C9B5,0x02020000,&consumed)==HN_SCENE_OK&&hn_read32(&memory,controls+0x90)==0xE700CDAB);
    CHECK(!memory.fault);free(rom);reset();
}
static void actor_snapshot_contract(void)
{
    reset();const GameAddr slots=0x02010000;uint32_t consumed=99;
    hn_write32(&memory,HN_RENDER_SLOTS,slots);hn_write8(&memory,HN_RENDER_INFO+16,8);
    const GameAddr classes[]={HN_RENDER_CLASS_LEADER,0x03002274,HN_RENDER_CLASS_FOLLOWER,0x03002270};
    for(unsigned i=0;i<4;i++){
        hn_write8(&memory,classes[i],(uint8_t)i);
        for(unsigned j=0;j<104;j++)hn_write8(&memory,slots+i*104+j,(uint8_t)(i*16+j));
    }
    hn_write8(&memory,slots+2*104+96,0);
    CHECK(hn_scene_callback(&memory,0x080C1C41,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(!memcmp(memory.ewram+0x2EB0,memory.ewram+0x10000,416));
    memset(memory.ewram+0x10000,0xA5,416);
    CHECK(hn_scene_callback(&memory,0x080C1CBD,0,&consumed)==HN_SCENE_OK);
    CHECK(!hn_read8(&memory,slots+99)&&!hn_read8(&memory,slots+104+99)&&!hn_read8(&memory,slots+312+99));
    CHECK(hn_read8(&memory,slots+208)==0xA5&&hn_read8(&memory,slots+208+99)==0xA5);
    CHECK(hn_read32(&memory,slots+104)==0x13121110&&!memory.fault);
}
static void map_choice_contract(void)
{
    reset();uint32_t consumed=99,value=99,yield=0xA5B6C7D8;GameAddr args=0x02020000;
    CHECK(hn_map_choice_apply(&memory,0x08001001,&value)==HN_SCENE_UNKNOWN&&!memory.fault);
    CHECK(hn_scene_callback(&memory,0x0802F665,args,&consumed)==HN_SCENE_UNKNOWN); /* Void task preset. */
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02015000);hn_write8(&memory,HN_GRAPHICS_COUNT,16);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,1);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,2);
    GameAddr a=hn_render_slot(&memory,1),b=hn_render_slot(&memory,2);
    hn_write32(&memory,a+0x18,0);hn_write32(&memory,a+0x20,0);hn_write32(&memory,b+0x18,0x10000);hn_write32(&memory,b+0x20,0);
    CHECK(hn_render_direction_between(&memory,1,2,false)==4&&hn_render_direction_between(&memory,1,2,true)==3);
    hn_write32(&memory,b+0x18,0);hn_write32(&memory,b+0x20,0x10000);
    CHECK(hn_render_direction_between(&memory,1,2,false)==2&&hn_render_direction_between(&memory,1,2,true)==5);
    hn_write32(&memory,b+0x18,0xFFFF0000);hn_write32(&memory,b+0x20,0);
    CHECK(hn_render_direction_between(&memory,1,2,false)==3&&hn_render_direction_between(&memory,1,2,true)==7);
    hn_write32(&memory,b+0x18,0);hn_write32(&memory,b+0x20,0xFFFF0000);
    CHECK(hn_render_direction_between(&memory,1,2,false)==1&&hn_render_direction_between(&memory,1,2,true)==1);
    CHECK(!hn_render_direction_between(&memory,1,40,false)&&!memory.fault);
    hn_write16(&memory,HN_MAP_CHOICE_NEXT,0x4321);hn_write16(&memory,HN_MAP_CHOICE_FLAGS,0x0101);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A0,1);hn_write8(&memory,args,1);hn_write8(&memory,0x030021D6,1);
    CHECK(hn_scene_callback(&memory,0x0802F111,args,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x1A0)==1&&hn_read8(&memory,0x030021D6)==2);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A0,1);hn_write8(&memory,args,2);
    CHECK(hn_scene_callback(&memory,0x0802F111,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A0)==2);
    CHECK(hn_read8(&memory,0x030021D6)==2); /* A zero nibble retains the old 2B1 flag. */
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A0,1);hn_write8(&memory,args,255);
    CHECK(hn_scene_callback(&memory,0x0802F111,args,&consumed)==HN_SCENE_OK&&!hn_read8(&memory,HN_SCENE_CONTROLS+0x1A0));
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A0,12);
    CHECK(hn_scene_callback(&memory,0x0802F18D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_scene_callback(&memory,0x0802F18D,0,&consumed)==HN_SCENE_OK&&consumed==5);
    hn_write8(&memory,args,1);hn_write8(&memory,args+1,14);
    hn_write32(&memory,a+0x18,0x11223344);hn_write32(&memory,a+0x1C,0x55667788);hn_write32(&memory,a+0x20,0x99AABBCC);
    CHECK(hn_scene_callback(&memory,0x08030611,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_read32(&memory,0x03002BE8)==0x11223344&&hn_read32(&memory,0x03002BEC)==0x55667788&&hn_read32(&memory,0x03002BF0)==0x99AABBCC);
    hn_write8(&memory,args,2);CHECK(hn_scene_callback(&memory,0x08030651,args,&consumed)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,b+0x18)==0x11223344&&hn_read32(&memory,b+0x1C)==0x55667788&&hn_read32(&memory,b+0x20)==0x99AABBCC);
    hn_write8(&memory,args,1);hn_write32(&memory,args+1,0x08000101);hn_write32(&memory,args+5,0x02020100);
    hn_write8(&memory,a+0x60,1);hn_write32(&memory,a+4,0x08000101);
    CHECK(hn_scene_callback(&memory,0x08030739,args,&consumed)==HN_SCENE_OK&&consumed==0xFFFFFFFC&&hn_read32(&memory,0x03002C60)==0x02020100);
    hn_write8(&memory,a+0x60,0);CHECK(hn_scene_callback(&memory,0x08030739,args,&consumed)==HN_SCENE_OK&&consumed==9);
    hn_write32(&memory,a+0x18,0x8000);hn_write32(&memory,a+0x20,0x8000);hn_write16(&memory,args,1);hn_write16(&memory,args+2,1);
    CHECK(hn_scene_callback(&memory,0x08031451,args,&consumed)==HN_SCENE_OK&&consumed==4&&hn_read32(&memory,a+0x18)==0x10000&&!hn_read16(&memory,0x03001FBC));
    CHECK(hn_scene_callback(&memory,0x08031451,args,&consumed)==HN_SCENE_OK&&hn_read32(&memory,a+0x18)==0x18000&&!hn_read16(&memory,0x03001FBC));
    CHECK(hn_scene_callback(&memory,0x08031451,args,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x03001FBC)==1);
    hn_write32(&memory,a+0x18,0x688000);hn_write32(&memory,a+0x20,0x778000);
    CHECK(hn_map_choice_apply(&memory,0x08031745,&value)==HN_SCENE_OK&&value==1);
    hn_write32(&memory,a+0x18,0x687FFF);CHECK(hn_map_choice_apply(&memory,0x08031745,&value)==HN_SCENE_OK&&!value);
    hn_write32(&memory,a+0x18,0x688000);hn_write8(&memory,HN_MAP_CHOICE_PHASE,0);hn_write8(&memory,0x03002182,0);
    CHECK(hn_scene_animation_callback(&memory,0x08031121,a,0,&consumed,&yield)==HN_SCENE_OK&&consumed==0&&yield==1);
    CHECK(hn_read8(&memory,HN_MAP_CHOICE_PHASE)==1&&hn_read32(&memory,hn_render_slot(&memory,13)+4)==0x088B6FD7&&hn_read8(&memory,hn_render_slot(&memory,33)+0x65)==13);
    hn_write32(&memory,a+0x18,0);CHECK(hn_scene_animation_callback(&memory,0x08031121,a,0,&consumed,&yield)==HN_SCENE_OK);
    CHECK(!hn_read8(&memory,HN_MAP_CHOICE_PHASE)&&hn_read32(&memory,hn_render_slot(&memory,13)+4)==0x088B7318);
    hn_write8(&memory,0x03002C00,2);hn_write32(&memory,b+0x1C,0x12345678);hn_write32(&memory,b+0x20,0x87654321);yield=0xA5B6C7D8;
    CHECK(hn_scene_animation_callback(&memory,0x08032CE9,a,0,&consumed,&yield)==HN_SCENE_OK&&yield==0xA5B6C7D8);
    CHECK(hn_read32(&memory,b+0x1C)==0x87654321&&hn_read32(&memory,b+0x20)==0x12345678);
    hn_write32(&memory,b+0x1C,0x7FFF);CHECK(hn_map_choice_slot(&memory,0x08030121,b,&value)==HN_SCENE_OK&&hn_read32(&memory,b+0x1C)==0x27FFF);
    CHECK(hn_map_choice_slot(&memory,0x08030121,b,&value)==HN_SCENE_OK&&hn_read32(&memory,b+0x1C)==0x8000);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x60,2);hn_write8(&memory,HN_SCENE_CONTROLS+0x61,1);hn_write8(&memory,HN_SCENE_CONTROLS+0x62,31);
    hn_write32(&memory,a+0x18,0x18000);hn_write32(&memory,a+0x1C,0x8000);hn_write32(&memory,a+0x20,0x28000);
    hn_write32(&memory,b+0x18,0x8000);hn_write32(&memory,b+0x1C,0x8000);hn_write32(&memory,b+0x20,0x8000);
    CHECK(hn_scene_callback(&memory,0x0801CC79,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read32(&memory,b+0x24)==0x800&&hn_read32(&memory,b+0x2C)==0x1000&&!memory.fault);
}
static void particles_contract(void)
{
    reset();uint32_t consumed=99;hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write32(&memory,HN_RENDER_STATES,0x02016000);hn_write8(&memory,HN_SCENE_SHARED_STATE,1);hn_write8(&memory,HN_SCENE_SHARED_STATE+1,2);
    hn_write32(&memory,HN_SCENE_SHARED_STATE+4,0x02018000);hn_write32(&memory,HN_PARTICLE_INDICES,3);hn_write32(&memory,HN_PARTICLE_INDICES+4,4);
    for(unsigned i=0;i<2;i++){
        GameAddr p=0x02018000+i*28;hn_write32(&memory,p,16+i*16);hn_write32(&memory,p+4,i*16);
        hn_write32(&memory,p+8,0x10000);hn_write32(&memory,p+12,0xFFFF8000);hn_write32(&memory,p+16,0x800);hn_write32(&memory,p+20,0x400);hn_write32(&memory,p+24,0xFFFFFFFF);
    }
    CHECK(hn_particles_initialize(&memory));GameAddr a=hn_render_slot(&memory,3),b=hn_render_slot(&memory,4);
    CHECK(hn_read32(&memory,a+0x18)==0x8000&&hn_read32(&memory,a+0x1C)==0xFF008000&&hn_read32(&memory,a+0x20)==0x01008000);
    CHECK(hn_read32(&memory,a+0x24)==0x10000&&hn_read32(&memory,a+0x2C)==0xFFFF8000&&hn_read16(&memory,a+0x44)==65535);
    hn_write32(&memory,a+0x18,0xFFE00000);hn_write32(&memory,a+0x20,0x01F00000);hn_write32(&memory,b+0x18,0x01600001);
    hn_write32(&memory,b+0x30,0x12345678);
    CHECK(hn_scene_callback(&memory,0x0802EF35,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(!hn_read8(&memory,HN_SCENE_SHARED_STATE)&&hn_read32(&memory,a+0x18)==0xFFE00000&&hn_read32(&memory,a+0x20)==0x01F00000);
    CHECK(hn_read32(&memory,b+0x18)==0x00108000&&hn_read32(&memory,b+0x30)==0x12345678&&hn_read16(&memory,b+0x3C)==256);
    CHECK(hn_scene_callback(&memory,0x0802F009,0,&consumed)==HN_SCENE_OK&&!hn_read8(&memory,a+0x60)&&!hn_read8(&memory,b+0x60));
    CHECK(hn_read32(&memory,a+4)==0x088E6A70&&hn_read16(&memory,a+0x44)==65535&&!memory.fault);
    hn_write32(&memory,HN_PARTICLE_INDICES,40);CHECK(!hn_particles_initialize(&memory)&&!memory.fault);
}
static void preset_contract(void)
{
    reset();uint32_t consumed=99;
    CHECK(hn_preset_apply(&memory,0x08001001)==HN_SCENE_UNKNOWN&&!memory.fault);
    CHECK(hn_scene_callback(&memory,0x080CC8A9,0,&consumed)==HN_SCENE_UNKNOWN); /* Void ABI is not a Main return. */
    CHECK(hn_scene_callback(&memory,0x080CD105,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_preset_apply(&memory,0x080CC941)==HN_SCENE_RESOURCE&&!memory.fault);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02015000);hn_write8(&memory,HN_GRAPHICS_COUNT,8);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,19);hn_write8(&memory,0x03002C04,19);hn_write8(&memory,0x03002170,0xA5);
    CHECK(hn_preset_apply(&memory,0x0801536D)==HN_SCENE_OK&&hn_read8(&memory,0x03002170)==0xA5);
    GameAddr actor=hn_render_slot(&memory,19);
    CHECK(hn_read32(&memory,actor+4)==0x088CAB5E);
    CHECK(hn_preset_apply(&memory,0x0801542D)==HN_SCENE_OK&&hn_read8(&memory,0x03002170)==0);
    CHECK(hn_read32(&memory,actor+4)==0x088CACA8);
    GameAddr slot12=hn_render_slot(&memory,12),slot14=hn_render_slot(&memory,14);
    hn_write32(&memory,slot12+4,0x08000101);hn_write32(&memory,slot12+0x24,0x12345678);
    hn_write8(&memory,slot12+0x60,7);hn_write8(&memory,slot12+0x62,0xE3);hn_write8(&memory,slot14+0x60,9);
    GameAddr task=0x02019000,script=0x02020000;hn_write32(&memory,task+0x10,script);hn_write8(&memory,script,7);
    hn_write32(&memory,HN_MAP_WORDS+28,0x080CCF09);HNTasks tasks;hn_tasks_init(&tasks,&memory);uint32_t keep=0;
    CHECK(hn_task_command(&tasks,task,0xF4,&keep)&&keep==1&&hn_read32(&memory,task+0x10)==script+1);
    CHECK(!hn_read8(&memory,slot12+0x60)&&!hn_read8(&memory,slot14+0x60));
    CHECK(hn_read32(&memory,slot12+4)==0x08000101&&hn_read32(&memory,slot12+0x24)==0x12345678&&hn_read8(&memory,slot12+0x62)==0xE3);
    hn_write8(&memory,HN_SCENE_CONTROLS+0xC6,6);hn_write32(&memory,hn_render_slot(&memory,22)+4,0x08000111);
    CHECK(hn_preset_apply(&memory,0x080CCE79)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,22)+4)==0x08000111);
    hn_write8(&memory,HN_SCENE_CONTROLS+0xC6,7);
    CHECK(hn_preset_apply(&memory,0x080CCE79)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,22)+4)==0x088F4248);
    CHECK(hn_preset_apply(&memory,0x080CCF85)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,0x03001FAC)==19&&hn_read32(&memory,0x03001F7C)==0x088C5C60);
    CHECK(hn_preset_apply(&memory,0x080CCAF1)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,16)+0x18)==0x00288000&&hn_read32(&memory,hn_render_slot(&memory,25)+0x20)==0x00808000);
    CHECK(hn_read32(&memory,0x02015000+4*44+0x18)==0x088DB4C3&&hn_read32(&memory,0x02015000+5*44+0x18)==0x088D111A);
    hn_write32(&memory,0x03004ED4,0x80000000);
    CHECK(hn_scene_callback(&memory,0x080CC879,0,&consumed)==HN_SCENE_OK&&consumed==0&&hn_read8(&memory,HN_SCENE_CONTROLS)==1);
    hn_write32(&memory,0x03004ED4,0x7FFFFFFF);
    CHECK(hn_scene_callback(&memory,0x080CC879,0,&consumed)==HN_SCENE_OK&&!hn_read8(&memory,HN_SCENE_CONTROLS));
    reset();hn_write32(&memory,HN_RENDER_SLOTS,0x030026C0);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write8(&memory,0x03002C00,13);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,2);
    CHECK(hn_preset_apply(&memory,0x0801BAFD)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,0x03002C00)==1&&hn_read32(&memory,hn_render_slot(&memory,2)+4)==0x088D48D6);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,0)+0x65)==1&&hn_read8(&memory,hn_render_slot(&memory,2)+0x65)==1&&!memory.fault);
}
static void scene_motion_contract(void)
{
    reset();uint32_t consumed=99,yield=0x12345678;GameAddr args=0x02020000;
    CHECK(hn_scene_callback(&memory,0x0801ED51,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read16(&memory,0x0201DC84)==176&&hn_read16(&memory,0x0201DC86)==200);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+102)==16&&hn_read8(&memory,HN_SCENE_CONTROLS+103)==19&&hn_read8(&memory,HN_SCENE_CONTROLS+104)==1);
    CHECK(hn_scene_callback(&memory,0x0801F0FD,0,&consumed)==HN_SCENE_RESOURCE&&!memory.fault);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    CHECK(hn_scene_callback(&memory,0x0801F0FD,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,9)+0x62)==0xB0&&hn_read8(&memory,hn_render_slot(&memory,9)+0x65)==16);
    hn_write8(&memory,args,2);hn_write8(&memory,args+1,1);hn_write32(&memory,args+2,0xFEDCBA98);
    CHECK(hn_scene_callback(&memory,0x080B9B7D,args,&consumed)==HN_SCENE_OK&&consumed==6);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,2)+0x28)==0xFEDCBA98);
    hn_write8(&memory,args+1,255);CHECK(hn_scene_callback(&memory,0x080B9BCD,args,&consumed)==HN_SCENE_OK&&consumed==6);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,2)+0x1C)==0);
    hn_write8(&memory,args,128);hn_write8(&memory,args+1,1);
    CHECK(hn_scene_callback(&memory,0x080BA4D9,args,&consumed)==HN_SCENE_OK&&consumed==2&&hn_read16(&memory,HN_SCENE_BACKGROUND_OFFSET)==0xFF80);
    hn_write8(&memory,0x030021E0,0x80);
    CHECK(hn_scene_callback(&memory,0x080BA3C1,0x030021E0,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read8(&memory,0x030021E0)==0x84&&hn_read32(&memory,HN_CHOICE_PENDING)==0xFF80);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,1);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,2);
    GameAddr leader=hn_render_slot(&memory,1),follower=hn_render_slot(&memory,2),probe=hn_render_slot(&memory,26);
    hn_write32(&memory,leader+0x18,100u<<16);hn_write32(&memory,leader+0x20,100u<<16);
    hn_write32(&memory,follower+0x18,200u<<16);hn_write32(&memory,follower+0x20,100u<<16);
    hn_write32(&memory,probe+0x18,150u<<16);hn_write32(&memory,probe+0x20,100u<<16);
    CHECK(hn_scene_callback(&memory,0x080B9F15,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_SHARED_STATE)==3);
    hn_write32(&memory,probe+0x18,72u<<16);
    CHECK(hn_scene_callback(&memory,0x080B9F15,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_SHARED_STATE)==0);
    hn_write32(&memory,probe+0x18,320u<<16);hn_write32(&memory,probe+0x20,217u<<16);
    CHECK(hn_scene_callback(&memory,0x080B9F15,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_SHARED_STATE)==4);
    hn_write16(&memory,HN_SCENE_BACKGROUND_OFFSET,64);hn_write8(&memory,HN_SCENE_SHARED_STATE+2,0);
    CHECK(hn_scene_callback(&memory,0x080BA429,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,HN_SCENE_BACKGROUND_OFFSET)==66&&hn_read8(&memory,HN_SCENE_SHARED_STATE+2)==1);
    hn_write16(&memory,HN_SCENE_BACKGROUND_OFFSET,0xFFE1);
    CHECK(hn_scene_callback(&memory,0x080BA489,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_SHARED_STATE)==1);
    GameAddr anchor=hn_render_slot(&memory,30);
    hn_write16(&memory,HN_SCENE_BACKGROUND_OFFSET,20);
    hn_write16(&memory,HN_VIDEO_STATE+0x3A,10);hn_write16(&memory,HN_VIDEO_STATE+0x3C,10);
    hn_write16(&memory,HN_VIDEO_STATE+0x6A,100);hn_write16(&memory,HN_VIDEO_STATE+0x6C,60);
    hn_write32(&memory,anchor+0x18,0x00C88000);hn_write32(&memory,anchor+0x1C,0x00148000);hn_write32(&memory,anchor+0x20,0x012C8000);
    CHECK(hn_scene_animation_callback(&memory,0x080B9C65,0,args,&consumed,&yield)==HN_SCENE_OK&&consumed==0&&yield==1);
    CHECK(hn_read32(&memory,anchor+0x18)==0x00398000&&hn_read32(&memory,anchor+0x20)==0x00A98000);
    CHECK(hn_scene_callback(&memory,0x080BA515,0,&consumed)==HN_SCENE_OK);
    GameAddr slot37=hn_render_slot(&memory,37);hn_write32(&memory,slot37+0x18,0x12345678);
    CHECK(hn_scene_callback(&memory,0x080BA515,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,slot37+0x18)==0x12345678);
    hn_write8(&memory,HN_CHOICE_STATE,7);CHECK(hn_scene_callback(&memory,0x080BA55D,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x1AF)==7&&!memory.fault);
    uint8_t *rom=calloc(1,0x1000000);CHECK(rom!=NULL);rom[HN_SCENE_CLONE_PAIRS-0x08000000]=3;rom[HN_SCENE_CLONE_PAIRS-0x08000000+2]=4;
    memory.rom=rom;memory.rom_size=0x1000000;
    CHECK(hn_scene_callback(&memory,0x080BA0F9,0,&consumed)==HN_SCENE_OK);
    hn_write16(&memory,probe+0x4C,5);hn_write32(&memory,probe+0x30,0x11223344);
    CHECK(hn_scene_callback(&memory,0x080BA1FD,0,&consumed)==HN_SCENE_OK);
    GameAddr copy=hn_render_slot(&memory,3);CHECK(hn_read32(&memory,copy)==0x088976AC&&hn_read32(&memory,copy+4)==0x088F4BE3);
    CHECK(hn_read32(&memory,copy+0x30)==0x11223344&&hn_read32(&memory,probe+0x1C)==0x00108000);
    CHECK(hn_read8(&memory,HN_SCENE_SHARED_STATE+4)==1&&hn_read8(&memory,HN_SCENE_SHARED_STATE+5)==1);
    hn_write8(&memory,HN_SCENE_SHARED_STATE+4,7);hn_write8(&memory,HN_SCENE_SHARED_STATE+5,0);
    CHECK(hn_scene_callback(&memory,0x080BA1FD,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_SHARED_STATE+5)==0);
    hn_write8(&memory,HN_SCENE_SHARED_STATE+4,0);
    CHECK(hn_scene_callback(&memory,0x080BA1FD,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_SHARED_STATE+5)==255);
    free(rom);
}
static void runtime_contract(void)
{
    HNGame *game=malloc(sizeof(*game));CHECK(game!=NULL);
    CHECK(!hn_game_initialize(game,NULL,0,NULL));CHECK(game->fault==HN_GAME_INITIALIZATION&&!game->initialized);
    CHECK(!hn_game_frame(game,HN_A)&&!game->frames);
    uint8_t *rom=calloc(1,0x1000000);CHECK(rom!=NULL);
    for(unsigned i=0;i<2048;i++)rom[0xFC+i]=(uint8_t)(i*17+3);
    hn_memory_init(&game->memory,rom,0x1000000);
    CHECK(hn_game_hardware_initialize(&game->memory));
    CHECK(!memcmp(game->memory.iwram+0x2280,rom+0xFC,2048));
    CHECK(hn_read32(&game->memory,0x03007FFC)==0x03002280);
    CHECK(!hn_game_initialize(game,rom,0x1000000,NULL));
    CHECK(game->fault!=HN_GAME_OK&&!game->initialized&&!game->frames);
    CHECK(!hn_game_frame(game,0));
    /* A pending unsupported IRQ fails before running Main, and remains sticky. */
    game->memory.fault=false;game->fault=HN_GAME_OK;game->initialized=true;
    hn_write16(&game->memory,0x04000200,4);hn_write16(&game->memory,0x04000208,1);
    game->memory.io[0x202]=4;
    CHECK(!hn_game_frame(game,0));CHECK(game->fault==HN_GAME_IRQ&&game->opcode==4);
    CHECK(!game->frames&&!hn_game_frame(game,HN_START));
    free(rom);free(game);
}
int main(void)
{
    dma_contract();transfer_contract();video_contract();map_contract();expression_contract();save_contract();scene_contract();
    text_contract();panel_contract();actor_contract();counter_contract();motion_contract();task_contract();choice_contract();following_contract();collision_contract();indicator_contract();book_contract();actor_snapshot_contract();preset_contract();map_choice_contract();particles_contract();scene_motion_contract();runtime_contract();
    printf("%u native game subsystem checks passed.\n",checks);return 0;
}
