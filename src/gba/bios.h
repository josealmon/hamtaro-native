#ifndef HN_BIOS_H
#define HN_BIOS_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
void hn_bios_obj_matrix(int16_t x_scale,int16_t y_scale,uint16_t angle,int16_t out[4]);
bool hn_bios_obj_affine(HNMemory *,GameAddr source,GameAddr destination,uint32_t count,uint32_t stride);
bool hn_bios_cpu_set(HNMemory *, GameAddr source, GameAddr destination, uint32_t control);
bool hn_bios_cpu_fast_set(HNMemory *, GameAddr source, GameAddr destination, uint32_t control);
bool hn_bios_lz77(HNMemory *, GameAddr source, GameAddr destination, size_t capacity);
int32_t hn_runtime_divide(int32_t numerator, int32_t denominator);
int32_t hn_runtime_remainder(int32_t numerator, int32_t denominator);
int16_t hn_bios_arctan(int32_t value);
uint16_t hn_bios_arctan2(int32_t x,int32_t y);
uint32_t hn_bios_sqrt(uint32_t value);
#ifdef __cplusplus
}
#endif
#endif
