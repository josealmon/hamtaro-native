#ifndef HN_GAME_COUNTER_SCENE_H
#define HN_GAME_COUNTER_SCENE_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Direct family calls retain the original R0 word; indicators consume its low byte. */
HNSceneResult hn_counter_scene_apply(HNMemory *,GameAddr callback,uint32_t word);
HNSceneResult hn_counter_scene_callback(HNMemory *,GameAddr callback,uint32_t *consumed);
HNSceneResult hn_counter_scene_task(HNMemory *,GameAddr callback);
#ifdef __cplusplus
}
#endif
#endif
