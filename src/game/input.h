#ifndef HN_GAME_INPUT_H
#define HN_GAME_INPUT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { HN_A=1, HN_B=2, HN_SELECT=4, HN_START=8, HN_RIGHT=16, HN_LEFT=32,
       HN_UP=64, HN_DOWN=128, HN_R=256, HN_L=512 };
typedef struct {
    uint16_t held, pressed, repeat, released, previous, held_frames;
    uint16_t first_delay, next_delay, counter;
} HNKeys;
void hn_keys_init(HNKeys *);
void hn_keys_step(HNKeys *, uint16_t buttons);
#ifdef __cplusplus
}
#endif
#endif
