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
#include "game/random.h"
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
    CHECK(hn_scene_callback(&memory,0x08001001,0,&consumed)==HN_SCENE_UNKNOWN);CHECK(!memory.fault);free(rom);
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
int main(void)
{
    dma_contract();transfer_contract();video_contract();map_contract();expression_contract();save_contract();scene_contract();
    printf("%u native game subsystem checks passed.\n",checks);return 0;
}
