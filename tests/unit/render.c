#include "game/render.h"
#include "game/background.h"
#include "game/state.h"
#include "game/script.h"
#include "game/scene.h"
#include "game/transfer.h"
#include "gba/bios.h"
#include "gba/video.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static HNMemory memory;
static uint8_t *rom;
static uint32_t pixels[HN_WIDTH*HN_HEIGHT];
static unsigned checks;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c);exit(1);}}while(0)
static void reset(void)
{
    hn_memory_init(&memory,rom,0x600000);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02010000);hn_write32(&memory,HN_RENDER_PRIORITY,0x0200F000);
    hn_write32(&memory,HN_RENDER_ORDER,0x0200E000);hn_write8(&memory,HN_RENDER_INFO+0x10,16);
    hn_write8(&memory,HN_RENDER_INFO+0x11,8);hn_write8(&memory,HN_RENDER_INFO+0x12,15);
    hn_write32(&memory,HN_RENDER_INFO,0x400000);hn_write32(&memory,HN_RENDER_INFO+4,0x400000);
    hn_write16(&memory,HN_RENDER_INFO+8,0x1C0);hn_write16(&memory,HN_RENDER_INFO+10,0xC0);
    hn_render_reset(&memory);
    for(unsigned i=0;i<32;i++)hn_write8(&memory,HN_AFFINE_USAGE+i,255);
}
static void configuration(void)
{
    reset();hn_write8(&memory,0x03001F50,3);hn_write8(&memory,0x03002170,1);hn_write8(&memory,0x03002182,0x85);
    CHECK(hn_render_configure(&memory,3,1,0x70,-1,0x18000,0x10000,0x02020000));
    GameAddr slot=hn_render_slot(&memory,3);CHECK(hn_read32(&memory,slot+4)==0x02020000);
    CHECK(hn_read32(&memory,slot+0x18)==0xFFFF8000&&hn_read32(&memory,slot+0x1C)==0x80008000);
    CHECK(hn_read32(&memory,slot+0x20)==0x8000&&hn_read8(&memory,slot+0x61)==0x68);
    CHECK(hn_read8(&memory,slot+0x60)==1&&hn_read8(&memory,slot+0x62)==0x70);
    CHECK(hn_read8(&memory,0x03002170)==0&&hn_read8(&memory,0x03002182)==5);
    CHECK(!hn_render_configure(&memory,16,1,0,0,0,0,0));CHECK(!memory.fault);
    CHECK(hn_render_configure(&memory,3,0,0x80,1,2,3,0));CHECK(hn_read32(&memory,slot+4)==0);
}
static void matrices(void)
{
    int16_t out[4];hn_bios_obj_matrix(256,512,0,out);
    CHECK(out[0]==256&&out[1]==0&&out[2]==0&&out[3]==512);
    hn_bios_obj_matrix(256,512,0x4000,out);
    CHECK(out[0]==0&&out[1]==-256&&out[2]==512&&out[3]==0);
    reset();hn_write16(&memory,0x02020000,256);hn_write16(&memory,0x02020002,512);
    hn_write16(&memory,0x02020004,0x4000);
    CHECK(hn_bios_obj_affine(&memory,0x02020000,0x03003086,1,8));
    CHECK(hn_read16(&memory,0x03003086)==0);
    CHECK((int16_t)hn_read16(&memory,0x0300308E)==-256);
    CHECK(hn_read16(&memory,0x03003096)==512);
    CHECK(!hn_bios_obj_affine(&memory,0x02020000,0x03003086,1025,8));
    for(unsigned i=0;i<3;i++){
        GameAddr p=hn_render_slot(&memory,i);hn_write8(&memory,p+0x60,1);hn_write8(&memory,p+0x64,0);
    }
    hn_write16(&memory,hn_render_slot(&memory,2)+0x40,256);
    CHECK(hn_render_assign_affine(&memory));
    CHECK(hn_read8(&memory,hn_render_slot(&memory,0)+0x64)==0);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,1)+0x64)==0);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,2)+0x64)==1);
}
static void animations(void)
{
    reset();GameAddr slot=hn_render_slot(&memory,0);HNRender render;
    hn_render_init(&render,&memory,NULL,0);hn_write8(&memory,slot+0x60,1);
    hn_write32(&memory,slot+4,0x02020000);
    const uint8_t program[]={0x87,3,0xA9,1,0,2,0,0,2,2,0x88,0x80};
    for(unsigned i=0;i<sizeof(program);i++)hn_write8(&memory,0x02020000+i,program[i]);
    for(unsigned i=0;i<6;i++)CHECK(hn_render_animation(&render,slot,32));
    CHECK(hn_read32(&memory,slot+0x18)==3*65536);
    CHECK(hn_read8(&memory,slot+0x60)==1);
    CHECK(hn_render_animation(&render,slot,32));
    CHECK(hn_read8(&memory,slot+0x60)==0);
    hn_render_init(&render,&memory,NULL,0);hn_write32(&memory,slot+4,0x02020000);hn_write16(&memory,slot+0x4C,0);
    hn_write8(&memory,0x02020000,0x91);
    CHECK(!hn_render_animation(&render,slot,8));
    CHECK(render.fault==HN_RENDER_BUDGET&&render.fault_address==0x02020000);
    hn_render_init(&render,&memory,NULL,0);hn_write8(&memory,0x02020000,0x8D);
    hn_write32(&memory,0x02020001,0x08001001);
    CHECK(!hn_render_animation(&render,slot,8));
    CHECK(render.fault==HN_RENDER_CALLBACK&&render.fault_address==0x08001001);
    hn_render_init(&render,&memory,NULL,0);hn_write8(&memory,0x02020000,0xD0);
    GameAddr cursor=0x02020000;uint32_t yield;
    CHECK(!hn_animation_command(&render,slot,0xD0,&cursor,&yield));
    CHECK(render.fault==HN_RENDER_RESOURCE);
    hn_render_init(&render,&memory,NULL,0);hn_write8(&memory,0x03002182,64);
    CHECK(hn_animation_command(&render,slot,0xD0,&cursor,&yield));
    CHECK(cursor==0x02020003&&!yield);
}
static uint32_t callback(HNMemory *m,GameAddr slot,GameAddr args,uint32_t *yield)
{
    hn_write8(m,slot+0x67,hn_read8(m,args));*yield=1;return 1;
}
static void callbacks_and_motion(void)
{
    reset();GameAddr slot=hn_render_slot(&memory,0);
    HNAnimationCallback binding={0x08001000,callback};HNRender render;
    hn_render_init(&render,&memory,&binding,1);GameAddr cursor=0x02020000;uint32_t yield;
    hn_write32(&memory,cursor+1,0x08001001);hn_write8(&memory,cursor+5,0xA0);
    CHECK(hn_animation_command(&render,slot,0x8D,&cursor,&yield));
    CHECK(yield==1&&cursor==0x02020006&&hn_read8(&memory,slot+0x67)==0xA0);
    for(unsigned thumb=0;thumb<2;thumb++){
        const uint32_t yields[]={0,1,0x80000000,0xA5C37E91};
        for(unsigned i=0;i<sizeof(yields)/sizeof(yields[0]);i++){
            uint32_t consumed=99;yield=yields[i];
            CHECK(hn_scene_animation_callback(&memory,0x0806BAD4|thumb,0xFFFFFFFF,0xFFFFFFFF,&consumed,&yield)==HN_SCENE_OK);
            CHECK(consumed==0&&yield==yields[i]&&!memory.fault);
        }
        hn_render_init(&render,&memory,NULL,0);cursor=0x02020100;
        hn_write32(&memory,slot+4,cursor);hn_write16(&memory,slot+0x4C,0);
        hn_write8(&memory,slot+0x60,1);hn_write8(&memory,cursor,0x8D);
        hn_write32(&memory,cursor+1,0x0806BAD4|thumb);hn_write8(&memory,cursor+5,0x80);
        CHECK(hn_animation_command(&render,slot,0x8D,&cursor,&yield));
        CHECK(cursor==0x02020105&&hn_read16(&memory,slot+0x4C)==5&&yield==0);
        CHECK(hn_animation_command(&render,slot,0x80,&cursor,&yield));
        CHECK(yield==1&&hn_read8(&memory,slot+0x60)==0&&!memory.fault);
    }
    hn_write32(&memory,slot+0x18,0x7FFFFFFF);hn_write32(&memory,slot+0x24,1);hn_write32(&memory,slot+0x30,0xFFFFFFFF);
    hn_write16(&memory,slot+0x3C,510);hn_write16(&memory,slot+0x44,10);
    hn_write16(&memory,slot+0x40,65535);hn_write16(&memory,slot+0x48,1);
    hn_render_physics(&memory,slot);
    CHECK(hn_read32(&memory,slot+0x18)==0x80000000);
    CHECK(hn_read32(&memory,slot+0x24)==0);
    CHECK(hn_read16(&memory,slot+0x3C)==512);
    CHECK(hn_read16(&memory,slot+0x40)==0);
    CHECK(!hn_render_reorder(&memory,3,255,0));
    CHECK(!hn_render_reorder(&memory,3,4,2));
}
static void drawing(void)
{
    reset();HNRender render;hn_render_init(&render,&memory,NULL,0);
    GameAddr slot=hn_render_slot(&memory,0);
    hn_write8(&memory,slot+0x60,1);hn_write8(&memory,slot+0x61,0);
    hn_write32(&memory,slot,0x02020000);hn_write8(&memory,0x02020000,1);
    for(unsigned i=0;i<128;i++)hn_write16(&memory,HN_OAM_SHADOW+i*8+6,0xBEEF);
    CHECK(hn_render_draw(&render));
    CHECK(hn_read16(&memory,HN_OAM_SHADOW)==0&&hn_read16(&memory,HN_OAM_SHADOW+2)==0);
    CHECK(hn_read16(&memory,HN_OAM_SHADOW+8)==160&&hn_read16(&memory,HN_OAM_SHADOW+10)==240);
    CHECK(hn_read16(&memory,HN_OAM_SHADOW+6)==0xBEEF);
    CHECK(hn_memory_copy(&memory,0x07000000,HN_OAM_SHADOW,1024));
    hn_write16(&memory,0x04000000,0x1040);hn_write16(&memory,0x05000202,31);
    for(unsigned i=0;i<16;i++)hn_write16(&memory,0x06010000+i*2,0x1111);
    hn_video_render(&memory,pixels);
    CHECK(pixels[0]==0xFF0000FF&&pixels[7]==0xFF0000FF);
    CHECK(pixels[8]==0xFF000000);
    hn_write8(&memory,slot+0x64,0);hn_write16(&memory,slot+0x3C,512);hn_write16(&memory,slot+0x3E,512);
    CHECK(hn_render_draw(&render));
    CHECK(hn_read16(&memory,HN_OAM_SHADOW+6)==128&&hn_read16(&memory,HN_OAM_SHADOW+30)==128);
    reset();hn_render_init(&render,&memory,NULL,0);
    hn_write8(&memory,HN_RENDER_INFO+0x12,10);
    hn_write32(&memory,hn_render_slot(&memory,10)+0x20,5<<16);
    hn_write32(&memory,hn_render_slot(&memory,9)+0x20,4<<16);
    hn_write32(&memory,hn_render_slot(&memory,8)+0x20,5<<16);
    GameAddr priority=hn_read32(&memory,HN_RENDER_PRIORITY);
    hn_write8(&memory,priority+8*8+5,10);hn_write8(&memory,priority+9*8+5,9);hn_write8(&memory,priority+10*8+5,8);
    CHECK(hn_render_draw(&render));
    CHECK(hn_read8(&memory,priority+8*8+5)==10);
    CHECK(hn_read8(&memory,priority+9*8+5)==8);
    CHECK(hn_read8(&memory,priority+10*8+5)==9);
    CHECK(!memory.fault);
}
static void pixel_contracts(void)
{
    reset();
    for(unsigned i=0;i<128;i++)hn_write16(&memory,0x07000000+i*8,0x200);
    hn_write16(&memory,0x07000000,0x2000);hn_write16(&memory,0x07000004,1);
    hn_write16(&memory,0x04000000,0x1040);
    hn_write16(&memory,0x06010000,0x0202);hn_write16(&memory,0x06010020,0x0101);
    hn_write16(&memory,0x05000202,31);hn_write16(&memory,0x05000204,31<<5);
    hn_video_render(&memory,pixels);CHECK(pixels[0]==0xFF0000FF);
    hn_write16(&memory,0x04000000,0x1000);hn_video_render(&memory,pixels);
    CHECK(pixels[0]==0xFF00FF00);
    /* Horizontal columns wrap within a 2D OBJ tile row. */
    hn_write16(&memory,0x07000002,0x4000);hn_write16(&memory,0x07000004,30);
    for(unsigned i=0;i<4;i++){
        hn_write16(&memory,0x06010000+30*32+i*2,0x0101);
        hn_write16(&memory,0x06010000+i*2,0x0303);
        hn_write16(&memory,0x06010000+32*32+i*2,0x0202);
    }
    hn_write16(&memory,0x05000206,31<<10);hn_video_render(&memory,pixels);
    CHECK(pixels[0]==0xFF0000FF&&pixels[8]==0xFFFF0000);
    /* The global mosaic phase remains fixed across a window boundary. */
    reset();hn_write16(&memory,0x04000000,0x2100);hn_write16(&memory,0x04000008,0x0140);
    hn_write16(&memory,0x06000800,1);
    hn_write16(&memory,0x06000020,0x1111);hn_write16(&memory,0x06000022,0x2222);
    hn_write16(&memory,0x05000000,31<<10);hn_write16(&memory,0x05000002,31);hn_write16(&memory,0x05000004,31<<5);
    hn_write16(&memory,0x0400004C,3);hn_write16(&memory,0x04000040,0x0107);
    hn_write16(&memory,0x04000044,0x0008);hn_write16(&memory,0x04000048,1);
    hn_video_render(&memory,pixels);
    CHECK(pixels[0]==0xFFFF0000&&pixels[1]==0xFF0000FF&&pixels[3]==0xFF0000FF);
    CHECK(pixels[4]==0xFF00FF00&&pixels[6]==0xFF00FF00&&pixels[7]==0xFFFF0000);
    /* Pixels outside mode 5 use the backdrop's effect selection. */
    reset();hn_write16(&memory,0x04000000,0x0405);
    hn_write16(&memory,0x04000020,256);hn_write16(&memory,0x04000026,256);
    hn_write16(&memory,0x05000000,31);hn_write16(&memory,0x06000000,31<<10);
    hn_write16(&memory,0x04000050,0x0084);hn_write16(&memory,0x04000054,8);
    hn_video_render(&memory,pixels);
    CHECK(pixels[0]==0xFFFF7B7B&&pixels[150*240+200]==0xFF0000FF);
    /* A transparent later OBJ can promote a visible OBJ's priority. */
    reset();hn_write16(&memory,0x04000000,0x1140);hn_write16(&memory,0x04000008,0x0101);
    hn_write16(&memory,0x06000800,1);hn_write16(&memory,0x06000020,0x2222);
    hn_write16(&memory,0x05000004,31<<5);hn_write16(&memory,0x05000202,31);
    for(unsigned i=0;i<128;i++)hn_write16(&memory,0x07000000+i*8,0x200);
    hn_write16(&memory,0x07000000,0);hn_write16(&memory,0x07000004,0x0800);
    hn_write16(&memory,0x06010000,0x1111);
    hn_video_render(&memory,pixels);CHECK(pixels[0]==0xFF00FF00);
    hn_write16(&memory,0x07000008,0);hn_write16(&memory,0x0700000C,1);
    hn_video_render(&memory,pixels);CHECK(pixels[0]==0xFF0000FF);
}
static void clipped_text_tiles(void)
{
    /* Analytical 4bpp colors retain sub-tile window/flip coverage where
       mGBA 0.10.5's flipped suffix optimization reads the wrong nibble. */
    reset();hn_write16(&memory,0x04000000,0x2100);hn_write16(&memory,0x04000008,0x1000);
    hn_write16(&memory,0x04000044,160);hn_write16(&memory,0x04000048,1);
    for(unsigned i=1;i<=8;i++)hn_write16(&memory,0x05000000+i*2,(uint16_t)i);
    for(unsigned row=0;row<8;row++){
        hn_write16(&memory,0x06000020+row*4,0x4321);hn_write16(&memory,0x06000022+row*4,0x8765);
    }
    for(unsigned flip=0;flip<2;flip++){
        for(unsigned entry=0;entry<1024;entry++)hn_write16(&memory,0x06008000+entry*2,(uint16_t)(1|(flip<<10)));
        for(unsigned scroll=0;scroll<8;scroll++)for(unsigned width=1;width<=8;width++){
            unsigned left=240-width;hn_write16(&memory,0x04000010,(uint16_t)scroll);
            hn_write16(&memory,0x04000040,(uint16_t)((left<<8)|240));hn_video_render_line(&memory,0,pixels);
            CHECK(pixels[left-1]==0xFF000000u);
            for(unsigned x=left;x<240;x++){
                unsigned index=(x+scroll)&7;index=1+(flip?7-index:index);
                CHECK(pixels[x]==(0xFF000000u|(index<<3)|(index>>2)));
            }
        }
    }
    CHECK(!memory.fault);
}
static void backgrounds(void)
{
    reset();HNBackground background;hn_background_init(&background,&memory);
    hn_write32(&memory,HN_RENDER_STATES,0x02018000);hn_write8(&memory,HN_BACKGROUND_COUNT,2);
    hn_background_state_init(&memory);
    CHECK(!hn_background_configure(&memory,2,1,0x02020000));
    CHECK(hn_background_configure(&memory,0,1,0x02020000));
    const uint8_t motion[]={0x8B,5,0,0x87,0,0,1,0,0x81,2,0,0x80};
    for(unsigned i=0;i<sizeof(motion);i++)hn_write8(&memory,0x02020000+i,motion[i]);
    CHECK(hn_background_update(&background,64));
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS)==0x58000);
    CHECK(hn_read16(&memory,0x02018012)==2);
    CHECK(hn_background_update(&background,64));
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS)==0x68000);
    CHECK(hn_background_update(&background,64));
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS)==0x78000);
    CHECK(hn_read16(&memory,HN_VIDEO_STATE+0x3A)==7);
    CHECK(hn_background_update(&background,64));
    CHECK(hn_read8(&memory,0x02018038)==0);
    /* Delay commands keep executing until a yielding command is reached. */
    const uint8_t delayed[]={0x92,1,0,0x81,1,0,0x80};
    for(unsigned i=0;i<sizeof(delayed);i++)hn_write8(&memory,0x02020000+i,delayed[i]);
    CHECK(hn_background_configure(&memory,0,0x81,0x02020000));
    hn_write32(&memory,0x0201800C,0x10000);
    CHECK(hn_background_update(&background,64));CHECK(hn_background_update(&background,64));
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS)==0x78000);
    CHECK(hn_background_update(&background,64));
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS)==0x88000);
    /* Repeat jumps update the local cursor before the next command commits it. */
    const uint8_t repeat[]={0x90,2,0x89,1,0,0x91,0xA0,0,1,2,2,0x80};
    for(unsigned i=0;i<sizeof(repeat);i++)hn_write8(&memory,0x02020000+i,repeat[i]);
    hn_write8(&memory,0x02020100,0x8A);hn_write16(&memory,0x02020101,12);hn_write8(&memory,0x02020103,0x80);
    CHECK(hn_background_configure(&memory,0,1,0x02020000));
    CHECK(hn_background_update(&background,64));
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS+0x10)==0x20000);
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS+4)==0xC8000);
    CHECK(hn_read8(&memory,0x0201803D)==0&&hn_read8(&memory,0x02018038)==0);
    /* Clamping preserves fractions until a bound is reached, then centers them. */
    hn_write8(&memory,HN_BACKGROUND_BOUNDS+8,1);hn_write16(&memory,HN_BACKGROUND_BOUNDS+2,512);
    hn_write32(&memory,HN_LAYER_OFFSETS,0xFFFF1234);
    hn_background_clamp(&memory,0,HN_LAYER_OFFSETS,0);
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS)==0xFFFF1234);
    hn_write32(&memory,HN_LAYER_OFFSETS,0x10000);hn_background_clamp(&memory,0,HN_LAYER_OFFSETS,0);
    CHECK(hn_read32(&memory,HN_LAYER_OFFSETS)==0x8000);
    hn_write8(&memory,HN_DISPLAY_SHADOW,2);
    hn_write32(&memory,HN_LAYER_OFFSETS+2*44,0);hn_write32(&memory,HN_LAYER_OFFSETS+2*44+4,0);
    CHECK(hn_background_apply(&memory,2));
    CHECK(hn_read16(&memory,HN_VIDEO_STATE+0x98+6)==256);
    CHECK(hn_read16(&memory,HN_VIDEO_STATE+0x98+12)==256);
    /* Text tilemap rows cross screen blocks without copying unrelated tiles. */
    hn_write8(&memory,HN_DISPLAY_SHADOW,0);hn_write8(&memory,0x02018038,4);
    hn_write16(&memory,HN_VIDEO_STATE+0x98,0x0200);
    GameAddr tile_cursor=0x02021000;uint32_t tile_yield;
    hn_write32(&memory,tile_cursor,0x02022000);
    hn_write8(&memory,tile_cursor+4,30);hn_write8(&memory,tile_cursor+5,31);
    hn_write8(&memory,tile_cursor+6,33);hn_write8(&memory,tile_cursor+7,32);
    for(unsigned i=0;i<8;i++)hn_write16(&memory,0x02022000+i*2,(uint16_t)(i+1));
    CHECK(hn_background_command(&background,0x02018000,0xA1,&tile_cursor,&tile_yield));
    CHECK(hn_read16(&memory,0x06001000+31*64+60)==1);
    CHECK(hn_read16(&memory,0x06001800+31*64)==3);
    CHECK(hn_read16(&memory,0x06002000+60)==5);
    CHECK(hn_read16(&memory,0x06002800)==7);
    /* Queue byte/halfword requests share the original live source word. */
    GameAddr cursor=0x02021000;uint32_t yield;
    hn_write32(&memory,cursor,0x02025000);hn_write16(&memory,cursor+4,0xBEEF);
    CHECK(hn_background_command(&background,0x02018000,0x9D,&cursor,&yield));
    CHECK(hn_read16(&memory,HN_BACKGROUND_QUEUED_VALUE)==0xBEEF);
    CHECK(hn_read32(&memory,HN_TRANSFER_B)==HN_BACKGROUND_QUEUED_VALUE);
    CHECK(hn_read8(&memory,HN_CPU_TRANSFER_WRITE)==3);
    hn_write8(&memory,HN_CPU_TRANSFER_WRITE,127);
    hn_transfer_cpu_queue(&memory,0x02026000,0x02025000,4);
    CHECK(hn_read32(&memory,HN_TRANSFER_B+127*4)==0x02026000);
    CHECK(hn_read32(&memory,HN_TRANSFER_B)==0x02025000&&hn_read8(&memory,HN_CPU_TRANSFER_WRITE)==2);
    /* Saturated stacks and cyclic scripts fail explicitly without spilling fields. */
    hn_write8(&memory,0x0201803D,4);cursor=0x02021000;
    CHECK(!hn_background_command(&background,0x02018000,0xA0,&cursor,&yield));
    CHECK(background.fault==HN_BACKGROUND_STACK&&hn_read8(&memory,0x0201803D)==4);
    hn_background_init(&background,&memory);CHECK(hn_background_configure(&memory,0,1,0x02020000));
    hn_write8(&memory,0x02020000,0x82);hn_write32(&memory,0x02020001,0x02020000);
    CHECK(!hn_background_update(&background,16));CHECK(background.fault==HN_BACKGROUND_BUDGET);
    hn_background_init(&background,&memory);hn_write8(&memory,0x02020000,0xBD);
    CHECK(!hn_background_update(&background,16));CHECK(background.fault==HN_BACKGROUND_OPCODE);
    CHECK(!hn_background_apply(&memory,4));CHECK(!memory.fault);
}
int main(void)
{
    rom=calloc(1,0x600000);CHECK(rom!=NULL);
    for(unsigned i=0;i<24;i++)rom[HN_OBJECT_SIZE_TABLE-HN_ROM_BASE+i]=8;
    for(unsigned i=0;i<320;i++){
        int16_t sine=(int16_t)lround(sin(i*3.14159265358979323846/128)*256);
        unsigned p=HN_RENDER_SINE_TABLE-HN_ROM_BASE+i*2;rom[p]=(uint8_t)sine;rom[p+1]=(uint8_t)(sine>>8);
    }
    configuration();matrices();animations();callbacks_and_motion();drawing();pixel_contracts();clipped_text_tiles();backgrounds();
    free(rom);printf("%u native renderer checks passed.\n",checks);return 0;
}
