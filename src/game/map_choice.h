#ifndef HN_GAME_MAP_CHOICE_H
#define HN_GAME_MAP_CHOICE_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit guest calling contracts; void task calls ignore value. */
HNSceneResult hn_map_choice_apply(HNMemory *,GameAddr callback,uint32_t *value);
HNSceneResult hn_map_choice_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_map_choice_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,uint32_t *consumed,uint32_t *yield);
HNSceneResult hn_map_choice_slot(HNMemory *,GameAddr callback,GameAddr slot,uint32_t *value);
#ifdef __cplusplus
}
#endif
#endif
