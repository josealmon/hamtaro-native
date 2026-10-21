#include "game/world.h"
#include "game/state.h"
#include "game/heap.h"
#include "game/video.h"
#include "game/save.h"
static void zero_words(HNMemory *m,GameAddr p,unsigned count)
{for(unsigned i=0;i<count;i++)hn_write32(m,p+i*4,0);}
bool hn_runtime_tables_init(HNMemory *m)
{return hn_memory_copy(m,0x02000000,0x088FE018,0x08900EC8-0x088FE018);}
void hn_task_contexts_init(HNMemory *m)
{
    hn_write32(m,0x03000608,0x03000610);
    hn_write32(m,0x03000610+0x10,0x0858C6B0);
    hn_write32(m,0x03000610+0x28,0);
    hn_write32(m,0x03000610+0x2C,0x03000610);
    for(unsigned i=0;i<9;i++){
        hn_write8(m,0x03002BA0+i,hn_read8(m,0x0858E774+i));
        hn_write8(m,0x03001F70+i,hn_read8(m,0x0858E77D+i));
    }
}
void hn_core_storage_clear(HNMemory *m,int32_t selector)
{
    switch(selector){
    case 0:
        zero_words(m,0x03002180,0x18);zero_words(m,0x02004520,0x80);
        zero_words(m,0x0201DC40,0x80);zero_words(m,0x03002BB0,0x10);break;
    case 1:
        zero_words(m,0x03002188,0x16);zero_words(m,0x02004540,0x78);
        zero_words(m,0x0201DC80,0x38);zero_words(m,0x03002BB0,0x10);break;
    case 2:
        for(unsigned i=0;i<8;i++)hn_write16(m,0x030021D0+i*2,0);
        zero_words(m,0x020046A0,0x20);zero_words(m,0x0201DDC0,0x10);
        zero_words(m,0x03002BB0,0x10);break;
    }
}
void hn_core_reset(HNMemory *m)
{
    hn_save_default_fields(m);
    for(unsigned i=0;i<8;i++)hn_write32(m,0x0201CC20+i*4,UINT32_MAX);
    for(unsigned i=0;i<0x2B;i++)hn_write16(m,0x030037F0+i*2,0);
    hn_write8(m,0x03003E30,0);hn_core_storage_clear(m,0);
    zero_words(m,0x03000040,4);zero_words(m,0x03000050,0x165);
    zero_words(m,0x03001FC0,0x68);zero_words(m,0x030005F0,4);
    zero_words(m,0x03000600,1);zero_words(m,0x03000604,1);
    zero_words(m,0x0201CB20,0x40);zero_words(m,0x0201EE40,8);
    hn_write16(m,0x03001FBC,0);hn_write8(m,0x03001F94,0);
    hn_write8(m,0x03002160,0);hn_write8(m,0x03001F58,0);
    hn_write16(m,0x030021E4,0);hn_write16(m,0x03001F64,0);
}
bool hn_core_initialize(HNMemory *m)
{
    hn_core_reset(m);zero_words(m,0x02004720,0x6000);
    return hn_save_initialize(m)&&hn_save_load_group(m,2)&&hn_save_load_group(m,3)&&hn_map_initialize(m,0,0);
}
void hn_render_slot_reset(HNMemory *m,uint32_t index)
{
    GameAddr slot=hn_read32(m,HN_RENDER_SLOTS)+index*0x68;
    hn_write32(m,slot,0);hn_write32(m,slot+4,0);
    for(unsigned i=0;i<4;i++){hn_write32(m,slot+8+i*4,0);hn_write16(m,slot+0x56+i*2,0);}
    zero_words(m,slot+0x24,6);
    hn_write16(m,slot+0x3C,256);hn_write16(m,slot+0x3E,256);
    hn_write16(m,slot+0x40,0);hn_write16(m,slot+0x44,0);hn_write16(m,slot+0x46,0);
    hn_write16(m,slot+0x4A,0);hn_write16(m,slot+0x4C,0);
    hn_write16(m,slot+0x4E,240);hn_write16(m,slot+0x50,160);
    hn_write16(m,slot+0x52,0x1800);hn_write16(m,slot+0x54,0x2000);
    for(unsigned i=0;i<8;i++)hn_write8(m,slot+0x60+i,0);
    hn_write8(m,slot+0x64,255);hn_write8(m,slot+0x61,32);
}
int32_t hn_render_key(HNMemory *m,uint32_t index)
{
    GameAddr slots=hn_read32(m,HN_RENDER_SLOTS),slot=slots+index*0x68;
    uint32_t key=hn_read32(m,slot+0x20);
    if((hn_read8(m,slot+0x62)&192)==128){
        unsigned other=hn_read8(m,slot+0x65);
        key=(key&0xFFFF0000u)+hn_read32(m,slots+other*0x68+0x20);
    }
    return (int32_t)key;
}
void hn_render_order_init(HNMemory *m)
{
    GameAddr p=hn_read32(m,HN_RENDER_ORDER);unsigned count=hn_read8(m,HN_RENDER_INFO+0x10);
    for(unsigned i=0;i<count;i++)hn_write8(m,p+i,(uint8_t)i);
}
void hn_render_priority_init(HNMemory *m)
{
    GameAddr p=hn_read32(m,HN_RENDER_PRIORITY),slots=hn_read32(m,HN_RENDER_SLOTS);
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10);
    for(unsigned i=0;i<count;i++){
        hn_write32(m,p+i*8,(uint32_t)hn_render_key(m,i));
        hn_write8(m,p+i*8+4,(hn_read8(m,slots+i*0x68+0x61)&12)>>2);
        hn_write8(m,p+i*8+5,(uint8_t)i);
    }
}
void hn_render_reset(HNMemory *m)
{
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10);
    for(unsigned i=0;i<count;i++)hn_render_slot_reset(m,i);
    hn_render_order_init(m);hn_render_priority_init(m);
}
static void active_range(HNMemory *m,uint8_t first,uint8_t last)
{
    unsigned count=hn_read8(m,HN_RENDER_INFO+0x10);
    if(!count || first>=count-1 || last>=count || first>=last){
        unsigned index=hn_read32(m,0x03002C78);
        hn_write16(m,0x03002DE0+index*2,4);hn_write32(m,0x03002C78,(index+1)&7);
    }else{
        hn_write8(m,HN_RENDER_INFO+0x11,first);hn_write8(m,HN_RENDER_INFO+0x12,last);
    }
}
void hn_background_state_init(HNMemory *m)
{
    zero_words(m,hn_read32(m,HN_RENDER_STATES),hn_read8(m,HN_BACKGROUND_COUNT)*20);
    zero_words(m,HN_LAYER_OFFSETS,44);
    for(unsigned i=0;i<4;i++){
        GameAddr p=HN_LAYER_OFFSETS+i*44;
        hn_write16(m,p+0x18,256);hn_write16(m,p+0x1A,256);
        hn_write16(m,p+0x22,256);hn_write16(m,p+0x28,256);
    }
}
void hn_background_layers_reset(HNMemory *m,uint8_t mask)
{
    for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){
        GameAddr p=HN_LAYER_OFFSETS+i*44;
        zero_words(m,p,6);
        hn_write16(m,p+0x18,256);hn_write16(m,p+0x1A,256);
        hn_write16(m,p+0x1C,0);hn_write16(m,p+0x1E,0);hn_write16(m,p+0x20,0);
        for(unsigned j=0;j<4;j++)hn_write16(m,p+0x22+j*2,256);
    }
}
bool hn_map_initialize(HNMemory *m,uint8_t bank,uint8_t index)
{
    hn_write8(m,HN_MAP_BANK,bank);hn_write8(m,0x0200452D,bank);
    hn_write8(m,HN_MAP_INDEX,index);hn_write8(m,0x0200452E,index);
    GameAddr table=hn_read32(m,HN_MAP_BANKS+bank*4),descriptor=table+index*16;
    GameAddr words=hn_read32(m,descriptor+4);
    unsigned word_count=words?hn_read32(m,words):0;
    if(word_count>0x4000 || m->fault)return false;
    unsigned render_count=hn_read8(m,descriptor+8);
    hn_write8(m,HN_BACKGROUND_COUNT,hn_read8(m,descriptor+10));
    hn_write8(m,HN_GRAPHICS_COUNT,hn_read8(m,descriptor+11));
    hn_write32(m,0x03002C60,hn_read32(m,descriptor));
    hn_write32(m,0x03000014,0);hn_write8(m,0x03000010,0);
    GameAddr p=hn_read32(m,HN_GRAPHICS_SCRIPTS);if(p)hn_heap_free(m,p);
    p=hn_read32(m,HN_RENDER_STATES);if(p)hn_heap_free(m,p);
    p=hn_read32(m,HN_RENDER_PRIORITY);if(p)hn_heap_free(m,p);
    p=hn_read32(m,HN_RENDER_ORDER);if(p)hn_heap_free(m,p);
    p=hn_read32(m,HN_RENDER_SLOTS);if(p)hn_heap_free(m,p);
    GameAddr slots=hn_heap_allocate(m,render_count*0x68,true);
    GameAddr order=hn_heap_allocate(m,render_count,false);
    GameAddr priority=hn_heap_allocate(m,render_count*8,false);
    if(!slots||!order||!priority)return false;
    hn_write32(m,HN_RENDER_SLOTS,slots);hn_write32(m,HN_RENDER_ORDER,order);
    hn_write32(m,HN_RENDER_PRIORITY,priority);
    hn_write16(m,HN_RENDER_INFO+8,0x1C0);hn_write16(m,HN_RENDER_INFO+10,0xC0);
    hn_write32(m,HN_RENDER_INFO,0x400000);hn_write32(m,HN_RENDER_INFO+4,0x400000);
    hn_write16(m,HN_RENDER_INFO+12,0x69);hn_write16(m,HN_RENDER_INFO+14,0x41);
    hn_write8(m,HN_RENDER_INFO+16,(uint8_t)render_count);
    active_range(m,8,hn_read8(m,descriptor+9));hn_render_reset(m);
    zero_words(m,HN_BACKGROUND_BOUNDS,12);
    p=hn_heap_allocate(m,hn_read8(m,HN_BACKGROUND_COUNT)*80,false);
    if(!p)return false;
    hn_write32(m,HN_RENDER_STATES,p);hn_background_state_init(m);
    p=hn_heap_allocate(m,hn_read8(m,HN_GRAPHICS_COUNT)*44,true);if(!p)return false;
    hn_write32(m,HN_GRAPHICS_SCRIPTS,p);hn_video_blend_targets(m,255,0);
    for(unsigned i=0;i<word_count;i++)hn_write32(m,HN_MAP_WORDS+i*4,hn_read32(m,words+4+i*4));
    if(hn_read8(m,descriptor+12))hn_write8(m,0x03002C0C,hn_read8(m,descriptor+12));
    if(hn_read8(m,descriptor+13))hn_write8(m,0x03001F50,hn_read8(m,descriptor+13));
    if(hn_read8(m,descriptor+14))hn_write8(m,0x03002C04,hn_read8(m,descriptor+14));
    if(hn_read8(m,descriptor+15))hn_write8(m,0x03002C00,hn_read8(m,descriptor+15));
    return !m->fault;
}
