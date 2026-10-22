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
#include "game/bank2.h"
#include "game/bank3.h"
#include "game/bank4.h"
#include "game/bank5.h"
#include "game/bank6.h"
#include "game/save_group3.h"
#include "game/record_menu.h"
#include "game/audio.h"
#include "game/wander.h"
#include "game/scene_actor.h"
#include "game/records.h"
#include "game/counter_scene.h"
#include "game/raster.h"
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
    hn_write16(&memory,0x02030000,0x2211);hn_write16(&memory,0x02030002,0x4433);
    CHECK(hn_transfer_strided_rows(&memory,0x02030000,0x02030100,0xABCD0002,0xCAFE0002,0xBEEF0008));
    CHECK(hn_read16(&memory,0x02030100)==0x2211&&hn_read16(&memory,0x02030108)==0x4433);
    CHECK(hn_transfer_strided_rows(&memory,0,0,3,0,123)&&!memory.fault);
    hn_heap_init(&memory);hn_write32(&memory,0x02030000,0x410);hn_write8(&memory,0x02030004,0);
    hn_write32(&memory,0x02030005,0x44332211);
    CHECK(hn_transfer_decompress_rows(&memory,0x02030000,0x02030100,2,2,8));
    CHECK(hn_read16(&memory,0x02030100)==0x2211&&hn_read16(&memory,0x02030108)==0x4433);
    hn_write32(&memory,0x03002E04,0x030010C8);
    CHECK(!hn_transfer_decompress_rows(&memory,0x02030000,0x02030100,2,2,8)&&!memory.fault);
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
static void put16(uint8_t *p,uint16_t v)
{p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
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
static void bank2_contract(void)
{
    reset();uint32_t consumed=99,yield=0xA5B6C7D8;GameAddr args=0x02020000;
    CHECK(hn_bank2_apply(&memory,0x08001001)==HN_SCENE_UNKNOWN&&!memory.fault);
    CHECK(hn_scene_callback(&memory,0x0802D369,args,&consumed)==HN_SCENE_UNKNOWN); /* Void task contract. */
    CHECK(hn_bank2_apply(&memory,0x0802D839)==HN_SCENE_UNKNOWN); /* Requires script arguments. */
    CHECK(hn_scene_callback(&memory,0x08043119,args,&consumed)==HN_SCENE_UNKNOWN); /* Main51 only. */
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02015000);hn_write8(&memory,HN_GRAPHICS_COUNT,16);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);hn_write8(&memory,0x03002C04,16);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x3A,5);hn_write8(&memory,0x030021AD,0);
    CHECK(hn_scene_callback(&memory,0x0804B3DD,args,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x1D3)==3);
    hn_write8(&memory,0x030021AD,0x80);
    CHECK(hn_scene_callback(&memory,0x0804B3DD,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1D3)==1);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x3A,3);
    CHECK(hn_scene_callback(&memory,0x0804B3DD,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1D3)==2);
    for(unsigned i=23;i<=27;i++){
        GameAddr slot=hn_render_slot(&memory,i);hn_write32(&memory,slot+0x1C,0xFFE80000);hn_write8(&memory,slot+0x60,1);
    }
    GameAddr last=hn_render_slot(&memory,27);hn_write32(&memory,last+0x20,1);
    CHECK(hn_scene_callback(&memory,0x0804B361,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,last+0x60)==1);
    hn_write32(&memory,last+0x20,0);
    CHECK(hn_scene_callback(&memory,0x0804B361,args,&consumed)==HN_SCENE_OK);
    for(unsigned i=23;i<=27;i++)CHECK(hn_read8(&memory,hn_render_slot(&memory,i)+0x60)==0);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,16)+4)==0x088CE7CA);
    GameAddr actor=hn_render_slot(&memory,16);hn_write32(&memory,actor+0x20,0xFFD80000);
    CHECK(hn_scene_animation_callback(&memory,0x0804B43D,0,args,&consumed,&yield)==HN_SCENE_OK&&yield==1&&consumed==0);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==0);
    hn_write32(&memory,actor+0x20,0xFFD7FFFF);
    CHECK(hn_scene_animation_callback(&memory,0x0804B43D,0,args,&consumed,&yield)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==1);
    hn_write16(&memory,0x0201DC4C,3277);
    CHECK(hn_scene_callback(&memory,0x0802DD9D,args,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC4C)==4); /* Halfword wrap precedes clamp. */
    hn_write16(&memory,0x0201DC4C,50);
    CHECK(hn_scene_callback(&memory,0x0802DD9D,args,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC4C)==999);
    hn_write8(&memory,args,22);hn_write16(&memory,args+1,0);hn_write16(&memory,args+3,65535);hn_write16(&memory,0x0201DF8C,3);
    CHECK(hn_scene_callback(&memory,0x0802D839,args,&consumed)==HN_SCENE_OK&&consumed==5&&hn_read16(&memory,0x0201DF8C)==0);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,26)+0x18)==0xFFF88000);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,27)+0x18)==0x00008000);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,28)+0x18)==0x00388000);
    hn_write8(&memory,args,26);hn_write8(&memory,args+1,27);
    hn_write8(&memory,hn_render_slot(&memory,26)+0x67,0xA5);
    CHECK(hn_scene_callback(&memory,0x0802E8A9,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(!memcmp(memory.ewram+(hn_render_slot(&memory,26)-0x02000000),memory.ewram+(hn_render_slot(&memory,27)-0x02000000),104));
    HNScript script;hn_script_init(&script,&memory,NULL,0);hn_write32(&memory,args,0x08043119);
    CHECK(hn_main_command(&script,0x51,args)&&hn_read32(&memory,0x03000014)==4);
    hn_write32(&memory,args,0);hn_write32(&memory,0x03002BB0,0x08043119);
    CHECK(!hn_main_command(&script,0x51,args)&&script.fault==HN_SCRIPT_CALLBACK&&script.fault_address==0x03002BB0);
    CHECK(hn_read32(&memory,0x030005E8)==0x08043119&&!memory.fault);
}
static void wander_contract(void)
{
    reset();uint32_t consumed=99,yield=0xA5B6C7D8,blocked=99;GameAddr args=0x02020000,context=HN_WANDER_CONTEXTS;
    uint8_t *rom=calloc(1,0x600000);CHECK(rom!=NULL);memory.rom=rom;memory.rom_size=0x600000;
    rom[HN_WANDER_STOP_FLAGS-0x08000000]=20;rom[HN_WANDER_STOP_FLAGS-0x08000000+2]=21;
    for(unsigned i=0;i<4;i++)rom[HN_TURN_DIRECTIONS-0x08000000+i]=(uint8_t)(i*2);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);GameAddr slot=hn_render_slot(&memory,16),other=hn_render_slot(&memory,17);
    hn_write32(&memory,hn_render_slot(&memory,10)+0x18,0x03008000);
    hn_write32(&memory,hn_render_slot(&memory,10)+0x20,0x03008000);
    for(unsigned i=0;i<4;i++)hn_write16(&memory,args+i*2,(uint16_t)(i*100));
    hn_write16(&memory,args+8,0x0100);hn_write8(&memory,args+10,1);hn_write8(&memory,args+11,1);
    hn_write8(&memory,args+14,16);hn_write16(&memory,context+24,0xA55A);hn_write8(&memory,context+35,0x5A);
    CHECK(hn_scene_animation_callback(&memory,0x0801E45D,slot,args,&consumed,&yield)==HN_SCENE_OK&&consumed==16&&yield==0);
    CHECK(hn_read32(&memory,context)==0x00008000&&hn_read32(&memory,context+12)==0x012C8000);
    CHECK(hn_read32(&memory,context+16)==0x10000&&hn_read32(&memory,context+20)==0);
    CHECK(hn_read16(&memory,context+24)==0xA55A&&hn_read8(&memory,context+35)==0x5A);
    CHECK(hn_scene_callback(&memory,0x0801E45D,args,&consumed)==HN_SCENE_UNKNOWN);
    hn_write8(&memory,args+14,19);CHECK(hn_wander_initialize(&memory,args)&&hn_read8(&memory,context+36+30)==19);
    hn_write8(&memory,args,23);yield=0xA5B6C7D8;
    CHECK(hn_scene_animation_callback(&memory,0x0801E4E5,slot,args,&consumed,&yield)==HN_SCENE_OK&&consumed==1&&yield==0xA5B6C7D8);
    hn_wander_velocity(&memory,slot,INT32_MIN,0);
    CHECK(hn_read32(&memory,slot+0x24)==0&&hn_read32(&memory,slot+0x2C)==0x80000000);
    hn_wander_velocity(&memory,slot,0x10000,255);CHECK(hn_read32(&memory,slot+0x2C)==0x80000000);
    hn_write32(&memory,context+8,0x00C88000);hn_write32(&memory,context+12,0x00C88000);
    hn_write32(&memory,slot+0x18,0x00C88000);hn_write32(&memory,slot+0x20,0x00808000);
    hn_write32(&memory,slot+0x24,1);hn_write32(&memory,slot+0x2C,1234);
    CHECK(hn_wander_bounds(&memory,context,slot,255,&blocked)&&blocked==1);
    CHECK(hn_read32(&memory,slot+0x24)==0&&hn_read32(&memory,slot+0x2C)==1234);
    hn_write32(&memory,context+8,0x7FFFFFFF);hn_write32(&memory,context+12,0x7FFFFFFF);
    hn_write32(&memory,slot+0x18,0x00640000);hn_write32(&memory,slot+0x20,0x00D00001);
    hn_write32(&memory,slot+0x24,1);hn_write8(&memory,HN_SCENE_CONTROLS+0xAD,1);
    hn_write32(&memory,0x03002BB0,0x08032C81);
    CHECK(hn_wander_bounds(&memory,context,slot,255,&blocked)&&blocked==1);
    CHECK(hn_read32(&memory,slot+0x24)==0&&hn_read32(&memory,slot+0x2C)==0);
    hn_write32(&memory,0x03002BB0,0x08001001);
    CHECK(!hn_wander_bounds(&memory,context,slot,255,&blocked)&&!memory.fault);
    hn_write8(&memory,HN_SCENE_CONTROLS+0xAD,0);hn_write8(&memory,args,16);
    hn_write8(&memory,context+32,3);CHECK(!hn_wander_update(&memory,slot,args,&yield)); /* GBA spins on this invalid state. */
    hn_write8(&memory,context+32,0);hn_write32(&memory,slot+0x18,0x00808000);hn_write32(&memory,slot+0x20,0x00808000);
    hn_write32(&memory,context+20,0);
    CHECK(hn_wander_update(&memory,slot,args,&yield)&&yield==1&&hn_read8(&memory,context+32)==1);
    CHECK(hn_read32(&memory,context+20)==0x80000&&hn_read8(&memory,slot+0x60)==1);
    hn_write8(&memory,context+30,16);hn_write8(&memory,context+31,0);hn_write8(&memory,slot+0x60,2);
    hn_write32(&memory,HN_WANDER_IDLE+4,0x12345678);hn_write8(&memory,other+0x60,4);
    hn_write8(&memory,0x03002182,0x10);
    CHECK(hn_wander_update(&memory,other,args,&yield));
    CHECK(hn_read8(&memory,other+0x60)==0&&hn_read8(&memory,slot+0x60)==2);
    CHECK(hn_read32(&memory,slot+4)==0x12345678&&hn_read8(&memory,context+32)==0&&hn_read32(&memory,context+20)==0);
    hn_write32(&memory,HN_WANDER_IDLE-4,0x23456789);hn_write8(&memory,0x03002182,0x10);
    CHECK(hn_wander_update(&memory,slot,args,&yield));
    CHECK(hn_read8(&memory,slot+0x60)==0&&hn_read32(&memory,slot+4)==0x23456789); /* Original signed -1 table column. */
    hn_write8(&memory,args+1,1);hn_write8(&memory,slot+0x60,3);
    hn_write32(&memory,HN_WANDER_TURNS+32+4*4,0x34567890);
    CHECK(hn_scene_callback(&memory,0x0801DA39,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_read32(&memory,slot+4)==0x34567890&&hn_read8(&memory,slot+0x60)==3);
    for(unsigned offset=0x24;offset<=0x38;offset+=4)CHECK(hn_read32(&memory,slot+offset)==0);
    for(unsigned i=0;i<16;i++)rom[HN_TURN_NEXT-0x08000000+i]=2; /* Synthetic turn table. */
    hn_write8(&memory,args,16);hn_write8(&memory,args+1,10);hn_write8(&memory,args+2,1);
    hn_write8(&memory,slot+0x60,1);hn_write32(&memory,slot+0x18,0x00808000);hn_write32(&memory,slot+0x20,0x00808000);
    hn_write32(&memory,hn_render_slot(&memory,10)+0x18,0x00808000);hn_write32(&memory,hn_render_slot(&memory,10)+0x20,0x00818000);
    hn_write32(&memory,HN_WANDER_TURNS+32+2*4,0x456789AB);
    CHECK(hn_scene_animation_callback(&memory,0x0801E719,slot,args,&consumed,&yield)==HN_SCENE_OK&&consumed==3&&yield==0);
    CHECK(hn_read8(&memory,HN_ACTOR_TURN_CONTROL+1)==16&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL+2)==10);
    CHECK(hn_scene_animation_callback(&memory,0x0801E741,slot,args,&consumed,&yield)==HN_SCENE_OK&&consumed==0&&yield==1);
    CHECK(hn_read8(&memory,HN_ACTOR_TURN_CONTROL)==1&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL+4)==4);
    CHECK(hn_read8(&memory,slot+0x60)==2&&hn_read32(&memory,slot+4)==0x456789AB);
    for(unsigned i=0;i<4;i++)CHECK(hn_wander_turn_update(&memory));
    CHECK(hn_read8(&memory,HN_ACTOR_TURN_CONTROL)==0&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL+4)==0);
    CHECK(hn_wander_turn_update(&memory)&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL)==2);
    hn_write32(&memory,hn_render_slot(&memory,10)+0x20,0x00808000-0x10000);
    CHECK(hn_wander_turn_update(&memory)&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL)==0);
    hn_write8(&memory,HN_ACTOR_TURN_CONTROL,255);hn_write8(&memory,HN_ACTOR_TURN_CONTROL+4,0);
    CHECK(hn_wander_turn_update(&memory)&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL)==255&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL+4)==0);
    hn_write8(&memory,HN_ACTOR_TURN_CONTROL,1);
    CHECK(hn_wander_turn_update(&memory)&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL+4)==255);
    hn_write8(&memory,args,16);hn_write8(&memory,args+1,255);hn_write8(&memory,args+2,255);
    hn_write32(&memory,slot+0x18,0xFFFE8000);hn_write32(&memory,slot+0x1C,0x00078000);hn_write32(&memory,slot+0x20,0x80008000);
    CHECK(hn_scene_callback(&memory,0x0801C51D,args,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read16(&memory,0x0201DC40)==0xFFFE&&hn_read16(&memory,0x0201DC42)==0x8000&&hn_read32(&memory,0x03001134)==1);
    CHECK(hn_scene_callback(&memory,0x0801C54D,args,&consumed)==HN_SCENE_OK&&consumed==3);
    CHECK(hn_read16(&memory,0x0201DC84)==0x00FD&&hn_read16(&memory,0x0201DC86)==0x80FF&&hn_read32(&memory,0x03001134)==3);
    CHECK(hn_scene_callback(&memory,0x0801C595,args,&consumed)==HN_SCENE_OK&&consumed==1&&hn_read16(&memory,0x0201DC40)==7);
    CHECK(hn_read32(&memory,0x03001134)==3); /* This reader retains the shared count. */
    hn_write32(&memory,slot+0x18,0);hn_write32(&memory,slot+0x20,0);
    hn_write32(&memory,other+0x18,0);hn_write32(&memory,other+0x20,0x10000);hn_write8(&memory,args+1,17);
    CHECK(hn_scene_callback(&memory,0x0801C5B5,args,&consumed)==HN_SCENE_OK&&consumed==2&&hn_read32(&memory,0x03001134)==2);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS)==2&&hn_read8(&memory,HN_SCENE_CONTROLS+1)==1);
    hn_write8(&memory,HN_SCENE_CONTROLS,16);hn_write8(&memory,HN_SCENE_CONTROLS+1,17);
    CHECK(hn_scene_callback(&memory,0x0801C5F5,0,&consumed)==HN_SCENE_OK&&consumed==0&&hn_read32(&memory,0x03001134)==0);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS)==2&&hn_read8(&memory,HN_SCENE_CONTROLS+1)==1);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x12,17);
    CHECK(hn_scene_callback(&memory,0x0801C635,args,&consumed)==HN_SCENE_OK&&consumed==1);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x17,17);
    CHECK(hn_scene_callback(&memory,0x0801C651,args,&consumed)==HN_SCENE_OK&&consumed==1);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);
    hn_write32(&memory,hn_render_slot(&memory,10)+0x18,0);hn_write32(&memory,hn_render_slot(&memory,10)+0x20,0);
    hn_write32(&memory,hn_render_slot(&memory,13)+0x18,0);hn_write32(&memory,hn_render_slot(&memory,13)+0x20,0);
    hn_write8(&memory,args,17);hn_write8(&memory,0x03002170,0xA5);
    hn_write32(&memory,HN_WANDER_TURNS+4*4,0x6789ABCD);hn_write32(&memory,HN_WANDER_TURNS+32+4*4,0x789ABCDE);
    CHECK(hn_scene_callback(&memory,0x0801C6BD,args,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,10)+4)==0x6789ABCD&&hn_read32(&memory,hn_render_slot(&memory,13)+4)==0x789ABCDE);
    CHECK(hn_read8(&memory,0x03002170)==0);
    GameAddr turn=HN_SCENE_CONTROLS+0x70;hn_write8(&memory,args,0x70);
    hn_write8(&memory,turn,6);hn_write8(&memory,turn+1,1);hn_write8(&memory,turn+2,0xA5);hn_write8(&memory,turn+3,99);
    CHECK(hn_scene_callback(&memory,0x0801CA3D,args,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read8(&memory,turn)==5&&hn_read8(&memory,turn+2)==0xA5&&hn_read8(&memory,turn+3)==0);
    hn_write8(&memory,turn,0);hn_write8(&memory,turn+1,7);
    CHECK(hn_scene_callback(&memory,0x0801CA3D,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,turn)==255&&hn_read8(&memory,turn+2)==1);
    hn_write8(&memory,turn,2);hn_write8(&memory,turn+1,0);hn_write8(&memory,turn+2,1);hn_write8(&memory,turn+3,0);
    hn_write8(&memory,args+1,16);hn_write8(&memory,args+2,1);
    hn_write32(&memory,HN_WANDER_TURNS+32+4,0x778899AA);hn_write8(&memory,0x03002182,128);
    CHECK(hn_scene_callback(&memory,0x0801CA75,args,&consumed)==HN_SCENE_OK&&consumed==3);
    CHECK(hn_read8(&memory,turn+1)==1&&hn_read8(&memory,turn+3)==4&&hn_read32(&memory,slot+4)==0x778899AA);
    CHECK((hn_read8(&memory,slot+0x61)&64)==0&&(hn_read8(&memory,0x03002182)&128)==0); /* Two original setters. */
    CHECK(hn_scene_callback(&memory,0x0801CA75,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,turn+3)==3);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,0);hn_write8(&memory,args+1,0x81);hn_write8(&memory,turn+2,2);
    CHECK(hn_scene_callback(&memory,0x0801CA75,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,turn+2)==0&&hn_read8(&memory,turn+3)==3);
    hn_write8(&memory,args,16);hn_write8(&memory,args+1,2);hn_write8(&memory,slot+0x61,255);
    CHECK(hn_scene_callback(&memory,0x0801CB25,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,slot+0x61)==127);
    hn_write8(&memory,args+1,3);
    CHECK(hn_scene_callback(&memory,0x0801CB25,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,slot+0x61)==255);
    hn_write8(&memory,args,255);hn_write32(&memory,HN_VIDEO_STATE,0x80000000);
    CHECK(hn_scene_callback(&memory,0x0801CB39,args,&consumed)==HN_SCENE_OK&&consumed==1&&hn_read32(&memory,HN_VIDEO_STATE)==0x8000003C);
    GameAddr negative=0x02010000-104;hn_write8(&memory,args,255);hn_write8(&memory,args+1,16);
    for(unsigned i=0;i<3;i++)hn_write32(&memory,negative+0x24+i*4,(i+1)*0x11111111);
    hn_write32(&memory,slot+0x30,1234);
    CHECK(hn_scene_callback(&memory,0x0801CB49,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_read32(&memory,slot+0x24)==0x11111111&&hn_read32(&memory,slot+0x28)==0x22222222&&hn_read32(&memory,slot+0x2C)==0x33333333);
    CHECK(hn_read32(&memory,slot+0x30)==1234);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x61,16);hn_write8(&memory,HN_SCENE_CONTROLS+0x62,255);
    hn_write16(&memory,0x0201DC84,256);hn_write16(&memory,0x0201DC86,256);
    hn_write32(&memory,slot+0x18,0);hn_write32(&memory,slot+0x1C,0);hn_write32(&memory,slot+0x20,0);
    CHECK(hn_scene_callback(&memory,0x0801CCE5,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read32(&memory,slot+0x24)==0x10000&&hn_read32(&memory,slot+0x2C)==0x10000&&hn_read32(&memory,slot+0x28)==0x22222222);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x68,2);hn_write8(&memory,HN_SCENE_CONTROLS+0x69,0);
    CHECK(hn_scene_callback(&memory,0x0801CB7D,0,&consumed)==HN_SCENE_OK&&consumed==0&&hn_read8(&memory,HN_SCENE_CONTROLS+0x69)==255);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x69,1);
    CHECK(hn_scene_callback(&memory,0x0801CB7D,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x68)==3);
    hn_write8(&memory,0x0200086E,0x64);hn_write8(&memory,0x0200086F,0x81);hn_write8(&memory,0x02000870,1);
    hn_write8(&memory,hn_render_slot(&memory,0)+0x60,0xA7);
    CHECK(hn_scene_callback(&memory,0x0801CB7D,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x68)==4);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,0)+0x60)==0xA2); /* Original zero-follower route. */
    GameAddr aliased_base=HN_ACTOR_TURN_CONTROL+1-16*104-0x60;
    hn_write32(&memory,HN_RENDER_SLOTS,aliased_base);slot=hn_render_slot(&memory,16);
    hn_write32(&memory,slot+0x18,0);hn_write32(&memory,slot+0x20,0);
    hn_write32(&memory,hn_render_slot(&memory,10)+0x18,0);hn_write32(&memory,hn_render_slot(&memory,10)+0x20,0x10000);
    hn_write8(&memory,HN_ACTOR_TURN_CONTROL,0);hn_write8(&memory,HN_ACTOR_TURN_CONTROL+1,16);
    hn_write8(&memory,HN_ACTOR_TURN_CONTROL+2,10);hn_write8(&memory,HN_ACTOR_TURN_CONTROL+3,0);
    rom[HN_TURN_NEXT-0x08000000+61]=41;hn_write32(&memory,HN_WANDER_TURNS,0x56789ABC);
    CHECK(hn_wander_turn_update(&memory)&&hn_read8(&memory,HN_ACTOR_TURN_CONTROL+1)==41);
    CHECK(hn_read32(&memory,aliased_base+41*104+4)==0x56789ABC&&!memory.fault);
    free(rom);
}
static void scene_actor_contract(void)
{
    reset();GameAddr args=0x02020000;uint32_t consumed=99;
    uint8_t *rom=calloc(1,0x600000);CHECK(rom!=NULL);memory.rom=rom;memory.rom_size=0x600000;
    for(unsigned direction=0;direction<4;direction++)for(unsigned axis=0;axis<2;axis++){
        uint32_t value=axis?0u-(direction+1):direction+2;
        for(unsigned byte=0;byte<4;byte++)rom[HN_ACTOR_APPROACH_DIRECTIONS-0x08000000+direction*8+axis*4+byte]=(uint8_t)(value>>(byte*8));
    }
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02028000);hn_write8(&memory,HN_GRAPHICS_COUNT,16);
    CHECK(hn_scene_actor_task(&memory,0x08001001)==HN_SCENE_UNKNOWN);
    CHECK(hn_scene_actor_task(&memory,0x0801D121)==HN_SCENE_UNKNOWN); /* Requires nine argument bytes. */
    hn_write8(&memory,args,0xFF);hn_write8(&memory,args+1,0x80);
    CHECK(hn_scene_callback(&memory,0x0801D73D,args,&consumed)==HN_SCENE_OK&&consumed==2&&hn_read32(&memory,HN_RENDER_INFO)==0x80FF0000);
    GameAddr a=hn_render_slot(&memory,5),b=hn_render_slot(&memory,6);
    hn_write8(&memory,args,5);hn_write8(&memory,args+1,0x34);hn_write8(&memory,args+2,0x80);
    CHECK(hn_scene_callback(&memory,0x0801D9C5,args,&consumed)==HN_SCENE_OK&&consumed==3&&hn_read32(&memory,a+0x30)==0xFF803400);
    CHECK(hn_scene_callback(&memory,0x0801D9F9,args,&consumed)==HN_SCENE_OK&&hn_read32(&memory,a+0x34)==0x00348000);
    CHECK(hn_scene_callback(&memory,0x0801DA19,args,&consumed)==HN_SCENE_OK&&hn_read32(&memory,a+0x38)==0x00348000);
    for(unsigned i=0;i<104;i++){hn_write8(&memory,a+i,(uint8_t)(i+1));hn_write8(&memory,b+i,(uint8_t)(255-i));}
    hn_write8(&memory,args+1,255);hn_write8(&memory,HN_ACTOR_RENDER_SNAPSHOT+0x1A0+104+0x63,0xA5);
    CHECK(hn_scene_callback(&memory,0x0801D799,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x14)==5&&hn_read8(&memory,HN_SCENE_CONTROLS+0x15)==255);
    CHECK(!memcmp(memory.ewram+(a-0x02000000),memory.ewram+(HN_ACTOR_RENDER_SNAPSHOT+0x1A0-0x02000000),104));
    hn_write32(&memory,a,0);hn_write8(&memory,a+0x63,0xFF);
    CHECK(hn_scene_actor_task(&memory,0x0801D7E9)==HN_SCENE_OK&&hn_read32(&memory,a)==0x04030201&&hn_read8(&memory,a+0x63)==0);
    CHECK(hn_read8(&memory,HN_ACTOR_RENDER_SNAPSHOT+0x1A0+104+0x63)==0xA5);
    hn_write8(&memory,args,5);hn_write8(&memory,args+1,6);
    CHECK(hn_scene_callback(&memory,0x0801DAD1,args,&consumed)==HN_SCENE_OK&&consumed==2&&hn_read8(&memory,a)==255&&hn_read8(&memory,b)==1);
    CHECK(hn_scene_callback(&memory,0x0801DAA1,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(!memcmp(memory.ewram+(a-0x02000000),memory.ewram+(b-0x02000000),104));
    GameAddr leader=hn_render_slot(&memory,10),follower=hn_render_slot(&memory,13);
    hn_write32(&memory,leader+0x18,0xFFFF1234);hn_write32(&memory,leader+0x1C,0x00021234);hn_write32(&memory,leader+0x20,0x00031234);
    hn_write32(&memory,follower+0x18,0x7FFF4321);hn_write32(&memory,follower+0x1C,0x80004321);hn_write32(&memory,follower+0x20,0xFFFE4321);
    hn_write8(&memory,leader+0x60,0xA1);hn_write8(&memory,follower+0x60,0xB4);
    CHECK(hn_scene_actor_task(&memory,0x0801FDC9)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x74)==1&&hn_read8(&memory,HN_SCENE_CONTROLS+0x75)==4);
    CHECK(hn_read16(&memory,0x0201DC40+46*2)==65535&&hn_read16(&memory,0x0201DC40+49*2)==32768);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x75,255);hn_write32(&memory,follower+4,0x12345678);
    CHECK(hn_scene_actor_task(&memory,0x080200AD)==HN_SCENE_OK&&hn_read32(&memory,follower+4)==0x12345678);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,12)+0x65)==13&&hn_read8(&memory,hn_render_slot(&memory,12)+0x62)==0xB0);
    CHECK(hn_scene_actor_task(&memory,0x0801F419)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,9)+0x62)==0xB0&&hn_read8(&memory,hn_render_slot(&memory,9)+0x65)==10);
    CHECK(hn_read8(&memory,0x03001FAC)==10&&hn_read32(&memory,0x03001F7C)==0x088A503E);
    GameAddr actor=hn_render_slot(&memory,16);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,16);hn_write8(&memory,0x03002170,0xA5);
    hn_write32(&memory,actor+0x18,0x000A1234);hn_write32(&memory,actor+0x1C,0);hn_write32(&memory,actor+0x20,0x0014FEDC);
    hn_write8(&memory,actor+0x60,0xA3);hn_write32(&memory,actor+0x30,0x11223344);
    hn_write16(&memory,0x0201DC40,11);hn_write16(&memory,0x0201DC42,20);
    const uint8_t approach[]={255,0,1,0,16,0,0,5,0};
    for(unsigned i=0;i<sizeof(approach);i++)hn_write8(&memory,args+i,approach[i]);
    hn_write8(&memory,HN_SCENE_CONTROLS+255,0);hn_write8(&memory,HN_SCENE_CONTROLS+256,0xAA);
    hn_write32(&memory,HN_ACTOR_APPROACH_MOVING+3*4,0x12345678);hn_write32(&memory,HN_ACTOR_APPROACH_IDLE+3*4,0x23456789);
    CHECK(hn_scene_callback(&memory,0x0801D121,args,&consumed)==HN_SCENE_OK&&consumed==9);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+255)==1&&hn_read8(&memory,HN_SCENE_CONTROLS+256)==3&&hn_read8(&memory,0x03002170)==0);
    CHECK(hn_read32(&memory,actor+0x24)==0x00050000&&hn_read32(&memory,actor+0x28)==0&&hn_read32(&memory,actor+0x2C)==0xFFFC0000);
    CHECK(hn_read32(&memory,actor+4)==0x12345678&&hn_read32(&memory,actor+0x30)==0x11223344);
    hn_write16(&memory,0x0201DC40,10);
    CHECK(hn_scene_callback(&memory,0x0801D121,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+255)==2);
    CHECK(hn_read32(&memory,actor+4)==0x23456789&&hn_read8(&memory,actor+0x60)==0xA4);
    CHECK(hn_read32(&memory,actor+0x18)==0x000A8000&&hn_read32(&memory,actor+0x20)==0x00148000);
    CHECK(hn_read32(&memory,actor+0x24)==0&&hn_read32(&memory,actor+0x28)==0&&hn_read32(&memory,actor+0x2C)==0);
    hn_write8(&memory,HN_SCENE_CONTROLS+255,255);hn_write32(&memory,actor+0x18,0xDEADBEEF);
    CHECK(hn_scene_callback(&memory,0x0801D121,args,&consumed)==HN_SCENE_OK&&consumed==9&&hn_read32(&memory,actor+0x18)==0xDEADBEEF);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x61,16);hn_write8(&memory,HN_SCENE_CONTROLS+0x62,255);hn_write8(&memory,HN_SCENE_CONTROLS+0x63,36);
    hn_write32(&memory,actor+0x18,0xFFFE8000);hn_write32(&memory,actor+0x1C,0);hn_write32(&memory,actor+0x20,0);
    hn_write16(&memory,0x0201DC88,10);hn_write16(&memory,0x0201DC86,0);
    CHECK(hn_scene_actor_task(&memory,0x0801CD4D)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC84)==10&&hn_read32(&memory,actor+0x24)==0xB80);
    hn_write8(&memory,args,1);hn_write16(&memory,args+1,65535);hn_write16(&memory,args+3,0xABCD);hn_write32(&memory,args+5,0x02030000);
    hn_write16(&memory,0x0201DC40+65535*2,0xABCD);hn_write32(&memory,0x03002C60,0x02020111);
    CHECK(hn_scene_callback(&memory,0x08050C85,args,&consumed)==HN_SCENE_OK&&consumed==9&&hn_read32(&memory,0x03002C60)==0x02020111);
    hn_write16(&memory,args+3,0xABCE);
    CHECK(hn_scene_callback(&memory,0x08050C85,args,&consumed)==HN_SCENE_OK&&consumed==0xFFFFFFFC&&hn_read32(&memory,0x03002C60)==0x02030000);
    for(unsigned i=9;i;i--)hn_write8(&memory,args+4+i-1,hn_read8(&memory,args+i-1));hn_write32(&memory,args,0x08050C85);
    HNScript script;hn_script_init(&script,&memory,NULL,0);
    CHECK(hn_main_command(&script,0x1C,args)&&hn_read32(&memory,0x03000014)==0&&hn_read32(&memory,0x03002C60)==0x02030000);
    free(rom);
}
static void actor_presets_contract(void)
{
    reset();uint32_t consumed=99;hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02015000);hn_write8(&memory,HN_GRAPHICS_COUNT,16);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);
    hn_write8(&memory,0x03002C04,16);hn_write8(&memory,0x03002C00,19);hn_write8(&memory,0x03002170,0xA5);hn_write8(&memory,0x03002182,128);
    CHECK(hn_preset_apply(&memory,0x08013015)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,10)+4)==0x088A590A&&hn_read8(&memory,0x03001FAC)==10&&hn_read32(&memory,0x03001F7C)==0x088A590A);
    CHECK(hn_read8(&memory,0x03002170)==0xA5&&(hn_read8(&memory,hn_render_slot(&memory,10)+0x61)&64)!=0);
    CHECK(hn_scene_callback(&memory,0x08013015,0,&consumed)==HN_SCENE_UNKNOWN&&!memory.fault);
    CHECK(hn_preset_apply(&memory,0x08013039)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,9)+0x65)==10&&hn_read8(&memory,hn_render_slot(&memory,11)+0x65)==10);
    CHECK(hn_preset_apply(&memory,0x08013911)==HN_SCENE_OK&&hn_read8(&memory,0x03002170)==0);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,12)+0x65)==13&&hn_read8(&memory,hn_render_slot(&memory,14)+0x65)==13);
    GameAddr base=HN_RENDER_CLASS_LEADER-9*104-0x60;hn_write32(&memory,HN_RENDER_SLOTS,base);hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);
    CHECK(hn_preset_apply(&memory,0x08013039)==HN_SCENE_OK&&!memory.fault);
    CHECK(hn_read8(&memory,HN_RENDER_CLASS_LEADER)==1); /* First auxiliary initialization aliases the selector. */
    CHECK(hn_read8(&memory,base+9*104+0x65)==10&&hn_read8(&memory,base+2*104+0x65)==1);
    CHECK(hn_preset_apply(&memory,0x08001001)==HN_SCENE_UNKNOWN&&!memory.fault);
    reset();hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,1);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02018000);hn_write8(&memory,HN_GRAPHICS_COUNT,1);
    hn_write8(&memory,0x03002C04,10);hn_write8(&memory,0x03002C00,13);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);
    CHECK(!hn_render_configure(&memory,10,1,0xB0,0,0,0,0x08500000));
    CHECK(!hn_render_modes(&memory,10,2,3,13)&&!hn_graphics_configure(&memory,5,0x08500000));
    CHECK(hn_preset_apply(&memory,0x0801799D)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,10)+4)==0x088D8AC1);
    CHECK(hn_read8(&memory,0x03001FAC)==13&&hn_read32(&memory,0x03001F7C)==0x088D8AC1);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,9)+0x65)==10&&hn_read8(&memory,hn_render_slot(&memory,11)+0x65)==10);
    CHECK(hn_preset_apply(&memory,0x08015BE9)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,0x02018000+5*44+0x18)==0x088E13AB&&hn_read8(&memory,0x02018000+5*44)==1);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,13)+4)==0x088CD0A1&&!hn_read8(&memory,0x03002170));
    hn_write8(&memory,0x03002C00,255);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,255);hn_write8(&memory,0x03002170,0xA5);
    CHECK(hn_preset_apply(&memory,0x080146F5)==HN_SCENE_OK&&!hn_read8(&memory,0x03002170));
    CHECK(hn_read32(&memory,hn_render_slot(&memory,255)+4)==0x088BD285&&hn_read8(&memory,hn_render_slot(&memory,0)+0x65)==255);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,0);
    hn_write32(&memory,hn_render_slot(&memory,255)+0x18,0);hn_write32(&memory,hn_render_slot(&memory,255)+0x20,0);
    hn_write32(&memory,hn_render_slot(&memory,0)+0x18,0xFFFF0000);hn_write32(&memory,hn_render_slot(&memory,0)+0x20,0);
    CHECK(hn_preset_apply(&memory,0x08014745)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,255)+4)==0x088BD22A);
    hn_write32(&memory,hn_render_slot(&memory,0)+0x18,0);hn_write32(&memory,hn_render_slot(&memory,0)+0x20,0x10000);
    CHECK(hn_preset_apply(&memory,0x08014745)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,255)+4)==0x088BD22A);
    CHECK(hn_preset_apply(&memory,0x0801499D)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,255)+4)==0x088C5C71);
    CHECK(hn_preset_apply(&memory,0x08013385)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,255)+4)==0x088B5325);
    CHECK(hn_scene_callback(&memory,0x0801499D,0,&consumed)==HN_SCENE_UNKNOWN&&!memory.fault);
}
static void records_contract(void)
{
    reset();uint8_t *rom=calloc(1,0x600000);CHECK(rom!=NULL);memory.rom=rom;memory.rom_size=0x600000;
    uint32_t descriptor=2|(3<<2)|(1<<9)|(5<<11);
    for(unsigned byte=0;byte<4;byte++){
        rom[HN_RECORD_DESCRIPTORS-0x08000000+4+byte]=(uint8_t)(descriptor>>(byte*8));
        rom[HN_RECORD_BANKS-0x08000000+2*4+byte]=(uint8_t)(0x02030400u>>(byte*8));
        rom[HN_COUNTER_TEMPLATES-0x08000000+4+byte]=(uint8_t)(0x0202F000u>>(byte*8));
    }
    hn_write32(&memory,0x02030400+3*4,0x12345678);hn_write32(&memory,HN_RECORD_SLOTS,0xABCD9000);
    CHECK(hn_records_count(&memory)==0&&hn_record_allocate(&memory,0x0181)==0&&hn_records_count(&memory)==1);
    CHECK(hn_read32(&memory,HN_RECORD_SLOTS)==0xABCD9281); /* Seven-bit key, five-bit count, retained upper fields. */
    GameAddr result=0x02031000;
    for(unsigned i=0;i<12;i++)hn_write8(&memory,result+i,0xA5);
    CHECK(hn_record_query(&memory,0,result)==5&&hn_read32(&memory,result)==0x12345678);
    CHECK(hn_read8(&memory,result+4)==2&&hn_read8(&memory,result+5)==3&&hn_read8(&memory,result+6)==5&&hn_read8(&memory,result+7)==5&&hn_read8(&memory,result+8)==1);
    CHECK(hn_read8(&memory,result+9)==0xA5&&hn_read8(&memory,result+11)==0xA5);
    hn_write8(&memory,result,0xB6);
    CHECK(hn_record_query(&memory,1,result)==0&&hn_read8(&memory,result)==0xB6);
    CHECK(hn_record_consume(&memory,0,0,result)==-1&&((hn_read16(&memory,HN_RECORD_SLOTS)>>7)&31)==5);
    CHECK(hn_record_consume(&memory,0,1,result)==4&&hn_read8(&memory,result+7)==4);
    hn_write16(&memory,HN_RECORD_SLOTS,hn_read16(&memory,HN_RECORD_SLOTS)&~0xF80u);
    CHECK(hn_record_consume(&memory,0,255,result)==-1&&((hn_read16(&memory,HN_RECORD_SLOTS)>>7)&31)==31);
    CHECK(hn_read8(&memory,result+7)==255); /* Original underflow differs from the stored five-bit count. */
    hn_write8(&memory,0x030021B2,2);
    CHECK(hn_record_consume(&memory,0,0,result)==0&&hn_records_count(&memory)==0);
    rom[HN_RECORD_COUNTER_DESCRIPTORS-0x08000000+3*20+5]=10;hn_write8(&memory,HN_RECORD_COUNTERS+3,255);
    CHECK(hn_record_adjust_counter(&memory,3,7)==1&&hn_record_counter(&memory,3)==7);
    CHECK(hn_record_adjust_counter(&memory,3,5)==0&&hn_record_counter(&memory,3)==10);
    CHECK(hn_record_adjust_counter(&memory,3,INT32_MIN)==1&&hn_record_counter(&memory,3)==0);
    hn_write8(&memory,HN_RECORD_COUNTERS+32,0x55);CHECK(hn_record_counter(&memory,32)==255);
    for(unsigned i=0;i<64;i++)hn_write32(&memory,HN_RECORD_SLOTS+i*4,(i&127)|(1<<7));
    CHECK(hn_records_count(&memory)==64&&hn_record_allocate(&memory,1)==-1);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write16(&memory,0x0201DC84,1);hn_write32(&memory,HN_COUNTER_DELTA,0x1234);
    hn_write32(&memory,HN_COUNTER_STEPS,0x5678);hn_write32(&memory,HN_COUNTER_DELAY,0x9ABC);
    hn_write8(&memory,0x03002182,8);uint32_t consumed=99;
    CHECK(hn_scene_callback(&memory,0x0801E00D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read16(&memory,0x0201DC4C)==0&&hn_read16(&memory,0x0201DC56)==64&&hn_read16(&memory,0x0201DC4E)==64);
    CHECK((hn_read8(&memory,0x03002182)&8)==0&&hn_read32(&memory,HN_COUNTER_DELTA)==0x1234&&hn_read32(&memory,HN_COUNTER_STEPS)==0x5678&&hn_read32(&memory,HN_COUNTER_DELAY)==0x9ABC);
    hn_write16(&memory,HN_RECORD_SLOTS+63*4,0);GameAddr args=0x02020000;hn_write16(&memory,args,0x0181);
    CHECK(hn_scene_callback(&memory,0x0801DE15,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_records_count(&memory)==64&&hn_read16(&memory,0x0201DC4C)==1&&(hn_read8(&memory,0x03002182)&8)!=0);
    CHECK(hn_read32(&memory,HN_COUNTER_DELTA)==1&&hn_read32(&memory,HN_COUNTER_STEPS)==1&&hn_read32(&memory,HN_COUNTER_DELAY)==1);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,2)+0x18)==0x00B08000&&hn_read32(&memory,hn_render_slot(&memory,2)+0x1C)==0xFFF08000);
    for(unsigned i=0;i<64;i++)hn_write16(&memory,HN_RECORD_SLOTS+i*4,0);
    hn_write16(&memory,args,0);CHECK(hn_scene_callback(&memory,0x0801DE15,args,&consumed)==HN_SCENE_OK);
    CHECK(hn_records_count(&memory)==0&&hn_read16(&memory,0x0201DC56)==1&&hn_read16(&memory,0x0201DC4C)==1); /* Zero maxCount still reports allocation success. */
    CHECK(!memory.fault);free(rom);
}
static void wave_contract(void)
{
    reset();uint8_t *rom=calloc(1,0x600000);CHECK(rom!=NULL);memory.rom=rom;memory.rom_size=0x600000;
    for(unsigned i=0;i<256;i++){
        uint16_t value=(uint16_t)(((int)i-128)*16);
        rom[HN_RENDER_SINE_TABLE-0x08000000+i*2]=(uint8_t)value;rom[HN_RENDER_SINE_TABLE-0x08000000+i*2+1]=(uint8_t)(value>>8);
    }
    hn_write16(&memory,HN_VIDEO_STATE+0x6C,77);uint32_t consumed=99;
    CHECK(hn_scene_callback(&memory,0x0802E22D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,HN_HBLANK_SELECTOR)==1&&hn_read16(&memory,HN_SCANLINE_RADIUS)==77&&hn_read8(&memory,HN_SCANLINE_PHASE)==0&&hn_read8(&memory,HN_SCANLINE_PHASE+1)==45);
    hn_write32(&memory,0x02000004,0x0802E269);hn_write16(&memory,HN_SCANLINE_RADIUS,0);memory.io[6]=55;
    CHECK(hn_raster_hblank(&memory)&&hn_read16(&memory,0x04000018)==0xFFE4&&hn_read16(&memory,0x04000052)==0x0808);
    memory.io[6]=56;CHECK(hn_raster_wave(&memory)&&hn_read16(&memory,0x04000052)==0x0907);
    memory.io[6]=113;CHECK(hn_raster_wave(&memory)&&hn_read16(&memory,0x04000052)==0x1000);
    memory.io[6]=227;hn_write8(&memory,HN_SCANLINE_PHASE,255);hn_write8(&memory,HN_SCANLINE_PHASE+1,255);
    CHECK(hn_raster_hblank(&memory)&&hn_read8(&memory,HN_SCANLINE_PHASE)==0&&hn_read8(&memory,HN_SCANLINE_PHASE+1)==0);
    CHECK(hn_read16(&memory,HN_SCANLINE_RADIUS)==0xFEB3); /* Wrapped signed sine offset minus background Y. */
    hn_write32(&memory,0x02000004,0x08001001);CHECK(!hn_raster_hblank(&memory)&&!memory.fault);
    CHECK(hn_scene_callback(&memory,0x0802E259,0,&consumed)==HN_SCENE_OK&&consumed==0&&hn_read8(&memory,HN_HBLANK_SELECTOR)==0);
    free(rom);
}
static void counter_scene_contract(void)
{
    uint8_t *rom=calloc(1,0x700000);CHECK(rom!=NULL);if(!rom)return;
    hn_memory_init(&memory,rom,0x700000);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02018000);hn_write8(&memory,HN_GRAPHICS_COUNT,8);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,23);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x60,4);hn_write16(&memory,0x0201DC84,65535);
    uint32_t consumed=99;GameAddr tiles=HN_COUNTER_SCENE_TILES+80;
    CHECK(hn_scene_callback(&memory,0x080C2C3D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read16(&memory,tiles+6)==0xF1CF&&hn_read16(&memory,tiles+12)==0xF143&&hn_read16(&memory,tiles+18)==0xF145);
    hn_write16(&memory,0x0201DC84,25600);CHECK(hn_counter_scene_apply(&memory,0x080C2C3C,0)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,tiles+6)==0xF262&&hn_read16(&memory,tiles+12)==0xF262&&hn_read16(&memory,tiles+18)==0xF140);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x60,5);hn_write16(&memory,tiles+6,0x1234);
    CHECK(hn_counter_scene_apply(&memory,0x080C2C3C,0)==HN_SCENE_RESOURCE&&hn_read16(&memory,tiles+6)==0x1234&&!memory.fault);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x60,0);
    CHECK(hn_counter_scene_apply(&memory,0x080C2D00,0)==HN_SCENE_OK&&hn_read16(&memory,tiles+6)==0x0234);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x60,1);
    CHECK(hn_counter_scene_apply(&memory,0x080C2D00,0)==HN_SCENE_OK&&hn_read16(&memory,tiles+6)==0xF234);
    hn_write8(&memory,HN_DISPLAY_SHADOW+1,255);
    CHECK(hn_counter_scene_apply(&memory,0x080C2DFC,0)==HN_SCENE_OK&&hn_read8(&memory,HN_DISPLAY_SHADOW+1)==0xDF);
    CHECK(hn_read16(&memory,0x04000042)==0xB9CF&&(hn_read32(&memory,HN_VIDEO_STATE)&1));
    CHECK(hn_counter_scene_apply(&memory,0x080C2E84,0)==HN_SCENE_OK&&hn_read8(&memory,HN_DISPLAY_SHADOW+1)==31);
    CHECK(hn_read16(&memory,0x04000042)==0&&hn_read16(&memory,0x04000046)==0);
    hn_write16(&memory,0x0201DC58,1);hn_write16(&memory,0x0201DC5A,128);
    CHECK(hn_counter_scene_apply(&memory,0x080C302C,0)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,23)+4)==0x088F7F73&&hn_read32(&memory,hn_render_slot(&memory,23)+0x20)==0x006E8000);
    hn_write16(&memory,0x0201DC5A,54);CHECK(hn_counter_scene_apply(&memory,0x080C3304,0)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,25)+4)==0x088F7DF7&&hn_read32(&memory,hn_render_slot(&memory,27)+4)==0x088F7E74);
    CHECK(hn_read16(&memory,0x04000046)==0x2082);
    hn_write16(&memory,0x0201DC58,4);hn_write16(&memory,0x0201DC5A,8);
    CHECK(hn_counter_scene_apply(&memory,0x080C321C,1)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,19)+4)==0x088F7D40&&hn_read32(&memory,hn_render_slot(&memory,22)+4)==0x088F7CCF);
    CHECK(hn_scene_callback(&memory,0x080C321C,0,&consumed)==HN_SCENE_UNKNOWN);
    CHECK(hn_counter_scene_task(&memory,0x080C321C)==HN_SCENE_UNKNOWN);
    hn_write16(&memory,0x0201DC58,6);hn_write16(&memory,0x0201DC5A,56);hn_write16(&memory,0x0201DC88,65535);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x61,4);
    CHECK(hn_counter_scene_apply(&memory,0x080C3528,0)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC88)==0);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS+0x61)==10&&hn_read16(&memory,0x04000046)==0x1882);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,25)+0x2C)==0xFFFF8000);
    rom[HN_COUNTER_SCENE_STEPS-HN_ROM_BASE]=1;hn_write8(&memory,HN_SCENE_CONTROLS+0x79,0);
    hn_write16(&memory,0x0201DC58,0);hn_write16(&memory,0x0201DC5A,13);hn_write8(&memory,HN_SCENE_CONTROLS+0x61,6);
    CHECK(hn_counter_scene_apply(&memory,0x080C3528,0)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,0x0201DC58)==1&&hn_read16(&memory,0x0201DC5A)==14&&hn_read8(&memory,HN_SCENE_CONTROLS+0x61)==11);
    CHECK(hn_read32(&memory,0x02018000+5*44+0x18)==0x088F8898);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,28)+4)==0x088F7AF3&&hn_read32(&memory,hn_render_slot(&memory,28)+0x20)==0x00548000);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,7)+4)==0x088F7C83);
    CHECK(hn_counter_scene_apply(&memory,0x080C2C38,0)==HN_SCENE_OK&&hn_counter_scene_apply(&memory,0x08000000,0)==HN_SCENE_UNKNOWN);
    CHECK(!memory.fault);free(rom);
}
static void shared_motion_contract(void)
{
    uint8_t *rom=calloc(1,0x600000);CHECK(rom!=NULL);if(!rom)return;
    hn_memory_init(&memory,rom,0x600000);hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,40);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);
    GameAddr leader=hn_render_slot(&memory,10),actor=hn_render_slot(&memory,5),target=hn_render_slot(&memory,6),args=0x02028000;
    hn_write32(&memory,leader+0x18,0x7FFF8000);hn_write32(&memory,leader+0x1C,0xFFFF8000);hn_write32(&memory,leader+0x20,0x80008000);
    hn_write16(&memory,HN_SCENE_DURATION,2);hn_write32(&memory,HN_SCRIPT_RENDER_OFFSETS,0xFFFFFFF9);hn_write32(&memory,HN_SCRIPT_RENDER_OFFSETS+4,0x80000000);
    hn_write8(&memory,args,5);uint32_t consumed=99;
    CHECK(hn_scene_callback(&memory,0x08011B88,args,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read32(&memory,actor+0x24)==0xFFFFFFFD&&hn_read32(&memory,actor+0x28)==0&&hn_read32(&memory,actor+0x2C)==0xC0000000);
    CHECK(hn_read32(&memory,actor+0x18)==0x7FFF8000&&hn_read32(&memory,actor+0x20)==0x80008000);
    hn_write8(&memory,args,10);hn_write32(&memory,HN_SCRIPT_RENDER_OFFSETS,1);hn_write32(&memory,HN_SCRIPT_RENDER_OFFSETS+4,0xFFFFFFFD);
    CHECK(hn_scene_callback(&memory,0x08011BF4,args,&consumed)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,leader+0x18)==0x7FFF8001&&hn_read32(&memory,leader+0x20)==0x80007FFD);
    CHECK(hn_read32(&memory,leader+0x24)==0&&hn_read32(&memory,leader+0x28)==0&&hn_read32(&memory,leader+0x2C)==0);
    hn_write16(&memory,0x0201DC84,65535);hn_write16(&memory,0x0201DC86,32768);hn_write16(&memory,0x0201DC88,0x1234);hn_write8(&memory,args,5);
    CHECK(hn_scene_callback(&memory,0x0801D754,args,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read32(&memory,actor+0x18)==0xFFFF8000&&hn_read32(&memory,actor+0x1C)==0x80008000&&hn_read32(&memory,actor+0x20)==0x12348000);
    hn_write32(&memory,actor+0x18,0);hn_write32(&memory,actor+0x1C,0);hn_write32(&memory,actor+0x20,0);
    hn_write32(&memory,target+0x18,0x80000000);hn_write32(&memory,target+0x1C,0x80000000);hn_write32(&memory,target+0x20,0);
    hn_write32(&memory,args,0x01010605);hn_write8(&memory,HN_SCENE_CONTROLS+0x60,0x55);
    CHECK(hn_scene_callback(&memory,0x0801D318,args,&consumed)==HN_SCENE_OK&&consumed==4&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==0x55);
    hn_write32(&memory,target+0x18,0x10000);
    CHECK(hn_scene_callback(&memory,0x0801D318,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==1);
    GameAddr raw_from=hn_render_slot(&memory,200),raw_to=hn_render_slot(&memory,255);
    hn_write32(&memory,raw_from+0x18,0);hn_write32(&memory,raw_from+0x20,0);hn_write32(&memory,raw_to+0x18,65536);hn_write32(&memory,raw_to+0x20,0);
    CHECK(hn_render_direction_raw(&memory,200,255,false)==4&&hn_render_direction_between(&memory,200,255,false)==0);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,200);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,255);hn_write8(&memory,args,0);
    CHECK(hn_scene_callback(&memory,0x0801C910,args,&consumed)==HN_SCENE_OK&&consumed==1&&hn_read8(&memory,HN_SCENE_CONTROLS+10)==4);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,200);hn_write8(&memory,HN_SCENE_CONTROLS+0x12,0);
    hn_write16(&memory,leader+0x50,70);hn_write16(&memory,raw_from+0x50,71);
    CHECK(hn_scene_callback(&memory,0x0801CE90,0,&consumed)==HN_SCENE_OK&&consumed==0&&hn_read16(&memory,0x03001FBC)==2);
    hn_write8(&memory,args,5);hn_write8(&memory,args+1,2);hn_write16(&memory,actor+0x4E,120);hn_write16(&memory,actor+0x50,0);
    CHECK(hn_scene_callback(&memory,0x08011AE0,args,&consumed)==HN_SCENE_OK&&consumed==2&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==2);
    hn_write8(&memory,args+1,0);
    CHECK(hn_scene_callback(&memory,0x08011AE0,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==0x80);
    hn_write8(&memory,args+1,255);hn_write16(&memory,actor+0x4E,65535);hn_write16(&memory,actor+0x50,65535);
    CHECK(hn_scene_callback(&memory,0x08011AE0,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==0x80);
    hn_write32(&memory,actor+0x18,0x7FFF8000);hn_write32(&memory,actor+0x20,0);hn_write32(&memory,actor+0x24,0x8000);hn_write32(&memory,actor+0x2C,0);
    hn_write32(&memory,target+0x18,0x7FFF8000);hn_write32(&memory,target+0x20,0);hn_write8(&memory,target+0x60,0xA0);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,6);hn_write8(&memory,0x03002170,1);
    hn_write8(&memory,HN_SCENE_CONTROLS,0x5A);hn_write8(&memory,HN_SCENE_CONTROLS+255,0);
    hn_write32(&memory,HN_ACTOR_APPROACH_MOVING+12,0x08500000);hn_write32(&memory,HN_ACTOR_APPROACH_IDLE+12,0x08500004);
    rom[HN_ACTOR_APPROACH_DIRECTIONS-HN_ROM_BASE+3*8]=1;
    const uint8_t parameters[]={255,0,5,0,0,6,0,0,255,0};for(unsigned i=0;i<10;i++)hn_write8(&memory,args+i,parameters[i]);
    CHECK(hn_scene_callback(&memory,0x0801CF00,args,&consumed)==HN_SCENE_OK&&consumed==10);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS)==0x5A&&hn_read8(&memory,HN_SCENE_CONTROLS+255)==1&&hn_read8(&memory,HN_SCENE_CONTROLS+256)==3);
    CHECK(hn_read8(&memory,0x03002170)==0&&hn_read32(&memory,target+4)==0x08500000&&hn_read32(&memory,target+0x24)==65536);
    hn_write32(&memory,actor+0x18,0x00001234);hn_write32(&memory,actor+0x20,0xFFFF5678);hn_write32(&memory,actor+0x24,0);
    hn_write32(&memory,target+0x18,0x00001234);hn_write32(&memory,target+0x20,0xFFFF5678);
    CHECK(hn_scene_callback(&memory,0x0801CF00,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+255)==2);
    CHECK(hn_read32(&memory,target+4)==0x08500004&&hn_read32(&memory,target+0x18)==0x00008000&&hn_read32(&memory,target+0x20)==0xFFFF8000);
    CHECK(hn_read8(&memory,target+0x60)==0xA4&&hn_read32(&memory,target+0x24)==0);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x66,10);hn_write8(&memory,HN_SCENE_CONTROLS+0x67,13);
    CHECK(hn_scene_callback(&memory,0x0801EB34,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,11)+4)==0x088E3726&&hn_read8(&memory,hn_render_slot(&memory,9)+0x65)==13);
    hn_write8(&memory,HN_DISPLAY_SHADOW,0xAA);CHECK(hn_scene_callback(&memory,0x0801EB98,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_DISPLAY_SHADOW)==0xA1);
    CHECK(!memory.fault);free(rom);
}
static void scene_effects_contract(void)
{
    uint8_t *rom=calloc(1,0x600000);CHECK(rom!=NULL);if(!rom)return;
    hn_memory_init(&memory,rom,0x600000);hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);
    hn_write8(&memory,HN_RENDER_INFO+16,1);
    uint32_t consumed=99,yield=99;GameAddr control=HN_SCENE_CONTROLS,layer2=HN_LAYER_OFFSETS+2*44;
    hn_write16(&memory,HN_DISPLAY_SHADOW,0xC5B7);hn_write32(&memory,HN_VIDEO_STATE,0x80000000);
    CHECK(hn_scene_callback(&memory,0x0801EC3D,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read16(&memory,HN_DISPLAY_SHADOW)==0xE5B7&&hn_read32(&memory,HN_VIDEO_STATE)==0x80000001);
    CHECK(hn_read16(&memory,0x04000040)==240&&hn_read16(&memory,0x04000044)==160);
    CHECK(hn_scene_callback(&memory,0x0801EC6D,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,HN_DISPLAY_SHADOW)==0xD5B7&&!hn_read16(&memory,0x04000040)&&!hn_read16(&memory,0x04000044));
    CHECK(hn_scene_callback(&memory,0x0801EBBD,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,HN_DISPLAY_SHADOW)==0xD5B0);
    hn_write16(&memory,0x0201DC84,1);hn_write16(&memory,0x0201DC86,2);
    hn_write16(&memory,HN_VIDEO_STATE+0x3A+48,65535);hn_write16(&memory,HN_VIDEO_STATE+0x3C+48,65535);
    CHECK(hn_scene_callback(&memory,0x0801EBDD,0,&consumed)==HN_SCENE_OK);
    CHECK(!hn_read16(&memory,layer2+0x1E)&&hn_read16(&memory,layer2+0x20)==1);
    CHECK(hn_read32(&memory,HN_VIDEO_STATE)==0x80000011);
    CHECK(hn_scene_callback(&memory,0x0801EC21,0,&consumed)==HN_SCENE_OK&&!hn_read32(&memory,layer2+0x1E));
    rom[HN_RENDER_SINE_TABLE-HN_ROM_BASE+8]=0xFF;rom[HN_RENDER_SINE_TABLE-HN_ROM_BASE+9]=0xFE;
    rom[HN_RENDER_SINE_TABLE-HN_ROM_BASE+136]=1;
    hn_write8(&memory,control+0x60,0);hn_write8(&memory,control+0x61,255);hn_write8(&memory,control+0x62,3);
    hn_write8(&memory,control+0x63,1);hn_write8(&memory,control+0x64,0);hn_write8(&memory,control+0x65,0);
    CHECK(hn_scene_callback(&memory,0x0801F119,0,&consumed)==HN_SCENE_OK&&consumed==0);
    GameAddr slot=hn_render_slot(&memory,255);
    CHECK(hn_read32(&memory,slot+0x18)==0xFFFE0000&&hn_read32(&memory,slot+0x1C)==0xFFFF0000&&!hn_read32(&memory,slot+0x20));
    CHECK(hn_read8(&memory,control+0x64)==1&&!hn_read8(&memory,control+0x65)&&hn_read8(&memory,control+0x62)==2);
    rom[HN_RENDER_SINE_TABLE-HN_ROM_BASE]=1;rom[HN_RENDER_SINE_TABLE-HN_ROM_BASE+128]=0xFF;rom[HN_RENDER_SINE_TABLE-HN_ROM_BASE+129]=0xFE;
    hn_write16(&memory,0x0201DC84,65535);hn_write16(&memory,0x0201DC86,0);
    hn_write16(&memory,HN_VIDEO_STATE+0x3A+48,65530);hn_write16(&memory,HN_VIDEO_STATE+0x3C+48,7);
    hn_write8(&memory,control+0x60,1);hn_write8(&memory,control+0x61,10);hn_write8(&memory,control+0x62,128);
    hn_write8(&memory,control+0x63,0);hn_write8(&memory,control+0x64,63);hn_write8(&memory,control+0x65,255);
    CHECK(hn_scene_callback(&memory,0x0801F119,0,&consumed)==HN_SCENE_OK);
    for(unsigned layer=1;layer<4;layer+=2){
        CHECK(hn_read16(&memory,HN_VIDEO_STATE+0x3A+layer*48)==65401&&hn_read16(&memory,HN_VIDEO_STATE+0x3C+layer*48)==136);
    }
    CHECK(hn_read16(&memory,layer2+0x1E)==65401&&hn_read16(&memory,layer2+0x20)==136);
    CHECK(!hn_read8(&memory,control+0x64)&&!hn_read8(&memory,control+0x65)&&hn_read8(&memory,control+0x62)==128);
    hn_write8(&memory,control+0x64,99);hn_write8(&memory,control+0x65,99);
    CHECK(hn_scene_callback(&memory,0x0801ECC5,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,control+0x60)==0x01800401);
    CHECK(hn_read8(&memory,control+0x64)==99&&hn_read8(&memory,control+0x65)==99);
    CHECK(hn_scene_callback(&memory,0x0801ECAD,0,&consumed)==HN_SCENE_OK&&!hn_read16(&memory,control+0x64));
    HNTasks tasks;hn_tasks_init(&tasks,&memory);GameAddr task=0x02019000,args=0x02028000;
    const GameAddr callbacks[]={0x0801EBBC,0x0801EBDC,0x0801EC20,0x0801EC3C,0x0801EC6C,0x0801ECAC,0x0801ECC4,0x0801F118};
    for(unsigned i=0;i<sizeof(callbacks)/sizeof(callbacks[0]);i++){
        hn_write32(&memory,task+0x10,args);hn_write8(&memory,args,1);hn_write32(&memory,0x03001FC4,callbacks[i]|1);
        CHECK(hn_task_command(&tasks,task,0xF4,&yield)&&yield==1&&hn_read32(&memory,task+0x10)==args+1);
        CHECK(hn_scene_animation_callback(&memory,callbacks[i],slot,args,&consumed,&yield)==HN_SCENE_UNKNOWN);
    }
    CHECK(!memory.fault);free(rom);
}
static void bank3_contract(void)
{
    uint8_t *rom=calloc(1,0x700000);CHECK(rom!=NULL);if(!rom)return;
    hn_memory_init(&memory,rom,0x700000);hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write8(&memory,HN_RENDER_INFO+16,1);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02018000);hn_write8(&memory,HN_GRAPHICS_COUNT,1);
    hn_write8(&memory,0x03002C04,16);hn_write8(&memory,0x03002C00,19);
    CHECK(hn_bank3_task(&memory,0x08051525)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,29)+0x20)==0xFFF08000);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,30)+0x18)==0xFFC08000&&hn_read8(&memory,hn_render_slot(&memory,31)+0x65)==16);
    CHECK(hn_read32(&memory,0x02018000+4*44+0x18)==0x088E1412);
    GameAddr player=0x02020000,tracks=0x02021000;
    for(unsigned i=0;i<4;i++)rom[HN_AUDIO_PLAYER_RECORDS-HN_ROM_BASE+i]=(uint8_t)(player>>(i*8));
    hn_write32(&memory,player+0x34,HN_AUDIO_IDENT);hn_write8(&memory,player+8,10);hn_write32(&memory,player+0x2C,tracks);
    for(unsigned i=0;i<10;i++){hn_write8(&memory,tracks+i*80,128);hn_write8(&memory,tracks+i*80+0x15,0x55);}
    uint32_t consumed=99,yield=99;hn_write8(&memory,0x02001658,22);
    CHECK(hn_scene_animation_callback(&memory,0x0805110D,0,0,&consumed,&yield)==HN_SCENE_OK&&consumed==0&&yield==0);
    for(unsigned i=0;i<10;i++)CHECK(hn_read8(&memory,tracks+i*80+0x15)==(i<8?83:0x55));
    hn_write8(&memory,0x02001658,1);
    CHECK(hn_bank3_animation(&memory,0x0805110D,&consumed,&yield)==HN_SCENE_OK&&hn_read8(&memory,0x02001658)==22);
    CHECK(hn_read8(&memory,tracks+0x15)==83);
    hn_write8(&memory,0x02001658,0);
    CHECK(hn_bank3_animation(&memory,0x0805110D,&consumed,&yield)==HN_SCENE_OK&&hn_read8(&memory,tracks+0x15)==119);
    hn_write8(&memory,0x02001659,2);hn_write16(&memory,player+0x1C,150);
    CHECK(hn_bank3_animation(&memory,0x08051169,&consumed,&yield)==HN_SCENE_OK&&hn_read16(&memory,player+0x1E)==128&&hn_read16(&memory,player+0x20)==75);
    hn_write16(&memory,player+0x1E,77);
    CHECK(hn_bank3_animation(&memory,0x08051169,&consumed,&yield)==HN_SCENE_OK&&hn_read16(&memory,player+0x1E)==77);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x57,0xFE);
    CHECK(hn_bank3_animation(&memory,0x080516F1,&consumed,&yield)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x57)==0xFF&&yield==0);
    GameAddr source=hn_render_slot(&memory,27);hn_write16(&memory,source+0x3C,256);hn_write16(&memory,source+0x3E,320);
    for(unsigned i=0;i<32;i++)hn_write8(&memory,HN_AFFINE_USAGE+i,255);
    for(unsigned i=29;i<32;i++)hn_write8(&memory,hn_render_slot(&memory,i)+0x64,255);
    CHECK(hn_bank3_animation(&memory,0x08051709,&consumed,&yield)==HN_SCENE_OK&&consumed==0&&yield==1);
    for(unsigned i=29;i<32;i++){
        GameAddr slot=hn_render_slot(&memory,i);
        CHECK(hn_read16(&memory,slot+0x3C)==0xFF00&&hn_read16(&memory,slot+0x3E)==320);
        CHECK(hn_read8(&memory,slot+0x64)==i-29&&hn_read8(&memory,HN_AFFINE_USAGE+i-29)==1);
    }
    for(unsigned i=0;i<32;i++)hn_write8(&memory,HN_AFFINE_USAGE+i,0);
    hn_write8(&memory,hn_render_slot(&memory,29)+0x64,255);
    CHECK(hn_bank3_animation(&memory,0x08051709,&consumed,&yield)==HN_SCENE_OK&&hn_read8(&memory,hn_render_slot(&memory,29)+0x64)==32&&hn_read8(&memory,HN_AFFINE_USAGE+32)==1);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,255);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,0);
    hn_write8(&memory,hn_render_slot(&memory,255)+0x60,1);hn_write32(&memory,hn_render_slot(&memory,255)+0x18,0x12345678);
    CHECK(hn_scene_save_actor_slots_raw(&memory)&&hn_read32(&memory,HN_ACTOR_RENDER_SNAPSHOT+0x18)==0x12345678);
    for(unsigned i=0;i<4;i++)rom[HN_PRIMARY_ANIMATION_FAMILIES-HN_ROM_BASE+4+i]=(uint8_t)(0x02030000u>>(i*8));
    hn_write32(&memory,0x02030000,0x08500000);hn_write32(&memory,hn_render_slot(&memory,255)+0x18,0);
    CHECK(hn_scene_restore_actor_slots_raw(&memory)&&hn_read32(&memory,hn_render_slot(&memory,255)+0x18)==0x12345678&&hn_read32(&memory,hn_render_slot(&memory,255)+4)==0x08500000);
    CHECK(hn_bank3_task(&memory,0x0805110D)==HN_SCENE_UNKNOWN&&hn_bank3_task(&memory,0x080511AD)==HN_SCENE_OK);
    CHECK(hn_scene_callback(&memory,0x08050FC9,0,&consumed)==HN_SCENE_UNKNOWN);
    CHECK(hn_scene_animation_callback(&memory,0x08051799,0,0,&consumed,&yield)==HN_SCENE_UNKNOWN&&!memory.fault);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,255);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,255);hn_write8(&memory,0x03002170,0xA5);
    CHECK(hn_preset_apply(&memory,0x08012EE1)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,255)+4)==0x088AD9DC&&hn_read8(&memory,0x03002170)==0xA5);
    CHECK(hn_preset_apply(&memory,0x08012D7D)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,255)+4)==0x088AC08F&&hn_read8(&memory,0x03002170)==0);
    CHECK(hn_scene_callback(&memory,0x08012D79,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_scene_callback(&memory,0x08012EE1,0,&consumed)==HN_SCENE_UNKNOWN);
    hn_write8(&memory,0x03002C04,16);hn_write8(&memory,0x03002C00,19);hn_write8(&memory,HN_SCENE_CONTROLS+0xA7,4);
    CHECK(hn_bank3_task(&memory,0x0804FF35)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,19)+4)==0x088E9E04);
    hn_write8(&memory,HN_SCENE_CONTROLS+0xA7,0);
    CHECK(hn_bank3_task(&memory,0x0804FF35)==HN_SCENE_OK&&hn_read32(&memory,hn_render_slot(&memory,16)+4)==0x088E9E04);
    GameAddr graphics=0x02018000+5*44;for(unsigned i=0;i<44;i++)hn_write8(&memory,graphics+i,0xA5);
    CHECK(hn_graphics_initialize_raw(&memory,5,0,0x08500000)&&hn_read8(&memory,graphics)==0&&hn_read32(&memory,graphics+0x18)==0x08500000);
    CHECK(hn_read8(&memory,graphics+7)==0xA5&&hn_read16(&memory,graphics+0x1C)==0&&hn_read32(&memory,graphics+8)==0);
    CHECK(!hn_graphics_configure(&memory,5,0)&&hn_read32(&memory,graphics+0x18)==0x08500000);
    CHECK(hn_bank3_task(&memory,0x08050CFD)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC40+0x57*2)==352);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,22)+0x18)==0xFFD88000&&hn_read32(&memory,hn_render_slot(&memory,22)+0x1C)==0xFFF88000);
    hn_write32(&memory,hn_render_slot(&memory,255)+0x18,0);hn_write32(&memory,hn_render_slot(&memory,255)+0x20,0);
    hn_write32(&memory,hn_render_slot(&memory,254)+0x18,0x10000);hn_write32(&memory,hn_render_slot(&memory,254)+0x20,0);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x12,254);
    CHECK(hn_scene_callback(&memory,0x0801C635,HN_RENDER_CLASS_LEADER,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read8(&memory,HN_SCENE_CONTROLS)==4&&hn_read8(&memory,HN_SCENE_CONTROLS+1)==3);
    CHECK(hn_bank3_boxes_overlap(10,10,20,20,15,15,25,25)==1);
    CHECK(hn_bank3_boxes_overlap(10,10,20,20,0,0,30,30)==0);
    CHECK(hn_bank3_boxes_overlap(10,10,20,20,10,10,20,20)==0);
    CHECK(hn_bank3_boxes_overlap(10,10,20,20,20,20,25,25)==0);
    hn_write32(&memory,0x03000000,0x3AC2);hn_write32(&memory,0x03000004,0x4DF3);hn_write32(&memory,0x03000008,1);
    CHECK(hn_scene_callback(&memory,0x0804CCDD,0,&consumed)==HN_SCENE_OK&&consumed==0);
    unsigned kinds[7]={0};for(unsigned i=0;i<9;i++){unsigned kind=hn_read8(&memory,HN_SCENE_CONTROLS+0x1A6+i);CHECK(kind>=1&&kind<=6);kinds[kind]++;}
    CHECK(kinds[1]==2&&kinds[2]==2&&kinds[3]==2&&kinds[4]==2&&kinds[5]+kinds[6]==1);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1B1,1);hn_write8(&memory,HN_SCENE_CONTROLS+0x1B2,2);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A6,5);hn_write8(&memory,HN_SCENE_CONTROLS+0x1A7,3);hn_write8(&memory,0x02028000,0);
    CHECK(hn_scene_callback(&memory,0x0804CE3D,0x02028000,&consumed)==HN_SCENE_OK&&consumed==1&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==5);
    hn_write8(&memory,0x02028000,1);
    CHECK(hn_scene_callback(&memory,0x0804CE3D,0x02028000,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==3);
    hn_write8(&memory,HN_SCENE_CONTROLS+7,255);
    CHECK(hn_scene_callback(&memory,0x0804CE81,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+7)==0&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1E4)==3);
    CHECK(hn_scene_callback(&memory,0x0804CEDD,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A5)==20&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A6)==255&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A7)==255);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1A5,0x95);
    CHECK(hn_scene_callback(&memory,0x0804CFB9,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A5)==0x94&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==1);
    CHECK(hn_scene_callback(&memory,0x0804CFB9,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A5)==0x90&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==3);
    CHECK(hn_scene_callback(&memory,0x0804CFB9,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1A5)==0x90&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==0);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1AF,255);
    CHECK(hn_scene_callback(&memory,0x0804CF89,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x1AF)==0&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==0);
    hn_write8(&memory,HN_SCENE_CONTROLS+0x1AF,3);
    CHECK(hn_scene_callback(&memory,0x0804CF89,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,HN_SCENE_CONTROLS+0x60)==1);
    CHECK(hn_bank3_task(&memory,0x0804CE3D)==HN_SCENE_UNKNOWN);
    hn_write16(&memory,0x0201DC40+224*2,3660);
    CHECK(hn_scene_callback(&memory,0x0804C8C9,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0200154C+6)==0x7589&&hn_read16(&memory,0x0200154C+18)==0x7146&&hn_read16(&memory,0x0200154C+24)==0x714A);
    hn_write16(&memory,0x0201DC40+225*2,65535);
    CHECK(hn_scene_callback(&memory,0x0804C931,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x02001566+6)==0x7145+65&&hn_read16(&memory,0x02001566+12)==0x714A);
    hn_write16(&memory,0x0201DC40+66*2,0);
    CHECK(hn_scene_callback(&memory,0x0804C9B1,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x02001580+12)==0x814D&&hn_read16(&memory,0x02001580+18)==0x814B);
    hn_write16(&memory,0x0201DC40+11*2,40);hn_write16(&memory,0x0201DC40+6*2,20);
    CHECK(hn_scene_callback(&memory,0x0804CB19,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC40+6*2)==10&&hn_read16(&memory,0x0201DC40+41*2)==100);
    hn_write16(&memory,0x0201DC40+11*2,65535);hn_write16(&memory,0x0201DC40+6*2,2);
    CHECK(hn_scene_callback(&memory,0x0804CB19,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC40+6*2)==2&&hn_read16(&memory,0x0201DC40+41*2)==0);
    hn_write16(&memory,0x0201DC40+6*2,52);
    CHECK(hn_scene_callback(&memory,0x0804CB19,0,&consumed)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC40+6*2)==0&&hn_read16(&memory,0x0201DC40+41*2)==520);
    hn_write8(&memory,0x03003EA0+8+3,0xCC);hn_write8(&memory,0x03003EA0+8+4,0xDD);hn_write8(&memory,0x03003EA0+8+5,0xEE);
    hn_write8(&memory,0x03003DE0+5,0x5A);hn_write16(&memory,0x03003DE0+6,0xBEEF);
    CHECK(hn_scene_callback(&memory,0x0804BA25,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,0x03003EA0)==3&&hn_read16(&memory,0x03003EA0+6)==24);
    CHECK(hn_read8(&memory,0x03003EA0+8+3)==0xCC&&hn_read8(&memory,0x03003EA0+8+4)==0xDD&&hn_read8(&memory,0x03003EA0+8+5)==0xEE&&hn_read16(&memory,0x03003EA0+8+6)==0);
    CHECK(hn_read8(&memory,0x03003DE0+5)==0x5A&&hn_read16(&memory,0x03003DE0+6)==0xBEEF);
    hn_write8(&memory,0x03003DE0+1,4);hn_write32(&memory,hn_render_slot(&memory,26)+0x24,0xAABBCCDD);
    hn_write32(&memory,hn_render_slot(&memory,26)+0x30,0x11223344);hn_write32(&memory,hn_render_slot(&memory,26)+0x38,0x55667788);
    CHECK(hn_bank3_entry_motion(&memory,0)&&hn_read8(&memory,0x03003DE0+4)==16&&hn_read32(&memory,hn_render_slot(&memory,26)+0x2C)==0x20000);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,26)+0x24)==0xAABBCCDD&&hn_read32(&memory,hn_render_slot(&memory,26)+0x30)==0x11223344&&hn_read32(&memory,hn_render_slot(&memory,26)+0x38)==0x55667788);
    for(unsigned i=0;i<10;i++)hn_write8(&memory,0x03003DE0+i*8,1);
    hn_write8(&memory,0x03003E30,0xB5);hn_write8(&memory,0x03003EA0+1,4);
    CHECK(hn_bank3_entry_spawn(&memory,0)&&hn_read8(&memory,0x03003E30)==0xB5&&hn_read8(&memory,0x03003E30+4)==16&&hn_read8(&memory,0x03003E30+1)==4);
    CHECK(hn_bank3_entry_configure(&memory,0x123400FF)&&hn_read8(&memory,hn_render_slot(&memory,22)+0x60)==1&&hn_read32(&memory,hn_render_slot(&memory,278)+0x2C)==0x10000);
    GameAddr entries=0x030035D0,flags=0x03003570,available=0x03003B40,control=HN_SCENE_CONTROLS;
    for(unsigned i=0;i<9;i++)hn_write8(&memory,available+i,(uint8_t)(8-i));
    CHECK(hn_bank3_chain_find(&memory,0x12340001,0x89ABCDEF)==8);
    CHECK(hn_bank3_chain_find(&memory,0x123400FF,0x89ABCDEF)==0x89ABCDEF);
    hn_bank3_chain_set_flag(&memory,0x12340001,0x567800A5);
    CHECK(hn_read8(&memory,flags+8)==0xA5);
    hn_bank3_chain_set_flag(&memory,255,0x567800A5);
    CHECK(hn_read8(&memory,flags+0xA5)==0xA5);
    GameAddr edge=entries+255*8;
    hn_write32(&memory,edge,0x04030201);hn_write32(&memory,edge+4,0xAABBCC05);hn_write8(&memory,control+0x1C1,255);
    hn_bank3_chain_prepare(&memory,0x123400FF);
    CHECK(hn_read32(&memory,edge)==0xFF0000FF&&hn_read32(&memory,edge+4)==0xAABBCCFF);
    hn_write8(&memory,entries+3,6);hn_write8(&memory,entries+4,8);hn_write32(&memory,entries,0x06030201);
    hn_bank3_chain_consume(&memory,0x12340000);
    CHECK(!hn_read8(&memory,entries)&&!hn_read16(&memory,entries+1)&&hn_read8(&memory,entries+3)==6&&hn_read8(&memory,entries+4)==8);
    hn_bank3_chain_clear(&memory,0x12340100,0);CHECK(hn_read8(&memory,entries+4)==255&&hn_read8(&memory,entries+3)==6);
    hn_bank3_chain_clear(&memory,0x12340001,0);CHECK(hn_read8(&memory,entries+3)==255);
    hn_write8(&memory,edge+4,8);hn_bank3_chain_clear(&memory,0,255);CHECK(hn_read8(&memory,edge+4)==8);
    for(unsigned i=0;i<16;i++)hn_write8(&memory,entries+i*8,255);
    hn_write8(&memory,entries,0);hn_write8(&memory,entries+16,1);
    hn_write32(&memory,hn_render_slot(&memory,22)+0x24,123);hn_write32(&memory,hn_render_slot(&memory,23)+0x24,456);
    hn_write32(&memory,hn_render_slot(&memory,24)+0x24,0u-789);
    hn_write8(&memory,control+0x1B3,0x80);hn_write8(&memory,control+0x1B4,255);
    hn_write8(&memory,control+0x1C0,7);hn_write8(&memory,control+0x1C1,4);hn_write8(&memory,control+0x1D6,0);
    CHECK(hn_bank3_chain_effect(&memory,0,1,17));
    CHECK(hn_read8(&memory,control+0x1B3)==0x81&&hn_read8(&memory,control+0x1B4)==97);
    CHECK(hn_read8(&memory,control+0x1C0)==4&&hn_read8(&memory,control+0x1C1)==7&&hn_read8(&memory,control+0x1D6)==3);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,22)+0x24)==0u-123&&hn_read32(&memory,hn_render_slot(&memory,23)+0x24)==456&&hn_read32(&memory,hn_render_slot(&memory,24)+0x24)==789);
    for(unsigned i=0;i<9;i++)hn_write8(&memory,flags+i,1);
    hn_write8(&memory,control+0x1C2,0);hn_write8(&memory,control+0x1E7,0);
    CHECK(hn_bank3_task(&memory,0x0804ED69)==HN_SCENE_OK&&hn_read8(&memory,control+0x1C2)==8&&hn_read8(&memory,control+0x1E7)==2);
    hn_write8(&memory,control+0x1E7,0xA5);
    CHECK(hn_bank3_task(&memory,0x0804ED69)==HN_SCENE_OK&&hn_read8(&memory,control+0x1E7)==0xA5);
    for(unsigned i=3;i<9;i++)hn_write8(&memory,flags+i,0);
    CHECK(hn_bank3_task(&memory,0x0804ED69)==HN_SCENE_OK&&hn_read8(&memory,control+0x1C2)==1&&hn_read8(&memory,control+0x1E7)==1);
    hn_write8(&memory,control+0x60,0);hn_write8(&memory,control+0x61,8);consumed=99;yield=99;
    CHECK(hn_scene_animation_callback(&memory,0x0804EE89,0,0,&consumed,&yield)==HN_SCENE_OK&&consumed==0&&yield==0);
    CHECK(hn_read8(&memory,control+0x60)==1&&!hn_read8(&memory,control+0x61)&&hn_read16(&memory,0x020015A6)==0x81D7&&hn_read16(&memory,0x020015BA)==0x81D7);
    hn_write8(&memory,control+0x60,255);
    CHECK(hn_bank3_chain_animation(&memory,0x0804EE89,&consumed,&yield)==HN_SCENE_OK&&!hn_read8(&memory,control+0x60));
    CHECK(hn_bank3_task(&memory,0x0804E1BD)==HN_SCENE_UNKNOWN&&hn_scene_callback(&memory,0x0804EE89,0,&consumed)==HN_SCENE_UNKNOWN);
    /* Inconsistent two-node next cycles must fail before exhausting host stack. */
    hn_write8(&memory,entries+6*8,12);hn_write8(&memory,entries+6*8+3,255);hn_write8(&memory,entries+6*8+4,7);
    hn_write8(&memory,entries+7*8,12);hn_write8(&memory,entries+7*8+3,255);hn_write8(&memory,entries+7*8+4,6);
    CHECK(!hn_bank3_chain_effect(&memory,7,1,12));
    for(unsigned i=0;i<208;i++)hn_write8(&memory,HN_ACTOR_RENDER_SNAPSHOT+0x1A0+i,(uint8_t)(i*13));
    hn_write8(&memory,control+0x14,10);hn_write8(&memory,control+0x15,13);
    CHECK(hn_scene_callback(&memory,0x0801D841,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,10)+0x63)==((99u*13u)&255)&&hn_read8(&memory,hn_render_slot(&memory,13)+0x63)==((203u*13u)&255));
    CHECK(hn_scene_callback(&memory,0x0801F705,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,10)+4)==0x088A56EF&&hn_read32(&memory,hn_render_slot(&memory,13)+4)==0x088A9628);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,9)+0x65)==10&&hn_read8(&memory,hn_render_slot(&memory,11)+0x65)==10&&hn_read8(&memory,hn_render_slot(&memory,12)+0x65)==13);
    CHECK(hn_read32(&memory,0x02018000+4*44+0x18)==0x088E1403&&hn_read32(&memory,0x03001F7C)==0x088A56EF);
    for(unsigned key=0;key<128;key++)rom[HN_RECORD_DESCRIPTORS-HN_ROM_BASE+key*4+1]=8;
    put32(rom+HN_COUNTER_TEMPLATES-HN_ROM_BASE+4,0x0202F000);
    const unsigned amounts[]={0,1,30,31,32,33,64,65,65535};
    for(unsigned edge_case=0;edge_case<3;edge_case++)for(unsigned test=0;test<sizeof(amounts)/sizeof(*amounts);test++){
        unsigned occupied=edge_case==0?0:edge_case==1?63:64,amount=amounts[test],expected=amount<64-occupied?amount:64-occupied;
        for(unsigned i=0;i<64;i++){hn_write32(&memory,HN_RECORD_SLOTS+i*4,i<occupied?128:0);hn_write16(&memory,0x0201DC40+(128+i)*2,1);}
        hn_write16(&memory,0x0201DC4C,(uint16_t)amount);hn_write16(&memory,0x02028000,128);
        CHECK(hn_scene_callback(&memory,0x0801E0BD,0x02028000,&consumed)==HN_SCENE_OK&&consumed==2);
        CHECK(hn_read16(&memory,0x0201DC4C)==expected&&hn_read16(&memory,0x0201DC56)==occupied+expected&&hn_read16(&memory,0x0201DC4E)==occupied+expected);
        CHECK(hn_records_count(&memory)==occupied+expected&&((hn_read8(&memory,0x03002182)&8)!=0)==(occupied<64));
        CHECK(hn_read32(&memory,HN_COUNTER_STEPS)==(expected>=1&&expected<=31?expected:32));
        CHECK(hn_read32(&memory,HN_COUNTER_DELAY)==(expected>=1&&expected<=31?32/expected:1));
        CHECK(hn_read32(&memory,HN_COUNTER_DELTA)==(expected>=1&&expected<=31?1:expected/32));
    }
    hn_write32(&memory,0x03004EC4,HN_AUDIO_IDENT);hn_write8(&memory,0x03004E98,2);hn_write32(&memory,0x03004EBC,0x02030000);
    hn_write8(&memory,0x02030000,128);hn_write8(&memory,0x02030050,0);hn_write8(&memory,0x02030063,0xA5);hn_write8(&memory,control+0x60,255);
    CHECK(hn_scene_callback(&memory,0x08050F91,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,0x02030013)==255&&hn_read8(&memory,0x02030063)==0xA5);
    for(unsigned i=0;i<160;i++)hn_write8(&memory,0x02030000+i,0xA5);
    hn_write8(&memory,0x02030000,0xC7);hn_write8(&memory,0x02030050,0x80);
    CHECK(hn_audio_restart_tracks(&memory,0x03004E90));
    CHECK(hn_read8(&memory,0x02030000)==0x80&&hn_read8(&memory,0x0203000F)==2&&hn_read8(&memory,0x02030013)==64&&hn_read8(&memory,0x02030019)==0x16&&hn_read8(&memory,0x02030024)==1);
    CHECK(!hn_read32(&memory,0x02030020)&&hn_read32(&memory,0x02030040)==0xA5A5A5A5&&hn_read8(&memory,0x02030063)==0xA5);
    CHECK(!memory.fault);
    free(rom);
}
static void group3_contract(void)
{
    reset();GameAddr list=0x0201EE40,controls=HN_SCENE_CONTROLS;uint32_t consumed=99;
    for(unsigned i=0;i<32;i++)hn_write8(&memory,list+i,(uint8_t)(i+1));
    hn_group3_remove_list(&memory,1);
    CHECK(hn_read8(&memory,list)==1&&hn_read8(&memory,list+1)==3&&hn_read8(&memory,list+30)==32&&!hn_read8(&memory,list+31));
    hn_write8(&memory,list+31,123);hn_group3_remove_list(&memory,255);
    CHECK(hn_read8(&memory,list+30)==32&&!hn_read8(&memory,list+31));
    hn_group3_field_bit(&memory,2,0,1);hn_group3_field_bit(&memory,2,7,1);
    CHECK(hn_read8(&memory,0x03003C58)==0x81);hn_group3_field_bit(&memory,2,32,0);
    CHECK(hn_read8(&memory,0x03003C58)==0x81);hn_group3_field_bit(&memory,2,0,0);
    CHECK(hn_read8(&memory,0x03003C58)==1);
    hn_write8(&memory,controls+0x60,1);hn_write8(&memory,controls+0x180,0);
    hn_write16(&memory,0x02001C32+10,0x4000);hn_write16(&memory,0x02001C32+16,0);
    CHECK(hn_scene_callback(&memory,0x080C61E1,0,&consumed)==HN_SCENE_OK&&!consumed);
    CHECK(hn_read16(&memory,0x02001C32+10)==0x41FF&&hn_read16(&memory,0x02001C32+16)==0xFFFF);
    hn_write8(&memory,0x03003D00,0x81);hn_write8(&memory,controls+0x70,8);
    CHECK(hn_scene_callback(&memory,0x080CA245,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x70)==1);
    CHECK(hn_scene_callback(&memory,0x080CA295,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x70)==8);
    CHECK(hn_group3_task(&memory,0x080CA529)==HN_SCENE_UNKNOWN);
    reset();hn_heap_init(&memory);hn_write32(&memory,HN_TASK_HEAD,HN_TASK_SENTINEL);
    hn_write8(&memory,list,1);hn_write32(&memory,0x02001D9C,0x02030000);hn_write32(&memory,0x02030000,0x02030100);
    hn_write8(&memory,0x02030100,2);hn_write8(&memory,0x02030101,3);hn_write8(&memory,0x02030102,4);
    CHECK(hn_scene_callback(&memory,0x080C641D,0,&consumed)==HN_SCENE_OK&&!consumed);
    GameAddr task=hn_read32(&memory,0x030037A0);
    CHECK(task&&hn_read32(&memory,task+0x10)==HN_GROUP3_LIST_TEXT);
    CHECK(hn_read8(&memory,HN_GROUP3_LIST_TEXT)==2&&!hn_read8(&memory,HN_GROUP3_LIST_TEXT+1));
    CHECK(hn_read8(&memory,controls+0x60)==1&&hn_read8(&memory,task+1)==13&&hn_read8(&memory,task+2)==2);
    for(unsigned i=0;i<0x400;i++)hn_write8(&memory,0x03007800+i,0xA5);
    CHECK(hn_read8(&memory,HN_GROUP3_LIST_TEXT)==2&&!hn_read8(&memory,HN_GROUP3_LIST_TEXT+1));
    CHECK(hn_task_remove(&memory,task));hn_write8(&memory,controls+0x60,1);
    CHECK(hn_group3_list_text(&memory,HN_GROUP3_LIST_TEXT));
    CHECK(hn_read8(&memory,HN_GROUP3_LIST_TEXT+1)==3&&!hn_read8(&memory,HN_GROUP3_LIST_TEXT+2));
    CHECK(hn_task_remove(&memory,hn_read32(&memory,0x030037A0)));hn_write8(&memory,controls+0x60,2);
    CHECK(hn_group3_list_text(&memory,HN_GROUP3_LIST_TEXT)&&hn_read8(&memory,controls+0x60)==255);
    CHECK(hn_read8(&memory,HN_GROUP3_LIST_TEXT+2)==4&&!hn_read8(&memory,HN_GROUP3_LIST_TEXT+3));
    CHECK(hn_task_remove(&memory,hn_read32(&memory,0x030037A0)));
    CHECK(hn_group3_list_text(&memory,HN_GROUP3_LIST_TEXT)&&!hn_read8(&memory,HN_GROUP3_LIST_TEXT));
    CHECK(hn_task_remove(&memory,hn_read32(&memory,0x030037A0)));
    hn_write32(&memory,0x03002E04,0x030010C8);
    CHECK(!hn_group3_list_text(&memory,HN_GROUP3_LIST_TEXT)&&!memory.fault);
    hn_heap_init(&memory);hn_write8(&memory,controls+0x60,18);
    for(unsigned i=0;i<19;i++)hn_write8(&memory,0x02030100+i,2);
    hn_write8(&memory,0x02030113,0);
    CHECK(hn_group3_list_text(&memory,HN_GROUP3_LIST_TEXT)&&!hn_read8(&memory,HN_GROUP3_LIST_TEXT+19));
    CHECK(hn_task_remove(&memory,hn_read32(&memory,0x030037A0)));
    hn_write8(&memory,0x02030113,2);hn_write8(&memory,controls+0x60,19);hn_write8(&memory,HN_GROUP3_LIST_TEXT+20,0xA5);
    CHECK(!hn_group3_list_text(&memory,HN_GROUP3_LIST_TEXT)&&hn_read8(&memory,HN_GROUP3_LIST_TEXT+20)==0xA5&&!memory.fault);
    uint8_t *rom=calloc(1,0x1000000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x1000000);
    hn_heap_init(&memory);hn_write32(&memory,HN_TASK_HEAD,HN_TASK_SENTINEL);hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);
    hn_write8(&memory,controls+0x180,128);
    CHECK(hn_scene_callback(&memory,0x080C5789,0,&consumed)==HN_SCENE_OK);
    GameAddr first=hn_read32(&memory,HN_TASK_HEAD);CHECK(first&&hn_read32(&memory,first+0x2C)==HN_TASK_SENTINEL);
    CHECK(hn_read32(&memory,first+0x10)==HN_TASK_DEFAULT_SCRIPT&&hn_read16(&memory,0x02001C0E)==0xF2D5);
    CHECK(hn_task_remove(&memory,first));hn_write8(&memory,controls+0x180,0);hn_write8(&memory,list,1);
    hn_write32(&memory,0x02001D9C,0x02030000);hn_write32(&memory,0x02030000,0x02030100);hn_write32(&memory,0x0203000C,0x02030100);
    hn_write32(&memory,0x02030100,0x00620161);
    CHECK(hn_scene_callback(&memory,0x080C5789,0,&consumed)==HN_SCENE_OK);
    first=hn_read32(&memory,HN_TASK_HEAD);task=hn_read32(&memory,0x030037A0);
    CHECK(first!=task&&hn_read32(&memory,first+0x2C)==task&&hn_read32(&memory,task+0x2C)==HN_TASK_SENTINEL);
    CHECK(hn_read32(&memory,0x03003650)==0x000D010C&&hn_read32(&memory,task+0x10)==0x03003650);
    CHECK(hn_scene_callback(&memory,0x080C5BB9,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,first+0x2C)==HN_TASK_SENTINEL);
    CHECK(hn_task_remove(&memory,first));hn_write8(&memory,controls+0xEB,3);
    CHECK(hn_scene_callback(&memory,0x080C5789,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,0x03003650)==0x0062010C);
    hn_write8(&memory,controls+0x180,29);hn_write8(&memory,0x0300219F,0);hn_write8(&memory,0x0203F000,0);
    CHECK(hn_scene_callback(&memory,0x080C5BCD,0x0203F000,&consumed)==HN_SCENE_OK&&consumed==1);
    CHECK(hn_read8(&memory,controls+0x180)==157&&hn_read16(&memory,0x03001FBC)==1);
    hn_write8(&memory,0x0203F000,1);
    CHECK(hn_scene_callback(&memory,0x080C5BCD,0x0203F000,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x180)==29&&hn_read16(&memory,0x03001FBC)==2);
    CHECK(hn_group3_task(&memory,0x080C5BCD)==HN_SCENE_UNKNOWN&&!memory.fault);
    free(rom);
}
static void record_menu_contract(void)
{
    reset();GameAddr controls=HN_SCENE_CONTROLS;uint32_t consumed=99;
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);
    hn_write8(&memory,controls+0x8A,0);
    CHECK(hn_record_menu_callback(&memory,0x080C4635,0,&consumed)==HN_SCENE_OK&&!consumed&&hn_read8(&memory,controls+0x180)==255);
    hn_write8(&memory,controls+0x8A,26);
    CHECK(hn_record_menu_callback(&memory,0x080C4635,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x180)==25&&hn_read8(&memory,controls+0x181)==1);
    hn_write8(&memory,controls+0x8A,255);
    CHECK(hn_record_menu_callback(&memory,0x080C4635,0,&consumed)==HN_SCENE_OK&&!hn_read8(&memory,controls+0x180)&&!hn_read8(&memory,controls+0x60));
    hn_write16(&memory,HN_RASTER_SCANLINE,0xDCE0);
    CHECK(hn_record_menu_task(&memory,0x080C4991)==HN_SCENE_OK&&hn_read16(&memory,HN_RASTER_SCANLINE)==0xE0E0&&hn_read8(&memory,controls+0x60)==1);
    hn_write16(&memory,HN_RASTER_SCANLINE,0xFEE0);
    CHECK(hn_record_menu_task(&memory,0x080C4991)==HN_SCENE_OK&&hn_read16(&memory,HN_RASTER_SCANLINE)==0x02E0&&!hn_read8(&memory,controls+0x60));
    CHECK(hn_record_menu_task(&memory,0x080C48ED)==HN_SCENE_UNKNOWN&&hn_record_menu_task(&memory,0x080C4CBD)==HN_SCENE_UNKNOWN);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,19);hn_write8(&memory,0x03002170,0xA5);
    CHECK(hn_preset_apply(&memory,0x080221CD)==HN_SCENE_OK&&!hn_read8(&memory,0x03002170));
    CHECK(hn_read8(&memory,0x03001FAC)==19&&hn_read32(&memory,0x03001F7C)==0x088C2821);
    CHECK(hn_preset_apply(&memory,0x08022A81)==HN_SCENE_OK&&hn_read8(&memory,hn_render_slot(&memory,31)+0x60)==1);
    CHECK(hn_read8(&memory,0x03001FAC)==31&&hn_read32(&memory,0x03001F7C)==0x088E4EE1);
    CHECK(hn_scene_callback(&memory,0x080221CD,0,&consumed)==HN_SCENE_UNKNOWN&&!memory.fault);
}
static void item_selection_contract(void)
{
    /* Generated metadata exercises paging and task lifetime without a game dump. */
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);
    hn_memory_init(&memory,rom,0x900000);hn_heap_init(&memory);hn_task_contexts_init(&memory);
    const GameAddr controls=HN_SCENE_CONTROLS,items=0x03003900,contexts=0x030037A0,args=0x0203F000;
    uint32_t consumed=99;hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);
    memset(memory.iwram+0x3900,255,256);hn_write8(&memory,items,1);
    /* The text offset is a halfword; each synthetic bundle has five languages. */
    rom[0x6E78B4]=0;rom[0x6E78B5]=1;rom[0x6E77D0+5+4]=6;
    for(unsigned i=0;i<7;i++){
        GameAddr bundle=0x02030000+i*32;put32(rom+0x6E7910+(256+i)*4,bundle);
        for(unsigned language=0;language<5;language++)hn_write32(&memory,bundle+language*4,0x02031000+i*32+language*4);
    }
    hn_write8(&memory,controls+0xEB,4);
    CHECK(hn_record_menu_callback(&memory,0x080C3E45,0,&consumed)==HN_SCENE_OK&&!consumed);
    for(unsigned i=0;i<4;i++){
        GameAddr task=hn_read32(&memory,contexts+i*4);CHECK(task&&hn_read8(&memory,task+1)==14);
        CHECK(hn_read32(&memory,task+0x10)==0x02031010+i*32);
    }
    CHECK(hn_record_menu_task(&memory,0x080C4055)==HN_SCENE_OK&&hn_read32(&memory,HN_TASK_HEAD)==HN_TASK_SENTINEL);
    hn_write8(&memory,controls+0x183,3);
    CHECK(hn_record_menu_task(&memory,0x080C3F51)==HN_SCENE_OK);
    for(unsigned i=0;i<4;i++){
        GameAddr task=hn_read32(&memory,contexts+i*4);
        CHECK(hn_read8(&memory,task+1)==(i?14:0));
        CHECK(hn_read32(&memory,task+0x10)==(i?0x02031070+i*32:HN_TASK_DEFAULT_SCRIPT));
    }
    CHECK(hn_record_menu_task(&memory,0x080C4055)==HN_SCENE_OK&&hn_read32(&memory,HN_TASK_HEAD)==HN_TASK_SENTINEL);
    hn_write8(&memory,items,255);
    CHECK(hn_record_menu_task(&memory,0x080C3F51)==HN_SCENE_OK);
    for(unsigned i=0;i<4;i++){GameAddr task=hn_read32(&memory,contexts+i*4);CHECK(task&&!hn_read8(&memory,task+1)&&hn_read32(&memory,task+0x10)==HN_TASK_DEFAULT_SCRIPT);}
    CHECK(hn_record_menu_task(&memory,0x080C4055)==HN_SCENE_OK&&hn_read32(&memory,HN_TASK_HEAD)==HN_TASK_SENTINEL);
    hn_write8(&memory,items,1);hn_write8(&memory,controls+0x183,3);
    CHECK(hn_record_menu_task(&memory,0x080C3F01)==HN_SCENE_OK&&hn_read8(&memory,controls+0x183)==3);
    hn_write8(&memory,controls+0x183,255);
    CHECK(hn_record_menu_task(&memory,0x080C3F01)==HN_SCENE_OK&&!hn_read8(&memory,controls+0x183));
    hn_write8(&memory,controls+0x183,9);rom[0x6E77D9]=3;hn_write8(&memory,hn_render_slot(&memory,31)+0x60,7);
    CHECK(hn_record_menu_task(&memory,0x080C4571)==HN_SCENE_OK&&!hn_read8(&memory,hn_render_slot(&memory,31)+0x60)&&hn_read8(&memory,controls+0x183)==9);
    rom[0x6E77D9]=6;
    CHECK(hn_record_menu_task(&memory,0x080C4571)==HN_SCENE_OK&&!hn_read8(&memory,controls+0x183)&&hn_read8(&memory,hn_render_slot(&memory,31)+0x60)==1);
    /* Row edges must not wrap horizontally; both item-validity flags still update. */
    hn_write8(&memory,args,2);
    CHECK(hn_record_menu_callback(&memory,0x080C4079,args,&consumed)==HN_SCENE_OK&&consumed==1&&hn_read16(&memory,0x03001FBC)==1);
    CHECK((hn_read8(&memory,0x0300219F)&128)&&(hn_read8(&memory,0x030021A0)&1));
    hn_write8(&memory,args,3);
    CHECK(hn_record_menu_callback(&memory,0x080C4079,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x180)==1&&!hn_read16(&memory,0x03001FBC));
    CHECK((hn_read8(&memory,0x0300219F)&128)&&!(hn_read8(&memory,0x030021A0)&1));
    CHECK(hn_record_menu_task(&memory,0x080C4079)==HN_SCENE_UNKNOWN&&!memory.fault);
    /* A scrolled cursor in row zero underflows to 248 before slot filtering. */
    hn_write8(&memory,controls+0x180,0);hn_write32(&memory,hn_render_slot(&memory,0)+0x20,0x600001);
    for(unsigned i=4;i<29;i++)hn_write8(&memory,hn_render_slot(&memory,i)+0x60,7);
    CHECK(hn_record_menu_task(&memory,0x080C4381)==HN_SCENE_OK);
    for(unsigned i=4;i<28;i++)CHECK(!hn_read8(&memory,hn_render_slot(&memory,i)+0x60));
    CHECK(hn_read8(&memory,hn_render_slot(&memory,28)+0x60)==7);
    /* Debug keys have priority, and the disabled branch must not read input. */
    hn_write8(&memory,0x03002180,4);hn_write16(&memory,0x03001F64,1023);hn_write16(&memory,HN_KEY_STATE,0x340);
    CHECK(hn_record_menu_task(&memory,0x080C44C1)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC40)==0x340);
    for(unsigned i=0;i<45;i++){unsigned id=0x9E + i;CHECK(((hn_read8(&memory,0x03002180+(id>>3))>>(id&7))&1)==(i<16));}
    memset(memory.ewram+0x1CC20,0xA5,32);
    CHECK(hn_record_menu_task(&memory,0x080C45F1)==HN_SCENE_OK);
    for(unsigned i=0;i<32;i++)CHECK(hn_read8(&memory,0x0201CC20+i)==(i&&i<=30?1:0xA5));
    memset(memory.ewram+0x1EE40,0xA5,32);
    CHECK(hn_record_menu_task(&memory,0x080C56B9)==HN_SCENE_OK);
    for(unsigned i=0;i<32;i++)CHECK(hn_read8(&memory,0x0201EE40+i)==(i<30?i+1:0xA5));
    hn_write16(&memory,HN_KEY_STATE,0x40);
    CHECK(hn_record_menu_task(&memory,0x080C56B9)==HN_SCENE_OK);
    for(unsigned i=0;i<32;i++)CHECK(hn_read8(&memory,0x0201EE40+i)==(i<20?i+61:i<30?0:0xA5));
    hn_write16(&memory,HN_KEY_STATE,0x80);CHECK(hn_record_menu_task(&memory,0x080C44C1)==HN_SCENE_OK);
    for(unsigned i=0;i<45;i++){unsigned id=0x9E + i;CHECK(!(hn_read8(&memory,0x03002180+(id>>3))&(1u<<(id&7))));}
    hn_write8(&memory,0x03002180,0);hn_write16(&memory,0x0201DC40,0xA5A5);
    CHECK(hn_record_menu_task(&memory,0x080C44C1)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC40)==0xA5A5);
    CHECK(hn_record_menu_task(&memory,0x080C45F1)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC40)==0xA5A5);
    CHECK(hn_record_menu_task(&memory,0x080C56B9)==HN_SCENE_OK&&hn_read16(&memory,0x0201DC40)==0xA5A5);
    /* A panel creates its linked overlay only for selector seven. */
    rom[0x6E7098+28]=0xEC;rom[0x6E7098+29]=255;rom[0x6E7098+30]=41;
    hn_write8(&memory,controls+0xE5,7);
    CHECK(hn_record_menu_task(&memory,0x080C1B85)==HN_SCENE_OK&&!hn_read8(&memory,controls+0xE5));
    CHECK((int32_t)hn_read32(&memory,hn_render_slot(&memory,26)+0x18)==-20*65536+32768&&hn_read32(&memory,hn_render_slot(&memory,26)+0x1C)==41*65536+32768);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,27)+0x65)==26);
    hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,26);hn_write8(&memory,0x03002170,1);
    CHECK(hn_record_menu_task(&memory,0x080C1B85)==HN_SCENE_OK&&!hn_read8(&memory,0x03002170));
    CHECK(hn_record_menu_task(&memory,0x080C3CBD)==HN_SCENE_OK&&hn_read16(&memory,0x04000042)==0x14D4&&hn_read16(&memory,0x04000046)==0x5484);
    CHECK((hn_read8(&memory,HN_DISPLAY_SHADOW+1)&64)&&(hn_read32(&memory,HN_VIDEO_STATE)&1)&&!memory.fault);
    free(rom);
}
static void bank4_contract(void)
{
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x900000);
    const GameAddr controls=HN_SCENE_CONTROLS,args=0x0203F000,base=0x02008000;uint32_t consumed=99;
    hn_write32(&memory,HN_RENDER_SLOTS,base);hn_write8(&memory,HN_RENDER_INFO+16,1);
    hn_write32(&memory,HN_GRAPHICS_SCRIPTS,0x02016000);hn_write32(&memory,HN_RENDER_STATES,0x02020000);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);
    /* Coordinates are generated metadata, not original game resources. */
    put32(rom+0x67039C+8,8*65536);put32(rom+0x67039C+12,24*65536);
    hn_write8(&memory,controls,3);hn_write8(&memory,controls+0x1A6,3);hn_write8(&memory,controls+0x1A5,1);hn_write8(&memory,controls+0x1A9,255);
    CHECK(hn_bank4_callback(&memory,0x08068B21,0,&consumed)==HN_SCENE_OK&&!consumed);
    CHECK(!hn_read8(&memory,controls+0x1A5)&&hn_read8(&memory,controls+0x1AA)==5);
    CHECK((int32_t)hn_read32(&memory,base+30*104+0x24)==-9362&&hn_read32(&memory,base+30*104+0x2C)==0);
    CHECK(hn_read32(&memory,base+30*104+0x24)==hn_read32(&memory,base+31*104+0x24)&&hn_read32(&memory,base+30*104+0x34)==11702);
    hn_write8(&memory,controls+0x60,255);hn_write8(&memory,args,0);
    CHECK(hn_bank4_callback(&memory,0x08068D61,args,&consumed)==HN_SCENE_OK&&consumed==1&&!hn_read8(&memory,controls+0x60)&&!hn_read8(&memory,controls+0x61));
    hn_write32(&memory,args,args+2);hn_write16(&memory,args+4,0xBEEF);
    CHECK(hn_bank4_callback(&memory,0x0806BBE1,args,&consumed)==HN_SCENE_OK&&consumed==6&&hn_read16(&memory,args+2)==0xBEEF);
    GameAddr slot=base+20*104;hn_write32(&memory,slot+0x18,0x388001);hn_write32(&memory,slot+0x20,0x887FFF);
    CHECK(hn_bank4_projected_position(&memory,slot)==1&&!hn_read8(&memory,0x030021E0));
    hn_write32(&memory,slot+0x18,(uint32_t)-0x288001);
    CHECK(!hn_bank4_projected_position(&memory,slot)&&(hn_read8(&memory,0x030021E0)&4)&&hn_read16(&memory,HN_CHOICE_PENDING)==7);
    hn_write8(&memory,controls+0x1A3,9);hn_write8(&memory,controls+0x69,3);rom[0x670419]=3;
    CHECK(hn_bank4_task(&memory,0x0806A141)==HN_SCENE_OK&&!hn_read8(&memory,controls+0x1A3)&&hn_read8(&memory,controls+0x69)==255);
    hn_write8(&memory,controls+0x1A3,255);hn_write8(&memory,controls+0x69,0);
    CHECK(hn_bank4_task(&memory,0x0806A035)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,base+28*104+0x60)==1&&hn_read8(&memory,base+29*104+0x60)==1);
    for(unsigned i=30;i<40;i++)CHECK(!hn_read8(&memory,base+i*104+0x60));
    hn_write16(&memory,0x02030000,65535);CHECK(hn_bank4_accumulate(&memory,0x02030000,65535)==256&&hn_read16(&memory,0x02030000)==0xFEFF);
    hn_bank4_path_bounds(&memory,2,1,65535,0x02030000,0x02030000,0x02030000,0x02030000);
    CHECK(hn_read16(&memory,0x02030000)==19);
    hn_bank4_path_bounds(&memory,255,1,65535,0x02030000,0x02030000,0x02030000,0x02030000);
    CHECK(hn_read16(&memory,0x02030000)==19);
    CHECK(hn_bank4_path_blocked(5,6,10,11,10,10,11,11,1)&&!hn_bank4_path_blocked(5,6,10,11,10,10,11,11,0));
    /* Early waits preserve the original incoming SL decision explicitly. */
    const GameAddr state=controls+0x100,position=0x0201DC40+200,path=0x0202E000;
    hn_write8(&memory,args,16);hn_write16(&memory,args+1,0x100);hn_write16(&memory,args+3,100);
    hn_write16(&memory,args+5,96);hn_write32(&memory,args+7,path);hn_write8(&memory,args+11,0);
    hn_write8(&memory,0x0300218C,1);hn_write8(&memory,state,0);hn_write16(&memory,position,128);hn_write16(&memory,position+2,160);
    hn_write16(&memory,path,0x4000);hn_write16(&memory,path+2,2);hn_write32(&memory,0x020016D4,0x22222222);
    for(unsigned i=0;i<2;i++){
        hn_write8(&memory,state+1,2);hn_write32(&memory,base+16*104+4,0x11111111);
        CHECK(hn_bank4_follow_path(&memory,0x0806D795,args,i));
        CHECK(hn_read32(&memory,base+16*104+4)==(i?0x11111111u:0x22222222u)&&hn_read8(&memory,state+1)==1);
    }
    CHECK(!hn_bank4_follow_path(&memory,0x08000001,args,0)&&!memory.fault);
    CHECK(hn_bank4_callback(&memory,0x080678C1,0,&consumed)==HN_SCENE_UNKNOWN&&hn_bank4_task(&memory,0x0806BBE1)==HN_SCENE_UNKNOWN);
    free(rom);
}
static void bank5_contract(void)
{
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x900000);
    const GameAddr base=0x02008000,controls=HN_SCENE_CONTROLS,classes=0x0201DC40,args=0x0203F000;
    hn_write32(&memory,HN_RENDER_SLOTS,base);hn_write8(&memory,HN_RENDER_INFO+16,64);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);
    uint32_t consumed=99,yield=99;HNScript script;hn_script_init(&script,&memory,NULL,0);
    hn_write32(&memory,args,UINT32_MAX);CHECK(hn_main_command(&script,0x4E,args));
    CHECK(!hn_read32(&memory,0x03002B90)&&!hn_read32(&memory,0x030005E8)&&hn_read32(&memory,0x03000014)==4);
    hn_write32(&memory,0x03002BB0+3*4,0x080864B5);hn_write32(&memory,args,3);
    CHECK(hn_main_command(&script,0x4E,args)&&hn_read32(&memory,0x03002B90)==0x080864B5);
    CHECK(hn_read32(&memory,0x030005E8)==0x080864B5);
    CHECK(hn_bank5_callback(&memory,0x08084DC1,0,&consumed)==HN_SCENE_UNKNOWN);
    CHECK(hn_bank5_callback(&memory,0x080855F5,0,&consumed)==HN_SCENE_OK&&!consumed);
    CHECK(hn_bank5_irq(&memory,0x08086551)==HN_SCENE_UNKNOWN);
    hn_write16(&memory,classes+12,11);hn_write16(&memory,classes+18,10);
    CHECK(hn_bank5_callback(&memory,0x0808509D,0,&consumed)==HN_SCENE_OK&&!hn_read16(&memory,classes+12));
    /* An eight-bit sum, rather than an unbounded sum, determines the win. */
    hn_write8(&memory,0x020046D3,255);hn_write8(&memory,0x020046D4,1);hn_write8(&memory,0x030021D8,128);
    for(unsigned i=18;i<21;i++){hn_write32(&memory,base+i*104+0x24,123);hn_write32(&memory,base+i*104+0x28,456);}
    CHECK(hn_bank5_outcome(&memory)==1&&hn_read8(&memory,0x020046DE)==2);
    CHECK((hn_read8(&memory,HN_CHOICE_FLAGS)&16)&&hn_read32(&memory,HN_CHOICE_PENDING)==2);
    CHECK(!hn_read32(&memory,base+18*104+0x24)&&!hn_read32(&memory,base+18*104+0x28));
    hn_write8(&memory,0x020046D7,0);hn_write32(&memory,base+18*104+0x20,0x9C0001);
    CHECK(hn_bank5_outcome(&memory)==1&&hn_read8(&memory,0x020046DE)==1&&hn_read32(&memory,HN_CHOICE_PENDING)==3);
    hn_write32(&memory,base+18*104+0x20,0x800000);hn_write8(&memory,0x020046DA,1);
    hn_write32(&memory,base+21*104+0x20,0x700000);hn_write32(&memory,0x03001150,UINT32_MAX);
    CHECK(hn_bank5_primary_lane(&memory,0)&&hn_read8(&memory,0x020046D7)==1&&hn_read8(&memory,0x020046DA)==2);
    CHECK(!hn_read32(&memory,0x03001150)&&hn_read32(&memory,base+18*104+4)==0x088EFF04);
    hn_write8(&memory,0x020046DA,1);hn_write32(&memory,base+21*104+0x20,0xFFFFFFFF);
    CHECK(hn_bank5_partner_lane(&memory,0)&&hn_read8(&memory,0x020046DA)==2);
    hn_write32(&memory,0x03002BB0,150);
    for(unsigned i=0;i<3;i++)hn_write16(&memory,classes+(63+i)*2,(uint16_t)((i+1)*100));
    CHECK(hn_bank5_callback(&memory,0x08086CD1,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,controls)==2&&hn_read8(&memory,controls+2)==2&&hn_read8(&memory,controls+3)==50);
    CHECK(hn_read16(&memory,classes+126)==100&&hn_read16(&memory,classes+128)==150&&hn_read16(&memory,classes+130)==200);
    hn_write32(&memory,0x03002BB0,65536);hn_write8(&memory,controls,123);
    CHECK(hn_bank5_callback(&memory,0x08086CD1,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls)==123);
    const uint16_t depths[]={70,88,128,241,32768},expected[]={72,88,224,240,72};
    for(unsigned i=0;i<5;i++){
        hn_write32(&memory,base+13*104+0x20,((uint32_t)depths[i]<<16)|0x1234);
        CHECK(hn_bank5_callback(&memory,0x08087B05,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,base+13*104+0x20)==((uint32_t)expected[i]<<16)+0x8000);
    }
    hn_write8(&memory,0x02001880,0);
    CHECK(hn_bank5_animation(&memory,0x080875A5,base,0,&consumed,&yield)==HN_SCENE_OK&&yield==1&&hn_read8(&memory,0x02001880)==255);
    hn_write8(&memory,0x02001880,1);
    CHECK(hn_bank5_animation(&memory,0x080875A5,base,0,&consumed,&yield)==HN_SCENE_OK&&hn_read8(&memory,0x02001880)==192);
    hn_write8(&memory,controls+0x1B0,1);hn_write8(&memory,controls+0x1AD,3);hn_write8(&memory,controls+0x1AE,3);hn_write8(&memory,controls+0x1B2,255);
    CHECK(hn_bank5_animation(&memory,0x08087A59,base,0,&consumed,&yield)==HN_SCENE_OK&&!hn_read8(&memory,controls+0x1B2)&&hn_read8(&memory,controls+0x1B1)==1);
    GameAddr aliased=base+10*104+0x18;hn_write32(&memory,aliased,0x7F80FF0A);
    CHECK(hn_bank5_callback(&memory,0x08087049,aliased,&consumed)==HN_SCENE_OK&&consumed==4);
    CHECK(hn_read32(&memory,aliased)==0x7F7FFF0A&&hn_read32(&memory,aliased+4)==0xFF800000&&hn_read32(&memory,aliased+8)==0x7F0000);
    hn_write8(&memory,controls+0x75,4);hn_write16(&memory,classes+47*2,65535);
    hn_write16(&memory,classes+49*2,32768);hn_write16(&memory,classes+51*2,123);
    CHECK(hn_scene_callback(&memory,0x08020199,0,&consumed)==HN_SCENE_OK&&!consumed);
    CHECK(hn_read8(&memory,base+13*104+0x60)==4&&hn_read32(&memory,base+13*104+4)==0x088DB5DC);
    CHECK(hn_read32(&memory,base+13*104+0x18)==0xFFFF8000&&hn_read32(&memory,base+13*104+0x1C)==0x80008000);
    hn_write8(&memory,0x03002180,128);
    CHECK(hn_scene_callback(&memory,0x080202AD,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,0x03002180)==160);
    hn_write16(&memory,classes+56*2,255);GameAddr target=base+255*104;
    hn_write32(&memory,target+0x24,1);hn_write32(&memory,target+0x28,2);hn_write32(&memory,target+0x2C,3);hn_write32(&memory,target+0x34,4);yield=0xABCDEF01;
    CHECK(hn_scene_animation_callback(&memory,0x08020261,0,0,&consumed,&yield)==HN_SCENE_OK&&!consumed&&yield==0xABCDEF01);
    CHECK(!hn_read32(&memory,target+0x24)&&!hn_read32(&memory,target+0x28)&&!hn_read32(&memory,target+0x2C)&&!hn_read32(&memory,target+0x34));
    CHECK(!memory.fault);free(rom);
}
static void bank6_contract(void)
{
    uint8_t *rom=calloc(1,0x900000);CHECK(rom!=NULL);hn_memory_init(&memory,rom,0x900000);
    const GameAddr base=0x02008000,classes=0x0201DC40,controls=HN_SCENE_CONTROLS,args=0x0203F000;
    hn_write32(&memory,HN_RENDER_SLOTS,base);hn_write8(&memory,HN_RENDER_INFO+16,64);
    hn_write8(&memory,HN_RENDER_CLASS_LEADER,10);hn_write8(&memory,HN_RENDER_CLASS_FOLLOWER,13);
    hn_write8(&memory,0x0201EE40,5);hn_write8(&memory,0x0201EE41,7);hn_write8(&memory,0x0201EE42,7);hn_write8(&memory,0x0201EE5F,9);
    hn_write8(&memory,args,7);uint32_t consumed=99,yield=99;
    CHECK(hn_bank6_callback(&memory,0x080A0851,args,&consumed)==HN_SCENE_OK&&consumed==1&&hn_read8(&memory,controls+0x60)==1);
    CHECK(hn_bank6_callback(&memory,0x080A0891,args,&consumed)==HN_SCENE_OK&&hn_read8(&memory,0x0201EE41)==7&&hn_read8(&memory,0x0201EE42)==0);
    CHECK(hn_read8(&memory,0x0201EE5E)==9&&!hn_read8(&memory,0x0201EE5F));
    hn_write8(&memory,args,255);CHECK(hn_bank6_callback(&memory,0x080A0851,args,&consumed)==HN_SCENE_OK&&!hn_read8(&memory,controls+0x60));
    /* The low halfword of the class selector and the low byte of the slot are distinct. */
    hn_write16(&memory,classes+224*2,0xF000);hn_write32(&memory,base+25*104+0x18,10*65536+0x8000);hn_write32(&memory,base+25*104+0x20,20*65536+0x8000);
    put16(rom+0x58EA10+319*2,40);put16(rom+0x58EA10+255*2,(uint16_t)-8);
    CHECK(hn_bank6_orbit(&memory,0xABCD001A,0xABCD00E0));
    CHECK(hn_read16(&memory,classes+224*2)==1023&&hn_read32(&memory,base+26*104+0x18)==14*65536&&hn_read32(&memory,base+26*104+0x20)==14*65536);
    hn_write16(&memory,classes+224*2,1024);CHECK(hn_bank6_orbit(&memory,26,224)&&!hn_read16(&memory,classes+224*2));
    hn_write16(&memory,classes+224*2,65535);put16(rom+0x6BF6DC,1);CHECK(hn_bank6_orbit(&memory,26,224)&&!hn_read16(&memory,classes+224*2));
    GameAddr slot=base+16*104;hn_write32(&memory,slot+0x18,UINT32_MAX);hn_write32(&memory,slot+0x1C,0x80000000);
    hn_bank6_planar_transform(&memory,slot,2,3,UINT32_MAX,4);
    CHECK(hn_read32(&memory,slot+0x18)==0xFFFFFFF4&&hn_read32(&memory,slot+0x1C)==0x8000000A);
    /* Explicit retained descriptor, including the source's no-match path. */
    GameAddr set=0x02030000,positions=0x02031000;hn_write32(&memory,set,positions);hn_write16(&memory,set+12,0x100);hn_write16(&memory,set+14,0x101);
    hn_write8(&memory,controls+0x100,1);hn_write8(&memory,controls+0x101,2);hn_write8(&memory,args,255);
    hn_write16(&memory,positions+4,65535);hn_write16(&memory,positions+6,32768);hn_write16(&memory,positions+8,123);hn_write16(&memory,positions+10,456);
    CHECK(hn_bank6_restore_targets(&memory,args,set));
    CHECK(hn_read32(&memory,base+25*104+0x18)==0xFFFF8000&&hn_read32(&memory,base+25*104+0x20)==0x80008000);
    CHECK(hn_read32(&memory,base+26*104+0x18)==123*65536+0x8000&&hn_read32(&memory,base+26*104+0x20)==456*65536+0x8000);
    hn_write32(&memory,0x030005E8,positions);hn_write16(&memory,0x030005F4,0x100);hn_write16(&memory,0x030005F6,0x101);
    CHECK(hn_bank6_callback(&memory,0x080A28B5,args,&consumed)==HN_SCENE_OK&&consumed==1);
    for(unsigned i=0;i<32;i++)hn_write8(&memory,HN_AFFINE_USAGE+i,1);
    hn_write8(&memory,slot+0x64,255);hn_write16(&memory,classes+110,64);
    CHECK(hn_bank6_animation(&memory,0x080A2AD5,slot,0,&consumed,&yield)==HN_SCENE_OK&&!yield&&!consumed);
    CHECK(hn_read8(&memory,slot+0x64)==32&&hn_read8(&memory,HN_AFFINE_USAGE+32)==1);
    CHECK(hn_read16(&memory,slot+0x3C)==160&&hn_read16(&memory,slot+0x3E)==160&&hn_read16(&memory,classes+110)==63);
    hn_write8(&memory,args,14);hn_write8(&memory,args+1,1);hn_write8(&memory,args+2,2);hn_write8(&memory,args+3,20);hn_write8(&memory,args+4,20);
    hn_write32(&memory,base+14*104+0x18,100*65536);hn_write32(&memory,base+14*104+0x20,100*65536);hn_write8(&memory,base+14*104+0x60,1);
    hn_write32(&memory,base+12*104+0x18,90*65536);hn_write32(&memory,base+12*104+0x20,90*65536);
    CHECK(hn_bank6_callback(&memory,0x080A2519,args,&consumed)==HN_SCENE_OK&&consumed==5&&hn_read8(&memory,controls+0x60)==1);
    hn_write32(&memory,base+12*104+0x18,104*65536);
    CHECK(hn_bank6_callback(&memory,0x080A2519,args,&consumed)==HN_SCENE_OK&&!hn_read8(&memory,controls+0x60));
    hn_write32(&memory,base+12*104+0x18,hn_read32(&memory,base+14*104+0x18)+0x80000);
    hn_write32(&memory,base+12*104+0x20,hn_read32(&memory,base+14*104+0x20)+0x70000);
    CHECK(hn_bank6_callback(&memory,0x080A2351,args,&consumed)==HN_SCENE_OK&&consumed==1&&hn_read8(&memory,controls+0x1A1)==1);
    CHECK(!hn_read32(&memory,base+11*104+0x24)&&!hn_read32(&memory,base+11*104+0x2C));
    hn_write16(&memory,args,10);hn_write16(&memory,args+2,12);hn_write8(&memory,args+4,55);hn_write32(&memory,slot+0x18,0x18000);hn_write32(&memory,slot+0x20,0x38000);
    CHECK(hn_bank6_animation(&memory,0x080A0F45,slot,args,&consumed,&yield)==HN_SCENE_OK&&consumed==5&&!yield);
    CHECK(hn_read16(&memory,classes+110)==8&&hn_read32(&memory,slot+0x24)==0x11000&&hn_read32(&memory,slot+0x2C)==0x11000);
    hn_write8(&memory,args,1);hn_write8(&memory,args+1,2);hn_write32(&memory,args+2,0x08012345);
    hn_write32(&memory,base+1*104+0x18,0);hn_write32(&memory,base+2*104+0x18,UINT32_MAX);
    CHECK(hn_bank6_callback(&memory,0x080A1F49,args,&consumed)==HN_SCENE_OK&&consumed==UINT32_MAX-3&&hn_read32(&memory,0x03002C60)==0x08012345);
    hn_write32(&memory,slot+4,0);hn_write32(&memory,slot+0x18,0);hn_write32(&memory,base+22*104+0x18,0);
    CHECK(hn_bank6_callback(&memory,0x080A39CD,0,&consumed)==HN_SCENE_OK&&hn_read32(&memory,slot+0x24)==0x10000);
    CHECK(hn_bank6_callback(&memory,0x0809FE41,0,&consumed)==HN_SCENE_UNKNOWN&&hn_bank6_task(&memory,0x080A28B5)==HN_SCENE_UNKNOWN);
    CHECK(!memory.fault);free(rom);
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
    /* The real clock delivers the reviewed VBlank hook before audio service. */
    hn_memory_init(&game->memory,rom,0x1000000);game->fault=HN_GAME_OK;
    hn_write32(&game->memory,HN_AUDIO_CORE_POINTER,HN_AUDIO_CORE);
    hn_write32(&game->memory,HN_RENDER_SLOTS,0x02008000);
    hn_write32(&game->memory,0x03001F98,1);hn_write32(&game->memory,0x03001F5C,1);
    hn_write32(&game->memory,0x03002B90,0x080864B5);hn_write8(&game->memory,0x020046D3,1);
    hn_write16(&game->memory,0x04000200,1);hn_write16(&game->memory,0x04000208,1);game->memory.io[0x202]=1;
    hn_clock_advance(&game->clock,&game->memory,1);
    CHECK(hn_read32(&game->memory,0x03002BB0)==1&&game->fault==HN_GAME_OK);
    game->fault=HN_GAME_OK;hn_write32(&game->memory,0x03002B90,0x08086551);game->memory.io[0x202]=1;
    hn_clock_advance(&game->clock,&game->memory,1);
    CHECK(game->fault==HN_GAME_IRQ&&game->address==0x08086551&&hn_read32(&game->memory,0x03002BB0)==1);
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
    text_contract();panel_contract();actor_contract();counter_contract();motion_contract();task_contract();choice_contract();following_contract();collision_contract();indicator_contract();book_contract();actor_snapshot_contract();preset_contract();map_choice_contract();particles_contract();scene_motion_contract();bank2_contract();wander_contract();scene_actor_contract();actor_presets_contract();records_contract();wave_contract();counter_scene_contract();shared_motion_contract();runtime_contract();
    scene_effects_contract();bank3_contract();bank4_contract();bank5_contract();bank6_contract();group3_contract();record_menu_contract();item_selection_contract();printf("%u native game subsystem checks passed.\n",checks);return 0;
}
