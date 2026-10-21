#ifndef HN_TIMING_H
#define HN_TIMING_H
#include "gba/video.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { HN_CLOCK_HZ=16777216, HN_CYCLES_PER_LINE=1232, HN_HDRAW_CYCLES=1008,
       HN_TOTAL_LINES=228, HN_CYCLES_PER_FRAME=280896 };
typedef void (*HNInterrupt)(void *context, uint16_t pending);
typedef struct {
    uint64_t cycles, frames;
    unsigned line, line_cycle;
    HNInterrupt interrupt;
    void *context;
    uint32_t *framebuffer;
} HNClock;
void hn_clock_init(HNClock *, HNMemory *, uint32_t *rgba, HNInterrupt, void *context);
/* Advances native compatibility events, without interpreting CPU instructions. */
void hn_clock_advance(HNClock *, HNMemory *, uint32_t cycles);
#ifdef __cplusplus
}
#endif
#endif
