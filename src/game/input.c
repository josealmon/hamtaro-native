#include "game/input.h"
#include <string.h>
/* Adapted from AH3P's ReadKeys contract; the host supplies button bits. */
void hn_keys_init(HNKeys *k)
{
    memset(k,0,sizeof(*k)); k->first_delay=90; k->next_delay=4;
}
void hn_keys_step(HNKeys *k,uint16_t held)
{
    held &= 0x3FF;
    k->pressed=(uint16_t)(held & (held^k->previous));
    k->released=(uint16_t)(k->previous & (held^k->previous));
    k->repeat=k->pressed;
    if(k->pressed && k->pressed==held) k->counter=k->first_delay;
    k->counter--;
    if(!k->counter) { k->repeat=held; k->counter=k->next_delay; }
    if(held==k->previous && k->held_frames!=UINT16_MAX) k->held_frames++;
    else k->held_frames=0;
    k->previous=held; k->held=held;
}
