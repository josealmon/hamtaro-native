#ifndef HN_GAME_BANK5_H
#define HN_GAME_BANK5_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_bank5_presets(HNMemory *,GameAddr callback);
HNSceneResult hn_bank5_preset_callback(HNMemory *,GameAddr callback,uint32_t *consumed);
HNSceneResult hn_bank5_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank5_task(HNMemory *,GameAddr callback);
HNSceneResult hn_bank5_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                uint32_t *consumed,uint32_t *yield);
HNSceneResult hn_bank5_irq(HNMemory *,GameAddr callback);
bool hn_bank5_primary_lane(HNMemory *,uint32_t index);
bool hn_bank5_partner_lane(HNMemory *,uint32_t index);
uint32_t hn_bank5_outcome(HNMemory *);
#ifdef __cplusplus
}
#endif
#endif
