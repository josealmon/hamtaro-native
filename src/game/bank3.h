#ifndef HN_GAME_BANK3_H
#define HN_GAME_BANK3_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Reviewed late-scene contracts; the main minigame family is separate. */
HNSceneResult hn_bank3_task(HNMemory *,GameAddr callback);
HNSceneResult hn_bank3_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank3_animation(HNMemory *,GameAddr callback,uint32_t *consumed,uint32_t *yield);
#ifdef __cplusplus
}
#endif
#endif
