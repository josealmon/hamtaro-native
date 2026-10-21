#include "gba/timing.h"
#include <string.h>
static uint16_t reg(const HNMemory *m,unsigned o)
{ return (uint16_t)(m->io[o]|m->io[o+1]<<8); }
static void set(HNMemory *m,unsigned o,uint16_t value)
{ m->io[o]=(uint8_t)value;m->io[o+1]=(uint8_t)(value>>8); }
static void notify(HNClock *clock,HNMemory *m)
{
    uint16_t pending=(uint16_t)(reg(m,0x200)&reg(m,0x202));
    if((reg(m,0x208)&1)&&pending&&clock->interrupt) clock->interrupt(clock->context,pending);
}
void hn_clock_init(HNClock *clock,HNMemory *m,uint32_t *rgba,HNInterrupt callback,void *context)
{
    memset(clock,0,sizeof(*clock));clock->interrupt=callback;clock->context=context;
    clock->framebuffer=rgba;
    uint16_t status=(uint16_t)(reg(m,4)&~7u);
    if((status>>8)==0)status|=4;
    set(m,4,status);set(m,6,0);
}
void hn_clock_advance(HNClock *clock,HNMemory *m,uint32_t cycles)
{
    while(cycles && !m->fault) {
        unsigned next=clock->line_cycle<HN_HDRAW_CYCLES?HN_HDRAW_CYCLES:HN_CYCLES_PER_LINE;
        unsigned step=next-clock->line_cycle;
        if(step>cycles)step=cycles;
        clock->line_cycle+=step;clock->cycles+=step;cycles-=step;
        if(clock->line_cycle!=next)continue;
        uint16_t status=reg(m,4),irq=0;
        if(next==HN_HDRAW_CYCLES) {
            status|=2;set(m,4,status);
            if(clock->line<HN_HEIGHT) {
                if(clock->framebuffer)hn_video_render_line(m,clock->line,clock->framebuffer+clock->line*HN_WIDTH);
                hn_dma_trigger(m,2);
            }
            if(status&16)irq|=2;
        }else{
            clock->line_cycle=0;clock->line++;
            if(clock->line==HN_TOTAL_LINES){clock->line=0;clock->frames++;}
            set(m,6,(uint16_t)clock->line);status&=~6u;
            if(clock->line==(unsigned)(status>>8)) {
                status|=4;if(status&32)irq|=4;
            }
            if(clock->line==HN_HEIGHT) {
                status|=1;hn_dma_trigger(m,1);if(status&8)irq|=1;
            }
            if(clock->line==HN_TOTAL_LINES-1)status&=~1u;
            set(m,4,status);
        }
        set(m,0x202,(uint16_t)(reg(m,0x202)|irq));
        notify(clock,m);
    }
}
