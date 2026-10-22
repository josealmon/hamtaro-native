#ifndef HN_GAME_BANK2_H
#define HN_GAME_BANK2_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Task ABI ignores the source return; argument-taking functions are excluded. */
HNSceneResult hn_bank2_apply(HNMemory *,GameAddr callback);
HNSceneResult hn_bank2_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank2_animation(HNMemory *,GameAddr callback,uint32_t *consumed,uint32_t *yield);
#ifdef __cplusplus
}
#endif
#endif
