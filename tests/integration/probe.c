#include "gba/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HNMemory *hn_probe_create(const uint8_t *,size_t);
void hn_probe_destroy(HNMemory *);
int hn_probe_read(HNMemory *,GameAddr,uint8_t *,size_t);
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"probe check failed at %d\n",__LINE__);exit(1);}}while(0)

int main(void)
{
    const uint8_t rom[]={0x12,0x34,0x56,0x78};
    HNMemory *m=hn_probe_create(rom,sizeof(rom));CHECK(m!=NULL);
    HNMemory *reference=malloc(sizeof(*reference));CHECK(reference!=NULL);
    uint8_t *actual=malloc(0x40010),*expected=malloc(0x40010);CHECK(actual&&expected);
    for(size_t i=0;i<sizeof(m->ewram);i++)m->ewram[i]=(uint8_t)(i*37);
    memset(m->iwram,0x91,sizeof(m->iwram));memset(m->palette,0x52,sizeof(m->palette));
    memset(m->vram,0x73,sizeof(m->vram));memset(m->oam,0x34,sizeof(m->oam));
    memset(m->sram,0x15,sizeof(m->sram));
    const struct{GameAddr address;size_t count;} cases[]={
        {0x02000000,0x40000},{0x0203FFF0,16},{0x0203FFF0,32},
        {0x03000000,0x8000},{0x03007FFF,2},{0x05000000,0x400},
        {0x050003FF,2},{0x06000000,0x18000},{0x06017FFF,2},
        {0x07000000,0x400},{0x070003FF,2},{0x0E000000,0x8000},
        {0x0E007FFF,2},{0x0F000000,0x8000},{0x04000130,4},
        {0x08000000,4},{0x08000003,4},
        {0x01000000,4},{0xFFFFFFFF,2},{0x02000000,0}
    };
    for(unsigned fault=0;fault<2;fault++)for(size_t c=0;c<sizeof(cases)/sizeof(cases[0]);c++){
        m->fault=fault!=0;m->fault_address=0x12345678;*reference=*m;
        memset(actual,0xCC,0x40010);memset(expected,0xCC,0x40010);
        for(size_t i=0;i<cases[c].count&&!reference->fault;i++)
            expected[i]=hn_read8(reference,cases[c].address+(GameAddr)i);
        CHECK(hn_probe_read(m,cases[c].address,actual,cases[c].count)==!reference->fault);
        CHECK(memcmp(actual,expected,0x40010)==0);
        CHECK(m->fault==reference->fault&&m->fault_address==reference->fault_address);
    }
    free(actual);free(expected);free(reference);hn_probe_destroy(m);
    printf("%u snapshot/bus equivalence checks passed.\n",checks);return 0;
}
