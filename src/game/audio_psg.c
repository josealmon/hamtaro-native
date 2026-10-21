#include "game/audio.h"
#include "game/state.h"
bool hn_audio_psg_update(HNAudio *a)
{
    if(a->fault)return false;HNMemory *m=a->memory;GameAddr core=hn_read32(m,HN_AUDIO_CORE_POINTER);
    unsigned phase=hn_read8(m,core+10);phase=phase?phase-1:14;hn_write8(m,core+10,(uint8_t)phase);
    GameAddr channels=hn_read32(m,core+0x1C);
    const GameAddr ports[4][5]={
        {0x04000060,0x04000062,0x04000063,0x04000064,0x04000065},
        {0x04000061,0x04000068,0x04000069,0x0400006C,0x0400006D},
        {0x04000070,0x04000072,0x04000073,0x04000074,0x04000075},
        {0x04000071,0x04000078,0x04000079,0x0400007C,0x0400007D}};
    for(unsigned index=1;index<=4;index++){
        GameAddr c=channels+(index-1)*64;const GameAddr *p=ports[index-1];
#define C8(o) hn_read8(m,c+(o))
#define CW(o,v) hn_write8(m,c+(o),(uint8_t)(v))
#define P8(o) hn_read8(m,p[o])
#define PW(o,v) hn_write8(m,p[o],(uint8_t)(v))
        if(!(C8(0)&0xC7))continue;
        int previous_phase=(int)phase,envelope_step=P8(2);
        if(C8(0)&128){
            if(C8(0)&64)goto oscillator_off;
            CW(0,3);CW(0x1D,3);hn_audio_psg_volume(m,c);
            switch(index){
            case 1:PW(0,C8(0x1F));
            case 2:PW(1,(hn_read32(m,c+0x24)<<6)+C8(0x1E));goto initial_envelope;
            case 3: {
                GameAddr wave=hn_read32(m,c+0x24);
                if(wave!=hn_read32(m,c+0x28)){
                    PW(0,64);for(unsigned i=0;i<4;i++)hn_write32(m,0x04000090+i*4,hn_read32(m,wave+i*4));
                    hn_write32(m,c+0x28,wave);
                }
                PW(0,0);PW(1,C8(0x1E));CW(0x1A,C8(0x1E)?0xC0:128);break;
            }
            default:PW(1,C8(0x1E));PW(3,hn_read32(m,c+0x24)<<3);
initial_envelope:
                envelope_step=C8(4)+8;CW(0x1A,C8(0x1E)?64:0);break;
            }
            CW(11,C8(4));if(C8(4)){CW(9,0);goto envelope_step;}goto decay_start;
        }else if(C8(0)&4){
            CW(13,C8(13)-1);if((int8_t)C8(13)<=0){
oscillator_off:
                hn_audio_psg_stop(m,(uint8_t)index);CW(0,0);goto complete;
            }
            goto apply_parameters;
        }else if((C8(0)&64)&&(C8(0)&3)){
            CW(0,C8(0)&0xFC);CW(11,C8(7));
            if(C8(7)){CW(0x1D,C8(0x1D)|1);if(index!=3)envelope_step=C8(7);goto envelope_step;}goto echo_start;
        }else{
envelope_repeat:
            if(!C8(11)){
                if(index==3)CW(0x1D,C8(0x1D)|1);hn_audio_psg_volume(m,c);
                if(!(C8(0)&3)){
                    CW(9,C8(9)-1);if((int8_t)C8(9)<=0){
echo_start:
                        CW(9,(C8(10)*C8(12)+255)>>8);
                        if(C8(9)){CW(0,C8(0)|4);CW(0x1D,C8(0x1D)|1);if(index!=3)envelope_step=8;goto apply_parameters;}
                        goto oscillator_off;
                    }
                    CW(11,C8(7));
                }else if((C8(0)&3)==1){
sustain:
                    CW(9,C8(0x19));CW(11,7);
                }else if((C8(0)&3)==2){
                    CW(9,C8(9)-1);if((int8_t)C8(9)<=(int8_t)C8(0x19)){
sustain_start:
                        if(!C8(6)){CW(0,C8(0)&0xFC);goto echo_start;}
                        CW(0,C8(0)-1);CW(0x1D,C8(0x1D)|1);if(index!=3)envelope_step=8;goto sustain;
                    }
                    CW(11,C8(5));
                }else{
                    CW(9,C8(9)+1);if(C8(9)>=C8(10)){
decay_start:
                        CW(0,C8(0)-1);CW(11,C8(5));
                        if(C8(11)){CW(0x1D,C8(0x1D)|1);CW(9,C8(10));if(index!=3)envelope_step=C8(5);}
                        else goto sustain_start;
                    }else CW(11,C8(4));
                }
            }
        }
envelope_step:
        CW(11,C8(11)-1);if(!previous_phase){previous_phase--;goto envelope_repeat;}
apply_parameters:
        if(C8(0x1D)&2){
            uint32_t frequency=hn_read32(m,c+0x20);
            if(index<4&&(C8(1)&8)){
                unsigned rate=hn_read8(m,0x04000089);
                if(rate<64)frequency=(frequency+2)&0x7FC;else if(rate<128)frequency=(frequency+1)&0x7FE;
                hn_write32(m,c+0x20,frequency);
            }
            PW(3,index==4?(P8(3)&8)|frequency:frequency);
            CW(0x1A,(C8(0x1A)&0xC0)+hn_read8(m,c+0x21));PW(4,C8(0x1A));
        }
        if(C8(0x1D)&1){
            hn_write8(m,0x04000081,(uint8_t)((hn_read8(m,0x04000081)&~C8(0x1C))|C8(0x1B)));
            if(index==3){
                PW(2,hn_read8(m,HN_AUDIO_PSG_WAVE_VOLUMES+C8(9)));
                if(C8(0x1A)&128){PW(0,128);PW(4,C8(0x1A));CW(0x1A,C8(0x1A)&127);}
            }else{
                PW(2,(envelope_step&15)+(C8(9)<<4));PW(4,C8(0x1A)|128);
                if(index==1&&!(P8(0)&8))PW(4,C8(0x1A)|128);
            }
        }
complete:
        CW(0x1D,0);
#undef C8
#undef CW
#undef P8
#undef PW
        if(m->fault){a->fault=HN_AUDIO_MEMORY;a->fault_address=m->fault_address;return false;}
    }
    if(m->fault){a->fault=HN_AUDIO_MEMORY;a->fault_address=m->fault_address;return false;}return true;
}
