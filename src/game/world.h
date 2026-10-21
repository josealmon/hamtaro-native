#ifndef HN_GAME_WORLD_H
#define HN_GAME_WORLD_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_runtime_tables_init(HNMemory *);
void hn_task_contexts_init(HNMemory *);
void hn_core_storage_clear(HNMemory *,int32_t selector);
void hn_core_reset(HNMemory *);
bool hn_core_initialize(HNMemory *);
void hn_render_slot_reset(HNMemory *,uint32_t index);
void hn_render_reset(HNMemory *);
void hn_render_order_init(HNMemory *);
void hn_render_priority_init(HNMemory *);
int32_t hn_render_key(HNMemory *,uint32_t index);
void hn_background_state_init(HNMemory *);
void hn_background_layers_reset(HNMemory *,uint8_t mask);
bool hn_map_initialize(HNMemory *,uint8_t bank,uint8_t index);
#ifdef __cplusplus
}
#endif
#endif
