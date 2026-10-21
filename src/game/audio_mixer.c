#include "game/audio.h"
#include "game/state.h"
static bool fail(HNAudio *a,HNAudioFault fault,GameAddr address)
{a->fault=fault;a->fault_opcode=0;a->fault_address=address;return false;}
static uint32_t rotate(uint32_t value,unsigned bits)
{bits&=31;return bits?(value>>bits)|(value<<(32-bits)):value;}
typedef struct {GameAddr address;uint32_t right,left;} OutputWord;
static void add_sample(HNMemory *m,OutputWord *out,unsigned lane,int value,unsigned right,unsigned left)
{
    if(!lane){out->right=hn_read32(m,out->address);out->left=hn_read32(m,out->address+0x630);}
    uint32_t r=((right<<16)*(uint32_t)value)&~0x00FF0000u,l=((left<<16)*(uint32_t)value)&~0x00FF0000u;
    out->right=rotate(out->right,8)+r;out->left=rotate(out->left,8)+l;
}
static void store_word(HNMemory *m,OutputWord *out,unsigned rotation)
{
    hn_write32(m,out->address+0x630,rotate(out->left,rotation));hn_write32(m,out->address,rotate(out->right,rotation));out->address+=4;
}
static bool pcm_channel(HNAudio *a,GameAddr core,GameAddr channel,GameAddr output,unsigned samples)
{
    HNMemory *m=a->memory;unsigned status=hn_read8(m,channel);if(!(status&0xC7))return true;
    GameAddr wave=hn_read32(m,channel+0x24);unsigned envelope;
    if(status&128){
        if(status&64){hn_write8(m,channel,0);return true;}
        status=3;hn_write8(m,channel,3);hn_write32(m,channel+0x28,wave+16);
        hn_write32(m,channel+0x18,hn_read32(m,wave+12));envelope=0;hn_write8(m,channel+9,0);hn_write32(m,channel+0x1C,0);
        if(hn_read8(m,wave+3)&0xC0){status|=16;hn_write8(m,channel,(uint8_t)status);}
        envelope+=hn_read8(m,channel+4);if(envelope>=255){envelope=255;status--;hn_write8(m,channel,(uint8_t)status);}
    }else{
        envelope=hn_read8(m,channel+9);
        if(status&4){
            unsigned ticks=hn_read8(m,channel+13);hn_write8(m,channel+13,(uint8_t)(ticks-1));
            if(ticks<=1){hn_write8(m,channel,0);return true;}
        }else{
            bool echo=false;
            if(status&64){envelope=(envelope*hn_read8(m,channel+7))>>8;echo=envelope<=hn_read8(m,channel+12);}
            else if((status&3)==2){
                envelope=(envelope*hn_read8(m,channel+5))>>8;unsigned sustain=hn_read8(m,channel+6);
                if(envelope<=sustain){envelope=sustain;if(!sustain)echo=true;else{status--;hn_write8(m,channel,(uint8_t)status);}}
            }else if((status&3)==3){
                envelope+=hn_read8(m,channel+4);if(envelope>=255){envelope=255;status--;hn_write8(m,channel,(uint8_t)status);}
            }
            if(echo){
                envelope=hn_read8(m,channel+12);if(!envelope){hn_write8(m,channel,0);return true;}
                status|=4;hn_write8(m,channel,(uint8_t)status);
            }
        }
    }
    hn_write8(m,channel+9,(uint8_t)envelope);unsigned gain=((hn_read8(m,core+7)+1)*envelope)>>4;
    unsigned right=(uint8_t)((gain*hn_read8(m,channel+2))>>8),left=(uint8_t)((gain*hn_read8(m,channel+3))>>8);
    hn_write8(m,channel+10,(uint8_t)right);hn_write8(m,channel+11,(uint8_t)left);
    uint32_t loop_length=0;GameAddr loop_start=0;
    if(status&16){unsigned offset=hn_read32(m,wave+8);loop_start=wave+16+offset;loop_length=hn_read32(m,wave+12)-offset;}
    if((int32_t)loop_length<0)return fail(a,HN_AUDIO_RESOURCE,wave+8);
    uint32_t remaining=hn_read32(m,channel+0x18),phase=hn_read32(m,channel+0x1C);
    GameAddr cursor=hn_read32(m,channel+0x28);bool direct=(hn_read8(m,channel+1)&8)!=0,stopped=false;
    uint32_t step=hn_read32(m,core+0x18)*hn_read32(m,channel+0x20);int first=0,difference=0;
    if(!direct){first=(int8_t)hn_read8(m,cursor);difference=(int8_t)hn_read8(m,++cursor)-first;}
    OutputWord out={output,0,0};
    for(unsigned i=0;i<samples&&!m->fault;i++){
        unsigned lane=i&3;int value;
        if(direct)value=(int8_t)hn_read8(m,cursor++);
        else value=first+((int32_t)(phase*(uint32_t)difference)>>23);
        add_sample(m,&out,lane,value,right,left);
        if(direct){
            remaining--;
            if(!remaining){if(loop_length){remaining=loop_length;cursor=loop_start;}else stopped=true;}
        }else{
            phase+=step;unsigned advance=phase>>23;
            if(advance){
                phase&=~0x3F800000u;remaining-=advance;
                if((int32_t)remaining<=0){
                    if(!loop_length)stopped=true;
                    else{
                        cursor=loop_start;unsigned offset=0u-remaining,wraps=0;
                        while((int32_t)(remaining+=loop_length)<=0){offset-=loop_length;if(wraps++>=512)return fail(a,HN_AUDIO_BUDGET,wave);}
                        cursor+=offset;first=(int8_t)hn_read8(m,cursor);difference=(int8_t)hn_read8(m,++cursor)-first;
                    }
                }else{
                    advance--;if(!advance)first+=difference;else{cursor+=advance;first=(int8_t)hn_read8(m,cursor);}
                    difference=(int8_t)hn_read8(m,++cursor)-first;
                }
            }
        }
        if(stopped){hn_write8(m,channel,0);store_word(m,&out,(3-lane)*8);break;}
        if(lane==3)store_word(m,&out,0);
    }
    if(!stopped){
        if(!direct){cursor--;hn_write32(m,channel+0x1C,phase);}
        hn_write32(m,channel+0x18,remaining);hn_write32(m,channel+0x28,cursor);
    }
    return m->fault?fail(a,HN_AUDIO_MEMORY,m->fault_address):true;
}
bool hn_audio_mix(HNAudio *a,unsigned command_budget)
{
    if(a->fault)return false;HNMemory *m=a->memory;GameAddr core=hn_read32(m,HN_AUDIO_CORE_POINTER);
    if(hn_read32(m,core)!=HN_AUDIO_IDENT)return m->fault?fail(a,HN_AUDIO_MEMORY,m->fault_address):true;
    hn_write32(m,core,HN_AUDIO_IDENT+1);unsigned deadline=hn_read8(m,core+12);
    if(deadline){unsigned line=hn_read8(m,0x04000006);deadline+=line<160?line+228:line;}
    if(!hn_audio_update_players(a,command_budget))return false;
    GameAddr psg_binding=hn_read32(m,core+0x28);
    if((psg_binding&~1u)==0x080EB77C){if(!hn_audio_psg_update(a))return false;}
    else if((psg_binding&~1u)!=0x080EC0B4)return fail(a,HN_AUDIO_CALLBACK,psg_binding);
    unsigned samples=hn_read32(m,core+0x10),count=hn_read8(m,core+6),counter=hn_read8(m,core+4);
    if(samples<16||samples>1584||(samples&3)||count>12)return fail(a,HN_AUDIO_RESOURCE,core+0x10);
    GameAddr output=core+0x350;
    if(counter>1){
        int bank=(int)hn_read8(m,core+11)-(int)counter+1;
        if(bank<0||(unsigned)bank*samples>1584-samples)return fail(a,HN_AUDIO_RESOURCE,core+4);
        output+=(unsigned)bank*samples;
    }
    unsigned reverb=hn_read8(m,core+5);
    if(reverb){
        GameAddr next=counter==2?core+0x350:output+samples;
        for(unsigned i=0;i<samples&&!m->fault;i++){
            int sum=(int8_t)hn_read8(m,output+i)+(int8_t)hn_read8(m,output+i+0x630)+
                    (int8_t)hn_read8(m,next+i)+(int8_t)hn_read8(m,next+i+0x630);
            int value=(sum*(int)reverb)>>9;if(value&128)value++;
            hn_write8(m,output+i,(uint8_t)value);hn_write8(m,output+i+0x630,(uint8_t)value);
        }
    }else for(unsigned i=0;i<samples;i++){hn_write8(m,output+i,0);hn_write8(m,output+i+0x630,0);}
    if(!count)count=1;
    for(unsigned i=0;i<count;i++){
        if(deadline){unsigned line=hn_read8(m,0x04000006);if((line<160?line+228:line)>=deadline)break;}
        if(!pcm_channel(a,core,core+0x50+i*64,output,samples))return false;
    }
    hn_write32(m,core,HN_AUDIO_IDENT);return m->fault?fail(a,HN_AUDIO_MEMORY,m->fault_address):true;
}
