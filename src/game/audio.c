#include "game/audio.h"
#include "game/state.h"
enum {NOTE_ENTRY=0x080EA891,PLAYER_ENTRY=0x080EA5B5,NOOP_ENTRY=0x080EC0B5};
static GameAddr core(HNMemory *m){return hn_read32(m,HN_AUDIO_CORE_POINTER);}
static void clear(HNMemory *m,GameAddr address,unsigned bytes)
{for(unsigned i=0;i<bytes&&!m->fault;i++)hn_write8(m,address+i,0);}
static bool identified(HNMemory *m,GameAddr p){return hn_read32(m,p+0x34)==HN_AUDIO_IDENT;}
void hn_audio_init(HNAudio *a,HNMemory *m)
{a->memory=m;a->fault=HN_AUDIO_OK;a->fault_opcode=0;a->fault_address=0;}
bool hn_audio_ready(HNMemory *m)
{
    GameAddr p=core(m);if(!p)return false;uint32_t difference=hn_read32(m,p)-HN_AUDIO_IDENT;
    return !m->fault&&(difference<=1||difference==10||difference==11);
}
void hn_audio_dma_pause(HNMemory *m)
{
    GameAddr p=core(m);uint32_t id=hn_read32(m,p);
    if(id-HN_AUDIO_IDENT<=1){
        hn_write32(m,p,id+10);
        if(hn_read32(m,0x040000C4)&0x02000000)hn_write32(m,0x040000C4,0x84400004);
        if(hn_read32(m,0x040000D0)&0x02000000)hn_write32(m,0x040000D0,0x84400004);
        hn_write16(m,0x040000C6,0x0400);hn_write16(m,0x040000D2,0x0400);clear(m,p+0x350,0xC60);
    }
}
void hn_audio_dma_resume(HNMemory *m)
{
    GameAddr p=core(m);uint32_t id=hn_read32(m,p);
    if(id!=HN_AUDIO_IDENT){hn_write16(m,0x040000C6,0xB600);hn_write16(m,0x040000D2,0xB600);hn_write8(m,p+4,0);hn_write32(m,p,id-10);}
}
void hn_audio_dma_vblank(HNMemory *m)
{
    GameAddr p=core(m);if(hn_read32(m,p)-HN_AUDIO_IDENT>1)return;
    int count=(int)hn_read8(m,p+4)-1;hn_write8(m,p+4,(uint8_t)count);if(count>0)return;
    hn_write8(m,p+4,hn_read8(m,p+11));
    if(hn_read32(m,0x040000C4)&0x02000000)hn_write32(m,0x040000C4,0x84400004);
    if(hn_read32(m,0x040000D0)&0x02000000)hn_write32(m,0x040000D0,0x84400004);
    hn_write16(m,0x040000C6,0x0400);hn_write16(m,0x040000D2,0x0400);
    hn_write16(m,0x040000C6,0xB600);hn_write16(m,0x040000D2,0xB600);
}
bool hn_audio_frequency(HNMemory *m,uint32_t mode)
{
    unsigned index=(mode>>16)&15;if(!index||index>12)return false;
    GameAddr p=core(m);unsigned samples=hn_read16(m,HN_AUDIO_FREQUENCIES+(index-1)*2);if(!samples)return false;
    unsigned rate=(0x91D1Bu*samples+5000)/10000;if(!rate)return false;
    hn_write8(m,p+8,(uint8_t)index);hn_write32(m,p+0x10,samples);
    hn_write8(m,p+11,(uint8_t)(1584/samples));hn_write32(m,p+0x14,rate);
    hn_write32(m,p+0x18,((0x1000000u/rate)+1)>>1);
    hn_write16(m,0x04000102,0);hn_write16(m,0x04000100,(uint16_t)(0u-0x44940u/samples));
    hn_audio_dma_resume(m);
    /* The native scheduler supplies the scanline boundary; no host busy-wait. */
    hn_write16(m,0x04000102,0x80);return !m->fault;
}
bool hn_audio_mode(HNMemory *m,uint32_t mode)
{
    GameAddr p=core(m);if(hn_read32(m,p)!=HN_AUDIO_IDENT)return !m->fault;
    hn_write32(m,p,HN_AUDIO_IDENT+1);
    unsigned bits=mode&255;if(bits)hn_write8(m,p+5,(uint8_t)(bits&127));
    bits=mode&0xF00;if(bits){hn_write8(m,p+6,(uint8_t)(bits>>8));for(unsigned i=0;i<12;i++)hn_write8(m,p+0x50+i*0x40,0);}
    bits=mode&0xF000;if(bits)hn_write8(m,p+7,(uint8_t)(bits>>12));
    if(mode&0xB00000)hn_write8(m,0x04000089,(uint8_t)((hn_read8(m,0x04000089)&63)|((mode&0x300000)>>14)));
    bool ok=true;if(mode&0xF0000){hn_audio_dma_pause(m);ok=hn_audio_frequency(m,mode&0xF0000);}
    hn_write32(m,p,HN_AUDIO_IDENT);return ok&&!m->fault;
}
bool hn_audio_core_init(HNMemory *m,GameAddr p)
{
    hn_write32(m,p,0);
    if(hn_read32(m,0x040000C4)&0x02000000)hn_write32(m,0x040000C4,0x84400004);
    if(hn_read32(m,0x040000D0)&0x02000000)hn_write32(m,0x040000D0,0x84400004);
    hn_write16(m,0x040000C6,0x0400);hn_write16(m,0x040000D2,0x0400);
    hn_write16(m,0x04000084,0x008F);hn_write16(m,0x04000082,0xA90E);
    hn_write8(m,0x04000089,(hn_read8(m,0x04000089)&63)|64);
    hn_write32(m,0x040000BC,p+0x350);hn_write32(m,0x040000C0,0x040000A0);
    hn_write32(m,0x040000C8,p+0x980);hn_write32(m,0x040000CC,0x040000A4);
    hn_write32(m,HN_AUDIO_CORE_POINTER,p);clear(m,p,0xFB0);
    hn_write8(m,p+6,8);hn_write8(m,p+7,15);hn_write32(m,p+0x38,NOTE_ENTRY);
    hn_write32(m,p+0x28,NOOP_ENTRY);hn_write32(m,p+0x2C,NOOP_ENTRY);
    hn_write32(m,p+0x30,NOOP_ENTRY);hn_write32(m,p+0x3C,NOOP_ENTRY);
    if(!hn_memory_copy(m,HN_AUDIO_DISPATCH,HN_AUDIO_INITIAL_DISPATCH,36*4))return false;
    hn_write32(m,p+0x34,HN_AUDIO_DISPATCH);
    if(!hn_audio_frequency(m,0x40000))return false;hn_write32(m,p,HN_AUDIO_IDENT);return !m->fault;
}
bool hn_audio_psg_init(HNMemory *m,GameAddr channels)
{
    hn_write16(m,0x04000084,0x8F);hn_write16(m,0x04000080,0);
    hn_write8(m,0x04000063,8);hn_write8(m,0x04000069,8);hn_write8(m,0x04000079,8);
    hn_write8(m,0x04000065,128);hn_write8(m,0x0400006D,128);hn_write8(m,0x0400007D,128);
    hn_write8(m,0x04000070,0);hn_write8(m,0x04000080,0x77);
    GameAddr p=core(m);if(hn_read32(m,p)==HN_AUDIO_IDENT){
        hn_write32(m,p,HN_AUDIO_IDENT+1);
        /* Callback addresses remain guest metadata; native calls use typed dispatch. */
        const unsigned indexes[]={8,17,19,28,29,30,31,32,33};
        const GameAddr addresses[]={0x080EBE3D,0x080EAAF9,0x080EAB0D,0x080EBF95,0x080EAA91,
                                    0x080EB0BD,0x080EA81D,0x080EB4A1,0x080EB569};
        for(unsigned i=0;i<9;i++)hn_write32(m,HN_AUDIO_DISPATCH+indexes[i]*4,addresses[i]);
        hn_write32(m,p+0x1C,channels);hn_write32(m,p+0x28,0x080EB77D);
        hn_write32(m,p+0x2C,0x080EB6C5);hn_write32(m,p+0x30,0x080EB61D);hn_write8(m,p+12,0);
        clear(m,channels,256);
        for(unsigned i=0;i<4;i++){hn_write8(m,channels+i*64+1,(uint8_t)(i+1));hn_write8(m,channels+i*64+0x1C,(uint8_t)(0x11u<<i));}
        hn_write32(m,p,HN_AUDIO_IDENT);
    }
    return !m->fault;
}
bool hn_audio_player_configure(HNMemory *m,GameAddr player,GameAddr tracks,uint32_t count)
{
    count&=255;if(!count)return true;if(count>16)count=16;
    GameAddr p=core(m);if(hn_read32(m,p)==HN_AUDIO_IDENT){
        hn_write32(m,p,HN_AUDIO_IDENT+1);clear(m,player,64);hn_write32(m,player+0x2C,tracks);
        hn_write8(m,player+8,(uint8_t)count);hn_write32(m,player+4,0x80000000);
        for(unsigned i=0;i<count;i++)hn_write8(m,tracks+i*80,0);
        if(hn_read32(m,p+0x20)){
            hn_write32(m,player+0x38,hn_read32(m,p+0x20));hn_write32(m,player+0x3C,hn_read32(m,p+0x24));hn_write32(m,p+0x20,0);
        }
        hn_write32(m,p+0x24,player);hn_write32(m,p+0x20,PLAYER_ENTRY);
        hn_write32(m,p,HN_AUDIO_IDENT);hn_write32(m,player+0x34,HN_AUDIO_IDENT);
    }
    return !m->fault;
}
bool hn_audio_driver_init(HNMemory *m)
{
    /* The ROM's relocated mixer image is retained only as data, never executed. */
    if(!hn_memory_copy(m,0x03001B48,0x080E9F98,1024)||!hn_audio_core_init(m,HN_AUDIO_CORE)||
       !hn_audio_psg_init(m,HN_AUDIO_PSG)||!hn_audio_mode(m,0x0093D800))return false;
    for(unsigned i=0;i<HN_AUDIO_PLAYERS;i++){
        GameAddr record=HN_AUDIO_PLAYER_RECORDS+i*12,player=hn_read32(m,record);
        if(!hn_audio_player_configure(m,player,hn_read32(m,record+4),hn_read8(m,record+8)))return false;
        hn_write8(m,player+11,(uint8_t)hn_read16(m,record+10));hn_write32(m,player+0x18,0x03005160);
    }
    return !m->fault;
}
void hn_audio_psg_stop(HNMemory *m,uint8_t control)
{
    if(control==3){hn_write8(m,0x04000070,0);return;}
    GameAddr envelope=control==1?0x04000063:control==2?0x04000069:0x04000079;
    hn_write8(m,envelope,8);hn_write8(m,envelope+(control==1?2:4),128);
}
void hn_audio_channel_unlink(HNMemory *m,GameAddr channel)
{
    GameAddr owner=hn_read32(m,channel+0x2C);if(!owner)return;
    GameAddr next=hn_read32(m,channel+0x34),previous=hn_read32(m,channel+0x30);
    hn_write32(m,previous?previous+0x34:owner+0x20,next);
    if(next)hn_write32(m,next+0x30,previous);hn_write32(m,channel+0x2C,0);
}
bool hn_audio_track_stop(HNMemory *m,GameAddr track)
{
    if(!(hn_read8(m,track)&128))return !m->fault;
    GameAddr channel=hn_read32(m,track+0x20);unsigned visited=0;
    while(channel&&!m->fault){
        if(visited++==64)return false;
        if(hn_read8(m,channel)){
            unsigned kind=hn_read8(m,channel+1)&7;if(kind)hn_audio_psg_stop(m,(uint8_t)kind);hn_write8(m,channel,0);
        }
        hn_write32(m,channel+0x2C,0);channel=hn_read32(m,channel+0x34);
    }
    hn_write32(m,track+0x20,0);return !m->fault;
}
bool hn_audio_track_finish(HNMemory *m,GameAddr track)
{
    GameAddr channel=hn_read32(m,track+0x20);unsigned visited=0;
    while(channel&&!m->fault){
        if(visited++==64)return false;
        unsigned flags=hn_read8(m,channel);if(flags&0xC7)hn_write8(m,channel,(uint8_t)(flags|64));
        hn_audio_channel_unlink(m,channel);channel=hn_read32(m,channel+0x34);
    }
    hn_write8(m,track,0);return !m->fault;
}
bool hn_audio_player_start(HNMemory *m,GameAddr player,GameAddr song)
{
    if(!identified(m,player))return !m->fault;
    GameAddr tracks=hn_read32(m,player+0x2C);unsigned priority=hn_read8(m,song+2);
    if(hn_read8(m,player+11)){
        bool check=hn_read32(m,player)&&(hn_read8(m,tracks)&64);
        uint32_t state=hn_read32(m,player+4);
        if((check||((state&65535)&&(int32_t)state>=0))&&hn_read8(m,player+9)>priority)return !m->fault;
    }
    hn_write32(m,player+0x34,HN_AUDIO_IDENT+1);hn_write32(m,player+4,0);
    hn_write32(m,player,song);hn_write32(m,player+0x30,hn_read32(m,song+4));
    hn_write8(m,player+9,(uint8_t)priority);hn_write32(m,player+12,0);
    hn_write16(m,player+0x1C,150);hn_write16(m,player+0x20,150);hn_write16(m,player+0x1E,256);
    hn_write16(m,player+0x22,0);hn_write16(m,player+0x24,0);
    unsigned count=hn_read8(m,player+8),active=hn_read8(m,song);
    for(unsigned i=0;i<count;i++){
        GameAddr track=tracks+i*80;if(!hn_audio_track_stop(m,track))return false;
        hn_write8(m,track,(uint8_t)(i<active?0xC0:0));
        if(i<active){hn_write32(m,track+0x20,0);hn_write32(m,track+0x40,hn_read32(m,song+8+i*4));}
    }
    bool ok=true;unsigned mode=hn_read8(m,song+3);if(mode&128)ok=hn_audio_mode(m,mode);
    hn_write32(m,player+0x34,HN_AUDIO_IDENT);return ok&&!m->fault;
}
bool hn_audio_player_stop(HNMemory *m,GameAddr player)
{
    if(!identified(m,player))return !m->fault;hn_write32(m,player+0x34,HN_AUDIO_IDENT+1);
    hn_write32(m,player+4,hn_read32(m,player+4)|0x80000000u);
    GameAddr tracks=hn_read32(m,player+0x2C);
    for(unsigned i=0;i<hn_read8(m,player+8);i++)if(!hn_audio_track_stop(m,tracks+i*80))return false;
    hn_write32(m,player+0x34,HN_AUDIO_IDENT);return !m->fault;
}
void hn_audio_player_resume(HNMemory *m,GameAddr player)
{if(identified(m,player))hn_write32(m,player+4,hn_read32(m,player+4)&0x7FFFFFFF);}
bool hn_audio_song(HNMemory *m,uint16_t song,HNSongAction action)
{
    GameAddr record=HN_AUDIO_SONG_RECORDS+song*8;unsigned index=hn_read16(m,record+4);if(index>=HN_AUDIO_PLAYERS)return false;
    GameAddr player=hn_read32(m,HN_AUDIO_PLAYER_RECORDS+index*12),data=hn_read32(m,record),current=hn_read32(m,player);
    uint32_t state=hn_read32(m,player+4);
    switch(action){
    case HN_SONG_START:return hn_audio_player_start(m,player,data);
    case HN_SONG_CHANGE:if(current!=data||!(state&65535)||(int32_t)state<0)return hn_audio_player_start(m,player,data);break;
    case HN_SONG_CONTINUE:
        if(current!=data||!(state&65535))return hn_audio_player_start(m,player,data);
        if((int32_t)state<0)hn_audio_player_resume(m,player);break;
    case HN_SONG_STOP:if(current==data)return hn_audio_player_stop(m,player);break;
    case HN_SONG_RESUME:if(current==data)hn_audio_player_resume(m,player);break;
    default:return false;
    }
    return !m->fault;
}
bool hn_audio_all(HNMemory *m,bool resume)
{
    for(unsigned i=0;i<HN_AUDIO_PLAYERS;i++){
        GameAddr player=hn_read32(m,HN_AUDIO_PLAYER_RECORDS+i*12);
        if(resume)hn_audio_player_resume(m,player);else if(!hn_audio_player_stop(m,player))return false;
    }
    return !m->fault;
}
void hn_audio_fade(HNMemory *m,GameAddr player,uint16_t interval,unsigned mode)
{
    if(!identified(m,player))return;
    hn_write16(m,player+0x26,interval);hn_write16(m,player+0x24,interval);
    hn_write16(m,player+0x28,(uint16_t)(mode==2?2:mode==1?0x101:0x100));
    if(mode==2)hn_write32(m,player+4,hn_read32(m,player+4)&0x7FFFFFFF);
}
bool hn_audio_fade_update(HNMemory *m,GameAddr player)
{
    unsigned interval=hn_read16(m,player+0x24);if(!interval)return !m->fault;
    uint16_t countdown=(uint16_t)(hn_read16(m,player+0x26)-1);hn_write16(m,player+0x26,countdown);if(countdown)return !m->fault;
    hn_write16(m,player+0x26,(uint16_t)interval);
    uint16_t level=hn_read16(m,player+0x28);GameAddr tracks=hn_read32(m,player+0x2C);unsigned count=hn_read8(m,player+8);
    if(level&2){level=(uint16_t)(level+16);if(level>=256){level=256;hn_write16(m,player+0x24,0);}hn_write16(m,player+0x28,level);}
    else{
        level=(uint16_t)(level-16);hn_write16(m,player+0x28,level);
        if((int16_t)level<=0){
            for(unsigned i=0;i<count;i++){GameAddr track=tracks+i*80;if(!hn_audio_track_stop(m,track))return false;if(!(hn_read16(m,player+0x28)&1))hn_write8(m,track,0);}
            hn_write32(m,player+4,(level&1)?hn_read32(m,player+4)|0x80000000u:0x80000000u);hn_write16(m,player+0x24,0);return !m->fault;
        }
    }
    for(unsigned i=0;i<count;i++){
        GameAddr track=tracks+i*80;if(hn_read8(m,track)&128){hn_write8(m,track+0x13,(uint8_t)(level>>2));hn_write8(m,track,hn_read8(m,track)|3);}
    }
    return !m->fault;
}
void hn_audio_tempo(HNMemory *m,GameAddr player,uint16_t scale)
{
    if(!identified(m,player))return;hn_write16(m,player+0x1E,scale);
    uint32_t product=scale*(uint32_t)hn_read16(m,player+0x1C);hn_write16(m,player+0x20,(uint16_t)((int32_t)product>>8));
}
void hn_audio_modulation_reset(HNMemory *m,GameAddr track)
{
    hn_write8(m,track+0x1A,0);hn_write8(m,track+0x16,0);
    hn_write8(m,track,(uint8_t)(hn_read8(m,track)|(hn_read8(m,track+0x18)?3:12)));
}
void hn_audio_track_parameter(HNMemory *m,GameAddr player,uint32_t mask,uint32_t value,unsigned parameter)
{
    mask&=65535;value&=65535;if(!identified(m,player))return;hn_write32(m,player+0x34,HN_AUDIO_IDENT+1);
    GameAddr track=hn_read32(m,player+0x2C);unsigned count=hn_read8(m,player+8);
    for(unsigned i=0;i<count&&i<16;i++,track+=80)if((mask&(1u<<i))&&(hn_read8(m,track)&128)){
        switch(parameter){
        case 0:hn_write8(m,track+0x13,(uint8_t)(value>>2));hn_write8(m,track,hn_read8(m,track)|3);break;
        case 1:hn_write8(m,track+0x0B,(uint8_t)(value>>8));hn_write8(m,track+0x0D,(uint8_t)value);hn_write8(m,track,hn_read8(m,track)|12);break;
        case 2:hn_write8(m,track+0x15,(uint8_t)value);hn_write8(m,track,hn_read8(m,track)|3);break;
        case 3:case 4:{unsigned offset=parameter==3?0x17:0x19;hn_write8(m,track+offset,(uint8_t)value);if(!(value&255))hn_audio_modulation_reset(m,track);break;}
        }
    }
    hn_write32(m,player+0x34,HN_AUDIO_IDENT);
}
void hn_audio_volume_pitch(HNMemory *m,GameAddr t)
{
    unsigned status=hn_read8(m,t),type=hn_read8(m,t+0x18);int modulation=(int8_t)hn_read8(m,t+0x16);
    if(status&1){
        unsigned volume=(hn_read8(m,t+0x12)*hn_read8(m,t+0x13))>>5;if(type==1)volume=((modulation+128)*volume)>>7;
        int pan=(int8_t)hn_read8(m,t+0x14)*2+(int8_t)hn_read8(m,t+0x15);if(type==2)pan+=modulation;
        if(pan< -128)pan=-128;else if(pan>127)pan=127;
        hn_write8(m,t+0x10,(uint8_t)(((pan+128)*volume)>>8));hn_write8(m,t+0x11,(uint8_t)(((127-pan)*volume)>>8));
    }
    if(status&4){
        int bend=(int8_t)hn_read8(m,t+14)*hn_read8(m,t+15);
        int pitch=((int8_t)hn_read8(m,t+12)+bend)*4+(int8_t)hn_read8(m,t+10)*256+(int8_t)hn_read8(m,t+11)*256+hn_read8(m,t+13);
        if(!type)pitch+=modulation*16;hn_write8(m,t+8,(uint8_t)(pitch>>8));hn_write8(m,t+9,(uint8_t)pitch);
    }
    hn_write8(m,t,(uint8_t)(status&0xFA));
}
bool hn_audio_clear_channels(HNMemory *m)
{
    GameAddr p=core(m);if(hn_read32(m,p)!=HN_AUDIO_IDENT)return !m->fault;hn_write32(m,p,HN_AUDIO_IDENT+1);
    for(unsigned i=0;i<12;i++)hn_write8(m,p+0x50+i*64,0);
    GameAddr channel=hn_read32(m,p+0x1C);if(channel)for(unsigned i=0;i<4;i++){
        hn_audio_psg_stop(m,(uint8_t)(i+1));hn_write8(m,channel+i*64,0);
    }
    hn_write32(m,p,HN_AUDIO_IDENT);return !m->fault;
}
void hn_audio_psg_volume(HNMemory *m,GameAddr channel)
{
    unsigned right=hn_read8(m,channel+2),left=hn_read8(m,channel+3),volume=(right+left)>>4,pan=255;bool limited=false;
    if(right>=left){if(right/2>=left){pan=15;limited=true;}}
    else if(left/2>=right){pan=240;limited=true;}
    if(limited&&volume>15)volume=15;hn_write8(m,channel+10,(uint8_t)volume);
    hn_write8(m,channel+0x19,(uint8_t)((volume*hn_read8(m,channel+6)+15)>>4));
    hn_write8(m,channel+0x1B,(uint8_t)(pan&hn_read8(m,channel+0x1C)));
}
static uint32_t high(uint32_t a,uint32_t b){return (uint32_t)(((uint64_t)a*b)>>32);}
uint32_t hn_audio_wave_frequency(HNMemory *m,GameAddr wave,uint32_t key,uint32_t fraction)
{
    unsigned pitch=key&255;uint32_t scaled=fraction<<24;if(pitch>178){pitch=178;scaled=0xFF000000;}
    unsigned a=hn_read8(m,HN_AUDIO_WAVE_CODES+pitch),b=hn_read8(m,HN_AUDIO_WAVE_CODES+pitch+1);
    uint32_t first=hn_read32(m,HN_AUDIO_WAVE_BASES+(a&15)*4)>>(a>>4);
    uint32_t second=hn_read32(m,HN_AUDIO_WAVE_BASES+(b&15)*4)>>(b>>4);
    return high(hn_read32(m,wave+4),first+high(second-first,scaled));
}
uint32_t hn_audio_psg_frequency(HNMemory *m,uint8_t control,uint8_t key,uint8_t fraction)
{
    if(control==4){unsigned index=key<=20?0:key-21;if(index>59)index=59;return hn_read8(m,HN_AUDIO_NOISE_PERIODS+index);}
    unsigned index=0;if(key<=35)fraction=0;else{index=key-36;if(index>130){index=130;fraction=255;}}
    unsigned a=hn_read8(m,HN_AUDIO_PITCH_KEYS+index),b=hn_read8(m,HN_AUDIO_PITCH_KEYS+index+1);
    int first=(int16_t)hn_read16(m,HN_AUDIO_PITCH_PERIODS+(a&15)*2);first>>=a>>4;
    int second=(int16_t)hn_read16(m,HN_AUDIO_PITCH_PERIODS+(b&15)*2);second>>=b>>4;
    return (uint32_t)(first+(((second-first)*fraction)>>8)+2048);
}
