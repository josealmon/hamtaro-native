#ifndef HN_GAME_BANK6_H
#define HN_GAME_BANK6_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_bank6_presets(HNMemory *,GameAddr callback);
HNSceneResult hn_bank6_preset_callback(HNMemory *,GameAddr callback,uint32_t *consumed);
HNSceneResult hn_bank6_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank6_task(HNMemory *,GameAddr callback);
HNSceneResult hn_bank6_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                uint32_t *consumed,uint32_t *yield);
HNSceneResult hn_bank6_targets_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank6_targets_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                        uint32_t *consumed,uint32_t *yield);
bool hn_bank6_orbit(HNMemory *,uint32_t index,uint32_t class_word);
bool hn_bank6_target_motion(HNMemory *,GameAddr coordinates,uint32_t moving,uint32_t target);
bool hn_bank6_target_position(HNMemory *,GameAddr coordinates,uint32_t index);
/* The no-match legacy path reads the incoming R5 descriptor. */
bool hn_bank6_restore_targets(HNMemory *,GameAddr arguments,GameAddr retained_r5);
void hn_bank6_planar_transform(HNMemory *,GameAddr slot,uint32_t x,uint32_t y,uint32_t first,uint32_t second);
HNSceneResult hn_bank6_chain_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank6_chain_task(HNMemory *,GameAddr callback);
HNSceneResult hn_bank6_chain_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                     uint32_t *consumed,uint32_t *yield);
#ifdef __cplusplus
}
#endif
#endif
