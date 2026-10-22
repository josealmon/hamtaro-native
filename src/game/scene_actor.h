#ifndef HN_GAME_SCENE_ACTOR_H
#define HN_GAME_SCENE_ACTOR_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_scene_actor_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
/* Reviewed argument-free targets, with their source return ignored by F4. */
HNSceneResult hn_scene_actor_task(HNMemory *,GameAddr callback);
#ifdef __cplusplus
}
#endif
#endif
