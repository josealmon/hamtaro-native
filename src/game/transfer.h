#ifndef HN_GAME_TRANSFER_H
#define HN_GAME_TRANSFER_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
void hn_transfer_reset(HNMemory *);
void hn_transfer_queue(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes);
void hn_transfer_queue_fill(HNMemory *,uint32_t value,GameAddr destination,uint32_t bytes);
void hn_transfer_flush(HNMemory *);
void hn_transfer_copy(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes,bool fill);
void hn_transfer_tiles(HNMemory *,GameAddr source,GameAddr destination,uint32_t width,uint32_t rows);
void hn_transfer_cpu_queue(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes);
#ifdef __cplusplus
}
#endif
#endif
