#ifndef HN_GAME_RASTER_H
#define HN_GAME_RASTER_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Script-side state and DMA/IRQ setup. Scanline execution is a separate contract. */
bool hn_raster_profile(HNMemory *);
bool hn_raster_position(HNMemory *,uint8_t source);
/* Typed IRQ dispatch; false reports an unsupported guest callback. */
bool hn_raster_hblank(HNMemory *);
bool hn_raster_wave(HNMemory *);
HNSceneResult hn_raster_callback(HNMemory *,GameAddr callback,uint32_t *consumed);
bool hn_raster_command(HNMemory *,GameAddr state,unsigned opcode,GameAddr data,uint32_t *running);
#ifdef __cplusplus
}
#endif
#endif
