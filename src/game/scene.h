#ifndef HN_SCENE_H
#define HN_SCENE_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { HN_SCENE_UNKNOWN,HN_SCENE_OK,HN_SCENE_RESOURCE } HNSceneResult;
/* Typed native dispatch; guest callback words are metadata, never executed. */
HNSceneResult hn_scene_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_scene_animation_callback(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                         uint32_t *consumed,uint32_t *yield);
#ifdef __cplusplus
}
#endif
#endif
