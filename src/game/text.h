#ifndef HN_GAME_TEXT_H
#define HN_GAME_TEXT_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_text_blit_glyph(HNMemory *,GameAddr glyph,GameAddr shadow,uint32_t tile_width,
                        uint32_t horizontal_offset,uint32_t color_group);
uint8_t hn_scene_text_character(HNMemory *,uint8_t index,uint8_t value);
bool hn_scene_text_render(HNMemory *);
HNSceneResult hn_scene_text_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
