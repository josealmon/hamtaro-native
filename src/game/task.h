#ifndef HN_GAME_TASK_H
#define HN_GAME_TASK_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { HN_TASK_OK,HN_TASK_OPCODE,HN_TASK_CALLBACK,HN_TASK_BUDGET,HN_TASK_RESOURCE,HN_TASK_MEMORY } HNTaskFault;
typedef struct { HNMemory *memory;HNTaskFault fault;unsigned fault_opcode;GameAddr fault_address; } HNTasks;
void hn_tasks_init(HNTasks *,HNMemory *);
bool hn_task_command(HNTasks *,GameAddr task,unsigned opcode,uint32_t *keep_going);
bool hn_tasks_update(HNTasks *,unsigned command_budget);
bool hn_task_scroll_map(HNMemory *,GameAddr task,bool complete);
void hn_task_text_variant(HNMemory *,GameAddr task,uint8_t variant);
bool hn_task_text_retarget(HNMemory *,GameAddr variable);
void hn_task_text_number(HNMemory *,uint32_t format,uint32_t value);
GameAddr hn_task_allocate(HNMemory *);
bool hn_task_remove(HNMemory *,GameAddr task);
bool hn_task_graphics(HNMemory *,GameAddr task,uint32_t width,uint32_t height,uint32_t x,
                      uint32_t advance,uint32_t delay,uint32_t fill,uint32_t flags,GameAddr graphics,GameAddr map);
bool hn_task_render_disable(HNMemory *,GameAddr task);
bool hn_task_render_scroll(HNMemory *,GameAddr task,bool clip);
bool hn_task_indicator(HNMemory *,GameAddr task);
bool hn_task_row_transfer(HNMemory *,GameAddr task);
bool hn_task_text_bundle(HNMemory *,unsigned opcode,unsigned index,GameAddr *text);
GameAddr hn_task_text_adjust(HNMemory *,GameAddr text,uint32_t buffer_index);
bool hn_task_text_measure(HNMemory *,GameAddr text,uint8_t style,bool items,uint32_t *result);
bool hn_task_text_offset(HNMemory *,GameAddr task,uint32_t offset);
bool hn_task_set_script(HNMemory *,GameAddr task,GameAddr script);
bool hn_task_main_command(HNMemory *,unsigned opcode,GameAddr arguments);
#ifdef __cplusplus
}
#endif
#endif
