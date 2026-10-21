#ifndef HN_GAME_HEAP_H
#define HN_GAME_HEAP_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
void hn_heap_init(HNMemory *);
GameAddr hn_heap_allocate(HNMemory *, uint32_t bytes, bool clear);
uint32_t hn_heap_free(HNMemory *, GameAddr data);
void hn_heap_coalesce(HNMemory *);
#ifdef __cplusplus
}
#endif
#endif
