#ifndef HN_GAME_CHOICE_H
#define HN_GAME_CHOICE_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_choice_initial_selection(HNMemory *,GameAddr descriptor);
bool hn_choice_display(HNMemory *,GameAddr descriptor);
bool hn_choice_select_identifier(HNMemory *,GameAddr descriptor,uint32_t identifier);
bool hn_choice_update(HNMemory *,uint32_t count,GameAddr labels,GameAddr conditions,uint32_t *selected);
bool hn_choice_main_command(HNMemory *,unsigned opcode,GameAddr arguments);
bool hn_choice_reset(HNMemory *);
void hn_choice_request(HNMemory *,uint32_t identifier);
#ifdef __cplusplus
}
#endif
#endif
