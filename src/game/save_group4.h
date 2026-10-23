#ifndef HN_GAME_SAVE_GROUP4_H
#define HN_GAME_SAVE_GROUP4_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Shared local record/payload operations; no link transport is started. */
HNSceneResult hn_group4_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_group4_task(HNMemory *,GameAddr callback);
#ifdef __cplusplus
}
#endif
#endif
