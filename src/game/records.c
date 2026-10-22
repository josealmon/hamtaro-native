#include "game/records.h"
#include "game/state.h"
#include "game/render.h"
#include "game/text.h"
#include "gba/bios.h"

static void count_store(HNMemory *m,GameAddr slot,unsigned count)
{hn_write16(m,slot,(uint16_t)((hn_read16(m,slot)&~0xF80u)|((count&31)<<7)));}
uint8_t hn_records_count(HNMemory *m)
{
    uint8_t count=0;
    for(unsigned i=0;i<64;i++)if((hn_read16(m,HN_RECORD_SLOTS+i*4)>>7)&31)count++;
    return count;
}
int32_t hn_record_allocate(HNMemory *m,uint16_t key)
{
    for(unsigned i=0;i<64;i++){
        GameAddr slot=HN_RECORD_SLOTS+i*4;
        if((hn_read16(m,slot)>>7)&31)continue;
        /* The native key assignment masks nine bits into a seven-bit field. */
        hn_write8(m,slot,(uint8_t)((hn_read8(m,slot)&128)|(key&127)));
        unsigned stored=hn_read8(m,slot)&127;
        count_store(m,slot,hn_read16(m,HN_RECORD_DESCRIPTORS+stored*4)>>11);return (int32_t)i;
    }
    return -1;
}
static void result_store(HNMemory *m,GameAddr result,unsigned descriptor,int32_t count)
{
    if(!result)return;
    hn_write8(m,result+4,(uint8_t)(descriptor&3));hn_write8(m,result+5,(uint8_t)((descriptor>>2)&127));
    hn_write8(m,result+6,(uint8_t)((descriptor>>11)&31));hn_write8(m,result+7,(uint8_t)count);
    GameAddr bank=hn_read32(m,HN_RECORD_BANKS+hn_read8(m,result+4)*4);
    hn_write32(m,result,hn_read32(m,bank+hn_read8(m,result+5)*4));
    hn_write8(m,result+8,(uint8_t)((descriptor>>9)&3));
}
int32_t hn_record_query(HNMemory *m,uint8_t index,GameAddr result)
{
    GameAddr slot=HN_RECORD_SLOTS+index*4;unsigned raw=hn_read16(m,slot),count=(raw>>7)&31;
    if(!count)return 0;
    unsigned descriptor=hn_read16(m,HN_RECORD_DESCRIPTORS+(raw&127)*4);
    result_store(m,result,descriptor,(int32_t)count);return (int32_t)count;
}
int32_t hn_record_consume(HNMemory *m,uint16_t index,uint8_t comparison,GameAddr result)
{
    GameAddr slot=HN_RECORD_SLOTS+index*4;unsigned raw=hn_read16(m,slot);
    unsigned descriptor=hn_read16(m,HN_RECORD_DESCRIPTORS+(raw&127)*4);int32_t count=(raw>>7)&31;
    if(!(hn_read8(m,0x030021B2)&2)){
        if((int32_t)((descriptor>>11)&31)-count>=comparison)return -1;
        count--;count_store(m,slot,(unsigned)count);
    }else{count_store(m,slot,0);count=0;}
    result_store(m,result,descriptor,count);return count;
}
uint32_t hn_record_adjust_counter(HNMemory *m,uint8_t index,int32_t delta)
{
    int32_t value=hn_read8(m,HN_RECORD_COUNTERS+index);
    value=value==255?delta:(int32_t)((uint32_t)value+(uint32_t)delta);
    if(value<0)value=0;
    else{
        unsigned maximum=hn_read8(m,HN_RECORD_COUNTER_DESCRIPTORS+index*20+5);
        if(value>(int32_t)maximum)value=(int32_t)maximum+128;
    }
    hn_write8(m,HN_RECORD_COUNTERS+index,(uint8_t)(value&127));return (((uint32_t)value>>7)&1)^1;
}
uint8_t hn_record_counter(HNMemory *m,uint8_t index)
{return index<=31?hn_read8(m,HN_RECORD_COUNTERS+index):255;}
static HNSceneResult allocate_batch(HNMemory *m,GameAddr arguments)
{
    hn_write8(m,0x03001138,hn_read8(m,arguments));hn_write8(m,0x03001139,hn_read8(m,arguments+1));
    uint16_t occupied=hn_records_count(m);hn_write16(m,0x0201DC56,occupied);
    uint16_t amount=hn_read16(m,0x0201DC4C);hn_write32(m,HN_COUNTER_COUNTDOWN,0);
    if(occupied>63){amount=0;hn_write8(m,0x03002182,(uint8_t)(hn_read8(m,0x03002182)&~8u));}
    else{
        if(64-occupied<amount)amount=(uint16_t)(64-occupied);
        for(unsigned index=0;index<amount;index++)
            (void)hn_record_allocate(m,hn_read16(m,0x0201DC40+(hn_read16(m,0x03001138)+index)*2));
        hn_write8(m,0x03002182,(uint8_t)(hn_read8(m,0x03002182)|8));
    }
    if(!hn_text_counter(m,occupied,1))return HN_SCENE_RESOURCE;
    if((uint16_t)(amount-1)<=30){
        hn_write32(m,HN_COUNTER_DELTA,1);hn_write32(m,HN_COUNTER_STEPS,amount);
        hn_write32(m,HN_COUNTER_DELAY,(uint32_t)hn_runtime_divide(32,amount));
    }else{
        hn_write32(m,HN_COUNTER_DELTA,(uint32_t)hn_runtime_divide(amount,32));
        hn_write32(m,HN_COUNTER_STEPS,32);hn_write32(m,HN_COUNTER_DELAY,1);
    }
    hn_write16(m,0x0201DC4C,amount);hn_write16(m,0x0201DC56,(uint16_t)(hn_read16(m,0x0201DC56)+amount));
    hn_write16(m,0x0201DC4E,hn_read16(m,0x0201DC56));
    if(!hn_render_configure_raw(m,2,1,0,176,-16,0,HN_COUNTER_ANIMATION))return HN_SCENE_RESOURCE;
    hn_render_flags(m,2,12,0);return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_records_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    switch(callback&~1u){
    case 0x0801E0BC:*consumed=2;return allocate_batch(m,arguments);
    case 0x0801DE14:
        hn_write8(m,0x03001138,hn_read8(m,arguments));hn_write8(m,0x03001139,hn_read8(m,arguments+1));*consumed=2;break;
    case 0x0801E00C:hn_write16(m,0x03001138,hn_read16(m,0x0201DC84));*consumed=0;break;
    default:return HN_SCENE_UNKNOWN;
    }
    hn_write16(m,0x0201DC56,hn_records_count(m));hn_write32(m,HN_COUNTER_COUNTDOWN,0);
    GameAddr flags=0x03002182;unsigned previous=hn_read8(m,flags);
    if(hn_record_allocate(m,hn_read16(m,0x03001138))<0){hn_write16(m,0x0201DC4C,0);hn_write8(m,flags,(uint8_t)(previous&~8u));}
    else{
        hn_write16(m,0x0201DC4C,1);hn_write8(m,flags,(uint8_t)(previous|8));
        hn_write32(m,HN_COUNTER_DELTA,1);hn_write32(m,HN_COUNTER_STEPS,1);hn_write32(m,HN_COUNTER_DELAY,1);
    }
    if(!hn_text_counter(m,hn_read16(m,0x0201DC56),1))return HN_SCENE_RESOURCE;
    hn_write16(m,0x0201DC56,(uint16_t)(hn_read16(m,0x0201DC56)+hn_read16(m,0x0201DC4C)));
    hn_write16(m,0x0201DC4E,hn_read16(m,0x0201DC56));
    if(!hn_render_configure(m,2,1,0,176,-16,0,HN_COUNTER_ANIMATION))return HN_SCENE_RESOURCE;
    hn_render_flags(m,2,12,0);return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
