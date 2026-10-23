#ifndef HN_GAME_PAIR_LIST_H
#define HN_GAME_PAIR_LIST_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_pair_list_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_pair_list_task(HNMemory *,GameAddr callback);
bool hn_pair_list_palette_slots(HNMemory *);
bool hn_pair_list_variation_palette(HNMemory *,uint8_t category,uint8_t variation);
bool hn_pair_list_remaining(HNMemory *);
bool hn_pair_list_value(HNMemory *);
bool hn_pair_list_selected_value(HNMemory *);
bool hn_pair_list_icon(HNMemory *);
uint8_t hn_pair_list_classify(HNMemory *,uint32_t category,uint32_t mask);
bool hn_pair_list_display(HNMemory *,uint8_t index,uint8_t mask);
bool hn_pair_list_cursor(HNMemory *,uint8_t index);
bool hn_pair_list_highlight(HNMemory *,uint8_t index,uint8_t selected);
bool hn_pair_list_first_unset(HNMemory *,uint8_t index);
/* Original return-zero leaf; its incoming ABI has not been identified. */
uint32_t hn_pair_list_return_zero(void);
#ifdef __cplusplus
}
#endif
#endif
