#ifndef HN_BIOS_H
#define HN_BIOS_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_bios_cpu_set(HNMemory *, GameAddr source, GameAddr destination, uint32_t control);
bool hn_bios_cpu_fast_set(HNMemory *, GameAddr source, GameAddr destination, uint32_t control);
bool hn_bios_lz77(HNMemory *, GameAddr source, GameAddr destination, size_t capacity);
int32_t hn_runtime_divide(int32_t numerator, int32_t denominator);
int32_t hn_runtime_remainder(int32_t numerator, int32_t denominator);
#ifdef __cplusplus
}
#endif
#endif
