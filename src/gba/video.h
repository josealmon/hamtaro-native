#ifndef HN_VIDEO_H
#define HN_VIDEO_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { HN_WIDTH=240, HN_HEIGHT=160 };
/* RGBA bytes, little-endian packed; no color correction or host scaling. */
void hn_video_render_line(const HNMemory *, unsigned line, uint32_t *rgba);
void hn_video_render(const HNMemory *, uint32_t *rgba);
#ifdef __cplusplus
}
#endif
#endif
