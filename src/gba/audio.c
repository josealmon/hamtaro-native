#include "gba/audio.h"
#include <string.h>

static unsigned period(const HNApu *a){return 512u>>(a->bias>>14);}
void hn_apu_init(HNApu *a,HNMemory *m,HNApuSample callback,void *context)
{
    memset(a,0,sizeof(*a));a->memory=m;a->output=callback;a->context=context;
    a->bias=0x200;a->dac_remaining=512;a->host_remaining=512;m->apu=a;
    hn_psg_init(&a->psg);memset(m->io+0x60,0,0x22);
    m->io[0x82]=m->io[0x83]=m->io[0x84]=0;m->io[0x88]=0;m->io[0x89]=2;
}
bool hn_apu_register(unsigned o)
{return (o>=0x60&&o<=0x85)||(o>=0x88&&o<=0x89)||(o>=0x90&&o<=0xA7);}
uint8_t hn_apu_read(const HNApu *a,unsigned o)
{
    if(o>=0xA0)return 0; /* FIFO is write-only; native callers do not use open bus. */
    if(o<0x82||o>=0x90)return hn_psg_read(&a->psg,o);
    if(o==0x84)return (uint8_t)((a->enabled?0x80:0)|hn_psg_status(&a->psg));
    if(o==0x85)return 0;
    uint16_t value=o<0x88?a->control:a->bias;
    return (uint8_t)(value>>((o&1)*8));
}
void hn_apu_write(HNApu *a,unsigned o,uint32_t value,unsigned width)
{
    HNMemory *m=a->memory;
    if(o>=0xA0){
        HNAudioFifo *f=&a->fifo[(o-0xA0)/4];
        unsigned shift=(o&3)*8;
        uint32_t mask=width==4?UINT32_MAX:((1u<<(width*8))-1)<<shift;
        f->words[f->write]=(f->words[f->write]&~mask)|((value<<shift)&mask);
        f->write=(f->write+1)&7;return;
    }
    /* A word store can address two distinct halfword registers. */
    if(width==4){hn_apu_write(a,o,value&0xFFFF,2);if(hn_apu_register(o+2))hn_apu_write(a,o+2,value>>16,2);return;}
    if(o<0x82||o>=0x90){
        hn_psg_write(&a->psg,o,(uint8_t)value);
        if(width==2)hn_psg_write(&a->psg,o+1,(uint8_t)(value>>8));
        for(unsigned i=0x60;i<=0x81;i++)m->io[i]=hn_psg_read(&a->psg,i);
        m->io[0x84]=(uint8_t)((a->enabled?0x80:0)|hn_psg_status(&a->psg));return;
    }
    unsigned base=o&~1u;
    uint16_t v=(uint16_t)value;
    if(width==1){uint16_t old=base==0x82?a->control:base==0x88?a->bias:a->enabled?0x80:0;
        unsigned shift=(o&1)*8;v=(uint16_t)((old&~(255u<<shift))|((value&255)<<shift));}
    if(base==0x82){
        a->control=v&0x770F;
        for(unsigned i=0;i<2;i++)if(v&(0x0800u<<(i*4)))a->fifo[i].read=a->fifo[i].write=0;
    }else if(base==0x84){
        a->enabled=(v&0x80)!=0;
        hn_psg_power(&a->psg,a->enabled);
        if(!a->enabled){memset(m->io+0x60,0,0x22);a->control&=0xFF00;}
    }else if(base==0x88){
        unsigned old_period=period(a);a->bias=v&0xC3FE;
        if(period(a)!=old_period){
            /* PWM remains aligned to the global hardware clock. */
            a->dac_remaining=period(a)-(unsigned)(a->cycles&(period(a)-1));
        }
    }
    m->io[0x82]=(uint8_t)a->control;m->io[0x83]=(uint8_t)(a->control>>8);
    m->io[0x84]=(uint8_t)((a->enabled?0x80:0)|hn_psg_status(&a->psg));
    m->io[0x88]=(uint8_t)a->bias;m->io[0x89]=(uint8_t)(a->bias>>8);
}
void hn_apu_timer(HNApu *a,unsigned timer)
{
    if(!a->enabled)return;
    for(unsigned i=0;i<2;i++){
        unsigned shift=i*4;
        if(!((a->control>>(8+shift))&3)||((a->control>>(10+shift))&1u)!=timer)continue;
        HNAudioFifo *f=&a->fifo[i];unsigned queued=(f->write-f->read)&7;
        /* The request precedes moving the next word into the four-byte shifter. */
        bool refill=queued<=3;
        if(!f->remaining&&queued){f->shifter=f->words[f->read];f->read=(f->read+1)&7;f->remaining=4;}
        f->sample=(int8_t)(uint8_t)f->shifter;
        if(f->remaining){f->shifter>>=8;f->remaining--;}
        /* DMA completes after this overflow's sample has been latched. */
        if(refill)hn_dma_sound(a->memory,i);
    }
}
void hn_apu_levels(const HNApu *a,int16_t out[2])
{
    int level[2]={0,0};unsigned bias=a->bias&0x3FE;
    if(a->enabled){hn_psg_levels(&a->psg,level);unsigned shift=4-(a->control&3);level[0]>>=shift;level[1]>>=shift;}
    if(a->enabled)for(unsigned i=0;i<2;i++){
        int sample=a->fifo[i].sample*((a->control&(4u<<i))?4:2);
        if(a->control&(0x200u<<(i*4)))level[0]+=sample;
        if(a->control&(0x100u<<(i*4)))level[1]+=sample;
    }
    for(unsigned i=0;i<2;i++){
        int v=level[i]+(int)bias;if(v<0)v=0;if(v>1023)v=1023;out[i]=(int16_t)(v-(int)bias);
    }
}
uint32_t hn_apu_next(const HNApu *a)
{
    uint32_t next=a->dac_remaining<a->host_remaining?a->dac_remaining:a->host_remaining;
    uint32_t psg=hn_psg_next(&a->psg);return psg<next?psg:next;
}
void hn_apu_advance(HNApu *a,uint32_t cycles)
{
    while(cycles){
        unsigned next=hn_apu_next(a),step=next<cycles?next:cycles;
        for(unsigned i=0;i<2;i++)a->integral[i]+=a->dac[i]*(int)step;
        hn_psg_advance(&a->psg,step);
        a->memory->io[0x84]=(uint8_t)((a->enabled?0x80:0)|hn_psg_status(&a->psg));
        a->cycles+=step;a->dac_remaining-=step;a->host_remaining-=step;cycles-=step;
        if(!a->host_remaining){
            /* Time-average the held PWM levels into fixed 32768 Hz stereo. */
            int16_t left=(int16_t)(a->integral[0]/512),right=(int16_t)(a->integral[1]/512);
            a->samples++;a->integral[0]=a->integral[1]=0;a->host_remaining=512;
            if(a->output)a->output(a->context,left,right);
        }
        if(!a->dac_remaining){
            int16_t levels[2];hn_apu_levels(a,levels);
            unsigned bias=a->bias&0x3FE,mask=(1u<<(1+(a->bias>>14)))-1;
            for(unsigned i=0;i<2;i++){
                int value=(((levels[i]+(int)bias)&~(int)mask)-(int)bias)*64;
                if(value<-32768)value=-32768;if(value>32767)value=32767;a->dac[i]=(int16_t)value;
            }
            a->dac_remaining=period(a);
        }
    }
}
