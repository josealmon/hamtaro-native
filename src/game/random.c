#include "game/random.h"
/* AH3P's three-component Wichmann-Hill calculation. Integer wrap is intentional. */
void hn_random_init(HNRandom *r,uint8_t scanline)
{ r->seed[0]=0x3AC2; r->seed[1]=0x4DF3; r->seed[2]=(uint32_t)scanline+1; }
uint32_t hn_random_next(HNRandom *r)
{
    r->seed[0]=(r->seed[0]*171u)%30269u;
    r->seed[1]=(r->seed[1]*172u)%30307u;
    r->seed[2]=(r->seed[2]*176u)%30323u;
    return r->seed[0]*70946u+r->seed[1]*70857u+r->seed[2]*70820u;
}
