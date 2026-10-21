#include "game/audio.h"
#include "game/state.h"
#include "game/script.h"
#include <stdio.h>
#include <stdlib.h>
static HNMemory memory;
static uint8_t *rom;
static unsigned checks;
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c);exit(1);}}while(0)
static void rom_word(GameAddr address,uint32_t value,unsigned bytes)
{for(unsigned i=0;i<bytes;i++)rom[address-HN_ROM_BASE+i]=(uint8_t)(value>>(i*8));}
static void reset(void)
{hn_memory_init(&memory,rom,0x800000);CHECK(hn_audio_driver_init(&memory));}
static void initialization(void)
{
    reset();CHECK(hn_read32(&memory,HN_AUDIO_CORE_POINTER)==HN_AUDIO_CORE);
    CHECK(hn_read32(&memory,HN_AUDIO_CORE)==HN_AUDIO_IDENT);
    CHECK(hn_read8(&memory,HN_AUDIO_CORE+6)==8&&hn_read8(&memory,HN_AUDIO_CORE+7)==13);
    CHECK(hn_read32(&memory,HN_AUDIO_CORE+0x10)==220);
    CHECK(hn_read16(&memory,0x04000102)==0x80);
    CHECK(!hn_audio_frequency(&memory,0));CHECK(!hn_audio_frequency(&memory,0xD0000));
    CHECK(hn_read32(&memory,HN_AUDIO_CORE)==HN_AUDIO_IDENT);
    for(unsigned i=0;i<6;i++){
        CHECK(hn_read32(&memory,0x03006000+i*64+0x34)==HN_AUDIO_IDENT);
        CHECK(hn_read8(&memory,0x03006000+i*64+8)==2);
    }
    hn_write8(&memory,HN_AUDIO_CORE+4,0);hn_audio_dma_vblank(&memory);
    CHECK(hn_read8(&memory,HN_AUDIO_CORE+4)==7);
    hn_audio_dma_pause(&memory);CHECK(hn_read32(&memory,HN_AUDIO_CORE)==HN_AUDIO_IDENT+10);
    hn_audio_dma_resume(&memory);CHECK(hn_read32(&memory,HN_AUDIO_CORE)==HN_AUDIO_IDENT);
    CHECK(!memory.fault);
}
static void player_and_parameters(void)
{
    reset();GameAddr player=0x03006000,track=0x02010000;
    hn_write8(&memory,0x02020000,1);hn_write8(&memory,0x02020002,32);
    hn_write32(&memory,0x02020004,0x02022000);hn_write32(&memory,0x02020008,0x02023000);
    CHECK(hn_audio_song(&memory,1,HN_SONG_START));
    CHECK(hn_read32(&memory,player)==0x02020000&&hn_read8(&memory,track)==0xC0);
    CHECK(hn_read32(&memory,track+0x40)==0x02023000);
    hn_audio_track_parameter(&memory,player,1,128,0);CHECK(hn_read8(&memory,track+0x13)==32);
    hn_audio_track_parameter(&memory,player,1,0xFF80,1);
    CHECK(hn_read8(&memory,track+11)==255&&hn_read8(&memory,track+13)==128);
    hn_write8(&memory,track+0x17,32);hn_write8(&memory,track+0x16,11);hn_write8(&memory,track+0x1A,75);
    hn_audio_track_parameter(&memory,player,1,0,3);
    CHECK(hn_read8(&memory,track+0x17)==0&&hn_read8(&memory,track+0x16)==0&&hn_read8(&memory,track+0x1A)==0);
    hn_audio_tempo(&memory,player,128);CHECK(hn_read16(&memory,player+0x20)==75);
    hn_audio_fade(&memory,player,1,2);
    for(unsigned i=0;i<16;i++)CHECK(hn_audio_fade_update(&memory,player));
    CHECK(hn_read16(&memory,player+0x28)==256&&hn_read16(&memory,player+0x24)==0);
    CHECK(hn_audio_player_stop(&memory,player));CHECK(hn_read32(&memory,player+4)&0x80000000);
    hn_audio_player_resume(&memory,player);CHECK(!(hn_read32(&memory,player+4)&0x80000000));
    /* A high-priority active track rejects a lower-priority header. */
    hn_write8(&memory,player+11,1);hn_write8(&memory,player+9,80);hn_write32(&memory,player+4,1);
    CHECK(hn_audio_player_start(&memory,player,0x02020100));CHECK(hn_read32(&memory,player)==0x02020000);
    CHECK(!memory.fault);
}
static void channels(void)
{
    reset();GameAddr t=0x02010000,a=HN_AUDIO_CORE+0x50,b=a+64;
    hn_write8(&memory,t,0x80);hn_write32(&memory,t+0x20,a);
    hn_write8(&memory,a,0x81);hn_write32(&memory,a+0x2C,t);hn_write32(&memory,a+0x30,0);hn_write32(&memory,a+0x34,b);
    hn_write8(&memory,b,0x82);hn_write32(&memory,b+0x2C,t);hn_write32(&memory,b+0x30,a);hn_write32(&memory,b+0x34,0);
    hn_audio_channel_unlink(&memory,a);
    CHECK(hn_read32(&memory,t+0x20)==b&&hn_read32(&memory,b+0x30)==0);
    CHECK(hn_audio_track_finish(&memory,t));CHECK(hn_read8(&memory,b)==0xC2);
    CHECK(hn_read8(&memory,t)==0&&hn_read32(&memory,t+0x20)==0);
    hn_write8(&memory,t,0x80);hn_write32(&memory,t+0x20,a);hn_write8(&memory,a+1,1);
    CHECK(hn_audio_track_stop(&memory,t));CHECK(hn_read8(&memory,a)==0);
    CHECK(hn_read8(&memory,0x04000063)==8&&hn_read8(&memory,0x04000065)==128);
    CHECK(hn_audio_clear_channels(&memory));CHECK(!memory.fault);
}
static void commands(void)
{
    reset();GameAddr player=0x03006000,track=0x02010000;HNAudio audio;hn_audio_init(&audio,&memory);
    hn_write32(&memory,track+0x40,0x02020000);hn_write32(&memory,0x02020000,0x02021000);
    CHECK(hn_audio_sequence_command(&audio,player,track,0xB3));
    CHECK(hn_read8(&memory,track+2)==1&&hn_read32(&memory,track+0x40)==0x02021000);
    CHECK(hn_audio_sequence_command(&audio,player,track,0xB4));
    CHECK(hn_read8(&memory,track+2)==0&&hn_read32(&memory,track+0x40)==0x02020004);
    hn_write8(&memory,track+2,3);CHECK(hn_audio_sequence_command(&audio,player,track,0xB3));CHECK(hn_read8(&memory,track)==0);
    /* Memory commands retain wrapping byte arithmetic and typed branch dispatch. */
    hn_write32(&memory,player+0x18,0x02025000);hn_write8(&memory,0x02025003,255);
    hn_write32(&memory,track+0x40,0x02020000);
    hn_write8(&memory,0x02020000,1);hn_write8(&memory,0x02020001,3);hn_write8(&memory,0x02020002,2);
    CHECK(hn_audio_sequence_command(&audio,player,track,0xB9));CHECK(hn_read8(&memory,0x02025003)==1);
    hn_write32(&memory,track+0x40,0x02020000);hn_write8(&memory,0x02020000,6);hn_write8(&memory,0x02020002,1);
    hn_write32(&memory,0x02020003,0x02021000);
    CHECK(hn_audio_sequence_command(&audio,player,track,0xB9));CHECK(hn_read32(&memory,track+0x40)==0x02021000);
    hn_write32(&memory,HN_AUDIO_DISPATCH+4,0x08123457);
    CHECK(!hn_audio_sequence_command(&audio,player,track,0xB2));CHECK(audio.fault==HN_AUDIO_CALLBACK);
    hn_audio_init(&audio,&memory);CHECK(!hn_audio_sequence_command(&audio,player,track,0xCF));CHECK(audio.fault==HN_AUDIO_COMMAND);
    CHECK(!memory.fault);
}
static void notes_and_player(void)
{
    reset();HNAudio audio;hn_audio_init(&audio,&memory);GameAddr player=0x03006000,track=0x02010000,channel=HN_AUDIO_CORE+0x50;
    hn_write32(&memory,track+0x40,0x02020000);hn_write8(&memory,track+5,60);hn_write8(&memory,track+6,127);
    hn_write8(&memory,0x02020000,0x81);hn_write8(&memory,track+0x24,0);hn_write32(&memory,track+0x28,0x02024000);
    hn_write32(&memory,0x02024004,0x01000000);
    CHECK(hn_audio_note(&audio,player,track,4));
    CHECK(hn_read32(&memory,track+0x20)==channel&&hn_read32(&memory,channel+0x2C)==track);
    CHECK(hn_read8(&memory,channel)==128&&hn_read8(&memory,channel+0x10)==4);
    CHECK(hn_read32(&memory,channel+0x20)==0x00400000);
    CHECK(hn_read32(&memory,track+0x40)==0x02020000);
    /* A complete synthetic sequence advances, releases its note and stops. */
    reset();hn_audio_init(&audio,&memory);
    hn_write8(&memory,0x02020100,1);hn_write32(&memory,0x02020104,0x02022000);
    hn_write32(&memory,0x02020108,0x02023000);hn_write8(&memory,0x02022000,0);
    hn_write32(&memory,0x02022004,0x02024000);hn_write32(&memory,0x02024004,0x01000000);
    const uint8_t sequence[]={0xBD,0,0xD3,60,127,0x85,0xB1};
    for(unsigned i=0;i<sizeof(sequence);i++)hn_write8(&memory,0x02023000+i,sequence[i]);
    CHECK(hn_audio_player_start(&memory,player,0x02020100));
    CHECK(hn_audio_player_update(&audio,player,128));CHECK(hn_read8(&memory,track+1)==4);
    CHECK(hn_read8(&memory,channel+0x10)==4&&hn_read32(&memory,player+12)==1);
    for(unsigned i=0;i<4;i++)CHECK(hn_audio_player_update(&audio,player,128));
    CHECK(hn_read8(&memory,channel)&64);
    CHECK(hn_audio_player_update(&audio,player,128));CHECK(hn_read8(&memory,track)==0);
    CHECK(hn_audio_player_update(&audio,player,128));CHECK(hn_read32(&memory,player+4)==0x80000000);
    /* Cyclic command streams and channel chains have independent budgets. */
    hn_write8(&memory,0x02023000,0xB2);hn_write32(&memory,0x02023001,0x02023000);
    CHECK(hn_audio_player_start(&memory,player,0x02020100));
    CHECK(!hn_audio_player_update(&audio,player,16));CHECK(audio.fault==HN_AUDIO_BUDGET);
    reset();hn_audio_init(&audio,&memory);hn_write32(&memory,track+0x20,channel);
    hn_write8(&memory,channel,1);hn_write32(&memory,channel+0x2C,track);hn_write32(&memory,channel+0x34,channel);
    CHECK(!hn_audio_sequence_command(&audio,player,track,0xB1));CHECK(audio.fault==HN_AUDIO_BUDGET);
    CHECK(!memory.fault);
}
static void mix_fixture(void)
{
    reset();GameAddr core=HN_AUDIO_CORE,c=core+0x50,w=0x02024000;
    hn_write32(&memory,core+0x20,0);hn_write32(&memory,core+0x28,0x080EC0B5);
    hn_write32(&memory,core+0x10,16);hn_write32(&memory,core+0x18,1);
    hn_write8(&memory,core+4,0);hn_write8(&memory,core+5,0);hn_write8(&memory,core+6,1);hn_write8(&memory,core+7,15);
    hn_write8(&memory,core+11,99);hn_write8(&memory,c,128);hn_write8(&memory,c+1,8);
    hn_write8(&memory,c+2,255);hn_write8(&memory,c+3,255);hn_write8(&memory,c+4,255);
    hn_write8(&memory,c+5,255);hn_write8(&memory,c+6,255);hn_write8(&memory,c+7,255);
    hn_write32(&memory,c+0x24,w);hn_write32(&memory,w+12,4);hn_write32(&memory,w+16,0xC040807F);
}
static void mixing(void)
{
    mix_fixture();HNAudio audio;hn_audio_init(&audio,&memory);GameAddr c=HN_AUDIO_CORE+0x50,out=HN_AUDIO_CORE+0x350;
    hn_write8(&memory,c+3,128);CHECK(hn_audio_mix(&audio,128));
    CHECK(hn_read32(&memory,out)==0xC03F817E);CHECK(hn_read32(&memory,out+0x630)==0xE01FC03F);
    CHECK(hn_read32(&memory,out+4)==0&&hn_read8(&memory,c)==0);
    CHECK(hn_read32(&memory,c+0x18)==4&&hn_read32(&memory,c+0x28)==0x02024010);
    /* Interpolation wraps the original wave, without clipping its byte sum. */
    mix_fixture();hn_audio_init(&audio,&memory);hn_write8(&memory,c+1,0);
    hn_write32(&memory,c+0x20,0x400000);hn_write32(&memory,0x02024000,0x40000000);
    hn_write32(&memory,0x0202400C,2);hn_write16(&memory,0x02024010,64);
    CHECK(hn_audio_mix(&audio,128));
    CHECK(hn_read32(&memory,out)==0x1F001F3F&&hn_read32(&memory,out+12)==0x1F001F3F);
    CHECK(hn_read32(&memory,c+0x18)==2&&hn_read32(&memory,c+0x28)==0x02024010);
    CHECK(hn_read32(&memory,c+0x1C)==0&&(hn_read8(&memory,c)&16));
    /* Reverb includes both planes and the next DMA bank, with signed rounding. */
    mix_fixture();hn_audio_init(&audio,&memory);hn_write8(&memory,c,0);hn_write8(&memory,HN_AUDIO_CORE+5,64);
    for(unsigned i=0;i<16;i++){
        hn_write8(&memory,out+i,32);hn_write8(&memory,out+0x630+i,32);
        hn_write8(&memory,out+16+i,64);hn_write8(&memory,out+0x640+i,64);
    }
    CHECK(hn_audio_mix(&audio,128));CHECK(hn_read8(&memory,out)==24&&hn_read8(&memory,out+0x630)==24);
    for(unsigned i=0;i<16;i++){
        hn_write8(&memory,out+i,224);hn_write8(&memory,out+0x630+i,224);
        hn_write8(&memory,out+16+i,192);hn_write8(&memory,out+0x640+i,192);
    }
    CHECK(hn_audio_mix(&audio,128));CHECK(hn_read8(&memory,out)==233);
    hn_write32(&memory,HN_AUDIO_CORE+0x10,0);CHECK(!hn_audio_mix(&audio,128));CHECK(audio.fault==HN_AUDIO_RESOURCE);
    CHECK(!memory.fault);
}
static void psg_and_scripts(void)
{
    reset();HNAudio audio;hn_audio_init(&audio,&memory);GameAddr c=HN_AUDIO_PSG+128;
    hn_write8(&memory,c,128);hn_write8(&memory,c+1,3);hn_write8(&memory,c+2,128);hn_write8(&memory,c+3,128);
    hn_write8(&memory,c+4,1);hn_write8(&memory,c+5,1);hn_write8(&memory,c+6,8);
    hn_write32(&memory,c+0x20,0x0420);hn_write32(&memory,c+0x24,0x02024000);
    for(unsigned i=0;i<4;i++)hn_write32(&memory,0x02024000+i*4,0x12345678+i);
    CHECK(hn_audio_psg_update(&audio));CHECK(hn_read32(&memory,0x04000090)==0x12345678);
    CHECK(hn_read32(&memory,0x0400009C)==0x1234567B&&hn_read32(&memory,c+0x28)==0x02024000);
    CHECK(hn_read8(&memory,0x04000070)==128&&hn_read8(&memory,c+0x1D)==0);
    reset();HNScript script;hn_script_init(&script,&memory,NULL,0);GameAddr player=0x03006000;
    hn_write8(&memory,0x02020000,1);hn_write32(&memory,0x02020004,0x02022000);
    hn_write32(&memory,0x02020008,0x02023000);hn_write16(&memory,0x02021000,1);hn_write8(&memory,0x02021002,0);
    CHECK(hn_main_command(&script,0x0A,0x02021000));CHECK(hn_read16(&memory,HN_AUDIO_SONG_IDS)==1);
    hn_write8(&memory,0x02021002,2);CHECK(hn_main_command(&script,0x0B,0x02021000));
    CHECK(hn_read8(&memory,HN_AUDIO_FADE_BYTES)==2&&hn_read16(&memory,player+0x28)==0x101);
    CHECK(hn_main_command(&script,0x0A,0x02021000));
    CHECK(hn_read8(&memory,HN_AUDIO_FADE_BYTES)==0&&hn_read16(&memory,player+0x28)==2);
    hn_write8(&memory,0x03000010,0x5E);hn_write8(&memory,0x02021002,1);
    CHECK(hn_main_command(&script,0x5E,0x02021000));CHECK(hn_read8(&memory,0x0300001D)==1&&hn_read8(&memory,0x03000010)==0x5E);
    CHECK(hn_audio_player_stop(&memory,player));hn_write8(&memory,0x0300001D,0);
    CHECK(hn_main_command(&script,0x5E,0x02021000));CHECK(hn_read8(&memory,0x03000010)==0&&hn_read32(&memory,0x03000014)==3);
    hn_memory_init(&memory,rom,0x800000);hn_script_init(&script,&memory,NULL,0);
    CHECK(!hn_main_command(&script,0x0A,0x02021000));CHECK(script.fault==HN_SCRIPT_RESOURCE&&!memory.fault);
}
int main(void)
{
    rom=calloc(1,0x800000);CHECK(rom!=NULL);
    for(unsigned i=0;i<12;i++)rom_word(HN_AUDIO_FREQUENCIES+i*2,132+i*44,2);
    for(unsigned i=0;i<49;i++)rom_word(HN_AUDIO_CLOCK_VALUES+i,i,1);
    rom_word(HN_AUDIO_WAVE_BASES,0x40000000,4);
    for(unsigned i=0;i<36;i++)rom_word(HN_AUDIO_INITIAL_DISPATCH+i*4,HN_ROM_BASE+0x1001+i*4,4);
    rom_word(HN_AUDIO_INITIAL_DISPATCH+17*4,0x080EAAF9,4);rom_word(HN_AUDIO_INITIAL_DISPATCH+19*4,0x080EAB0D,4);
    rom_word(HN_AUDIO_INITIAL_DISPATCH+29*4,0x080EAA91,4);
    for(unsigned i=0;i<6;i++){
        rom_word(HN_AUDIO_PLAYER_RECORDS+i*12,0x03006000+i*64,4);
        rom_word(HN_AUDIO_PLAYER_RECORDS+i*12+4,0x02010000+i*0x1000,4);
        rom_word(HN_AUDIO_PLAYER_RECORDS+i*12+8,2,4);
    }
    rom_word(HN_AUDIO_SONG_RECORDS+8,0x02020000,4);
    initialization();player_and_parameters();channels();commands();notes_and_player();mixing();psg_and_scripts();free(rom);
    printf("%u native audio checks passed.\n",checks);return 0;
}
