#ifndef HN_GAME_PRESENTATION_H
#define HN_GAME_PRESENTATION_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_presentation_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_presentation_task(HNMemory *,GameAddr callback);
HNSceneResult hn_presentation_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield);
HNSceneResult hn_presentation_background_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_presentation_background_task(HNMemory *,GameAddr callback);
HNSceneResult hn_presentation_background_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield);
HNSceneResult hn_presentation_window_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_presentation_window_task(HNMemory *,GameAddr callback);
bool hn_presentation_actor_height(HNMemory *,GameAddr slot,unsigned operation);
uint32_t hn_presentation_region_motion(HNMemory *,GameAddr slot,GameAddr z_moving,GameAddr x_moving,GameAddr z_idle,GameAddr x_idle);
bool hn_presentation_scroll_row(HNMemory *,GameAddr descriptor,uint32_t bank,uint32_t position);
#ifdef __cplusplus
}
#endif
#endif
