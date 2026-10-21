#include "game/save.h"
#include "game/heap.h"
typedef struct { GameAddr address;unsigned bytes; } Block;
/* Save layouts and addresses are metadata. Payloads always come from live RAM. */
static const Block group01[]={
    {0x03002C10,0x40},{0x02002EB0,0x270},{0x0201DC40,0x40},
    {0x03002180,8},{0x02004520,0x20},{0x0201CC20,0x20},{0x030037F0,0x56}
};
static const Block group2[]={
    {0x03002BB0,0x40},{0x03001FC0,0x1A0},{0x0201DC80,0x1C0},
    {0x03002188,0x58},{0x02004540,0x1E0},{0x0201CB20,0x100},
    {0x03002BA0,9},{0x03001F70,9}
};
static const Block group3[]={{0x0201EE40,0x20},{0x03003730,0x60},{0x03003C50,0x78}};
static const GameAddr tail01[]={0x03002BF0,0x03003E30,0x03002C58,0x03001F68,0x030021E0,0x03001F60,0x03001F90};
static const GameAddr tail3[]={0x030035CC,0x0300379C,0x03003D00};
static const unsigned sizes[]={0xEE,0x5CA,0x70A,0x66A,0x2BD4};
static const unsigned offsets[]={0,0xEE,0x6B8,0xDC2,0x142C};
enum { SAVE_BUFFER=0x02004720,RESULTS=0x03002168,IDENTITY=0x080000A0 };
static void wait_control(HNMemory *m)
{hn_write16(m,0x04000204,(uint16_t)((hn_read16(m,0x04000204)&0xFFFC)|3));}
bool hn_sram_init(HNMemory *m)
{
    /* Preserve the guest callback images as data. Native save calls use the
       typed adapters below and never execute these copied ARM instructions. */
    if(!hn_memory_copy(m,0x03001AA0,0x080EC104,0x40)||
       !hn_memory_copy(m,0x03001A00,0x080EC184,0x4C))return false;
    hn_write32(m,0x030051B0,0x03001AA1);hn_write32(m,0x030051B4,0x03001A01);
    wait_control(m);return !m->fault;
}
bool hn_sram_read(HNMemory *m,GameAddr source,GameAddr destination,uint32_t bytes)
{
    wait_control(m);if(bytes>HN_SRAM_SIZE)return false;
    for(unsigned i=0;i<bytes&&!m->fault;i++)hn_write8(m,destination+i,hn_read8(m,source+i));
    return !m->fault;
}
bool hn_sram_write(HNMemory *m,GameAddr source,GameAddr destination,uint32_t bytes)
{return hn_sram_read(m,source,destination,bytes);}
uint32_t hn_sram_verify(HNMemory *m,GameAddr source,GameAddr destination,uint32_t bytes)
{
    wait_control(m);if(bytes>HN_SRAM_SIZE)return destination;
    for(unsigned i=0;i<bytes&&!m->fault;i++)if(hn_read8(m,source+i)!=hn_read8(m,destination+i))return destination+i;
    return m->fault?destination:0;
}
static bool write_verified(HNMemory *m,GameAddr source,GameAddr destination,unsigned bytes)
{
    for(unsigned attempt=0;attempt<3&&!m->fault;attempt++){
        if(!hn_sram_write(m,source,destination,bytes))return false;
        if(!hn_sram_verify(m,source,destination,bytes))return true;
    }
    return false;
}
uint16_t hn_save_checksum(HNMemory *m,GameAddr source,uint32_t bytes)
{
    uint32_t sum=0;
    if(bytes>HN_SRAM_SIZE){m->fault=true;m->fault_address=source;return 0;}
    while(bytes>1&&!m->fault){sum+=hn_read16(m,source);source+=2;bytes-=2;}
    if(bytes)sum+=hn_read8(m,source);
    sum=(sum&65535)+(sum>>16);sum=(sum&65535)+(sum>>16);
    return (uint16_t)~sum;
}
static unsigned payload(HNMemory *m,unsigned group,GameAddr p,bool consume)
{
    const Block *blocks=NULL;unsigned count=0;
    if(group<=1){blocks=group01;count=sizeof(group01)/sizeof(*group01);}
    if(group==2){blocks=group2;count=sizeof(group2)/sizeof(*group2);}
    if(group==3){blocks=group3;count=sizeof(group3)/sizeof(*group3);}
    unsigned bytes=0;
    for(unsigned i=0;i<count;i++){
        GameAddr destination=consume?blocks[i].address:p+bytes,source=consume?p+bytes:blocks[i].address;
        hn_memory_copy(m,destination,source,blocks[i].bytes);
        if(group<=1&&i==3&&!consume)hn_write8(m,destination,hn_read8(m,destination)|8);
        bytes+=blocks[i].bytes;
    }
    if(group<=1||group==3){
        const GameAddr *tail=group<=1?tail01:tail3;unsigned length=group<=1?7:3;
        for(unsigned i=0;i<length;i++){
            if(consume)hn_write8(m,tail[i],hn_read8(m,p+bytes+i));
            else hn_write8(m,p+bytes+i,hn_read8(m,tail[i]));
        }
        bytes+=length;
    }
    if(group==4){
        if(!consume){hn_memory_copy(m,p,0x03003D30,128);bytes=128;}
        else{
            GameAddr destination=hn_read32(m,0x03002BB0);
            for(unsigned i=0;i<51&&!m->fault;i++)hn_memory_copy(m,destination+i*0xDC,p+8+i*0xDC,212);
            bytes=0x2BD4;
        }
    }
    return bytes;
}
bool hn_save_produce(HNMemory *m,unsigned group,GameAddr destination,uint32_t *size)
{
    if(group>=5)return false;
    *size=payload(m,group,destination,false);return !m->fault;
}
bool hn_save_consume(HNMemory *m,unsigned group,GameAddr source)
{
    if(group>=5)return false;
    payload(m,group,source,true);return !m->fault;
}
static GameAddr record(HNMemory *m,unsigned group,unsigned index,unsigned *length)
{
    GameAddr data=hn_heap_allocate(m,sizes[group],true);if(!data)return 0;
    unsigned bytes=payload(m,group,data+8,false);
    hn_write32(m,data,bytes);hn_write8(m,data+4,(uint8_t)group);hn_write8(m,data+5,(uint8_t)index);
    hn_write16(m,data+6,0);hn_write16(m,data+6,hn_save_checksum(m,data,sizes[group]));
    *length=bytes+8;return data;
}
bool hn_save_write_record(HNMemory *m,unsigned group,uint32_t index)
{
    if(!group||group>=5||index>50)return false;
    unsigned bytes;GameAddr data=record(m,group,index,&bytes);if(!data)return false;
    unsigned offset=offsets[group]+index*0xDC;
    bool ok=offset+bytes<=0x4000 && write_verified(m,data,0x0E000000+offset,bytes)&&
        write_verified(m,data,0x0E004000+offset,bytes);
    hn_heap_free(m,data);return ok&&!m->fault;
}
bool hn_save_write_groups(HNMemory *m)
{
    for(unsigned copy=0;copy<2;copy++){
        GameAddr base=0x0E000000+copy*0x4000;
        /* Marker writes precede all record construction for each SRAM half. */
        wait_control(m);
        for(unsigned group=1;group<=3;group++)hn_write32(m,base+offsets[group],1);
        for(unsigned group=1;group<=3;group++){
            unsigned bytes;GameAddr data=record(m,group,0,&bytes);if(!data)return false;
            bool ok=write_verified(m,data,base+offsets[group],bytes);hn_heap_free(m,data);
            if(!ok)return false;
        }
    }
    return !m->fault;
}
bool hn_save_load_group(HNMemory *m,unsigned group)
{
    if(!group||group>=5)return false;
    GameAddr data=hn_heap_allocate(m,sizes[group],true);if(!data)return false;
    hn_write8(m,RESULTS+group,0);
    unsigned chunks=group==4?51:1,width=group==4?0xDC:sizes[group];
    for(unsigned i=0;i<chunks;i++){
        unsigned offset=offsets[group]+i*0xDC;GameAddr destination=data+i*0xDC;
        if(!hn_sram_read(m,0x0E000000+offset,destination,width))break;
        unsigned result=1;
        if(hn_save_checksum(m,destination,width)){
            hn_sram_read(m,0x0E004000+offset,destination,width);
            result=hn_save_checksum(m,destination,width)?0:2;
        }
        hn_write8(m,RESULTS+group,(uint8_t)result);
        if(group==4&&!result)break;
    }
    bool ok=hn_save_consume(m,group,data+(group==4?0:8));
    hn_heap_free(m,data);return ok&&!m->fault;
}
bool hn_save_load_groups(HNMemory *m)
{
    unsigned outcome=0;
    for(unsigned copy=0;copy<2;copy++){
        unsigned failures=0;
        for(unsigned group=1;group<=3;group++){
            GameAddr data=hn_heap_allocate(m,sizes[group],true);if(!data)return false;
            hn_sram_read(m,0x0E000000+copy*0x4000+offsets[group],data,sizes[group]);
            if(hn_save_checksum(m,data,sizes[group]))failures++;
            hn_save_consume(m,group,data+8);hn_heap_free(m,data);
        }
        if(!failures){outcome=copy+1;break;}
    }
    for(unsigned i=1;i<=3;i++)hn_write8(m,RESULTS+i,(uint8_t)outcome);
    return !m->fault;
}
void hn_save_default_fields(HNMemory *m)
{
    hn_write8(m,0x030035CC,0);
    for(unsigned i=0;i<96;i++)hn_write8(m,0x03003730+i,255);
    for(unsigned i=0;i<6;i++)hn_write8(m,0x03003730+i*16,0);
    for(unsigned i=0;i<30;i++)hn_write8(m,0x03003C50+i*4,hn_read8(m,0x086E9100+hn_read8(m,0x086E9092+i)));
    hn_write8(m,0x03003D00,3);
}
static bool indexed_record(HNMemory *m,unsigned index)
{
    GameAddr p=0x03003D30;
    for(unsigned i=0;i<32;i++)hn_write32(m,p+i*4,0);
    if(index==50)hn_write8(m,p,1);
    else{
        unsigned column=index%5;hn_write8(m,p+3,(uint8_t)(column+3));hn_write8(m,p+4,1);
        for(unsigned i=5;i<=14;i++)hn_write8(m,p+i,1);
        if(!column){
            GameAddr text=hn_read32(m,0x086E41EC+hn_read8(m,0x0200460B)*4);
            unsigned i=0;
            for(;i<7&&hn_read8(m,text+i);i++)hn_write8(m,p+5+i,hn_read8(m,text+i));
            hn_write8(m,p+5+i,0);
            GameAddr resource=0x086E8CCC+(index/5)*28;
            hn_memory_copy(m,p+17,hn_read32(m,resource+8),hn_read8(m,resource+0x16));
            hn_write8(m,p,1);hn_write8(m,p+1,1);hn_write8(m,p+2,(uint8_t)i);
        }
    }
    return !m->fault;
}
bool hn_save_initialize(HNMemory *m)
{
    uint32_t primary=hn_sram_verify(m,IDENTITY,0x0E000000,16);
    uint32_t backup=hn_sram_verify(m,IDENTITY,0x0E004000,16);
    for(unsigned copy=0;copy<2;copy++)hn_memory_copy(m,SAVE_BUFFER+copy*0x4000,IDENTITY,16);
    if(!hn_read8(m,0x03002BF4)&&(!primary||!backup)){
        if(primary&&!backup)return write_verified(m,SAVE_BUFFER,0x0E000000,16);
        if(!primary&&backup)return write_verified(m,SAVE_BUFFER+0x4000,0x0E004000,16);
        return !m->fault;
    }
    for(unsigned copy=0;copy<2;copy++){
        GameAddr p=SAVE_BUFFER+copy*0x4000;
        /* The transient clearing marker is generated by protocol, not a resource. */
        hn_write8(m,p+9,'C');hn_write8(m,p+10,'l');hn_write8(m,p+11,'r');
        hn_memory_copy(m,p+16,hn_read32(m,0x02002EA8),11);
        hn_memory_copy(m,p+27,hn_read32(m,0x02002EAC),8);
    }
    if(!write_verified(m,SAVE_BUFFER,0x0E000000,0x8000))return false;
    for(unsigned group=1;group<=3;group++)if(!hn_save_write_record(m,group,0))return false;
    for(unsigned index=0;index<=50;index++)if(!indexed_record(m,index)||!hn_save_write_record(m,4,index))return false;
    hn_write8(m,0x03002BF4,0);
    for(unsigned i=0;i<0x2000;i++)hn_write32(m,SAVE_BUFFER+i*4,0);
    if(!write_verified(m,SAVE_BUFFER+9,0x0E000009,3)||
       !write_verified(m,SAVE_BUFFER+9,0x0E004009,3))return false;
    return !m->fault;
}
