#ifndef HN_GAME_RENDER_H
#define HN_GAME_RENDER_H
#include "game/world.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef uint32_t (*HNAnimationFunction)(HNMemory *,GameAddr slot,GameAddr arguments,uint32_t *yield);
typedef struct { GameAddr address;HNAnimationFunction function; } HNAnimationCallback;
typedef enum { HN_RENDER_OK,HN_RENDER_OPCODE,HN_RENDER_CALLBACK,HN_RENDER_BUDGET,
               HN_RENDER_MEMORY,HN_RENDER_RESOURCE } HNRenderFault;
typedef struct {
    HNMemory *memory;const HNAnimationCallback *callbacks;size_t callback_count;
    HNRenderFault fault;unsigned fault_opcode;GameAddr fault_address;
} HNRender;
void hn_render_init(HNRender *,HNMemory *,const HNAnimationCallback *,size_t);
GameAddr hn_render_slot(HNMemory *,uint32_t index);
uint8_t hn_render_find_slot(HNMemory *,GameAddr slot);
int32_t hn_render_class(HNMemory *,GameAddr slot);
uint32_t hn_render_direction_between(HNMemory *,uint8_t from,uint8_t to,bool eight_directions);
/* Original raw byte selectors; mapped guest slots need not be in the active range. */
uint32_t hn_render_direction_raw(HNMemory *,uint8_t from,uint8_t to,bool eight_directions);
void hn_render_error(HNMemory *,uint16_t code);
void hn_render_set_animation(HNMemory *,uint32_t index,GameAddr animation);
void hn_render_flags(HNMemory *,uint32_t index,uint8_t clear_mask,uint8_t value);
bool hn_render_modes(HNMemory *,uint8_t index,uint8_t mode,uint8_t value,uint8_t field65);
/* Source wrappers address byte-selected mapped slots without an active-count guard. */
bool hn_render_modes_raw(HNMemory *,uint8_t index,uint8_t mode,uint8_t value,uint8_t field65);
bool hn_render_configure_raw(HNMemory *,uint8_t index,uint8_t field60,uint8_t field62,
                             int32_t x,int32_t y,int32_t z,GameAddr animation);
bool hn_render_configure(HNMemory *,uint8_t index,uint8_t field60,uint8_t field62,
                         int32_t x,int32_t y,int32_t z,GameAddr animation);
void hn_render_physics(HNMemory *,GameAddr slot);
void hn_render_screen_position(HNMemory *,GameAddr slot);
uint8_t hn_render_find_affine(HNMemory *);
void hn_render_mark_affine(HNMemory *);
bool hn_render_assign_affine(HNMemory *);
bool hn_render_reorder(HNMemory *,uint8_t moving,uint8_t anchor,uint8_t mode);
bool hn_animation_command(HNRender *,GameAddr slot,unsigned opcode,GameAddr *cursor,uint32_t *yield);
bool hn_render_animation(HNRender *,GameAddr slot,unsigned command_budget);
bool hn_render_draw(HNRender *);
bool hn_render_update(HNRender *,unsigned command_budget);
bool hn_render_number(HNMemory *,uint8_t index,uint32_t value,uint32_t x,uint32_t y);
#ifdef __cplusplus
}
#endif
#endif
