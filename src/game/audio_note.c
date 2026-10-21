#include "game/audio.h"
#include "game/state.h"
static bool fail(HNAudio *a,HNAudioFault fault,GameAddr address)
{a->fault=fault;a->fault_opcode=0xCF;a->fault_address=address;return false;}
void hn_audio_channel_volume(HNMemory *m,GameAddr channel,GameAddr track)
{
    int pan=(int8_t)hn_read8(m,channel+0x14);unsigned velocity=hn_read8(m,channel+0x12);
    unsigned right=((pan+128)*velocity*hn_read8(m,track+0x10))>>14;
    unsigned left=((127-pan)*velocity*hn_read8(m,track+0x11))>>14;
    hn_write8(m,channel+2,(uint8_t)(right>255?255:right));hn_write8(m,channel+3,(uint8_t)(left>255?255:left));
}
bool hn_audio_note(HNAudio *a,GameAddr player,GameAddr track,unsigned selector)
{
    if(a->fault)return false;HNMemory *m=a->memory;
    if(selector>48)return fail(a,HN_AUDIO_COMMAND,hn_read32(m,track+0x40));
    GameAddr core=hn_read32(m,HN_AUDIO_CORE_POINTER);
    hn_write8(m,track+4,hn_read8(m,HN_AUDIO_CLOCK_VALUES+selector));
    GameAddr cursor=hn_read32(m,track+0x40);
    if(hn_read8(m,cursor)<128){
        hn_write8(m,track+5,hn_read8(m,cursor++));
        if(hn_read8(m,cursor)<128){
            hn_write8(m,track+6,hn_read8(m,cursor++));
            if(hn_read8(m,cursor)<128)hn_write8(m,track+4,(uint8_t)(hn_read8(m,track+4)+hn_read8(m,cursor++)));
        }
        hn_write32(m,track+0x40,cursor);
    }
    GameAddr tone=track+0x24;unsigned root_type=hn_read8(m,tone),key=hn_read8(m,track+5);int pan=0;
    if(root_type&0xC0){
        unsigned index=key;if(root_type&64)index=hn_read8(m,hn_read32(m,track+0x2C)+key);
        tone=hn_read32(m,track+0x28)+index*12;
        if(hn_read8(m,tone)&0xC0)return !m->fault;
        if(root_type&128){unsigned value=hn_read8(m,tone+3);if(value&128)pan=((int)value-192)*2;key=hn_read8(m,tone+1);}
    }
    unsigned priority=hn_read8(m,player+9)+hn_read8(m,track+0x1D);if(priority>255)priority=255;
    unsigned kind=hn_read8(m,tone)&7;GameAddr selected=0;
    if(kind){
        GameAddr psg=hn_read32(m,core+0x1C);if(!psg)return !m->fault;
        if(kind>4)return fail(a,HN_AUDIO_RESOURCE,tone);
        GameAddr channel=psg+(kind-1)*64;unsigned status=hn_read8(m,channel);
        if((status&0xC7)&&!(status&64)){
            unsigned old_priority=hn_read8(m,channel+0x13);GameAddr owner=hn_read32(m,channel+0x2C);
            if(old_priority>priority||(old_priority==priority&&owner<track))return !m->fault;
        }
        selected=channel;
    }else{
        unsigned count=hn_read8(m,core+6);if(count>12)return fail(a,HN_AUDIO_RESOURCE,core+6);if(!count)count=1;
        unsigned best_priority=priority;GameAddr best_owner=track;bool released=false;
        for(unsigned i=0;i<count;i++){
            GameAddr channel=core+0x50+i*64;unsigned status=hn_read8(m,channel);
            if(!(status&0xC7)){selected=channel;break;}
            unsigned old_priority=hn_read8(m,channel+0x13);GameAddr owner=hn_read32(m,channel+0x2C);
            if(status&64){
                if(!released){released=true;best_priority=old_priority;best_owner=owner;selected=channel;continue;}
            }else if(released)continue;
            if(old_priority<best_priority||(old_priority==best_priority&&owner>=best_owner)){
                best_priority=old_priority;best_owner=owner;selected=channel;
            }
        }
        if(!selected)return !m->fault;
    }
    hn_audio_channel_unlink(m,selected);hn_write32(m,selected+0x30,0);
    GameAddr previous_head=hn_read32(m,track+0x20);hn_write32(m,selected+0x34,previous_head);
    if(previous_head)hn_write32(m,previous_head+0x30,selected);
    hn_write32(m,track+0x20,selected);hn_write32(m,selected+0x2C,track);
    unsigned lfo_delay=hn_read8(m,track+0x1B);hn_write8(m,track+0x1C,(uint8_t)lfo_delay);
    if(lfo_delay)hn_audio_modulation_reset(m,track);hn_audio_volume_pitch(m,track);
    hn_write32(m,selected+0x10,hn_read32(m,track+4));hn_write8(m,selected+0x13,(uint8_t)priority);
    hn_write8(m,selected+8,(uint8_t)key);hn_write8(m,selected+0x14,(uint8_t)pan);
    hn_write8(m,selected+1,hn_read8(m,tone));GameAddr wave=hn_read32(m,tone+4);
    hn_write32(m,selected+0x24,wave);hn_write32(m,selected+4,hn_read32(m,tone+8));
    hn_write16(m,selected+12,hn_read16(m,track+0x1E));hn_audio_channel_volume(m,selected,track);
    int pitch=(int)hn_read8(m,selected+8)+(int8_t)hn_read8(m,track+8);if(pitch<0)pitch=0;
    uint32_t frequency;
    if(kind){
        hn_write8(m,selected+0x1E,hn_read8(m,tone+2));unsigned length=hn_read8(m,tone+3);
        if((length&128)||!(length&0x70))length=8;hn_write8(m,selected+0x1F,(uint8_t)length);
        GameAddr binding=hn_read32(m,core+0x30);
        if((binding&~1u)!=0x080EB61C)return fail(a,HN_AUDIO_CALLBACK,binding);
        frequency=hn_audio_psg_frequency(m,(uint8_t)kind,(uint8_t)pitch,hn_read8(m,track+9));
    }else frequency=hn_audio_wave_frequency(m,wave,(unsigned)pitch,hn_read8(m,track+9));
    hn_write32(m,selected+0x20,frequency);hn_write8(m,selected,128);hn_write8(m,track,hn_read8(m,track)&0xF0);
    return m->fault?fail(a,HN_AUDIO_MEMORY,m->fault_address):true;
}
