#ifndef HN_MEMORY_H
#define HN_MEMORY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef uint32_t GameAddr;
enum { HN_ROM_BASE = 0x08000000, HN_SRAM_SIZE = 32768 };
typedef struct {
    const uint8_t *rom;
    size_t rom_size;
    uint8_t ewram[262144], iwram[32768], io[1024];
    uint8_t palette[1024], vram[98304], oam[1024], sram[HN_SRAM_SIZE];
    GameAddr fault_address;
    bool fault, sram_dirty, dma_active;
} HNMemory;
void hn_memory_init(HNMemory *, const uint8_t *rom, size_t size);
uint8_t hn_read8(HNMemory *, GameAddr);
uint16_t hn_read16(HNMemory *, GameAddr);
uint32_t hn_read32(HNMemory *, GameAddr);
void hn_write8(HNMemory *, GameAddr, uint8_t);
void hn_write16(HNMemory *, GameAddr, uint16_t);
void hn_write32(HNMemory *, GameAddr, uint32_t);
/* 0 = immediate, 1 = VBlank, 2 = HBlank. Sound DMA is not dispatched here. */
void hn_dma_trigger(HNMemory *, unsigned timing);
bool hn_memory_copy(HNMemory *, GameAddr destination, GameAddr source, size_t count);
#ifdef __cplusplus
}
#endif
#endif
