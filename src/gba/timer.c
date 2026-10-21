#include "gba/timer.h"
#include "gba/audio.h"
#include <string.h>

static unsigned divisor(uint16_t control)
{ return 1u << ((control&3)==0?0:(control&3)==1?6:(control&3)==2?8:10); }
static bool running(const HNTimers *t,unsigned i)
{ return (t->channel[i].control&0x80)!=0; }
static bool cascade(const HNTimers *t,unsigned i)
{ return i && (t->channel[i].control&4); }
void hn_timers_init(HNTimers *t,HNMemory *m)
{
    memset(t,0,sizeof(*t));t->memory=m;m->timers=t;
    memset(m->io+0x100,0,16);
}
uint8_t hn_timers_read(const HNTimers *t,unsigned offset)
{
    unsigned index=(offset-0x100)/4,part=(offset-0x100)%4;
    uint16_t value=part<2?t->channel[index].count:t->channel[index].control;
    return (uint8_t)(value>>((part&1)*8));
}
void hn_timers_write(HNTimers *t,unsigned offset,uint16_t value,unsigned width)
{
    unsigned index=(offset-0x100)/4,part=(offset-0x100)%4;
    uint16_t *target=part<2?&t->channel[index].reload:&t->channel[index].control;
    uint16_t old=*target;
    if(width==1){unsigned shift=(part&1)*8;value=(uint16_t)((old&~(0xFFu<<shift))|((value&255)<<shift));}
    if(part<2){*target=value;return;}
    *target=value&(index?0xC7u:0xC3u);
    if(!((old^*target)&0x80)||!(*target&0x80))return;
    t->channel[index].count=t->channel[index].reload;
}
uint32_t hn_timers_next(const HNTimers *t)
{
    uint32_t next=UINT32_MAX;
    for(unsigned i=0;i<4;i++)if(running(t,i)&&!cascade(t,i)){
        unsigned div=divisor(t->channel[i].control);
        uint32_t delay=(0x10000u-t->channel[i].count)*div-(unsigned)(t->cycles&(div-1));
        if(delay<next)next=delay;
    }
    return next;
}
static void overflow(HNTimers *t,unsigned i)
{
    HNMemory *m=t->memory;t->channel[i].count=t->channel[i].reload;
    if(t->channel[i].control&0x40)m->io[0x202]|=(uint8_t)(8u<<i);
    if(m->apu&&i<2)hn_apu_timer(m->apu,i);
    if(i<3&&running(t,i+1)&&cascade(t,i+1)&&++t->channel[i+1].count==0)overflow(t,i+1);
}
void hn_timers_advance(HNTimers *t,uint32_t cycles)
{
    while(cycles&&!t->memory->fault){
        uint32_t next=hn_timers_next(t),step=next<cycles?next:cycles;
        uint64_t before=t->cycles;t->cycles+=step;cycles-=step;
        bool wrapped[4]={false,false,false,false};
        for(unsigned i=0;i<4;i++)if(running(t,i)&&!cascade(t,i)){
            unsigned div=divisor(t->channel[i].control);
            uint32_t count=t->channel[i].count+(uint32_t)(t->cycles/div-before/div);
            wrapped[i]=count==0x10000;t->channel[i].count=(uint16_t)count;
        }
        /* Capture simultaneous overflows before callbacks can reconfigure timers. */
        for(unsigned i=0;i<4;i++)if(wrapped[i])overflow(t,i);
    }
}
