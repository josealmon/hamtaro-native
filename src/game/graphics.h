#ifndef HN_GAME_GRAPHICS_H
#define HN_GAME_GRAPHICS_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {HN_GRAPHICS_OK,HN_GRAPHICS_OPCODE,HN_GRAPHICS_RESOURCE,
              HN_GRAPHICS_BUDGET,HN_GRAPHICS_MEMORY} HNGraphicsFault;
typedef struct {
    HNMemory *memory;HNGraphicsFault fault;unsigned fault_opcode;GameAddr fault_address;
} HNGraphics;
void hn_graphics_init(HNGraphics *,HNMemory *);
bool hn_graphics_configure(HNMemory *,uint8_t slot,GameAddr script);
bool hn_graphics_configure_raw(HNMemory *,uint8_t slot,GameAddr script);
bool hn_graphics_initialize_raw(HNMemory *,uint8_t slot,uint8_t mode,GameAddr script);
bool hn_palette_prepare(HNMemory *,uint32_t amount,uint32_t first,uint32_t count);
bool hn_palette_update(HNMemory *,uint32_t first,uint32_t count);
void hn_mosaic_registers(HNMemory *,uint8_t selector,uint8_t x,uint8_t y);
void hn_mosaic_sizes(HNMemory *,uint8_t selector,uint8_t x,uint8_t y);
bool hn_graphics_command(HNGraphics *,GameAddr state,unsigned opcode,GameAddr arguments,uint32_t *keep_running);
bool hn_graphics_step(HNGraphics *,GameAddr state,uint32_t *keep_running);
bool hn_graphics_update(HNGraphics *,unsigned command_budget);
#ifdef __cplusplus
}
#endif
#endif
