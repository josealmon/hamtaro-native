#include "game/pair_list.h"
#include "game/state.h"
#include "game/render.h"
#include "game/world.h"
#include "game/save_group3.h"
#include "game/transfer.h"
#include "game/audio.h"
#include "gba/bios.h"

/* Guest layouts and table locations; all original graphics remain in the ROM. */
enum {
    SOURCES=0x02002ADC,ENTRIES=0x03001170,COUNT=0x03001168,SELECTIONS=0x03001188,
    ROW_GRAPHICS=0x02002AF4,HEADER=0x08577918,FOOTER=0x08577924,
    SLOT_X=0x086E97C8,ICON_POSITIONS=0x086E97E2,SLOT_ANIMATIONS=0x02002D4C,
    COMPLETED=0x088FCB52,REMAINING=0x088FCB42,SEPARATOR=0x088FCB0A,DIGIT=0x088FCAF2,
    FIELDS=0x03003C50,FIELD_COUNT=0x030035CC,DEFAULT_INDICES=0x086E9092,
    PALETTES=0x02002418,PAIR_ANIMATIONS=0x02002248,CLASSES=0x0201DC40,
    ROW_TILES=0x02013F20
};
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static uint8_t variant_mask(unsigned variant){return variant<32?(uint8_t)(128u>>variant):0;}
static uint8_t palette_offset(HNMemory *m,uint8_t category)
{
    uint8_t offset=0;
    for(unsigned i=category;i;i--)offset=(uint8_t)(offset+hn_read8(m,DEFAULT_INDICES+i-1));
    return offset;
}
static bool slot(HNMemory *m,uint8_t index,int x,int y,GameAddr animation)
{return hn_render_configure_raw(m,index,1,0x71,x,y,0,animation);}
static void priority(HNMemory *m,unsigned index){hn_render_flags(m,index,12,4);}
static void reset_range(HNMemory *m,unsigned first,unsigned last)
{for(unsigned i=first;i<=last;i++)hn_render_slot_reset(m,i);}

