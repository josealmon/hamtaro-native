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
bool hn_raster_vcount(HNMemory *);
bool hn_raster_vcount_blend(HNMemory *);
bool hn_raster_vcount_increment(HNMemory *);
bool hn_raster_wave(HNMemory *);
/* For positive radii, the original leaves the first scratch byte unwritten. */
bool hn_raster_scanline_profile(HNMemory *,uint8_t retained_first);
void hn_raster_scanline_phase_reset(HNMemory *);
HNSceneResult hn_raster_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
bool hn_raster_command(HNMemory *,GameAddr state,unsigned opcode,GameAddr data,uint32_t *running);
#ifdef __cplusplus
}
#endif
#endif
