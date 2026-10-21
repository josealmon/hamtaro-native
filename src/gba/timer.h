#ifndef HN_TIMER_H
#define HN_TIMER_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct HNTimers {
    HNMemory *memory;
    uint64_t cycles;
    struct { uint16_t reload, count, control; } channel[4];
} HNTimers;
void hn_timers_init(HNTimers *, HNMemory *);
uint8_t hn_timers_read(const HNTimers *, unsigned offset);
void hn_timers_write(HNTimers *, unsigned offset, uint16_t value, unsigned width);
uint32_t hn_timers_next(const HNTimers *);
/* May advance any number of cycles; cascades, audio requests and IF are ordered. */
void hn_timers_advance(HNTimers *, uint32_t cycles);
#ifdef __cplusplus
}
#endif
#endif