uint8_t hn_pair_list_classify(HNMemory *m,uint32_t category_word,uint32_t mask_word)
{
    uint8_t mask=(uint8_t)mask_word,value=hn_read8(m,FIELDS+(uint8_t)category_word*4)&mask;
    return !value?0:value==mask?2:1;
}
bool hn_pair_list_cursor(HNMemory *m,uint8_t index)
{
    GameAddr cursor=hn_render_slot(m,31);uint32_t row=(hn_read8(m,ENTRIES+index*4+1)&7u)-1;
    unsigned x=hn_read8(m,SLOT_X+row*5+hn_read8(m,SELECTIONS+index));
    hn_write32(m,cursor+0x18,x<<16);return !m->fault;
}
bool hn_pair_list_first_unset(HNMemory *m,uint8_t index)
{
    GameAddr entry=ENTRIES+index*4;unsigned variant=0;
    while(variant<=4&&hn_pair_list_classify(m,hn_read8(m,entry),128u>>variant))variant++;
    hn_write8(m,SELECTIONS+index,(uint8_t)(variant<=4?variant:0));return !m->fault;
}
bool hn_pair_list_highlight(HNMemory *m,uint8_t index,uint8_t selected)
{
    GameAddr tiles=ROW_TILES+selected*88;uint16_t tile=(uint16_t)((selected<<13)+0x7001+index*22);
    for(unsigned i=0;i<22;i++)hn_write16(m,tiles+i*2,tile++);
    hn_transfer_tilemap_rows(m,tiles,0x0600C000+((uint32_t)index<<7)+66,11,2,0);return !m->fault;
}
bool hn_pair_list_variation_palette(HNMemory *m,uint8_t category,uint8_t variation)
{
    uint8_t offset=palette_offset(m,category);
    hn_transfer_queue(m,hn_read32(m,PALETTES+((uint32_t)offset+variation)*4)+30,0x05000384,2);return !m->fault;
}
bool hn_pair_list_palette_slots(HNMemory *m)
{
    GameAddr entry=ENTRIES+control(m,0x60)*4;uint8_t category=hn_read8(m,entry);
    unsigned count=hn_read8(m,entry+1)&7,index=0,render_index=26;
    GameAddr positions=SLOT_X+((uint32_t)count-1)*5,palettes=PALETTES+palette_offset(m,category)*4;
    for(;index<count;index++,render_index++){
        if(!slot(m,(uint8_t)render_index,hn_read8(m,positions++),15,hn_read32(m,SLOT_ANIMATIONS+index*4)))return false;
        hn_transfer_queue(m,hn_read32(m,palettes)+30,0x05000384+index*2,2);palettes+=4;
        priority(m,render_index);
    }
    for(;index<=4;index++,render_index++)hn_write8(m,hn_render_slot(m,render_index)+0x60,0);
    return !m->fault;
}
bool hn_pair_list_remaining(HNMemory *m)
{
    reset_range(m,13,17);
    if(!hn_render_number_raw(m,13,hn_read8(m,FIELD_COUNT),184,20)||!slot(m,15,208,20,REMAINING))return false;
    priority(m,15);if(!slot(m,16,216,20,SEPARATOR))return false;priority(m,16);
    if(!slot(m,17,224,20,DIGIT))return false;priority(m,17);return !m->fault;
}
bool hn_pair_list_value(HNMemory *m)
{reset_range(m,18,20);return hn_render_number_raw(m,18,hn_read16(m,CLASSES+9*2),200,56);}
bool hn_pair_list_selected_value(HNMemory *m)
{
    reset_range(m,21,23);uint16_t value=hn_read16(m,ENTRIES+control(m,0x60)*4+2);
    hn_write16(m,CLASSES+34*2,value);return hn_render_number_raw(m,21,hn_read16(m,CLASSES+34*2),200,92);
}
bool hn_pair_list_icon(HNMemory *m)
{
    uint8_t category=hn_read8(m,ENTRIES+control(m,0x60)*4);
    unsigned x=hn_read16(m,ICON_POSITIONS+category*4),y=hn_read16(m,ICON_POSITIONS+category*4+2);
    hn_render_slot_reset(m,12);if(!hn_group3_pair_graphics(m,category,0))return false;
    if(!slot(m,12,(int16_t)x,(int16_t)y,hn_read32(m,PAIR_ANIMATIONS+(category%3)*4)))return false;
    priority(m,12);return !m->fault;
}
bool hn_pair_list_display(HNMemory *m,uint8_t index,uint8_t mask)
{
    GameAddr entry=ENTRIES+index*4;
    if(hn_pair_list_classify(m,hn_read8(m,entry),mask)==2){if(!slot(m,12,108,64,COMPLETED))return false;}
    else{
        if(!hn_pair_list_icon(m)||!hn_group3_pair_palette(m,hn_read8(m,entry),hn_read8(m,SELECTIONS+index),0,0))return false;
    }
    return hn_pair_list_palette_slots(m)&&hn_pair_list_selected_value(m)&&hn_pair_list_cursor(m,index);
}
static GameAddr list_source(HNMemory *m)
{return hn_read32(m,SOURCES+((uint32_t)control(m,0x21)-1)*4);}
static bool prepare(HNMemory *m)
{
    GameAddr source=list_source(m),destination=ENTRIES;hn_write32(m,COUNT,0);
    for(unsigned visited=0;;visited++){
        if(m->fault||visited==65536)return false;
        uint8_t category=hn_read8(m,source);if(category==255)break;
        hn_write8(m,destination,category);hn_write16(m,destination+2,hn_read16(m,source+2));
        GameAddr flags=FIELDS+hn_read8(m,source)*4;
        hn_write8(m,destination+1,(uint8_t)((hn_read8(m,source+1)&7)|((hn_read8(m,flags)|hn_read8(m,flags+1))&0xF8)));
        destination+=4;source+=4;hn_write32(m,COUNT,hn_read32(m,COUNT)+1);
    }
    for(unsigned i=0;i<6;i++)hn_write8(m,SELECTIONS+i,0);
    hn_write8(m,HN_BLEND,0x50);hn_write8(m,HN_BLEND+1,(uint8_t)((hn_read8(m,HN_BLEND+1)&0xC0)|1));
    hn_write8(m,HN_BLEND+2,4);hn_write8(m,HN_BLEND+3,8);hn_write8(m,HN_BLEND+4,0);
    store(m,0x60,0);return !m->fault;
}
static bool render(HNMemory *m)
{
    if(!hn_bios_lz77(m,HEADER,0x0600C002,0x40000))return false;
    GameAddr entry=ENTRIES;uint32_t remaining=hn_read32(m,COUNT),row=33,graphics=32;uint16_t tile=0x7001;
    if(remaining>65536)return false;
    while(remaining--){
        uint32_t offset=hn_read8(m,entry)*4+control(m,0xEB)*120;
        if(!hn_bios_lz77(m,hn_read32(m,ROW_GRAPHICS+offset),0x06004000+graphics,0x40000))return false;
        for(unsigned y=0;y<2;y++){
            GameAddr front=0x0600C000+row*2,back=0x0600E000+row*2;
            for(unsigned x=0;x<11;x++){hn_write16(m,front+x*2,tile++);hn_write16(m,back+x*2,(uint16_t)(0x9115+y));}
            row+=32;
        }
        entry+=4;graphics+=704;
    }
    if(!hn_bios_lz77(m,FOOTER,0x0600C000+row*2,0x40000)||!hn_pair_list_first_unset(m,control(m,0x60))||!hn_pair_list_highlight(m,0,1))return false;
    return hn_pair_list_display(m,control(m,0x60),variant_mask(hn_read8(m,SELECTIONS+control(m,0x60))))&&hn_pair_list_remaining(m)&&hn_pair_list_value(m);
}
static bool move_entry(HNMemory *m,bool next)
{
    if(next?control(m,0x60)<hn_read32(m,COUNT)-1:control(m,0x60)!=0){
        if(!hn_pair_list_highlight(m,control(m,0x60),0))return false;
        GameAddr cursor=hn_render_slot(m,25);store(m,0x60,control(m,0x60)+(next?1:255));
        if(!hn_pair_list_highlight(m,control(m,0x60),1))return false;
        hn_write32(m,cursor+0x1C,hn_read32(m,cursor+0x1C)+(next?0x100000:0xFFF00000));
        if(!hn_pair_list_first_unset(m,control(m,0x60))||!hn_pair_list_display(m,control(m,0x60),variant_mask(hn_read8(m,SELECTIONS+control(m,0x60)))))return false;
        return hn_audio_song(m,302,HN_SONG_START);
    }
    return !m->fault;
}
static bool move_variant(HNMemory *m,bool next)
{
    uint8_t index=control(m,0x60);GameAddr selected=SELECTIONS+index;unsigned variant=hn_read8(m,selected);
    int count=hn_read8(m,ENTRIES+index*4+1)&7;
    if(next?(int)variant<count-1:variant!=0){
        hn_write8(m,selected,(uint8_t)(variant+(next?1:255)));
        if(!hn_pair_list_display(m,control(m,0x60),variant_mask(hn_read8(m,selected))))return false;
        return hn_audio_song(m,302,HN_SONG_START);
    }
    return !m->fault;
}
static bool complete(HNMemory *m)
{
    GameAddr source=list_source(m);store(m,0x61,0);
    for(unsigned visited=0;;visited++,source+=4){
        if(m->fault||visited==65536)return false;
        uint8_t category=hn_read8(m,source);if(category==255)break;
        if(hn_pair_list_classify(m,category,0xF8)!=2){store(m,0x61,1);break;}
    }
    return !m->fault;
}
#define REQUIRE(operation) do {if(!(operation))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_pair_list_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080CD654:REQUIRE(prepare(m));break;
    case 0x080CD734:REQUIRE(render(m));break;
    case 0x080CD854:REQUIRE(move_entry(m,false));break;
    case 0x080CD8C0:REQUIRE(move_entry(m,true));break;
    case 0x080CD938:REQUIRE(move_variant(m,false));break;
    case 0x080CD974:REQUIRE(move_variant(m,true));break;
    case 0x080CD9C0:
        if(hn_read16(m,CLASSES+192*2)){
            hn_write16(m,CLASSES+192*2,(uint16_t)(hn_read16(m,CLASSES+192*2)-1));
            hn_write16(m,CLASSES+9*2,(uint16_t)(hn_read16(m,CLASSES+9*2)-1));REQUIRE(hn_pair_list_value(m));
        }break;
    case 0x080CD9EC:REQUIRE(hn_pair_list_remaining(m));break;
    case 0x080CD9F8:{
        unsigned index=control(m,0x60);GameAddr flags=FIELDS+hn_read8(m,ENTRIES+index*4)*4;
        hn_write8(m,flags,(uint8_t)(variant_mask(hn_read8(m,SELECTIONS+index))|hn_read8(m,flags)));
        REQUIRE(slot(m,12,108,64,COMPLETED));break;
    }
    case 0x080CDA54:
        REQUIRE(hn_pair_list_first_unset(m,control(m,0x60)));
        REQUIRE(hn_pair_list_display(m,control(m,0x60),variant_mask(hn_read8(m,SELECTIONS+control(m,0x60)))));break;
    case 0x080CDA84:{
        unsigned index=control(m,0x60);store(m,0x61,hn_pair_list_classify(m,hn_read8(m,ENTRIES+index*4),variant_mask(hn_read8(m,SELECTIONS+index))));break;
    }
    case 0x080CDABC:store(m,0x61,hn_pair_list_classify(m,hn_read8(m,ENTRIES+control(m,0x60)*4),0xF8));break;
    case 0x080CDAE4:REQUIRE(hn_pair_list_palette_slots(m));break;
    case 0x080CDAF0:store(m,1,30-hn_read8(m,FIELD_COUNT));break;
    case 0x080CDB08:REQUIRE(complete(m));break;
    case 0x080CDD7C:
        REQUIRE(hn_render_number_raw(m,hn_read8(m,arguments),control(m,0x60),hn_read8(m,arguments+1),hn_read8(m,arguments+2)));*consumed=3;break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_pair_list_task(HNMemory *m,GameAddr callback)
{
    GameAddr animation=0;
    switch(callback&~1u){
    case 0x080CD594:animation=0x088FCA21;break;
    case 0x080CD5A8:animation=0x088FCA29;break;
    case 0x080CD5C8:animation=0x088FCA3D;break;
    case 0x080CD5DC:animation=0x088FCA59;break;
    case 0x080CD5F0:animation=0x088FCA70;break;
    case 0x080CD604:animation=0x088FCA8C;break;
    case 0x080CD618:animation=0x088FCAB7;break;
    case 0x080CD62C:animation=0x088FCACE;break;
    case 0x080CD640:animation=0x088FCAE5;break;
    case 0x080CD654:case 0x080CD734:case 0x080CD854:case 0x080CD8C0:case 0x080CD938:
    case 0x080CD974:case 0x080CD9C0:case 0x080CD9EC:case 0x080CD9F8:case 0x080CDA54:
    case 0x080CDA84:case 0x080CDABC:case 0x080CDAE4:case 0x080CDAF0:case 0x080CDB08:{
        uint32_t ignored;return hn_pair_list_callback(m,callback,0,&ignored);
    }
    default:return HN_SCENE_UNKNOWN;
    }
    hn_render_set_animation(m,24,animation);
    if((callback&~1u)==0x080CD5A8){hn_write8(m,0x03001FAC,16);hn_write32(m,0x03001F7C,animation);}
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
uint32_t hn_pair_list_return_zero(void){return 0;}
