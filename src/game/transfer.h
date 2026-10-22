#ifndef HN_GAME_TRANSFER_H
#define HN_GAME_TRANSFER_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
void hn_transfer_reset(HNMemory *);
void hn_transfer_queue(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes);
void hn_transfer_queue_fill(HNMemory *,uint32_t value,GameAddr destination,uint32_t bytes);
bool hn_transfer_fill_words(HNMemory *,GameAddr destination,uint32_t value,uint32_t words);
void hn_transfer_flush(HNMemory *);
void hn_transfer_copy(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes,bool fill);
void hn_transfer_tiles(HNMemory *,GameAddr source,GameAddr destination,uint32_t width,uint32_t rows);
void hn_transfer_cpu_queue(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes);
bool hn_transfer_strided_rows(HNMemory *,GameAddr source,GameAddr destination,uint32_t row_bytes,uint32_t rows,uint32_t stride);
bool hn_transfer_decompress_rows(HNMemory *,GameAddr source,GameAddr destination,uint32_t row_bytes,uint32_t rows,uint32_t stride);
#ifdef __cplusplus
}
#endif
#endif
