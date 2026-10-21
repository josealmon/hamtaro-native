#ifndef HN_GAME_RANDOM_H
#define HN_GAME_RANDOM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t seed[3]; } HNRandom;
void hn_random_init(HNRandom *, uint8_t scanline);
uint32_t hn_random_next(HNRandom *);
#ifdef __cplusplus
}
#endif
#endif
