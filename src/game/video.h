#ifndef HN_GAME_VIDEO_H
#define HN_GAME_VIDEO_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
void hn_video_state_init(HNMemory *);
void hn_video_set_mode(HNMemory *,uint8_t mode);
void hn_video_set_layers(HNMemory *,uint8_t enabled,uint8_t mask);
void hn_video_forced_blank(HNMemory *,int32_t blank);
void hn_video_mark_backgrounds(HNMemory *,uint8_t mask);
void hn_video_hblank(HNMemory *,uint32_t selector);
void hn_video_blend_effect(HNMemory *,uint32_t effect);
void hn_video_blend_targets(HNMemory *,uint8_t first,uint8_t second);
void hn_video_blend_coefficients(HNMemory *,uint8_t first,uint8_t second,uint8_t brightness);
void hn_video_blend_scale(HNMemory *,uint8_t scale);
void hn_video_fade_phase(HNMemory *,uint8_t phase);
void hn_mosaic_init(HNMemory *);
void hn_mosaic_configure(HNMemory *,uint8_t enabled,uint8_t flags);
void hn_mosaic_square(HNMemory *,uint8_t size);
void hn_mosaic_shared(HNMemory *,uint8_t horizontal,uint8_t vertical);
void hn_mosaic_update(HNMemory *);
void hn_video_commit(HNMemory *);
#ifdef __cplusplus
}
#endif
#endif
