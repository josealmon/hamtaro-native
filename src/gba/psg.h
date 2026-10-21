#ifndef HN_PSG_H
#define HN_PSG_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t remaining;
    uint16_t frequency,length;
    uint8_t phase,volume,envelope_clock,sample;
    bool active,started,first,length_enabled,envelope_running;
} HNPsgChannel;
typedef struct {
    HNPsgChannel channel[4];
    uint8_t registers[0x22],wave[32];
    uint32_t frame_remaining;
    uint64_t cycles;
    uint16_t sweep_shadow,lfsr;
    uint8_t frame_step,sweep_clock;
    bool enabled,sweep_enabled,sweep_subtracted;
} HNPsg;
void hn_psg_init(HNPsg *);
void hn_psg_power(HNPsg *,bool enabled);
uint8_t hn_psg_read(const HNPsg *,unsigned offset);
void hn_psg_write(HNPsg *,unsigned offset,uint8_t value);
uint8_t hn_psg_status(const HNPsg *);
uint32_t hn_psg_next(const HNPsg *);
void hn_psg_advance(HNPsg *,uint32_t cycles);
/* Raw digital channel output, before routing and master volume. */
unsigned hn_psg_sample(const HNPsg *,unsigned channel);
/* Centered GBA DAC inputs, including opposite wave polarity; before SOUNDCNT_H. */
void hn_psg_levels(const HNPsg *,int stereo[2]);
#ifdef __cplusplus
}
#endif
#endif
