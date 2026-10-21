#include "game/audio.h"
#include "game/state.h"
static bool fail(HNAudio *a,HNAudioFault fault,GameAddr address)
{a->fault=fault;a->fault_opcode=0;a->fault_address=address;return false;}
static void initialize_track(HNMemory *m,GameAddr track)
{
    for(unsigned i=0;i<64;i++)hn_write8(m,track+i,0);
    hn_write8(m,track,128);hn_write8(m,track+15,2);hn_write8(m,track+0x13,64);
    hn_write8(m,track+0x19,22);hn_write8(m,track+0x24,1);
}
static bool channel_tick(HNAudio *a,GameAddr track)
{
    HNMemory *m=a->memory;GameAddr channel=hn_read32(m,track+0x20);unsigned visited=0;
    while(channel&&!m->fault){
        if(visited++==64)return fail(a,HN_AUDIO_BUDGET,channel);
        unsigned status=hn_read8(m,channel);
        if(status&0xC7){
            unsigned gate=hn_read8(m,channel+0x10);
            if(gate){gate--;hn_write8(m,channel+0x10,(uint8_t)gate);if(!gate)hn_write8(m,channel,(uint8_t)(status|64));}
        }else hn_audio_channel_unlink(m,channel);
        channel=hn_read32(m,channel+0x34);
    }
    return !m->fault;
}
static bool track_tick(HNAudio *a,GameAddr player,GameAddr track,unsigned *commands)
{
    HNMemory *m=a->memory;if(!channel_tick(a,track))return false;
    if(hn_read8(m,track)&64)initialize_track(m,track);
    while(!hn_read8(m,track+1)&&!m->fault){
        GameAddr cursor=hn_read32(m,track+0x40);if(!*commands)return fail(a,HN_AUDIO_BUDGET,cursor);(*commands)--;
        unsigned op=hn_read8(m,cursor);
        if(op<128)op=hn_read8(m,track+7);
        else{hn_write32(m,track+0x40,cursor+1);if(op>=0xBD)hn_write8(m,track+7,(uint8_t)op);}
        if(op>=0xCF){if(!hn_audio_note(a,player,track,op-0xCF))return false;}
        else if(op>0xB0){
            hn_write8(m,player+10,(uint8_t)(op-0xB1));if(!hn_audio_sequence_command(a,player,track,op))return false;
            if(!hn_read8(m,track))return !m->fault;
        }else{
            if(op<128){a->fault_opcode=op;return fail(a,HN_AUDIO_COMMAND,cursor);}
            hn_write8(m,track+1,hn_read8(m,HN_AUDIO_CLOCK_VALUES+op-128));
        }
    }
    hn_write8(m,track+1,(uint8_t)(hn_read8(m,track+1)-1));
    unsigned speed=hn_read8(m,track+0x19),depth=hn_read8(m,track+0x17);
    if(speed&&depth){
        unsigned delay=hn_read8(m,track+0x1C);
        if(delay)hn_write8(m,track+0x1C,(uint8_t)(delay-1));
        else{
            unsigned phase=hn_read8(m,track+0x1A)+speed;hn_write8(m,track+0x1A,(uint8_t)phase);
            int triangle=(int8_t)(phase-64)<0?(int8_t)phase:128-(int)phase;
            int value=((int)depth*triangle)>>6;
            if((uint8_t)value!=hn_read8(m,track+0x16)){
                hn_write8(m,track+0x16,(uint8_t)value);
                hn_write8(m,track,(uint8_t)(hn_read8(m,track)|(hn_read8(m,track+0x18)?3:12)));
            }
        }
    }
    return !m->fault;
}
static bool apply_track(HNAudio *a,GameAddr track)
{
    HNMemory *m=a->memory;unsigned flags=hn_read8(m,track);if(!(flags&128)||!(flags&15))return !m->fault;
    hn_audio_volume_pitch(m,track);GameAddr channel=hn_read32(m,track+0x20);unsigned visited=0;
    while(channel&&!m->fault){
        if(visited++==64)return fail(a,HN_AUDIO_BUDGET,channel);
        if(!(hn_read8(m,channel)&0xC7))hn_audio_channel_unlink(m,channel);
        else{
            unsigned kind=hn_read8(m,channel+1)&7;flags=hn_read8(m,track);
            if(flags&3){hn_audio_channel_volume(m,channel,track);if(kind)hn_write8(m,channel+0x1D,hn_read8(m,channel+0x1D)|1);}
            if(flags&12){
                int key=hn_read8(m,channel+8)+(int8_t)hn_read8(m,track+8);if(key<0)key=0;uint32_t frequency;
                if(kind){
                    GameAddr core=hn_read32(m,HN_AUDIO_CORE_POINTER),binding=hn_read32(m,core+0x30);
                    if((binding&~1u)!=0x080EB61C)return fail(a,HN_AUDIO_CALLBACK,binding);
                    frequency=hn_audio_psg_frequency(m,(uint8_t)kind,(uint8_t)key,hn_read8(m,track+9));
                    hn_write8(m,channel+0x1D,hn_read8(m,channel+0x1D)|2);
                }else frequency=hn_audio_wave_frequency(m,hn_read32(m,channel+0x24),(unsigned)key,hn_read8(m,track+9));
                hn_write32(m,channel+0x20,frequency);
            }
        }
        channel=hn_read32(m,channel+0x34);
    }
    hn_write8(m,track,hn_read8(m,track)&0xF0);return !m->fault;
}
static bool player_update(HNAudio *a,GameAddr player,unsigned command_budget,unsigned depth)
{
    if(a->fault)return false;HNMemory *m=a->memory;
    if(hn_read32(m,player+0x34)!=HN_AUDIO_IDENT)return m->fault?fail(a,HN_AUDIO_MEMORY,m->fault_address):true;
    if(depth>=64)return fail(a,HN_AUDIO_BUDGET,player);
    hn_write32(m,player+0x34,HN_AUDIO_IDENT+1);
    GameAddr next=hn_read32(m,player+0x38);
    if(next&&(next&~1u)!=0x080EC0B4){
        if((next&~1u)!=0x080EA5B4)return fail(a,HN_AUDIO_CALLBACK,next);
        if(!player_update(a,hn_read32(m,player+0x3C),command_budget,depth+1))return false;
    }
    if((int32_t)hn_read32(m,player+4)>=0){
        if(!hn_audio_fade_update(m,player))return fail(a,HN_AUDIO_RESOURCE,player);
        if((int32_t)hn_read32(m,player+4)>=0){
            unsigned count=hn_read8(m,player+8);if(count>16)return fail(a,HN_AUDIO_RESOURCE,player+8);if(!count)count=1;
            GameAddr tracks=hn_read32(m,player+0x2C);
            uint32_t accumulator=hn_read16(m,player+0x22)+hn_read16(m,player+0x20);
            hn_write16(m,player+0x22,(uint16_t)accumulator);unsigned ticks=0;
            while(accumulator>=150){
                if(ticks++>=1024)return fail(a,HN_AUDIO_BUDGET,player+0x22);
                uint32_t active=0;
                for(unsigned i=0;i<count;i++){
                    GameAddr track=tracks+i*80;if(hn_read8(m,track)&128){active|=1u<<i;if(!track_tick(a,player,track,&command_budget))return false;}
                }
                hn_write32(m,player+12,hn_read32(m,player+12)+1);
                if(!active){hn_write32(m,player+4,0x80000000);hn_write32(m,player+0x34,HN_AUDIO_IDENT);return !m->fault;}
                hn_write32(m,player+4,active);accumulator=(uint32_t)hn_read16(m,player+0x22)-150;
                hn_write16(m,player+0x22,(uint16_t)accumulator);
            }
            for(unsigned i=0;i<count;i++)if(!apply_track(a,tracks+i*80))return false;
        }
    }
    hn_write32(m,player+0x34,HN_AUDIO_IDENT);return m->fault?fail(a,HN_AUDIO_MEMORY,m->fault_address):true;
}
bool hn_audio_player_update(HNAudio *a,GameAddr player,unsigned command_budget)
{
    bool ok=player_update(a,player,command_budget,0);
    if(!ok&&!a->fault)return fail(a,a->memory->fault?HN_AUDIO_MEMORY:HN_AUDIO_RESOURCE,a->memory->fault?a->memory->fault_address:player);
    return ok;
}
bool hn_audio_update_players(HNAudio *a,unsigned command_budget)
{
    if(a->fault)return false;HNMemory *m=a->memory;GameAddr core=hn_read32(m,HN_AUDIO_CORE_POINTER),binding=hn_read32(m,core+0x20);
    if(!binding||(binding&~1u)==0x080EC0B4)return m->fault?fail(a,HN_AUDIO_MEMORY,m->fault_address):true;
    if((binding&~1u)!=0x080EA5B4)return fail(a,HN_AUDIO_CALLBACK,binding);
    return hn_audio_player_update(a,hn_read32(m,core+0x24),command_budget);
}
