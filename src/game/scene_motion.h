#ifndef HN_GAME_SCENE_MOTION_H
#define HN_GAME_SCENE_MOTION_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_scene_motion_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_scene_motion_animation(HNMemory *,GameAddr callback,uint32_t *consumed,uint32_t *yield);
#ifdef __cplusplus
}
#endif
#endif
