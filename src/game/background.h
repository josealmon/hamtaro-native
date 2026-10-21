#ifndef HN_GAME_BACKGROUND_H
#define HN_GAME_BACKGROUND_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {HN_BACKGROUND_OK,HN_BACKGROUND_OPCODE,HN_BACKGROUND_STACK,
              HN_BACKGROUND_BUDGET,HN_BACKGROUND_MEMORY,HN_BACKGROUND_RESOURCE} HNBackgroundFault;
typedef struct {
    HNMemory *memory;HNBackgroundFault fault;unsigned fault_opcode;GameAddr fault_address;
} HNBackground;
void hn_background_init(HNBackground *,HNMemory *);
void hn_background_clamp(HNMemory *,uint8_t layer,GameAddr coordinate,uint8_t axis);
bool hn_background_apply(HNMemory *,uint8_t layer);
bool hn_background_motion(HNMemory *,GameAddr state);
bool hn_background_configure(HNMemory *,uint8_t slot,uint8_t mask,GameAddr script);
bool hn_background_command(HNBackground *,GameAddr state,unsigned opcode,GameAddr *cursor,uint32_t *yield);
bool hn_background_process(HNBackground *,GameAddr state,unsigned command_budget);
bool hn_background_update(HNBackground *,unsigned command_budget);
#ifdef __cplusplus
}
#endif
#endif
