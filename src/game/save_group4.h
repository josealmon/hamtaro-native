#ifndef HN_GAME_SAVE_GROUP4_H
#define HN_GAME_SAVE_GROUP4_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Shared local record/payload operations; no link transport is started. */
HNSceneResult hn_group4_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_group4_task(HNMemory *,GameAddr callback);
HNSceneResult hn_group4_ui_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
uint8_t hn_group4_boundary(HNMemory *,uint32_t mode,GameAddr resource,uint32_t position,uint32_t selection);
bool hn_group4_text_line(HNMemory *,GameAddr text,GameAddr output,uint32_t palette);
bool hn_group4_text_lines(HNMemory *,GameAddr text,GameAddr output,uint32_t palette,uint32_t width);
#ifdef __cplusplus
}
#endif
#endif
