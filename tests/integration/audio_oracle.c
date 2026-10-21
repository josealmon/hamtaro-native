/* Optional hardware-only oracle. No ROM is loaded and no CPU instruction runs.
   Built separately against the caller's mGBA 0.10.5 source checkout. */
#include <mgba/flags.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>
#include <mgba/internal/gba/timer.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/core/timing.h>
#include <mgba/core/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
static void log_message(struct mLogger *logger,int category,enum mLogLevel level,const char *format,va_list args)
{
    (void)logger;(void)category;
    if(level&(mLOG_FATAL|mLOG_ERROR|mLOG_WARN|mLOG_GAME_ERROR)){vfprintf(stderr,format,args);fputc('\n',stderr);}
}
static void init(struct GBA *g,struct ARMCore *cpu)
{
    memset(g,0,sizeof(*g));memset(cpu,0,sizeof(*cpu));g->cpu=cpu;cpu->master=&g->d;
    mTimingInit(&g->timing,&cpu->cycles,&cpu->nextEvent);GBAMemoryInit(g);GBADMAReset(g);
    g->audio.p=g;GBAAudioInit(&g->audio,4096);GBAAudioReset(&g->audio);GBAIOInit(g);GBATimerInit(g);
    mTimingTick(&g->timing,0);
}
static void destroy(struct GBA *g)
{GBAAudioDeinit(&g->audio);GBAMemoryDeinit(g);mTimingDeinit(&g->timing);}
static void advance(struct GBA *g,uint32_t cycles)
{
    /* Visit every scheduled edge exactly; never call ARMRun/ARMStep. */
    while(cycles){
        int32_t next=mTimingNextEvent(&g->timing);
        if(next<=0){mTimingTick(&g->timing,0);continue;}
        uint32_t step=cycles<(uint32_t)next?cycles:(uint32_t)next;
        mTimingTick(&g->timing,(int32_t)step);cycles-=step;
    }
}
static void snapshot(struct GBA *g,uint32_t state[47])
{
    struct GBAAudio *a=&g->audio;state[0]=(uint32_t)mTimingCurrentTime(&g->timing);
    for(unsigned i=0;i<4;i++){
        /* GBAIORead subtracts two CPU bus cycles. There is no CPU transaction here. */
        GBATimerUpdateRegister(g,(int)i,0);
        state[1+i]=g->memory.io[(0x100+i*4)/2];state[5+i]=g->timers[i].reload;
        state[9+i]=GBAIORead(g,0x102+i*4);
    }
    state[13]=GBAIORead(g,REG_IF);state[14]=GBAIORead(g,REG_SOUNDCNT_HI);
    state[15]=GBAIORead(g,REG_SOUNDCNT_X);state[16]=GBAIORead(g,REG_SOUNDBIAS);
    for(unsigned i=0;i<2;i++){
        struct GBAAudioFIFO *f=i?&a->chB:&a->chA;unsigned o=17+i*4;
        state[o]=(uint32_t)f->fifoRead;state[o+1]=(uint32_t)f->fifoWrite;
        state[o+2]=(uint32_t)f->internalRemaining;state[o+3]=f->internalSample;
        for(unsigned j=0;j<8;j++)state[25+i*8+j]=f->fifo[j];
        state[41+i]=g->memory.dma[i+1].nextSource;
        state[43+i]=(uint32_t)(int32_t)f->samples[(2<<GBARegisterSOUNDBIASGetResolution(a->soundbias))-1];
    }
    /* Flush raw DAC samples before mGBA's desktop resampler/gain. */
    GBAAudioSample(a,mTimingCurrentTime(&g->timing));
    unsigned count=2<<GBARegisterSOUNDBIASGetResolution(a->soundbias);
    unsigned last=a->sampleIndex?(unsigned)a->sampleIndex-1:count-1;
    int levels[2]={a->currentSamples[last].left/48,a->currentSamples[last].right/48};
    int bias=a->soundbias&0x3FE,mask=(1<<(1+GBARegisterSOUNDBIASGetResolution(a->soundbias)))-1;
    for(unsigned i=0;i<2;i++){
        int v=((levels[i]+bias)&~mask)-bias;if(v<-512)v=-512;if(v>511)v=511;
        state[45+i]=(uint32_t)v;
    }
}
int main(void)
{
    struct mLogger logger={log_message,NULL};mLogSetDefaultLogger(&logger);
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
    struct GBA *g=calloc(1,sizeof(*g));struct ARMCore *cpu=calloc(1,sizeof(*cpu));
    if(!g||!cpu)return 2;init(g,cpu);
    uint32_t command[4];
    for(;;){
        size_t bytes=fread(command,1,sizeof(command),stdin);
        if(!bytes&&feof(stdin))break;if(bytes!=sizeof(command))return 3;
        unsigned op=command[0],address=command[1];uint32_t value=command[2];
        if(op==0){destroy(g);init(g,cpu);}
        else if(op==1)GBAIOWrite8(g,address,(uint8_t)value);
        else if(op==2)GBAIOWrite(g,address,(uint16_t)value);
        else if(op==4)GBAIOWrite32(g,address,value);
        else if(op==8){if(address>=0x40000||(address&3))return 4;memcpy((uint8_t *)g->memory.wram+address,&value,4);}
        else if(op==16)advance(g,value);
        else if(op==32){uint32_t state[47];snapshot(g,state);if(fwrite(state,sizeof(state),1,stdout)!=1||fflush(stdout))return 5;}
        else return 6;
    }
    destroy(g);free(cpu);free(g);return 0;
}
