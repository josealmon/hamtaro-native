#include "game/transfer.h"
#include "game/state.h"
static void zero_words(HNMemory *m,GameAddr address,unsigned count)
{for(unsigned i=0;i<count;i++)hn_write32(m,address+i*4,0);}
void hn_transfer_reset(HNMemory *m)
{
    zero_words(m,HN_TRANSFER_A,255);zero_words(m,HN_TRANSFER_B,255);zero_words(m,HN_TRANSFER_AUX,32);
    hn_write8(m,HN_TRANSFER_READ,0);hn_write8(m,HN_TRANSFER_WRITE,0);
    hn_write8(m,HN_CPU_TRANSFER_READ,0);hn_write8(m,HN_CPU_TRANSFER_WRITE,0);
    hn_write32(m,HN_TRANSFER_AUX_INDEX,0);
}
void hn_transfer_queue(HNMemory *m,GameAddr source,GameAddr destination,uint32_t bytes)
{
    if(!bytes)return;
    unsigned index=hn_read8(m,HN_TRANSFER_WRITE);GameAddr record=HN_TRANSFER_A+index*4;
    hn_write32(m,record,source);hn_write32(m,record+4,destination);hn_write32(m,record+8,bytes);
    hn_write8(m,HN_TRANSFER_WRITE,(uint8_t)(index==252?0:index+3));
}
void hn_transfer_queue_fill(HNMemory *m,uint32_t value,GameAddr destination,uint32_t bytes)
{
    unsigned index=hn_read32(m,HN_TRANSFER_AUX_INDEX);
    GameAddr word=HN_TRANSFER_AUX+index*4;hn_write32(m,word,value);
    hn_write32(m,HN_TRANSFER_AUX_INDEX,(index+1)&31);
    hn_transfer_queue(m,word|0x80000000u,destination,bytes);
}
void hn_transfer_copy(HNMemory *m,GameAddr source,GameAddr destination,uint32_t bytes,bool fill)
{
    bool word=((source|destination|bytes)&3)==0;
    hn_write32(m,0x040000D4,source);hn_write32(m,0x040000D8,destination);
    uint32_t control=word?0x84000000u:0x80000000u;
    if(fill)control|=0x01000000u;
    hn_write32(m,0x040000DC,control|(bytes>>(word?2:1)));
}
void hn_transfer_flush(HNMemory *m)
{
    if((hn_read16(m,0x04000006)&255)>219)return;
    unsigned budget=85;
    while(hn_read8(m,HN_TRANSFER_READ)!=hn_read8(m,HN_TRANSFER_WRITE)&&!m->fault&&budget--){
        unsigned index=hn_read8(m,HN_TRANSFER_READ);GameAddr record=HN_TRANSFER_A+index*4;
        GameAddr source=hn_read32(m,record),destination=hn_read32(m,record+4);
        uint32_t bytes=hn_read32(m,record+8);index+=3;
        hn_write8(m,HN_TRANSFER_READ,(uint8_t)(index==255?0:index));
        bool fill=(source&0x80000000u)!=0;
        hn_transfer_copy(m,source&0x7FFFFFFFu,destination,bytes,fill);
        if((hn_read16(m,0x04000006)&255)>219)return;
    }
}
