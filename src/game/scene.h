#ifndef HN_SCENE_H
#define HN_SCENE_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { HN_SCENE_UNKNOWN,HN_SCENE_OK,HN_SCENE_RESOURCE } HNSceneResult;
bool hn_scene_indicator_complete(HNMemory *,uint32_t index);
bool hn_scene_indicator_visible(HNMemory *,uint32_t index);
bool hn_scene_indicator_configure(HNMemory *,uint32_t index,uint32_t state);
bool hn_scene_indicator_update(HNMemory *,uint32_t index);
/* Original modal wrappers use mapped actor selectors without active-count guards. */
bool hn_scene_save_actor_slots_raw(HNMemory *);
bool hn_scene_restore_actor_slots_raw(HNMemory *);
/* Typed native dispatch; guest callback words are metadata, never executed. */
HNSceneResult hn_scene_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_scene_animation_callback(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                         uint32_t *consumed,uint32_t *yield);
#ifdef __cplusplus
}
#endif
#endif
