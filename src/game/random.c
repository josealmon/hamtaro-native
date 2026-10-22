#include "game/random.h"
/* AH3P's three-component Wichmann-Hill calculation. Integer wrap is intentional. */
void hn_random_init(HNRandom *r,uint8_t scanline)
{ r->seed[0]=0x3AC2; r->seed[1]=0x4DF3; r->seed[2]=(uint32_t)scanline+1; }
uint32_t hn_random_next(HNRandom *r)
{
    r->seed[0]=(uint32_t)((int32_t)(r->seed[0]*171u)%30269);
    r->seed[1]=(uint32_t)((int32_t)(r->seed[1]*172u)%30307);
    r->seed[2]=(uint32_t)((int32_t)(r->seed[2]*176u)%30323);
    return r->seed[0]*70946u+r->seed[1]*70857u+r->seed[2]*70820u;
}
uint32_t hn_random_memory_next(HNMemory *m)
{
    HNRandom r;for(unsigned i=0;i<3;i++)r.seed[i]=hn_read32(m,0x03000000+i*4);
    uint32_t value=hn_random_next(&r);for(unsigned i=0;i<3;i++)hn_write32(m,0x03000000+i*4,r.seed[i]);
    return value;
}
uint16_t hn_random_range(HNMemory *m,uint16_t lower,uint16_t upper)
{
    uint32_t value=(hn_random_memory_next(m)<<1)>>16;
    if(upper>lower)value=(((upper-lower)*value)>>16)+lower;
    return (uint16_t)value;
}
