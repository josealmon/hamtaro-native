#ifndef HN_GAME_ACTOR_PRESETS_H
#define HN_GAME_ACTOR_PRESETS_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Typed void/task ABI only; Main must not fabricate a consumed count. */
HNSceneResult hn_actor_presets_apply(HNMemory *,GameAddr callback);
#ifdef __cplusplus
}
#endif
#endif
