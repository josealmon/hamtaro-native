#ifndef HN_GAME_RANDOM_H
#define HN_GAME_RANDOM_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t seed[3]; } HNRandom;
void hn_random_init(HNRandom *, uint8_t scanline);
uint32_t hn_random_next(HNRandom *);
uint32_t hn_random_memory_next(HNMemory *);
uint16_t hn_random_range(HNMemory *,uint16_t lower,uint16_t upper);
#ifdef __cplusplus
}
#endif
#endif
