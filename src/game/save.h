#ifndef HN_GAME_SAVE_H
#define HN_GAME_SAVE_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_sram_init(HNMemory *);
bool hn_sram_read(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes);
bool hn_sram_write(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes);
uint32_t hn_sram_verify(HNMemory *,GameAddr source,GameAddr destination,uint32_t bytes);
uint16_t hn_save_checksum(HNMemory *,GameAddr source,uint32_t bytes);
bool hn_save_produce(HNMemory *,unsigned group,GameAddr destination,uint32_t *size);
bool hn_save_consume(HNMemory *,unsigned group,GameAddr source);
bool hn_save_write_record(HNMemory *,unsigned group,uint32_t index);
bool hn_save_write_groups(HNMemory *);
bool hn_save_load_group(HNMemory *,unsigned group);
bool hn_save_load_groups(HNMemory *);
void hn_save_default_fields(HNMemory *);
bool hn_save_reset_group4(HNMemory *);
bool hn_save_initialize(HNMemory *);
#ifdef __cplusplus
}
#endif
#endif
