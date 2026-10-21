#include "gba/psg.h"
#include <string.h>

static unsigned envelope_register(unsigned i){return i==0?3:i==1?9:0x19;}
static unsigned bank(const HNPsg *s){return s->enabled?((s->registers[0x10]>>6)&1):0;}
static uint32_t oscillator_period(const HNPsg *s,unsigned i)
{
    if(i<3)return (2048u-s->channel[i].frequency)*(i==2?8u:16u);
    unsigned noise=s->registers[0x1C],shift=noise>>4,ratio=noise&7;
    if(shift>=14)return UINT32_MAX;
    return (ratio?ratio*64u:32u)<<shift;
}
static bool clocked(const HNPsg *s,unsigned i)
{
    if(!s->channel[i].started)return false;
    if(i>=2&&!s->channel[i].active)return false;
    return i!=3||(s->registers[0x1C]>>4)<14;
}
void hn_psg_init(HNPsg *s)
{memset(s,0,sizeof(*s));s->frame_remaining=32768;}
void hn_psg_power(HNPsg *s,bool enabled)
{
    if(s->enabled==enabled)return;
    memset(s->channel,0,sizeof(s->channel));memset(s->registers,0,sizeof(s->registers));
    s->enabled=enabled;s->frame_step=0;s->sweep_enabled=false;s->sweep_subtracted=false;
    s->sweep_shadow=s->sweep_clock=0;s->lfsr=0;
}
uint8_t hn_psg_status(const HNPsg *s)
{
    uint8_t status=0;if(s->enabled)for(unsigned i=0;i<4;i++)if(s->channel[i].active)status|=(uint8_t)(1u<<i);
    return status;
}
static uint8_t readable_mask(unsigned offset)
{
    switch(offset){
    case 0x60:return 0x7F;
    case 0x62:case 0x68:return 0xC0;
    case 0x63:case 0x69:case 0x79:case 0x7C:case 0x81:return 0xFF;
    case 0x65:case 0x6D:case 0x75:case 0x7D:return 0x40;
    case 0x70:return 0xE0;case 0x73:return 0xE0;case 0x80:return 0x77;
    default:return 0;
    }
}
uint8_t hn_psg_read(const HNPsg *s,unsigned offset)
{
    if(offset>=0x90&&offset<=0x9F)return s->wave[(1-bank(s))*16+offset-0x90];
    if(offset<0x60||offset>0x81)return 0;
    return s->registers[offset-0x60]&readable_mask(offset);
}
static void envelope_trigger(HNPsg *s,unsigned i)
{
    HNPsgChannel *c=&s->channel[i];unsigned value=s->registers[envelope_register(i)],period=value&7;
    c->active=(value&0xF8)!=0;c->volume=(uint8_t)(value>>4);
    c->envelope_clock=(uint8_t)((period?period:8)+(s->frame_step==7));c->envelope_running=true;
}
static int sweep_frequency(HNPsg *s)
{
    unsigned sweep=s->registers[0];int delta=s->sweep_shadow>>(sweep&7);
    if(sweep&8){s->sweep_subtracted=true;return s->sweep_shadow-delta;}
    return s->sweep_shadow+delta;
}
static void trigger(HNPsg *s,unsigned i)
{
    HNPsgChannel *c=&s->channel[i];if(!c->started)c->first=true;c->started=true;
    if(!c->length)c->length=(uint16_t)((i==2?256:64)-((s->frame_step&1)&&c->length_enabled));
    if(i==2){c->active=(s->registers[0x10]&0x80)!=0;c->phase=0;}
    else envelope_trigger(s,i);
    c->remaining=oscillator_period(s,i);
    if(i<2)c->remaining-=(unsigned)(s->cycles&15); /* Preserve the divider's low bits. */
    if(i==3){s->lfsr=(s->registers[0x1C]&8)?0x40:0x4000;c->sample=0;}
    if(i==0){
        unsigned sweep=s->registers[0],pace=(sweep>>4)&7;s->sweep_shadow=c->frequency;
        s->sweep_clock=(uint8_t)(pace?pace:8);s->sweep_enabled=(pace||(sweep&7));s->sweep_subtracted=false;
        if((sweep&7)&&sweep_frequency(s)>2047)c->active=false;
    }
}
static void control(HNPsg *s,unsigned i,uint8_t value)
{
    HNPsgChannel *c=&s->channel[i];bool length=(value&0x40)!=0;
    if(!c->length_enabled&&length&&(s->frame_step&1)&&c->length&&!--c->length)c->active=false;
    c->length_enabled=length;if(value&0x80)trigger(s,i);
}
void hn_psg_write(HNPsg *s,unsigned offset,uint8_t value)
{
    if(offset>=0x90&&offset<=0x9F){s->wave[(1-bank(s))*16+offset-0x90]=value;return;}
    if(offset<0x60||offset>0x81)return;
    if(!s->enabled)return;
    unsigned o=offset-0x60;uint8_t previous=s->registers[o];
    switch(offset){
    case 0x60:
        s->registers[o]=value&0x7F;
        if((previous&8)&&!(value&8)&&s->sweep_subtracted)s->channel[0].active=false;return;
    case 0x62:case 0x68:case 0x78:{
        unsigned i=offset==0x62?0:offset==0x68?1:3;
        s->channel[i].length=(uint16_t)(64-(value&63));s->registers[o]=value;return;
    }
    case 0x63:case 0x69:case 0x79:{
        unsigned i=offset==0x63?0:offset==0x69?1:3;s->registers[o]=value;
        if(!(value&0xF8))s->channel[i].active=false;return;
    }
    case 0x64:case 0x6C:case 0x74:{
        unsigned i=offset==0x64?0:offset==0x6C?1:2;
        s->channel[i].frequency=(uint16_t)((s->channel[i].frequency&0x700)|value);s->registers[o]=value;return;
    }
    case 0x65:case 0x6D:case 0x75:{
        unsigned i=offset==0x65?0:offset==0x6D?1:2;
        s->channel[i].frequency=(uint16_t)((s->channel[i].frequency&255)|((value&7)<<8));
        s->registers[o]=value&0x47;control(s,i,value);return;
    }
    case 0x70:s->registers[o]=value&0xE0;if(!(value&0x80))s->channel[2].active=false;return;
    case 0x72:s->registers[o]=value;s->channel[2].length=(uint16_t)(256-value);return;
    case 0x73:s->registers[o]=value&0xE0;return;
    case 0x7C:
        s->registers[o]=value;
        if((previous>>4)>=14&&(value>>4)<14)s->channel[3].remaining=oscillator_period(s,3);return;
    case 0x7D:s->registers[o]=value&0x40;control(s,3,value);return;
    case 0x80:s->registers[o]=value&0x77;return;
    case 0x81:s->registers[o]=value;return;
    default:return;
    }
}
static void frame(HNPsg *s)
{
    unsigned step=s->frame_step;s->frame_step=(uint8_t)((step+1)&7);
    if(!(step&1))for(unsigned i=0;i<4;i++){
        HNPsgChannel *c=&s->channel[i];if(c->length_enabled&&c->length&&!--c->length)c->active=false;
    }
    if((step==2||step==6)&&s->sweep_enabled&&!--s->sweep_clock){
        unsigned sweep=s->registers[0],pace=(sweep>>4)&7;s->sweep_clock=(uint8_t)(pace?pace:8);
        if(pace){
            int frequency=sweep_frequency(s);
            if(frequency>2047)s->channel[0].active=false;
            else if(sweep&7){
                s->sweep_shadow=s->channel[0].frequency=(uint16_t)frequency;
                if(sweep_frequency(s)>2047)s->channel[0].active=false;
            }
        }
    }
    if(step==7)for(unsigned i=0;i<4;i++)if(i!=2){
        HNPsgChannel *c=&s->channel[i];unsigned value=s->registers[envelope_register(i)],pace=value&7;
        if(c->active&&c->envelope_running&&!--c->envelope_clock){
            c->envelope_clock=(uint8_t)(pace?pace:8);
            if(pace){
                int volume=c->volume+((value&8)?1:-1);
                if(volume<0||volume>15)c->envelope_running=false;else c->volume=(uint8_t)volume;
            }
        }
    }
}
static void oscillator(HNPsg *s,unsigned i)
{
    HNPsgChannel *c=&s->channel[i];
    if(i<2){c->phase=(uint8_t)((c->phase+1)&7);c->first=false;}
    else if(i==2){
        unsigned first=bank(s)*16,other=first^16,nibble=s->wave[first]>>4;
        unsigned carry=(s->registers[0x10]&0x20)?s->wave[other]>>4:nibble;
        for(unsigned j=0;j<16;j++)s->wave[first+j]=(uint8_t)((s->wave[first+j]<<4)|
            (j==15?carry:s->wave[first+j+1]>>4));
        if(s->registers[0x10]&0x20)for(unsigned j=0;j<16;j++)s->wave[other+j]=(uint8_t)((s->wave[other+j]<<4)|
            (j==15?nibble:s->wave[other+j+1]>>4));
        c->sample=(uint8_t)nibble;c->phase=(uint8_t)((c->phase+1)&63);
    }else{
        unsigned carry=s->lfsr&1;s->lfsr>>=1;
        if(carry)s->lfsr^=(s->registers[0x1C]&8)?0x60:0x6000;c->sample=(uint8_t)carry;
    }
    c->remaining=oscillator_period(s,i);
}
uint32_t hn_psg_next(const HNPsg *s)
{
    uint32_t next=s->frame_remaining;
    if(s->enabled)for(unsigned i=0;i<4;i++)if(clocked(s,i)&&s->channel[i].remaining<next)next=s->channel[i].remaining;
    return next;
}
void hn_psg_advance(HNPsg *s,uint32_t cycles)
{
    while(cycles){
        uint32_t next=hn_psg_next(s),step=next<cycles?next:cycles;
        s->cycles+=step;cycles-=step;s->frame_remaining-=step;
        if(s->enabled)for(unsigned i=0;i<4;i++)if(clocked(s,i))s->channel[i].remaining-=step;
        if(!s->frame_remaining){s->frame_remaining=32768;if(s->enabled)frame(s);}
        if(s->enabled)for(unsigned i=0;i<4;i++)if(clocked(s,i)&&!s->channel[i].remaining)oscillator(s,i);
    }
}
unsigned hn_psg_sample(const HNPsg *s,unsigned i)
{
    if(i>=4)return 0;
    const HNPsgChannel *c=&s->channel[i];if(!s->enabled||!c->active)return 0;
    if(i<2){
        if(c->first)return 0;
        unsigned duty=s->registers[i?8:2]>>6,phase=c->phase;
        bool high=duty==0?phase==7:duty==1?(phase==0||phase==7):duty==2?(phase==0||phase>=5):(phase>0&&phase<7);
        return high?c->volume:0;
    }
    if(i==3)return c->sample?c->volume:0;
    unsigned volume=s->registers[0x13]>>5;
    if(volume&4)return c->sample*3/4;
    return volume?c->sample>>(volume-1):0;
}
void hn_psg_levels(const HNPsg *s,int stereo[2])
{
    stereo[0]=stereo[1]=0;if(!s->enabled)return;
    unsigned route=s->registers[0x21],volume=s->registers[0x20];
    for(unsigned i=0;i<4;i++){
        /* GBA's digital DACs remain connected at digital zero when a generator
           stops. Center before adding FIFO PCM; channel 3 has opposite polarity. */
        int sample=(int)hn_psg_sample(s,i)*2-15;if(i==2)sample=-sample;
        if(route&(16u<<i))stereo[0]+=sample;
        if(route&(1u<<i))stereo[1]+=sample;
    }
    stereo[0]*=4*(1+((volume>>4)&7));stereo[1]*=4*(1+(volume&7));
}
