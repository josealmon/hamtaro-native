/* Synthetic local-record, composer and title contracts; no game assets. */
#include "game/state.h"
#include "game/heap.h"
#include "game/save.h"
#include "game/scene.h"
#include "game/save_group4.h"
#include "game/render.h"
#include "game/video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static HNMemory memory;
static unsigned checks;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c);exit(1);}}while(0)
static void reset(void){hn_memory_init(&memory,NULL,0);}
static void group4_contract(void)
{
    uint8_t *rom=calloc(1,0x700000);CHECK(rom!=NULL);
    hn_memory_init(&memory,rom,0x700000);
    const GameAddr controls=HN_SCENE_CONTROLS,received=0x03003D30,context=0x02020000,args=0x02001001;
    const GameAddr table=0x086E8CCC;uint32_t consumed=99;
    hn_write32(&memory,0x03002BB0,context);hn_write16(&memory,args,0x1FF);
    hn_write8(&memory,context+0x2B18,0xDA);
    CHECK(hn_scene_callback(&memory,0x080C8E71,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_read8(&memory,controls+0x1FF)==0xDA);
    hn_write8(&memory,controls+0x1FF,0xB9);
    CHECK(hn_group4_callback(&memory,0x080C8E49,args,&consumed)==HN_SCENE_OK&&consumed==2);
    CHECK(hn_read8(&memory,context+0x2B18)==0xB9);
    hn_write8(&memory,controls+0x180,2);hn_write8(&memory,controls+0x181,3);
    for(unsigned i=0;i<128;i++)hn_write8(&memory,context+2*1100+3*220+i,(uint8_t)(i^0xA5));
    CHECK(hn_group4_callback(&memory,0x080C8741,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,received)==0xA5&&hn_read8(&memory,received+127)==(127^0xA5));
    hn_write8(&memory,controls+0x181,4);hn_write8(&memory,received+127,0x72);
    CHECK(hn_group4_callback(&memory,0x080C8740,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,received)==0xA5&&hn_read8(&memory,received+126)==0&&hn_read8(&memory,received+127)==0x72);
    /* Clear-before-copy and forward byte overlap both matter for saved records. */
    hn_write8(&memory,controls+0x180,0);hn_write8(&memory,controls+0x181,0);
    hn_write32(&memory,0x03002BB0,received-1);hn_write8(&memory,received-1,0xC6);
    CHECK(hn_group4_callback(&memory,0x080C8740,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,received)==0xC6&&hn_read8(&memory,received+127)==0xC6);
    /* Synthetic descriptors, never extracted resource payloads. */
    const unsigned descriptor=table-0x08000000;
    for(unsigned i=0;i<4;i++)rom[descriptor+8+i]=(uint8_t)(0x02001800u>>(i*8));
    rom[descriptor+0x16]=3;hn_write8(&memory,controls+0x181,5);
    hn_write8(&memory,0x02001800,7);hn_write8(&memory,0x02001801,8);hn_write8(&memory,0x02001802,9);
    CHECK(hn_group4_callback(&memory,0x080C8740,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,received)==0xC6&&hn_read8(&memory,received+17)==7&&hn_read8(&memory,received+19)==9&&hn_read8(&memory,received+20)==0);
    hn_write8(&memory,controls+0x60,0);hn_write8(&memory,controls+0x62,0xA7);
    hn_write8(&memory,received+17,0);hn_write8(&memory,received+18,1);
    CHECK(hn_group4_callback(&memory,0x080C6EDD,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,controls+0x61)==0&&hn_read8(&memory,controls+0x62)==0xA7);
    hn_write8(&memory,received+19,255);
    CHECK(hn_group4_callback(&memory,0x080C6EDC,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x61)==1);
    hn_write8(&memory,controls+0x60,1);hn_write8(&memory,received+19,0x34);
    CHECK(hn_group4_callback(&memory,0x080C6EDC,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x61)==1);
    hn_write8(&memory,controls+0x60,2);rom[descriptor+0x16]=10;
    for(unsigned i=0;i<10;i++)rom[0x6E906A+i]=(uint8_t)(0x50+i);
    for(unsigned n=0;n<=10;n++){
        for(unsigned i=0;i<10;i++)hn_write8(&memory,received+17+i,(uint8_t)(i<n?11+i:0));
        CHECK(hn_group4_callback(&memory,0x080C6EDC,0,&consumed)==HN_SCENE_OK);
        CHECK(hn_read8(&memory,controls+0x62)==(n<=4?1:n<=7?2:n<=9?3:0));
        CHECK(hn_read8(&memory,controls+0x61)==(n==10));
    }
    /* The availability scan selects its descriptor with control 4, not 0x180. */
    rom[descriptor+28+0x16]=2;hn_write8(&memory,controls+4,1);
    hn_write8(&memory,received+17,2);hn_write8(&memory,received+18,255);
    hn_write8(&memory,0x03002180+(71>>3),1u<<(71&7));
    CHECK(hn_group4_callback(&memory,0x080C8E99,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x60)==0);
    hn_write8(&memory,0x03002180+(324>>3),1u<<(324&7));
    CHECK(hn_group4_callback(&memory,0x080C8E98,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x60)==1);
    CHECK(hn_group4_callback(&memory,0x080C8744,0,&consumed)==HN_SCENE_UNKNOWN&&consumed==0);
    hn_write32(&memory,HN_RENDER_SLOTS,0x02008000);hn_write8(&memory,HN_RENDER_INFO+16,0);
    for(unsigned i=0;i<9;i++)hn_write8(&memory,0x03002BA0+i,(uint8_t)(30+i));
    hn_write8(&memory,0x03002BA3,0);hn_write8(&memory,received+12,0xA7);
    CHECK(hn_scene_callback(&memory,0x080C8AF1,0,&consumed)==HN_SCENE_OK&&consumed==0);
    CHECK(hn_read8(&memory,received)==2&&hn_read8(&memory,received+2)==3);
    CHECK(hn_read8(&memory,received+5)==30&&hn_read8(&memory,received+11)==36&&hn_read8(&memory,received+12)==0xA7);
    hn_write8(&memory,0x03002180+(0x1E9>>3),1u<<(0x1E9&7));
    hn_write8(&memory,args,254);hn_write32(&memory,args+1,0x08812345);hn_write8(&memory,0x03002170,0xB6);
    CHECK(hn_scene_callback(&memory,0x080C8B55,args,&consumed)==HN_SCENE_OK&&consumed==5);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,4)+4)==0x08812345&&hn_read8(&memory,0x03002170)==0xB6);
    CHECK(hn_scene_callback(&memory,0x080C8B95,args,&consumed)==HN_SCENE_OK&&consumed==5);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,4)+0x60)==1&&hn_read32(&memory,hn_render_slot(&memory,4)+0x18)==0x8000);
    hn_write8(&memory,args,2);hn_write16(&memory,args+1,255);hn_write16(&memory,args+3,254);
    hn_write16(&memory,0x0201DC40+255*2,0xFFFF);hn_write16(&memory,0x0201DC40+254*2,0x8000);
    CHECK(hn_scene_callback(&memory,0x080C8C29,args,&consumed)==HN_SCENE_OK&&consumed==5);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,2)+0x18)==0xFFFF0000&&hn_read32(&memory,hn_render_slot(&memory,2)+0x1C)==0x80000000);
    CHECK(hn_read32(&memory,hn_render_slot(&memory,2)+0x20)==0);
    hn_write8(&memory,controls+0x180,0);hn_write8(&memory,controls+0x187,255);rom[descriptor+0x16]=0;
    CHECK(hn_scene_callback(&memory,0x080C8D95,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,30)+0x60)==1&&hn_read8(&memory,hn_render_slot(&memory,31)+0x60)==1);
    hn_write8(&memory,controls+0x18C,0);hn_write8(&memory,0x020046AB,255);
    CHECK(hn_scene_callback(&memory,0x080C8DF9,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,hn_render_slot(&memory,33)+0x60)==1);
    hn_write8(&memory,received,3);hn_write8(&memory,received+2,10);
    CHECK(hn_scene_callback(&memory,0x080C87C9,0,&consumed)==HN_SCENE_RESOURCE&&!memory.fault);
    CHECK(hn_scene_callback(&memory,0x080C9001,0,&consumed)==HN_SCENE_UNKNOWN);
    CHECK(hn_group4_task(&memory,0x080C8B29)==HN_SCENE_UNKNOWN);
    for(unsigned value=0;value<256;value++){
        hn_write8(&memory,controls+0x184,255);hn_write8(&memory,received+17+255,(uint8_t)value);
        CHECK(hn_scene_callback(&memory,0x080C8639,0,&consumed)==HN_SCENE_OK&&consumed==0);
        CHECK(hn_read8(&memory,controls+0x186)==(value==1?0:value));
    }
    hn_write32(&memory,0x03002BB0,context);hn_write8(&memory,controls+0x180,0);
    for(unsigned empty=0;empty<=5;empty++){
        for(unsigned i=0;i<5;i++)hn_write8(&memory,context+i*220,(uint8_t)(i!=empty));
        CHECK(hn_scene_callback(&memory,0x080C8695,0,&consumed)==HN_SCENE_OK);
        CHECK(hn_read8(&memory,controls+0x191)==empty);
    }
    hn_write8(&memory,controls+0x181,4);
    CHECK(hn_scene_callback(&memory,0x080C86E9,0,&consumed)==HN_SCENE_OK&&hn_read8(&memory,controls+0x191)==1);
    hn_write8(&memory,context+4*220,0);
    CHECK(hn_scene_callback(&memory,0x080C86E9,0,&consumed)==HN_SCENE_OK&&!hn_read8(&memory,controls+0x191));
    CHECK(hn_scene_callback(&memory,0x080C85F9,0,&consumed)==HN_SCENE_OK&&consumed==0);
    /* A composition stores the selected move, including both byte endpoints,
       without overwriting the record's name/header or neighboring moves. */
    for(unsigned i=0;i<2;i++){
        unsigned index=i?255:0;hn_write8(&memory,controls+0x187,(uint8_t)index);
        hn_write8(&memory,received+16+index,0xA9);hn_write8(&memory,received+18+index,0xB8);
        hn_write8(&memory,controls+0x191,(uint8_t)(i?255:1));
        CHECK(hn_scene_callback(&memory,0x080C8349,0,&consumed)==HN_SCENE_OK&&consumed==0);
        hn_write8(&memory,controls+0x191,0);
        CHECK(hn_scene_callback(&memory,0x080C83BD,0,&consumed)==HN_SCENE_OK);
        CHECK(hn_read8(&memory,controls+0x191)==(i?255:1));
        CHECK(hn_read8(&memory,received+16+index)==0xA9&&hn_read8(&memory,received+18+index)==0xB8);
    }
    /* Empty resources use signed count-1 comparisons, rather than wrapping to255. */
    hn_write8(&memory,args+0x16,0);hn_write16(&memory,args+0x14,0);
    CHECK(!hn_group4_boundary(&memory,2,args,0,255));
    CHECK(hn_group4_boundary(&memory,0,args,0,0));
    CHECK(!hn_group4_boundary(&memory,0,args,65535,0));
    hn_write8(&memory,args+0x16,3);CHECK(hn_group4_boundary(&memory,0x102,args,0,0x102));
    /* One beat advances the score, move selection and ring map together. */
    for(unsigned i=0;i<4;i++)rom[descriptor+4+i]=(uint8_t)(0x02001800u>>(i*8));
    rom[descriptor+12]=0;rom[descriptor+13]=0;rom[descriptor+14]=0;rom[descriptor+15]=8;
    rom[descriptor+0x14]=12;rom[descriptor+0x16]=3;
    hn_write8(&memory,0x02001807,1);hn_write8(&memory,0x02001808,2);
    hn_write8(&memory,controls+0x187,0);hn_write8(&memory,controls+0x192,31);
    hn_write8(&memory,0x020046B1,0);hn_write8(&memory,0x020046B2,0);
    hn_write32(&memory,0x03002BC0,0);hn_write16(&memory,0x0201DDC4,0);hn_write16(&memory,HN_VIDEO_STATE+0x9A,12);
    CHECK(hn_scene_callback(&memory,0x080C76D5,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read16(&memory,0x0201DDC4)==1&&hn_read16(&memory,0x0201DDC6)==1);
    CHECK(hn_read8(&memory,controls+0x187)==1&&hn_read8(&memory,controls+0x189)==2);
    CHECK(hn_read16(&memory,HN_VIDEO_STATE+0x9A)==4&&hn_read32(&memory,0x03002BC0)==0);
    hn_write8(&memory,0x020046B1,1);hn_write8(&memory,0x020046B2,0);hn_write8(&memory,controls+0x187,2);
    CHECK(hn_scene_callback(&memory,0x080C76D4,0,&consumed)==HN_SCENE_OK);
    CHECK(hn_read8(&memory,controls+0x189)==10&&hn_read16(&memory,0x0201DDC4)==1);
    /* Synthetic glyphs exercise unknown-byte skipping, the special symbol and
       multiline heap cleanup without embedding any game text or graphics. */
    hn_write32(&memory,0x02000D84,2u|(5u<<8)|(9u<<18));hn_write8(&memory,0x02000D88,0);
    hn_write8(&memory,args,2);hn_write8(&memory,args+1,4);hn_write8(&memory,args+2,0x37);hn_write8(&memory,args+3,0);
    CHECK(hn_group4_text_line(&memory,args,0x02010000,5));
    CHECK(hn_read16(&memory,0x02010000)==0x5005&&hn_read16(&memory,0x02010040)==0x5009);
    CHECK(hn_read16(&memory,0x02010002)==0x51F5&&hn_read16(&memory,0x02010042)==0x5215);
    hn_heap_init(&memory);hn_write8(&memory,args+1,0xE2);hn_write8(&memory,args+3,0xE0);
    CHECK(hn_group4_text_lines(&memory,args,0x02011000,3,4));
    CHECK(hn_read16(&memory,0x02011000)==0x3005&&hn_read16(&memory,0x02011080)==0x31F5);
    CHECK(hn_read32(&memory,0x03002C84)==0x030010C8);
    CHECK(!memory.fault);free(rom);reset();
}
static void title_record_contract(void)
{
    reset();const GameAddr args=0x02001001;uint32_t consumed=99;
    for(unsigned value=0;value<256;value++){
        hn_write32(&memory,0x03001F94,0x12345678);hn_write32(&memory,0x03001F9C,0x89ABCDEF);
        hn_write32(&memory,0x03001F98,0xA5B6C7D8);hn_write8(&memory,args,(uint8_t)value);
        CHECK(hn_scene_callback(&memory,0x080202E1,args,&consumed)==HN_SCENE_OK&&consumed==1);
        CHECK(hn_read32(&memory,0x03001F98)==value);
        CHECK(hn_read32(&memory,0x03001F94)==0x12345678&&hn_read32(&memory,0x03001F9C)==0x89ABCDEF);
    }
    hn_write32(&memory,0x03001F98,0x89ABCDEF);
    CHECK(hn_scene_callback(&memory,0x080202E0,0x03001F99,&consumed)==HN_SCENE_OK);
    CHECK(hn_read32(&memory,0x03001F98)==0xCD);
    /* Invented localized names and resource bytes; no original payloads. */
    uint8_t *rom=calloc(1,0x700000);CHECK(rom!=NULL);
    const unsigned lengths[]={0,1,6,7,8};
    for(unsigned language=0;language<5;language++){
        GameAddr name=0x02001800+language*16;
        for(unsigned b=0;b<4;b++)rom[0x6E41EC+language*4+b]=(uint8_t)(name>>(b*8));
    }
    for(unsigned row=0;row<10;row++){
        GameAddr data=0x02001900+row*8;
        for(unsigned b=0;b<4;b++)rom[0x6E8CCC+row*28+8+b]=(uint8_t)(data>>(b*8));
        rom[0x6E8CCC+row*28+0x16]=3;
    }
    for(unsigned language=0;language<5;language++){
        hn_memory_init(&memory,rom,0x700000);hn_heap_init(&memory);
        memset(memory.sram,0xA5,sizeof(memory.sram));
        for(unsigned l=0;l<5;l++)for(unsigned i=0;i<lengths[l];i++)hn_write8(&memory,0x02001800+l*16+i,(uint8_t)(40+l));
        for(unsigned row=0;row<10;row++)for(unsigned i=0;i<3;i++)hn_write8(&memory,0x02001900+row*8+i,(uint8_t)(60+row));
        hn_write8(&memory,0x0200460B,(uint8_t)language);hn_write16(&memory,0x0201DC52,73);
        CHECK(hn_scene_callback(&memory,0x080202BD,0,&consumed)==HN_SCENE_OK&&consumed==0);
        CHECK(memory.sram_dirty&&hn_read16(&memory,0x0201DC52)==73);
        for(unsigned i=0;i<0x142C;i++)CHECK(memory.sram[i]==0xA5&&memory.sram[0x4000+i]==0xA5);
        for(unsigned index=0;index<51;index++){
            GameAddr record=0x0E00142C+index*0xDC,payload=record+8;
            CHECK(hn_read32(&memory,record)==128&&hn_read8(&memory,record+4)==4&&hn_read8(&memory,record+5)==index);
            CHECK(hn_save_checksum(&memory,record,136)==0);
            if(index==50)CHECK(hn_read8(&memory,payload)==1&&hn_read8(&memory,payload+3)==0&&hn_read8(&memory,payload+127)==0);
            else{
                CHECK(hn_read8(&memory,payload+3)==index%5+3&&hn_read8(&memory,payload+4)==1);
                if(index%5==0){
                    unsigned length=lengths[language]<7?lengths[language]:7;
                    CHECK(hn_read8(&memory,payload)==1&&hn_read8(&memory,payload+2)==length&&hn_read8(&memory,payload+5+length)==0);
                    CHECK(hn_read8(&memory,payload+17)==60+index/5);
                }else CHECK(hn_read8(&memory,payload)==0&&hn_read8(&memory,payload+5)==1);
            }
            for(unsigned i=136;i<0xDC;i++)CHECK(hn_read8(&memory,record+i)==0xA5);
        }
        CHECK(memcmp(memory.sram,memory.sram+0x4000,0x4000)==0);
        CHECK(hn_read8(&memory,0x03003D30)==1&&hn_read8(&memory,0x03003DAF)==0&&!memory.fault);
    }
    free(rom);reset();
}
unsigned hn_test_group4(void)
{
    checks=0;group4_contract();title_record_contract();return checks;
}
