#ifndef HN_GAME_PRESETS_H
#define HN_GAME_PRESETS_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Void/task ABI: return values of the original functions are ignored. */
HNSceneResult hn_preset_apply(HNMemory *,GameAddr callback);
/* Main ABI: only reviewed functions with a defined zero consumed count. */
HNSceneResult hn_preset_callback(HNMemory *,GameAddr callback,uint32_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
