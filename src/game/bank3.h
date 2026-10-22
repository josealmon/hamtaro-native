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
HNSceneResult hn_bank3_games_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank3_games_task(HNMemory *,GameAddr callback);
HNSceneResult hn_bank3_games_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                      uint32_t *consumed,uint32_t *yield);
uint32_t hn_bank3_phase_kind(HNMemory *,uint32_t mode,uint32_t retained_r4);
uint32_t hn_bank3_boxes_overlap(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
bool hn_bank3_entry_configure(HNMemory *,uint32_t index_word);
bool hn_bank3_entry_spawn(HNMemory *,uint32_t index_word);
bool hn_bank3_entry_motion(HNMemory *,uint8_t index);
HNSceneResult hn_bank3_chain_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank3_chain_task(HNMemory *,GameAddr callback);
HNSceneResult hn_bank3_chain_animation(HNMemory *,GameAddr callback,uint32_t *consumed,uint32_t *yield);
uint32_t hn_bank3_chain_find(HNMemory *,uint32_t kind_word,uint32_t retained_r4);
void hn_bank3_chain_set_flag(HNMemory *,uint32_t kind_word,uint32_t value_word);
void hn_bank3_chain_clear(HNMemory *,uint32_t side_word,uint32_t index_word);
void hn_bank3_chain_prepare(HNMemory *,uint32_t index_word);
void hn_bank3_chain_consume(HNMemory *,uint32_t index_word);
bool hn_bank3_chain_variant(HNMemory *,uint32_t index_word,uint32_t *variant);
bool hn_bank3_chain_mark(HNMemory *,uint32_t index_word);
bool hn_bank3_chain_flags(HNMemory *,GameAddr legacy_old_flags);
bool hn_bank3_chain_effect(HNMemory *,uint32_t index_word,uint32_t capture_word,uint32_t kind_word);
#ifdef __cplusplus
}
#endif
#endif
