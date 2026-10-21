#ifndef HN_GAME_SCRIPT_H
#define HN_GAME_SCRIPT_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef uint32_t (*HNMainFunction)(HNMemory *, GameAddr arguments);
typedef struct { GameAddr address; HNMainFunction function; } HNMainCallback;
typedef enum { HN_SCRIPT_OK, HN_SCRIPT_OPCODE, HN_SCRIPT_CALLBACK,
               HN_SCRIPT_STACK, HN_SCRIPT_BUDGET, HN_SCRIPT_MEMORY,
               HN_SCRIPT_RESOURCE } HNScriptFault;
typedef struct {
    HNMemory *memory;
    const HNMainCallback *callbacks;
    size_t callback_count;
    HNScriptFault fault;
    uint32_t fault_opcode;
    GameAddr fault_address;
} HNScript;
void hn_script_init(HNScript *,HNMemory *,const HNMainCallback *,size_t count);
bool hn_main_command(HNScript *,unsigned opcode,GameAddr arguments);
bool hn_main_frame(HNScript *,unsigned command_budget);
#ifdef __cplusplus
}
#endif
#endif
