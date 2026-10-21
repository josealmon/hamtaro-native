#include "gba/bios.h"
#include <limits.h>
#include <math.h>
void hn_bios_obj_matrix(int16_t x,int16_t y,uint16_t angle,int16_t out[4])
{
    float radians=(float)(((angle>>8)/128.0f)*3.14159265358979323846);
    float sine=sinf(radians),cosine=cosf(radians),sx=x/256.0f,sy=y/256.0f;
    out[0]=(int16_t)(int32_t)((cosine*sx)*256.0f);
    out[1]=(int16_t)(int32_t)((-sine*sx)*256.0f);
    out[2]=(int16_t)(int32_t)((sine*sy)*256.0f);
    out[3]=(int16_t)(int32_t)((cosine*sy)*256.0f);
}
bool hn_bios_obj_affine(HNMemory *m,GameAddr source,GameAddr destination,uint32_t count,uint32_t stride)
{
    if(count>1024||stride>0x1000)return false;
    for(unsigned i=0;i<count&&!m->fault;i++){
        int16_t matrix[4];hn_bios_obj_matrix((int16_t)hn_read16(m,source),(int16_t)hn_read16(m,source+2),hn_read16(m,source+4),matrix);
        for(unsigned j=0;j<4;j++)hn_write16(m,destination+j*stride,(uint16_t)matrix[j]);
        source+=8;destination+=stride*4;
    }
    return !m->fault;
}
static bool copy_units(HNMemory *m, GameAddr src, GameAddr dst, uint32_t count, unsigned width, bool fill)
{
    src &= ~(width-1); dst &= ~(width-1);
    if (count > 0x100000u) return false;
    uint32_t value=0;
    if (fill && count) value=width==4?hn_read32(m,src):hn_read16(m,src);
    for (uint32_t i=0;i<count && !m->fault;i++) {
        if (!fill) value=width==4?hn_read32(m,src):hn_read16(m,src);
        if (width==4) hn_write32(m,dst,value); else hn_write16(m,dst,(uint16_t)value);
        if (!fill) src+=width;
        dst+=width;
    }
    return !m->fault;
}
bool hn_bios_cpu_set(HNMemory *m, GameAddr s, GameAddr d, uint32_t c)
{ return copy_units(m,s,d,c&0x1FFFFF,(c&(1u<<26))?4:2,(c&(1u<<24))!=0); }
bool hn_bios_cpu_fast_set(HNMemory *m, GameAddr s, GameAddr d, uint32_t c)
{ return copy_units(m,s,d,((c&0x1FFFFF)+7)&~7u,4,(c&(1u<<24))!=0); }
bool hn_bios_lz77(HNMemory *m, GameAddr source, GameAddr dst, size_t capacity)
{
    uint32_t header=hn_read32(m,source), size=header>>8, done=0;
    if (m->fault || (header&0xF0)!=0x10 || size>capacity || size>UINT32_MAX-dst) return false;
    source+=4;
    while(done<size && !m->fault) {
        uint8_t flags=hn_read8(m,source++);
        for(unsigned bit=0;bit<8 && done<size && !m->fault;bit++) {
            if (flags&(0x80u>>bit)) {
                unsigned hi=hn_read8(m,source++), lo=hn_read8(m,source++);
                unsigned length=(hi>>4)+3, distance=((hi&15)<<8)+lo+1;
                if (distance>done || length>size-done) return false;
                for(unsigned i=0;i<length;i++) {
                    uint8_t v=hn_read8(m,dst+done-distance);
                    /* Decompression targets RAM, or VRAM through halfword packing. */
                    GameAddr a=dst+done;
                    if ((a>>24)==5 || (a>>24)==6 || (a>>24)==7) {
                        uint16_t old=hn_read16(m,a&~1u);
                        uint16_t word=(a&1)?(uint16_t)((old&255)|(v<<8)):(uint16_t)((old&0xFF00)|v);
                        hn_write16(m,a&~1u,word);
                    } else hn_write8(m,a,v);
                    done++;
                }
            } else {
                uint8_t v=hn_read8(m,source++);
                GameAddr a=dst+done++;
                if ((a>>24)==5 || (a>>24)==6 || (a>>24)==7) {
                    uint16_t old=hn_read16(m,a&~1u);
                    hn_write16(m,a&~1u,(a&1)?(uint16_t)((old&255)|(v<<8)):(uint16_t)((old&0xFF00)|v));
                } else hn_write8(m,a,v);
            }
        }
    }
    return !m->fault;
}
int32_t hn_runtime_divide(int32_t n,int32_t d)
{
    if (!d) return 0;
    if(n==INT32_MIN && d==-1) return INT32_MIN;
    return n/d;
}
int32_t hn_runtime_remainder(int32_t n,int32_t d)
{
    if(!d || (n==INT32_MIN && d==-1)) return 0;
    return n%d;
}
