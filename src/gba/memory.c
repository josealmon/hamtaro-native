#include "gba/memory.h"
#include <string.h>

static uint8_t *resolve(HNMemory *m, GameAddr address, bool writing)
{
    unsigned offset = address & 0x00FFFFFFu;
    switch (address >> 24) {
    case 2: return m->ewram + (offset & 0x3FFFF);
    case 3: return m->iwram + (offset & 0x7FFF);
    case 4: if (offset < sizeof(m->io)) return m->io + offset; break;
    case 5: return m->palette + (offset & 0x3FF);
    case 6:
        offset &= 0x1FFFF;
        if (offset >= 0x18000) offset -= 0x8000;
        return m->vram + offset;
    case 7: return m->oam + (offset & 0x3FF);
    case 8: case 9: case 10: case 11: case 12: case 13:
        offset = (address - HN_ROM_BASE) & 0x01FFFFFFu;
        if (!writing && offset < m->rom_size) return (uint8_t *)(m->rom + offset);
        break;
    case 14: case 15:
        if (writing) m->sram_dirty = true;
        return m->sram + (offset & 0x7FFF);
    }
    if (!m->fault) m->fault_address = address;
    m->fault = true;
    return NULL;
}
void hn_memory_init(HNMemory *m, const uint8_t *rom, size_t size)
{
    memset(m, 0, sizeof(*m));
    m->rom = rom; m->rom_size = size;
    memset(m->sram, 0xFF, sizeof(m->sram));
    m->io[0x130] = 0xFF; m->io[0x131] = 3;
}
uint8_t hn_read8(HNMemory *m, GameAddr a)
{
    const uint8_t *p = resolve(m, a, false);
    return p ? *p : 0;
}
uint16_t hn_read16(HNMemory *m, GameAddr a)
{
    return (uint16_t)(hn_read8(m,a) | ((uint16_t)hn_read8(m,a+1) << 8));
}
uint32_t hn_read32(HNMemory *m, GameAddr a)
{
    return (uint32_t)hn_read16(m,a) | ((uint32_t)hn_read16(m,a+2) << 16);
}
static void raw8(HNMemory *m, GameAddr a, uint8_t v)
{
    uint8_t *p = resolve(m,a,true);
    if (p) *p = v;
}
static uint32_t io32(const HNMemory *m, unsigned o)
{
    return (uint32_t)m->io[o] | (uint32_t)m->io[o+1]<<8 |
           (uint32_t)m->io[o+2]<<16 | (uint32_t)m->io[o+3]<<24;
}
static void store_io32(HNMemory *m, unsigned o, uint32_t v)
{
    for (unsigned i=0;i<4;i++) m->io[o+i] = (uint8_t)(v >> (8*i));
}
void hn_write8(HNMemory *m, GameAddr a, uint8_t v)
{
    /* Palette/VRAM byte stores replicate to a halfword; OAM ignores byte stores. */
    if ((a>>24)==6) {
        unsigned mode=m->io[0]&7,offset=a&0x1FFFF;
        if(offset>=0x18000)offset-=0x8000;
        if(offset>=(mode>=3?0x14000u:0x10000u))return;
    }
    if ((a>>24)==5 || (a>>24)==6) {
        a &= ~1u; raw8(m,a,v); raw8(m,a+1,v); return;
    }
    if ((a>>24)==7) return;
    raw8(m,a,v);
}
void hn_write16(HNMemory *m, GameAddr a, uint16_t v)
{
    if(a==0x04000006)return; /* VCOUNT is read-only. */
    if(a==0x04000004)v=(uint16_t)((v&~7u)|(m->io[4]&7));
    if (a == 0x04000202) { /* IF is write-one-to-clear. */
        m->io[0x202] &= (uint8_t)~v; m->io[0x203] &= (uint8_t)~(v>>8); return;
    }
    raw8(m,a,(uint8_t)v); raw8(m,a+1,(uint8_t)(v>>8));
    if (!m->dma_active && a>=0x040000BA && a<=0x040000DE &&
        (a-0x040000BA)%12==0 && (v&0x8000)) hn_dma_trigger(m,0);
}
void hn_write32(HNMemory *m, GameAddr a, uint32_t v)
{
    hn_write16(m,a,(uint16_t)v);
    hn_write16(m,a+2,(uint16_t)(v>>16));
}
bool hn_memory_copy(HNMemory *m, GameAddr dst, GameAddr src, size_t n)
{
    if (n > 0x01000000u || n > UINT32_MAX-dst || n > UINT32_MAX-src) return false;
    for (size_t i=0;i<n && !m->fault;i++) {
        uint8_t v=hn_read8(m,src+(GameAddr)i);
        raw8(m,dst+(GameAddr)i,v);
    }
    return !m->fault;
}
void hn_dma_trigger(HNMemory *m, unsigned timing)
{
    if (m->dma_active || timing>2) return;
    m->dma_active=true;
    for (unsigned ch=0;ch<4;ch++) {
        unsigned o=0xB0+ch*12;
        uint32_t ctl=io32(m,o+8);
        if (!(ctl&0x80000000u) || ((ctl>>28)&3)!=timing) continue;
        GameAddr src=io32(m,o), dst=io32(m,o+4), initial_dst=dst;
        unsigned width=(ctl&(1u<<26))?4:2, count=ctl&(ch==3?0xFFFFu:0x3FFFu);
        unsigned sm=(ctl>>23)&3, dm=(ctl>>21)&3;
        if (sm==3) { m->fault=true; m->fault_address=0x040000B0+ch*12; break; }
        if (!count) count=ch==3?65536:16384;
        src &= ~(width-1); dst &= ~(width-1);
        for (unsigned i=0;i<count && !m->fault;i++) {
            if (width==4) { uint32_t v=hn_read32(m,src); hn_write32(m,dst,v); }
            else { uint16_t v=hn_read16(m,src); hn_write16(m,dst,v); }
            if (sm==0) src+=width; else if(sm==1) src-=width;
            if (dm==0 || dm==3) dst+=width; else if(dm==1) dst-=width;
        }
        store_io32(m,o,src);
        store_io32(m,o+4,dm==3?initial_dst:dst);
        if (ctl&(1u<<30)) {
            unsigned irq=1u<<(8+ch);
            m->io[0x202]|=(uint8_t)irq; m->io[0x203]|=(uint8_t)(irq>>8);
        }
        if (!(ctl&(1u<<25)) || timing==0) ctl &= ~0x80000000u;
        store_io32(m,o+8,ctl);
    }
    m->dma_active=false;
}
