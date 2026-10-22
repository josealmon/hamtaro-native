#ifndef HN_GAME_WANDER_H
#define HN_GAME_WANDER_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_wander_initialize(HNMemory *,GameAddr arguments);
void hn_wander_velocity(HNMemory *,GameAddr slot,int32_t rate,uint8_t direction);
bool hn_wander_bounds(HNMemory *,GameAddr context,GameAddr slot,uint8_t direction,uint32_t *blocked);
bool hn_wander_update(HNMemory *,GameAddr callback_slot,GameAddr arguments,uint32_t *yield);
bool hn_wander_reset(HNMemory *,GameAddr arguments);
bool hn_wander_turn_initialize(HNMemory *,GameAddr arguments);
bool hn_wander_turn_update(HNMemory *);
HNSceneResult hn_wander_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                  uint32_t *consumed,uint32_t *yield);
HNSceneResult hn_wander_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
