#ifndef HN_GBA_AUDIO_H
#define HN_GBA_AUDIO_H
#include "gba/memory.h"
#include "gba/psg.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { HN_AUDIO_OUTPUT_HZ=32768 };
typedef void (*HNApuSample)(void *context,int16_t left,int16_t right);
typedef struct {
    uint32_t words[8],shifter;
    unsigned read,write,remaining;
    int8_t sample;
} HNAudioFifo;
typedef struct HNApu {
    HNMemory *memory;
    HNAudioFifo fifo[2];
    HNPsg psg;
    uint16_t control,bias;
    bool enabled;
    unsigned dac_remaining,host_remaining;
    int16_t dac[2];
    int32_t integral[2];
    uint64_t cycles,samples;
    HNApuSample output;
    void *context;
} HNApu;
/* Native PSG, Direct Sound and PWM hardware, independent of game-driver state. */
void hn_apu_init(HNApu *,HNMemory *,HNApuSample,void *context);
bool hn_apu_register(unsigned offset);
uint8_t hn_apu_read(const HNApu *,unsigned offset);
void hn_apu_write(HNApu *,unsigned offset,uint32_t value,unsigned width);
void hn_apu_timer(HNApu *,unsigned timer);
uint32_t hn_apu_next(const HNApu *);
void hn_apu_advance(HNApu *,uint32_t cycles);
/* Clipped ten-bit levels before PWM quantization or host gain. */
void hn_apu_levels(const HNApu *,int16_t stereo[2]);
#ifdef __cplusplus
}
#endif
#endif
