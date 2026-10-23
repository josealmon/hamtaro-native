#ifndef HN_GAME_SCENE_SELECTION_H
#define HN_GAME_SCENE_SELECTION_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_scene_selection_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
/* Void presets have a task-only calling contract. */
HNSceneResult hn_scene_selection_task(HNMemory *,GameAddr callback);
#ifdef __cplusplus
}
#endif
#endif
