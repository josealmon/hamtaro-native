#include "game/input.h"
#include "game/state.h"
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
void hn_input_repeat(HNMemory *m,uint16_t first,uint16_t next)
{
    hn_write16(m,HN_KEY_STATE+12,first);hn_write16(m,HN_KEY_STATE+14,next);hn_write16(m,HN_KEY_STATE+16,0);
}
void hn_input_initialize(HNMemory *m)
{
    hn_input_repeat(m,90,4);hn_write16(m,HN_KEY_STATE+18,0);
    hn_write8(m,HN_KEY_STATE+20,0);hn_write8(m,HN_KEY_STATE+21,0);hn_write16(m,HN_PREVIOUS_KEYS,0);
}
bool hn_input_update(HNMemory *m,uint16_t buttons)
{
    HNKeys keys={0};keys.previous=hn_read16(m,HN_PREVIOUS_KEYS);
    keys.first_delay=hn_read16(m,HN_KEY_STATE+12);keys.next_delay=hn_read16(m,HN_KEY_STATE+14);
    keys.counter=hn_read16(m,HN_KEY_STATE+16);keys.held_frames=hn_read16(m,HN_KEY_STATE+18);
    hn_keys_step(&keys,buttons);
    /* The host supplies the same active-low register image as physical buttons. */
    m->io[0x130]=(uint8_t)~keys.held;m->io[0x131]=(uint8_t)((~keys.held>>8)&3);
    hn_write16(m,HN_KEY_STATE,keys.held);hn_write16(m,HN_KEY_STATE+2,keys.pressed);
    hn_write16(m,HN_KEY_STATE+4,keys.repeat);hn_write16(m,HN_KEY_STATE+6,keys.released);
    hn_write16(m,HN_KEY_STATE+16,keys.counter);hn_write16(m,HN_KEY_STATE+18,keys.held_frames);
    hn_write16(m,HN_PREVIOUS_KEYS,keys.previous);
    unsigned direction=(keys.held&0xF0)>>4;
    if(direction)hn_write16(m,HN_KEY_STATE+8,(uint16_t)((hn_read8(m,HN_KEY_DIRECTION_HIGH+direction)<<8)|
        hn_read8(m,HN_KEY_DIRECTION_LOW+direction)));
    return !m->fault;
}
uint16_t hn_input_query(HNMemory *m,uint16_t mask,uint8_t mode)
{return mode<4?(uint16_t)(hn_read16(m,HN_KEY_STATE+mode*2)&mask):0;}
uint16_t hn_input_script_mask(HNMemory *m,uint8_t mode)
{
    uint16_t keys=hn_input_query(m,hn_read16(m,0x03001F64),mode);
    hn_write16(m,0x0201DC40,keys);
    hn_write16(m,0x03002180,(uint16_t)((keys<<6)|(hn_read16(m,0x03002180)&63)));return keys;
}
