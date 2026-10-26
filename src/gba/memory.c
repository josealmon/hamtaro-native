#include "gba/memory.h"
#include "gba/timer.h"
#include "gba/audio.h"
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
    if(m->timers&&a>=0x04000100&&a<0x04000110)return hn_timers_read(m->timers,a-0x04000000);
    if(m->apu&&(a>>24)==4&&hn_apu_register(a&0xFFFFFF))return hn_apu_read(m->apu,a&0xFFFFFF);
    const uint8_t *p = resolve(m, a, false);
    return p ? *p : 0;
}
/* Plain RAM/ROM reads need one address lookup. Boundary-crossing reads and
   device registers retain the byte path, including mirrors and side effects. */
static const uint8_t *plain_read(const HNMemory *m,GameAddr a,unsigned width)
{
    unsigned offset;
    switch(a>>24){
    case 2:
        offset=a&0x3FFFF;
        return offset<=sizeof(m->ewram)-width?m->ewram+offset:NULL;
    case 3:
        offset=a&0x7FFF;
        return offset<=sizeof(m->iwram)-width?m->iwram+offset:NULL;
    case 8:case 9:case 10:case 11:case 12:case 13:
        offset=(a-HN_ROM_BASE)&0x1FFFFFF;
        if(offset<=0x2000000u-width&&offset<=m->rom_size&&width<=m->rom_size-offset)
            return m->rom+offset;
        return NULL;
    default:return NULL;
    }
}
uint16_t hn_read16(HNMemory *m, GameAddr a)
{
    const uint8_t *p=plain_read(m,a,2);
    if(p)return (uint16_t)(p[0]|(uint16_t)p[1]<<8);
    uint16_t low=hn_read8(m,a),high=hn_read8(m,a+1);
    return (uint16_t)(low|(high<<8));
}
uint32_t hn_read32(HNMemory *m, GameAddr a)
{
    const uint8_t *p=plain_read(m,a,4);
    if(p)return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
    uint32_t low=hn_read16(m,a),high=hn_read16(m,a+2);
    return low|(high<<16);
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
static void dma_control_written(HNMemory *m,unsigned channel)
{
    unsigned offset=0xB0+12*channel;
    m->dma_latched[channel]=(io32(m,offset+8)&0x80000000u)!=0;
    if(m->dma_latched[channel]){
        m->dma_source[channel]=io32(m,offset);
        m->dma_destination[channel]=io32(m,offset+4);
        if(!m->dma_active)hn_dma_trigger(m,0);
    }
}
void hn_write8(HNMemory *m, GameAddr a, uint8_t v)
{
    if(m->timers&&a>=0x04000100&&a<0x04000110){hn_timers_write(m->timers,a-0x04000000,v,1);return;}
    if(m->apu&&(a>>24)==4&&hn_apu_register(a&0xFFFFFF)){hn_apu_write(m->apu,a&0xFFFFFF,v,1);return;}
    if(a==0x04000006||a==0x04000007)return;
    if(a==0x04000004)v=(uint8_t)((v&~7u)|(m->io[4]&7));
    if(a==0x04000000)v&=0xF7;
    if(a==0x04000202||a==0x04000203){
        m->io[a-0x04000000]&=(uint8_t)~v;return;
    }
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
    if(a>=0x040000BB && a<=0x040000DF && (a-0x040000BB)%12==0)
        dma_control_written(m,(a-0x040000BB)/12);
}
void hn_write16(HNMemory *m, GameAddr a, uint16_t v)
{
    if(m->timers&&a>=0x04000100&&a<0x04000110){hn_timers_write(m->timers,a-0x04000000,v,2);return;}
    if(m->apu&&(a>>24)==4&&hn_apu_register(a&0xFFFFFF)){hn_apu_write(m->apu,a&0xFFFFFF,v,2);return;}
    if(a==0x04000000)v&=0xFFF7;
    if(a==0x04000006)return; /* VCOUNT is read-only. */
    if(a==0x04000004)v=(uint16_t)((v&~7u)|(m->io[4]&7));
    if (a == 0x04000202) { /* IF is write-one-to-clear. */
        m->io[0x202] &= (uint8_t)~v; m->io[0x203] &= (uint8_t)~(v>>8); return;
    }
    raw8(m,a,(uint8_t)v); raw8(m,a+1,(uint8_t)(v>>8));
    if(a>=0x040000BA && a<=0x040000DE && (a-0x040000BA)%12==0)
        dma_control_written(m,(a-0x040000BA)/12);
}
void hn_write32(HNMemory *m, GameAddr a, uint32_t v)
{
    if(m->apu&&(a>>24)==4&&hn_apu_register(a&0xFFFFFF)){hn_apu_write(m->apu,a&0xFFFFFF,v,4);return;}
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
static void dma_transfer(HNMemory *m,unsigned ch,unsigned timing,bool sound)
{
    unsigned o=0xB0+ch*12;
    uint32_t ctl=io32(m,o+8);
    if(!m->dma_latched[ch]){
        m->dma_source[ch]=io32(m,o);m->dma_destination[ch]=io32(m,o+4);m->dma_latched[ch]=true;
    }
    GameAddr src=m->dma_source[ch],dst=m->dma_destination[ch],initial_dst=io32(m,o+4);
    unsigned width=(ctl&(1u<<26))?4:2, count=ctl&(ch==3?0xFFFFu:0x3FFFu);
    unsigned sm=(ctl>>23)&3, dm=(ctl>>21)&3;
    if(sound){width=4;count=4;dm=2;}
    if (sm==3) { m->fault=true; m->fault_address=0x040000B0+ch*12; return; }
    if (!count) count=ch==3?65536:16384;
    src &= ~(width-1); dst &= ~(width-1);
    for (unsigned i=0;i<count && !m->fault;i++) {
        if (width==4) { uint32_t v=hn_read32(m,src); hn_write32(m,dst,v); }
        else { uint16_t v=hn_read16(m,src); hn_write16(m,dst,v); }
        if (sm==0) src+=width; else if(sm==1) src-=width;
        if (dm==0 || dm==3) dst+=width; else if(dm==1) dst-=width;
    }
    m->dma_source[ch]=src;
    m->dma_destination[ch]=dm==3?initial_dst:dst;
    if (ctl&(1u<<30)) {
        unsigned irq=1u<<(8+ch);
        m->io[0x202]|=(uint8_t)irq; m->io[0x203]|=(uint8_t)(irq>>8);
    }
    if (!(ctl&(1u<<25)) || timing==0){ctl&=~0x80000000u;m->dma_latched[ch]=false;}
    store_io32(m,o+8,ctl);
}
void hn_dma_trigger(HNMemory *m, unsigned timing)
{
    if (m->dma_active || timing>2) return;
    m->dma_active=true;
    for (unsigned ch=0;ch<4&&!m->fault;ch++) {
        uint32_t ctl=io32(m,0xB8+ch*12);
        if((ctl&0x80000000u)&&((ctl>>28)&3)==timing)dma_transfer(m,ch,timing,false);
    }
    m->dma_active=false;
}
void hn_dma_sound(HNMemory *m,unsigned fifo)
{
    if(m->dma_active||fifo>1)return;
    m->dma_active=true;
    for(unsigned ch=1;ch<=2&&!m->fault;ch++){
        unsigned o=0xB0+ch*12;uint32_t ctl=io32(m,o+8);
        GameAddr destination=m->dma_latched[ch]?m->dma_destination[ch]:io32(m,o+4);
        if((ctl&0xB0000000u)==0xB0000000u&&(destination&~3u)==0x040000A0+fifo*4)
            dma_transfer(m,ch,3,true);
    }
    m->dma_active=false;
}
