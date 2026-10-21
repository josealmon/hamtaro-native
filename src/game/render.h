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
void hn_render_error(HNMemory *,uint16_t code);
void hn_render_set_animation(HNMemory *,uint32_t index,GameAddr animation);
void hn_render_flags(HNMemory *,uint32_t index,uint8_t clear_mask,uint8_t value);
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
#ifdef __cplusplus
}
#endif
#endif
