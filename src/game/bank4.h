#ifndef HN_GAME_BANK4_H
#define HN_GAME_BANK4_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_bank4_presets(HNMemory *,GameAddr callback);
HNSceneResult hn_bank4_preset_callback(HNMemory *,GameAddr callback,uint32_t *consumed);
HNSceneResult hn_bank4_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank4_task(HNMemory *,GameAddr callback);
HNSceneResult hn_bank4_animation(HNMemory *,GameAddr callback,GameAddr slot,GameAddr arguments,
                                uint32_t *consumed,uint32_t *yield);
bool hn_bank4_clear_neighbors(HNMemory *,uint32_t index_word);
bool hn_bank4_distance_volume(HNMemory *,uint32_t player_word,uint32_t song_word,int32_t distance,
                              int32_t limit,uint32_t from_word,uint32_t to_word);
uint32_t hn_bank4_projected_position(HNMemory *,GameAddr slot);
HNSceneResult hn_bank4_path_animation(HNMemory *,GameAddr callback,GameAddr arguments,
                                     uint32_t *consumed,uint32_t *yield);
bool hn_bank4_advance_path(HNMemory *,GameAddr state,GameAddr path);
bool hn_bank4_advance_reversible(HNMemory *,GameAddr state,GameAddr path,uint16_t reverse_flag);
bool hn_bank4_path_image(HNMemory *,GameAddr slot,uint8_t animation,uint8_t overlay,uint8_t index);
bool hn_bank4_directional_image(HNMemory *,GameAddr slot,uint8_t animation,uint8_t direction,
                                uint8_t overlay,uint32_t index_word,bool blocked);
uint16_t hn_bank4_accumulate(HNMemory *,GameAddr accumulator,uint16_t speed);
void hn_bank4_path_bounds(HNMemory *,uint8_t direction,uint16_t x,uint16_t z,
                          GameAddr min_x,GameAddr max_x,GameAddr min_z,GameAddr max_z);
uint8_t hn_bank4_path_blocked(uint16_t player_x,uint16_t player_z,uint16_t follower_x,uint16_t follower_z,
                             uint16_t min_x,uint16_t max_x,uint16_t min_z,uint16_t max_z,uint8_t both);
/* Direct legacy adapter exposes the early-wait incoming SL contract explicitly. */
bool hn_bank4_follow_path(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t retained_sl);
#ifdef __cplusplus
}
#endif
#endif
