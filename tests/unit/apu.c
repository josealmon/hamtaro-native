#include "gba/audio.h"
#include "gba/timing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static HNMemory memory;
static HNClock clock_state;
static HNApu apu;
static unsigned checks,output_count;
static int16_t output[4096][2];
#define CHECK(c) do{checks++;if(!(c)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c);exit(1);}}while(0)
static void receive(void *context,int16_t left,int16_t right)
{(void)context;if(output_count<4096){output[output_count][0]=left;output[output_count][1]=right;}output_count++;}
static void reset(void)
{
    hn_memory_init(&memory,NULL,0);hn_clock_init(&clock_state,&memory,NULL,NULL,NULL);
    hn_apu_init(&apu,&memory,receive,NULL);output_count=0;memset(output,0,sizeof(output));
}
static void advance(unsigned cycles){hn_clock_advance(&clock_state,&memory,cycles);CHECK(!memory.fault);}
static uint16_t read_timer(unsigned i){return hn_read16(&memory,0x04000100+i*4);}
static void timer(unsigned i,uint16_t reload,uint16_t control)
{hn_write32(&memory,0x04000100+i*4,reload|((uint32_t)control<<16));}
static void timer_irq(void *context,uint16_t pending)
{
    unsigned *count=context;if(pending&8){(*count)++;hn_write16(&memory,0x04000202,8);
        if(*count==3)hn_write16(&memory,0x04000102,0);}
}
static void timers(void)
{
    reset();hn_write16(&memory,0x04000100,0xFFFC);CHECK(read_timer(0)==0);
    hn_write16(&memory,0x04000102,0xFFC7);CHECK(hn_read16(&memory,0x04000102)==0xC3);
    CHECK(read_timer(0)==0xFFFC);advance(4095);CHECK(read_timer(0)==0xFFFF);
    advance(1);CHECK(read_timer(0)==0xFFFC&&hn_read16(&memory,0x04000202)==8);
    hn_write16(&memory,0x04000102,0);advance(37);CHECK(read_timer(0)==0xFFFC);
    /* Prescalers use the shared clock phase, including enable between edges. */
    for(unsigned i=0;i<4;i++){
        static const unsigned div[4]={1,64,256,1024};
        reset();advance(17);timer(0,0xFFFE,(uint16_t)(0xC0|i));
        unsigned next=2*div[i]-(17&(div[i]-1));advance(next-1);
        CHECK(read_timer(0)==0xFFFF);advance(1);CHECK(read_timer(0)==0xFFFE);
        CHECK(hn_read16(&memory,0x04000202)==8);
    }
    reset();timer(0,0xFFFE,0xC0);timer(1,0xFFFF,0xC7);timer(2,0xFFFE,0xC4);timer(3,0xFFFF,0xC4);
    advance(4);CHECK(read_timer(0)==0xFFFE&&read_timer(1)==0xFFFF&&read_timer(2)==0xFFFE);
    CHECK(read_timer(3)==0xFFFF&&hn_read16(&memory,0x04000202)==0x78);
    /* Reload writes leave a running/frozen counter intact until overflow/start. */
    hn_write16(&memory,0x04000100,0xFFF8);advance(1);CHECK(read_timer(0)==0xFFFF);
    advance(1);CHECK(read_timer(0)==0xFFF8);hn_write8(&memory,0x04000103,0xFF);
    CHECK(hn_read16(&memory,0x04000102)==0xC0);hn_write8(&memory,0x04000102,0);
    hn_write8(&memory,0x04000100,0x34);hn_write8(&memory,0x04000101,0x12);
    CHECK(read_timer(0)==0xFFF8);hn_write8(&memory,0x04000102,0x80);CHECK(read_timer(0)==0x1234);
    reset();unsigned count=0;clock_state.interrupt=timer_irq;clock_state.context=&count;
    hn_write16(&memory,0x04000200,8);hn_write16(&memory,0x04000208,1);timer(0,0xFFFD,0xC0);
    advance(20);CHECK(count==3&&read_timer(0)==0xFFFD&&hn_read16(&memory,0x04000202)==0);
}
static void fifo(void)
{
    reset();hn_write16(&memory,0x04000084,0x8F);CHECK(hn_read16(&memory,0x04000084)==0x80);
    hn_write16(&memory,0x04000082,0x290C); /* A right, B left, full volume. */
    hn_write32(&memory,0x040000A0,0x807F01FF);hn_write32(&memory,0x040000A4,0x40302010);
    timer(0,0xFFF0,0x80);advance(16);
    CHECK(apu.fifo[0].sample==-1&&apu.fifo[1].sample==16&&apu.fifo[0].remaining==3);
    int16_t level[2];hn_apu_levels(&apu,level);CHECK(level[0]==64&&level[1]==-4);
    advance(16);CHECK(apu.fifo[0].sample==1&&apu.fifo[1].sample==32);
    advance(16);CHECK(apu.fifo[0].sample==127);advance(16);CHECK(apu.fifo[0].sample==-128);
    advance(16);CHECK(apu.fifo[0].sample==0&&apu.fifo[1].sample==0);
    CHECK(hn_read32(&memory,0x040000A0)==0);
    /* An 8/16-bit FIFO store modifies one word lane, then queues a full word. */
    reset();hn_write32(&memory,0x040000A0,0x11223344);apu.fifo[0].write=0;
    hn_write8(&memory,0x040000A1,0xAA);CHECK(apu.fifo[0].words[0]==0x1122AA44&&apu.fifo[0].write==1);
    apu.fifo[0].write=0;hn_write16(&memory,0x040000A2,0xBEEF);
    CHECK(apu.fifo[0].words[0]==0xBEEFAA44&&apu.fifo[0].write==1);
    hn_write16(&memory,0x04000084,0x80);hn_write16(&memory,0x04000082,0x0704);
    timer(0,0xFFFF,0x80);timer(1,0xFFFE,0x80);advance(1);CHECK(apu.fifo[0].remaining==0);
    advance(1);CHECK(apu.fifo[0].sample==0x44);
    hn_write16(&memory,0x04000082,0x0F04);CHECK(apu.fifo[0].read==0&&apu.fifo[0].write==0);
    CHECK(apu.fifo[0].remaining==3);CHECK(hn_read16(&memory,0x04000082)==0x0704);
    hn_write16(&memory,0x04000084,0);advance(8);CHECK(apu.fifo[0].remaining==3);
    hn_write16(&memory,0x04000060,0x77);hn_write16(&memory,0x04000084,0);
    CHECK(hn_read16(&memory,0x04000060)==0&&hn_read16(&memory,0x04000082)==0x0700);
}
static void dma(void)
{
    reset();for(unsigned i=0;i<128;i++)hn_write8(&memory,0x02000000+i,(uint8_t)(i+1));
    hn_write16(&memory,0x04000084,0x80);hn_write16(&memory,0x04000082,0x0104);
    hn_write32(&memory,0x040000BC,0x02000000);hn_write32(&memory,0x040000C0,0x040000A0);
    /* FIFO requests override width, destination increment and count. */
    hn_write32(&memory,0x040000C4,0xF2000100);timer(0,0xFFC0,0x80);advance(64);
    CHECK(apu.fifo[0].sample==0&&apu.fifo[0].write==4&&memory.dma_source[1]==0x02000010);
    CHECK(memory.dma_destination[1]==0x040000A0);CHECK(hn_read16(&memory,0x04000202)==0x200);
    advance(64);CHECK(apu.fifo[0].sample==1&&apu.fifo[0].remaining==3);
    CHECK(memory.dma_source[1]==0x02000010);advance(64);
    CHECK(apu.fifo[0].sample==2&&memory.dma_source[1]==0x02000020);
    CHECK(hn_read32(&memory,0x040000BC)==0x02000000);advance(64*23);
    CHECK(apu.fifo[0].sample==25&&memory.dma_source[1]==0x02000030);
    /* Non-repeat DMA disables after its first four-word refill. */
    reset();hn_write32(&memory,0x040000BC,0x0200000C);hn_write32(&memory,0x040000C0,0x040000A4);
    hn_write32(&memory,0x040000C4,0xB080FFFF);hn_dma_sound(&memory,1);
    CHECK(memory.dma_source[1]==0x01FFFFFC&&!(hn_read16(&memory,0x040000C6)&0x8000));
    CHECK(apu.fifo[1].write==4&&!memory.fault);
    /* Unrelated timing/destination/channel settings do not fill a sound FIFO. */
    reset();hn_write32(&memory,0x040000B0,0x02000000);hn_write32(&memory,0x040000B4,0x040000A0);
    hn_write32(&memory,0x040000B8,0xB2000004);hn_dma_sound(&memory,0);CHECK(apu.fifo[0].write==0);
}
static void output_contract(void)
{
    reset();hn_write16(&memory,0x04000084,0x80);hn_write16(&memory,0x04000082,0x0304);
    hn_write32(&memory,0x040000A0,0x01010101);hn_apu_timer(&apu,0);
    advance(512);CHECK(output_count==1&&output[0][0]==0&&apu.dac[0]==256);
    advance(512);CHECK(output_count==2&&output[1][0]==256&&output[1][1]==256);
    /* Both PCM inputs clip together after bias, before PWM quantization. */
    apu.fifo[0].sample=127;apu.fifo[1].sample=127;hn_write16(&memory,0x04000082,0x330C);
    int16_t level[2];hn_apu_levels(&apu,level);CHECK(level[0]==511&&level[1]==511);
    advance(512);CHECK(apu.dac[0]==32640);apu.fifo[0].sample=-128;apu.fifo[1].sample=-128;
    advance(512);CHECK(apu.dac[0]==-32768);hn_write16(&memory,0x04000088,0xFFFF);
    CHECK(hn_read16(&memory,0x04000088)==0xC3FE);advance(512);CHECK(apu.dac[0]==-32768);
    /* Resolution changes retain a 32768 Hz host cadence; averaging is partition-invariant. */
    for(unsigned resolution=0;resolution<4;resolution++){
        reset();hn_write16(&memory,0x04000084,0x80);hn_write16(&memory,0x04000082,0x0304);
        hn_write16(&memory,0x04000088,(uint16_t)(0x200|(resolution<<14)));
        apu.fifo[0].sample=7;advance(2048);CHECK(output_count==4&&apu.cycles==2048);
        unsigned mask=(1u<<(resolution+1))-1;
        CHECK(output[3][0]==(int16_t)((28&~mask)*64));
    }
    reset();advance(HN_CYCLES_PER_FRAME);CHECK(output_count==548&&apu.samples==548);
    CHECK(clock_state.frames==1&&clock_state.line==0);
}
static void pattern(void)
{
    reset();for(unsigned i=0;i<4096;i++)hn_write8(&memory,0x02000000+i,(uint8_t)(i*37+i/23));
    hn_write16(&memory,0x04000084,0x80);hn_write16(&memory,0x04000082,0xA90E);
    hn_write16(&memory,0x04000088,0x4200);
    for(unsigned i=0;i<2;i++){
        hn_write32(&memory,0x040000BC+i*12,0x02000000+i*2048);
        hn_write32(&memory,0x040000C0+i*12,0x040000A0+i*4);
        hn_write32(&memory,0x040000C4+i*12,0xB2000004);
    }
    timer(0,0xFEFF,0x80);
}
static void partition_contract(void)
{
    pattern();advance(HN_CYCLES_PER_FRAME);int16_t expected[548][2];memcpy(expected,output,sizeof(expected));
    HNAudioFifo fifo[2];memcpy(fifo,apu.fifo,sizeof(fifo));unsigned timer_count=read_timer(0);
    pattern();unsigned remaining=HN_CYCLES_PER_FRAME,step=31;
    while(remaining){step=(step*17+29)%1900+1;if(step>remaining)step=remaining;advance(step);remaining-=step;}
    CHECK(output_count==548&&memcmp(expected,output,sizeof(expected))==0);
    CHECK(memcmp(fifo,apu.fifo,sizeof(fifo))==0&&read_timer(0)==timer_count);
    CHECK(clock_state.cycles==HN_CYCLES_PER_FRAME&&apu.cycles==HN_CYCLES_PER_FRAME);
}
int main(void)
{timers();fifo();dma();output_contract();partition_contract();printf("%u native timer/FIFO/PWM checks passed.\n",checks);return 0;}
