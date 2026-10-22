#ifndef HN_GAME_SAVE_GROUP3_H
#define HN_GAME_SAVE_GROUP3_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native storage for the original escaping 20-byte stack text object. */
enum { HN_GROUP3_LIST_TEXT = 0x03007D00 };
HNSceneResult hn_group3_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_group3_task(HNMemory *,GameAddr callback);
bool hn_group3_pair_graphics(HNMemory *,uint8_t category,uint32_t bank_word);
bool hn_group3_pair_animation(HNMemory *,uint32_t slot,uint8_t category,uint32_t bank_word);
bool hn_group3_pair_palette(HNMemory *,uint8_t category,uint8_t variation,uint8_t unused,uint32_t bank_word);
bool hn_group3_number(HNMemory *,uint32_t slot,uint8_t value,uint8_t x,uint8_t y);
bool hn_group3_counts(HNMemory *);
bool hn_group3_selected_count(HNMemory *);
void hn_group3_field_bit(HNMemory *,uint8_t field,uint8_t bit,uint8_t value);
void hn_group3_remove_list(HNMemory *,uint8_t index);
bool hn_group3_remove_lookup(HNMemory *,uint8_t value);
bool hn_group3_tile_map(HNMemory *,GameAddr destination,GameAddr source,uint16_t attributes);
bool hn_group3_insert_pair(HNMemory *,GameAddr pair,GameAddr begin);
bool hn_group3_position(HNMemory *,uint8_t category,GameAddr x,GameAddr y,GameAddr z,uint8_t bank);
bool hn_group3_find_pair(HNMemory *,GameAddr pairs,uint8_t first,uint8_t second,uint8_t *index);
/* The original empty-category path reads an uninitialized guest stack word.
   Direct callers must supply that retained word rather than inventing host UB. */
bool hn_group3_pair_slot(HNMemory *,uint32_t slot,uint32_t parent,uint32_t category,
                         uint32_t variation,uint32_t bank,uint32_t retained_z);
bool hn_group3_scan_pairs(HNMemory *,GameAddr fields,GameAddr pairs,uint32_t first_row,
                          uint32_t group,uint32_t *count);
bool hn_group3_list_text(HNMemory *,GameAddr buffer);
#ifdef __cplusplus
}
#endif
#endif
