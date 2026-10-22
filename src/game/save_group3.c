#include "game/save_group3.h"
#include "game/save.h"
#include "game/state.h"
#include "game/render.h"
#include "game/transfer.h"
#include "game/graphics.h"
#include "game/background.h"
#include "game/video.h"
#include "game/input.h"
#include "game/world.h"
#include "game/heap.h"
#include "game/task.h"
#include "gba/bios.h"

enum {
    FIELDS=0x03003C50,LIST=0x0201EE40,COUNTS=0x0300357C,
    DEFAULT_INDICES=0x086E9092,DEFAULT_VALUES=0x086E9100,
    POSITIONS=0x086E912A,ALTERNATE_POSITIONS=0x086E91DE,
    LOOKUP_VALUES=0x086E93A2,DEFAULT_ANIMATION=0x088FB21F,
    PAIR_ANIMATIONS=0x02002248,GRAPHICS_BUFFERS=0x02002230,
    GRAPHICS_SOURCES=0x02002260,GRAPHICS_DIMENSIONS=0x086E908C,
    GRAPHICS_DESTINATIONS=0x086E9074,PAIR_PALETTES=0x02002418,
    NUMBER_ANIMATIONS=0x02002208,SELECTED_NUMBERS=0x02002AB4,
    CATALOG0=0x03003A50,CATALOG1=0x03003850,CATALOG2=0x03003680,
    CATALOGS=0x02002780,SELECTED=0x03003790,PENDING=0x03003DC0,
    RECORDS=0x03003730,FIELD_COUNT=0x030035CC,KNOWN_MASK=0x03003D00,
    PAIR_FLAGS=0x086E911A,SLIDE_VELOCITIES=0x086E9126,
    INDICATOR_X=0x086E910E,INDICATOR_Y=0x086E9106,INDICATOR_ANIMATIONS=0x02002758,
    ROW_X=0x086E9112,COMPLETED_BITS=0x086E94C2,
    LIST_CELLS=0x02001C32,LIST_BLANK_TILES=0x02004F3A,
    LIST_DESCRIPTORS=0x0858D344,LIST_TILE_INDICES=0x086E8218,
    LIST_HEADER_ANIMATION=0x088FA619,LIST_CURSOR_ANIMATION=0x088FA387,
    LIST_GRID_ANIMATION=0x088FA3EC,LIST_ITEM_ANIMATIONS=0x02001CF4,
    LIST_CATEGORY_ANIMATIONS=0x02001D1C,LIST_TEXT_BUNDLES=0x02001D98,
    LIST_NUMBER_FIELDS=0x0858D34E,LIST_STYLE_FRAMES=0x02001C08
};
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static bool flag(HNMemory *m,unsigned id){return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void set_flag(HNMemory *m,unsigned id,bool value)
{
    GameAddr address=0x03002180+(id>>3);uint8_t mask=(uint8_t)(1u<<(id&7));
    hn_write8(m,address,(uint8_t)((hn_read8(m,address)&~mask)|(value?mask:0)));
}
static void place(HNMemory *m,uint32_t index,uint32_t x,uint32_t y,uint32_t z)
{
    GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x18,x);hn_write32(m,slot+0x1C,y);hn_write32(m,slot+0x20,z);
}
static void vector(HNMemory *m,uint32_t index,uint32_t x,uint32_t y,uint32_t z)
{
    GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x24,x);hn_write32(m,slot+0x28,y);hn_write32(m,slot+0x2C,z);
}
static uint32_t lsl(uint32_t value,uint32_t count){count&=255;return count<32?value<<count:0;}
static uint32_t lsr(uint32_t value,uint32_t count){count&=255;return count<32?value>>count:0;}
static GameAddr resource(HNMemory *m,GameAddr table,uint32_t index)
{return hn_read32(m,table+index*4);}
static void priority(HNMemory *m,uint32_t slot,uint8_t value)
{hn_render_flags(m,slot,12,(uint8_t)(value<<2));}

bool hn_group3_pair_graphics(HNMemory *m,uint8_t category,uint32_t bank_word)
{
    if(category>109)return !m->fault;
    unsigned part=category<=29?category%3:category<=50?0:category<=67?1:2;
    uint8_t index=(uint8_t)((uint8_t)bank_word*3+part);
    GameAddr destination=resource(m,GRAPHICS_BUFFERS,index);
    uint32_t tiles=hn_read32(m,GRAPHICS_DESTINATIONS+index*4);
    uint8_t width=hn_read8(m,GRAPHICS_DIMENSIONS+part*2),height=hn_read8(m,GRAPHICS_DIMENSIONS+part*2+1);
    if(!hn_bios_lz77(m,resource(m,GRAPHICS_SOURCES,category),destination,0x40000))return false;
    hn_transfer_tiles(m,destination,tiles,width,height);return !m->fault;
}
bool hn_group3_pair_animation(HNMemory *m,uint32_t slot,uint8_t category,uint32_t bank_word)
{
    uint8_t bank=(uint8_t)bank_word;
    if(!hn_group3_pair_graphics(m,category,bank))return false;
    GameAddr animation=DEFAULT_ANIMATION;
    if(category<=109){
        unsigned part=category<=29?category%3:category<=50?0:category<=67?1:2;
        animation=resource(m,PAIR_ANIMATIONS,bank*3+part);
    }
    hn_render_set_animation(m,slot,animation);return !m->fault;
}
bool hn_group3_pair_palette(HNMemory *m,uint8_t category,uint8_t variation,uint8_t unused,uint32_t bank_word)
{
    (void)unused;uint8_t offset=0,bank=(uint8_t)bank_word;
    for(unsigned remaining=category;remaining;remaining--)offset=(uint8_t)(offset+hn_read8(m,DEFAULT_INDICES+remaining-1));
    unsigned palette=category<=29?category%3:category<=50?0:category<=67?1:2;
    hn_transfer_queue(m,resource(m,PAIR_PALETTES,(uint32_t)offset+variation),0x05000280+bank*96+palette*32,32);
    return !m->fault;
}
bool hn_group3_number(HNMemory *m,uint32_t slot,uint8_t value,uint8_t x,uint8_t y)
{
    if(!hn_render_configure_raw(m,(uint8_t)slot,1,0x74,x,y,0,DEFAULT_ANIMATION))return false;
    unsigned tens=value/10;if(tens)hn_render_set_animation(m,slot,resource(m,NUMBER_ANIMATIONS,tens));
    uint32_t second=slot+1;
    if(!hn_render_configure_raw(m,(uint8_t)second,1,0x74,(int32_t)x+8,y,0,resource(m,NUMBER_ANIMATIONS,value%10)))return false;
    priority(m,slot,1);priority(m,second,1);return !m->fault;
}
bool hn_group3_counts(HNMemory *m)
{
    for(unsigned i=0;i<=5;i++){
        GameAddr slot=hn_render_slot(m,15+i);
        hn_write32(m,slot+0x18,((i&1)*8+0x88)<<16);hn_write32(m,slot+0x1C,((i>>1)*3<<19)+0x160000);hn_write32(m,slot+0x20,0);
        priority(m,15+i,1);
    }
    for(unsigned i=0;i<=2;i++){
        unsigned slot=15+i*2;hn_render_set_animation(m,slot,DEFAULT_ANIMATION);
        uint8_t value=hn_read8(m,COUNTS+i);unsigned tens=value/10;
        if(tens)hn_render_set_animation(m,slot,resource(m,NUMBER_ANIMATIONS,tens));
        hn_render_set_animation(m,slot+1,resource(m,NUMBER_ANIMATIONS,value%10));
    }
    return !m->fault;
}
bool hn_group3_selected_count(HNMemory *m)
{
    int32_t group=(int32_t)hn_read8(m,HN_SCENE_CONTROLS+0x70)-1;
    uint32_t twice=(uint32_t)(group*2),slot=twice+15;
    hn_render_set_animation(m,slot,DEFAULT_ANIMATION);
    uint8_t value=hn_read8(m,COUNTS+(uint32_t)group);unsigned tens=value/10;
    if(tens)hn_render_set_animation(m,slot,resource(m,SELECTED_NUMBERS,tens));
    hn_render_set_animation(m,twice+16,resource(m,SELECTED_NUMBERS,value%10));
    uint32_t y=(twice+(uint32_t)group)*8+24;
    for(unsigned i=0;i<=1;i++,slot++){
        GameAddr actor=hn_render_slot(m,slot);
        hn_write32(m,actor+0x18,((i&1)*8+0x88)<<16);hn_write32(m,actor+0x1C,y<<16);hn_write32(m,actor+0x20,0);priority(m,slot,0);
    }
    return !m->fault;
}
void hn_group3_field_bit(HNMemory *m,uint8_t field,uint8_t bit,uint8_t value)
{
    GameAddr address=FIELDS+field*4;uint32_t mask=bit<32?0x80u>>bit:0;
    hn_write8(m,address,(uint8_t)(value?hn_read8(m,address)|mask:hn_read8(m,address)&~mask));
}
void hn_group3_remove_list(HNMemory *m,uint8_t index)
{
    for(;index<=30;index++)hn_write8(m,LIST+index,hn_read8(m,LIST+index+1));
    hn_write8(m,LIST+31,0);
}
bool hn_group3_remove_lookup(HNMemory *m,uint8_t value)
{
    unsigned identifier=0;
    while(identifier<=80&&value!=hn_read8(m,LOOKUP_VALUES+identifier-1))identifier++;
    for(unsigned index=0;index<=29;index++)if(hn_read8(m,LIST+index)==identifier){hn_group3_remove_list(m,(uint8_t)index);break;}
    return !m->fault;
}
bool hn_group3_tile_map(HNMemory *m,GameAddr destination,GameAddr source,uint16_t attributes)
{
    for(unsigned row=0;row<=5;row++,destination+=46)for(unsigned column=0;column<=8;column++,source+=2,destination+=2)
        hn_write16(m,destination,(uint16_t)((hn_read16(m,source)&0x0FFF)|attributes));
    return !m->fault;
}
bool hn_group3_insert_pair(HNMemory *m,GameAddr pair,GameAddr begin)
{
    unsigned visited=0;
    while(pair>begin&&hn_read8(m,pair)<hn_read8(m,pair-2)){
        if(visited++==4096)return false;
        uint8_t first=hn_read8(m,pair);hn_write8(m,pair,hn_read8(m,pair-2));hn_write8(m,pair-2,first);
        uint8_t second=hn_read8(m,pair+1);hn_write8(m,pair+1,hn_read8(m,pair-1));hn_write8(m,pair-1,second);
        pair-=2;
    }
    return !m->fault;
}
typedef struct {uint32_t x,y,z;int32_t y_adjust;} Position;
static Position position(HNMemory *m,uint8_t category,uint8_t bank)
{
    Position p={0,0,0,0};
    if(!bank){
        if(category<=29){
            p.x=(uint32_t)hn_read16(m,POSITIONS+category*6)<<16;p.y=(uint32_t)hn_read16(m,POSITIONS+category*6+2)<<16;
            p.z=(uint32_t)hn_read16(m,POSITIONS+category*6+4)<<16;
        }else if(category<=50){p.x=0xFFFFFFFF;p.y=0x90000;}
        else if(category<=67){p.x=0xFFFF0000;p.y=0x190000;}
        else {p.x=0xFFF70000;p.y=0x290000;}
        if(category==57||category==60||category==61||category==67)p.y_adjust=-0x80000;
        else if((uint8_t)(category-55)<=1||category==58||category==62||category==63||category==65)p.y_adjust=-0x70000;
    }else{
        p.x=(uint32_t)hn_read16(m,ALTERNATE_POSITIONS+category*4)<<16;p.y=(uint32_t)hn_read16(m,ALTERNATE_POSITIONS+category*4+2)<<16;
        if(category==12||category==27||category==18||category==21){p.y+=1;p.z=0xFFFFFFFF;}
    }
    return p;
}
bool hn_group3_position(HNMemory *m,uint8_t category,GameAddr x,GameAddr y,GameAddr z,uint8_t bank)
{
    Position p=position(m,category,bank);hn_write32(m,x,p.x);hn_write32(m,y,p.y);hn_write32(m,z,p.z);
    if(p.y_adjust)hn_write32(m,y,hn_read32(m,y)+(uint32_t)p.y_adjust);
    return !m->fault;
}
bool hn_group3_find_pair(HNMemory *m,GameAddr pairs,uint8_t first,uint8_t second,uint8_t *index)
{
    uint8_t i=0;
    for(unsigned visited=0;visited<4096;visited++,pairs+=2,i++){
        if(hn_read8(m,pairs)==255||(hn_read8(m,pairs)==first&&hn_read8(m,pairs+1)==second)){*index=i;return !m->fault;}
        if(m->fault)return false;
    }
    return false;
}
bool hn_group3_pair_slot(HNMemory *m,uint32_t slot_word,uint32_t parent_word,uint32_t category_word,
                         uint32_t variation_word,uint32_t bank_word,uint32_t retained_z)
{
    uint8_t slot=(uint8_t)slot_word,parent=(uint8_t)parent_word,category=(uint8_t)category_word;
    uint8_t variation=(uint8_t)variation_word,bank=(uint8_t)bank_word;
    Position p=category==255?(Position){0,0,retained_z,0}:position(m,category,0);p.y+=(uint32_t)p.y_adjust;
    if(!hn_render_configure_raw(m,slot,1,0xB0,(int32_t)p.x>>16,(int32_t)p.y>>16,(int32_t)p.z>>16,DEFAULT_ANIMATION))return false;
    priority(m,slot,1);hn_write8(m,hn_render_slot(m,slot)+0x65,parent);
    return hn_group3_pair_animation(m,slot,category,bank)&&hn_group3_pair_palette(m,category,variation,0,bank);
}
bool hn_group3_scan_pairs(HNMemory *m,GameAddr fields,GameAddr pairs,uint32_t first_row,uint32_t group,uint32_t *count)
{
    uint32_t total=0;
    for(unsigned row=0;row<=9;row++,fields+=12){
        uint8_t category=(uint8_t)((first_row+row)*3+group);
        uint32_t flags=(hn_read8(m,fields)|hn_read8(m,fields+1))&~hn_read8(m,DEFAULT_VALUES+hn_read8(m,DEFAULT_INDICES+category));
        for(unsigned variation=0;variation<=4;variation++,flags=(uint8_t)(flags<<1))if(flags&0x80){
            hn_write8(m,pairs++,category);hn_write8(m,pairs++,(uint8_t)variation);total++;
        }
    }
    *count=total;return !m->fault;
}
static bool refresh_flags(HNMemory *m)
{
    for(unsigned i=0;i<=5;i++){
        GameAddr pair=resource(m,CATALOGS,i%3)+hn_read8(m,SELECTED+i)*2;uint8_t category=hn_read8(m,pair);
        set_flag(m,hn_read16(m,PAIR_FLAGS+i*2),category!=255&&category>29);
    }
    return !m->fault;
}
static bool rebuild(HNMemory *m)
{
    for(unsigned group=0;group<=2;group++){
        GameAddr pair=resource(m,CATALOGS,group);for(unsigned i=0;i<=161;i++)hn_write8(m,pair+i,255);
    }
    const GameAddr bases[]={CATALOG0,CATALOG1,CATALOG2};GameAddr ends[3];
    for(unsigned group=0;group<=2;group++){
        uint32_t count;if(!hn_group3_scan_pairs(m,FIELDS+group*4,bases[group],0,group,&count))return false;
        hn_write8(m,COUNTS+group,(uint8_t)count);
    }
    for(unsigned group=0;group<=2;group++)ends[group]=bases[group]+hn_read8(m,COUNTS+group)*2;
    for(unsigned index=0;index<=29;index++){
        uint8_t value=hn_read8(m,LIST+index);if(!value)continue;
        value=hn_read8(m,LOOKUP_VALUES+value-1);unsigned group=value<=50?0:value<=67?1:2;
        hn_write8(m,ends[group]++,value);hn_write8(m,ends[group]++,0);
        if(!hn_group3_insert_pair(m,ends[group]-2,bases[group]))return false;
        hn_write8(m,COUNTS+group,(uint8_t)(hn_read8(m,COUNTS+group)+1));
    }
    uint8_t total=(uint8_t)(hn_read8(m,COUNTS)+hn_read8(m,COUNTS+1)+hn_read8(m,COUNTS+2));
    set_flag(m,0x280,total==0);return !m->fault;
}
static bool completed_mask(HNMemory *m)
{
    uint32_t mask=0;
    for(unsigned group=0;group<=4;group++){
        bool complete=true;
        for(unsigned half=0;half<2&&complete;half++)for(unsigned part=0;part<3;part++){
            if(group==3&&half==1&&part==1)continue;
            unsigned category=(group+half*5)*3+part;
            if(!(hn_read8(m,FIELDS+category*4)&~hn_read8(m,DEFAULT_VALUES+hn_read8(m,DEFAULT_INDICES+category)))){complete=false;break;}
        }
        if(complete)mask|=hn_read8(m,COMPLETED_BITS+group);
    }
    for(unsigned index=0;index<=31;index++)if(hn_read8(m,LIST+index)==2){mask=(uint8_t)(mask|0x80);break;}
    mask&=~hn_read8(m,KNOWN_MASK);store(m,0x60,mask);mask|=hn_read8(m,KNOWN_MASK);hn_write8(m,KNOWN_MASK,(uint8_t)mask);
    return !m->fault;
}
static bool advance_selection(HNMemory *m)
{
    uint32_t group=(uint8_t)(control(m,0x70)-1),bank=control(m,0x72),direction=control(m,0x61);
    uint32_t index=bank*3+group,slot=index+9,timer=control(m,0x62),state=control(m,0x60);
    bool alternate=flag(m,0xF7);GameAddr selected=SELECTED+index,other=SELECTED+(bank^1)*3+group;
    GameAddr table=CATALOGS+group*4,pair=hn_read32(m,table)+hn_read8(m,selected)*2;
    switch(state){
    case 0:
        vector(m,slot,(uint32_t)(int32_t)(int16_t)hn_read16(m,SLIDE_VELOCITIES+direction*2)<<8,0,0);
        hn_render_flags(m,slot,3,1);if(!hn_graphics_configure_raw(m,4,0x088FAD2B))return false;
        timer=hn_read8(m,pair)!=255?8:1;state=1;break;
    case 1:
        timer--;if(timer)break;vector(m,slot,0,0,0);hn_write8(m,hn_render_slot(m,slot)+0x60,0);
        if(!direction){
            if(hn_read8(m,pair)==255)hn_write8(m,selected,0);
            else{
                hn_write8(m,selected,(uint8_t)(hn_read8(m,selected)+1));
                if(alternate&&hn_read8(m,pair+2)==255)hn_write8(m,selected,0);
            }
        }else if(hn_read8(m,selected))hn_write8(m,selected,(uint8_t)(hn_read8(m,selected)-1));
        else{
            hn_write8(m,selected,0);pair=hn_read32(m,table);unsigned visited=0;
            while(hn_read8(m,pair)!=255){if(visited++==4096)return false;pair+=2;hn_write8(m,selected,(uint8_t)(hn_read8(m,selected)+1));}
            if(alternate&&hn_read8(m,selected))hn_write8(m,selected,(uint8_t)(hn_read8(m,selected)-1));
        }
        pair=hn_read32(m,table)+hn_read8(m,selected)*2;index=hn_read8(m,selected);
        if(!alternate&&index==hn_read8(m,other)&&hn_read8(m,pair)!=255){timer=1;break;}
        timer=hn_read8(m,pair)!=255?16:1;state=(uint8_t)(state+1);break;
    case 2:{
        timer--;if(timer)break;hn_write8(m,hn_render_slot(m,slot)+0x60,1);
        vector(m,slot,(uint32_t)(int32_t)(int16_t)hn_read16(m,SLIDE_VELOCITIES+direction*2)<<8,0,0);
        Position p=position(m,hn_read8(m,pair),alternate?1:0);p.y+=(uint32_t)p.y_adjust;
        p.x+=direction?0xFFE00000u:0x00200000u;place(m,slot,p.x,p.y,p.z);
        if(hn_read8(m,pair)!=255){
            if(!hn_group3_pair_animation(m,slot,hn_read8(m,pair),bank)||!hn_group3_pair_palette(m,hn_read8(m,pair),hn_read8(m,pair+1),0,bank))return false;
            timer=8;
        }else{hn_render_set_animation(m,slot,DEFAULT_ANIMATION);timer=1;}
        if(!hn_graphics_configure_raw(m,4,0x088FADF5))return false;store(m,0x63,hn_read8(m,pair));state=(uint8_t)(state+1);break;
    }
    case 3:
        timer--;if(timer)break;vector(m,slot,0,0,0);hn_render_flags(m,slot,3,0);state=4;break;
    }
    store(m,0x62,timer);store(m,0x60,state);return !m->fault;
}
static bool pair_text(HNMemory *m)
{
    uint8_t selection=control(m,0x60),remaining=(uint8_t)(selection-1),offset=0;
    GameAddr counts=0x086E9090+selection;
    while(remaining){offset=(uint8_t)(offset+hn_read8(m,counts--));remaining--;}
    if(selection<=30)hn_write32(m,0x03002BB0,resource(m,0x0200280C,(uint32_t)selection-1));
    else{
        GameAddr text=resource(m,0x0200280C,(uint32_t)selection-1);
        if(text!=0x0858C6B0)text=resource(m,text,control(m,0xEB));
        uint32_t normalized=0;uint8_t count=0,character=hn_read8(m,text++);
        if(!character)hn_write8(m,0x03003B20,0);
        else for(;;){
            if(character==1&&control(m,0xEB)!=3)normalized=0;
            else if(!normalized&&(uint8_t)(character-0x61)<=0x30){character=(uint8_t)(character-0x55);normalized=1;}
            else if((uint8_t)(character-0x0C)<=0x30)normalized=1;
            hn_write8(m,0x03003B20+count,character);count++;
            if(count>31)break;character=hn_read8(m,text++);
            if(!character){hn_write8(m,0x03003B20+count,0);break;}
        }
        hn_write8(m,0x03003B3F,0);hn_write32(m,0x03002BB0,0x02002A2C);
    }
    uint32_t selected=(uint32_t)offset+control(m,0x61)-1;
    hn_write32(m,0x03002BB4,resource(m,0x020029C4,hn_read8(m,0x086E93F2+selected)));return !m->fault;
}
static bool prepare_pending(HNMemory *m)
{
    unsigned slot,bank;if(control(m,0x72)){slot=12;bank=1;}else{slot=9;bank=0;}
    for(unsigned i=0;i<3;i++,slot++){
        GameAddr pair=resource(m,CATALOGS,i);unsigned category=hn_read8(m,pair);
        place(m,slot,(uint32_t)hn_read16(m,ALTERNATE_POSITIONS+category*4)<<16,(uint32_t)hn_read16(m,ALTERNATE_POSITIONS+category*4+2)<<16,0);
        category=hn_read8(m,pair);
        if(category==12||category==27||category==18||category==21)
            place(m,slot,(uint32_t)hn_read16(m,ALTERNATE_POSITIONS+category*4)<<16,((uint32_t)hn_read16(m,ALTERNATE_POSITIONS+category*4+2)<<16)+0x10000,0xFFFF0000);
        if(!hn_group3_pair_animation(m,slot,hn_read8(m,pair),bank)||!hn_group3_pair_palette(m,hn_read8(m,pair),hn_read8(m,pair+1),(uint8_t)i,bank))return false;
    }
    for(unsigned group=0;group<2;group++)for(unsigned i=0;i<3;i++){
        GameAddr index=SELECTED+group*3+i,destination=PENDING+group*6+i*2;
        GameAddr pair=resource(m,CATALOGS,i)+hn_read8(m,index)*2;
        hn_write8(m,destination,hn_read8(m,pair));hn_write8(m,destination+1,hn_read8(m,pair+1));hn_write8(m,index,0);
    }
    return refresh_flags(m);
}
static bool render_pending(HNMemory *m)
{
    GameAddr pending=PENDING;
    for(unsigned i=0;i<6;i++,pending+=2){
        GameAddr catalog=CATALOGS+(i%3)*4;uint8_t index;
        if(!hn_group3_find_pair(m,hn_read32(m,catalog),hn_read8(m,pending),hn_read8(m,pending+1),&index))return false;
        hn_write8(m,SELECTED+i,index);GameAddr pair=hn_read32(m,catalog)+hn_read8(m,SELECTED+i)*2;
        Position p=position(m,hn_read8(m,pair),0);place(m,i+9,p.x,p.y+(uint32_t)p.y_adjust,p.z);
        unsigned bank=i/3;
        if(!hn_group3_pair_animation(m,i+9,hn_read8(m,pair),bank)||!hn_group3_pair_palette(m,hn_read8(m,pair),hn_read8(m,pair+1),(uint8_t)i,bank))return false;
    }
    return refresh_flags(m);
}
/* Nonempty calls establish the helper's retained local Z. The first empty
   entry has an indeterminate source stack word: use zero for that undefined
   coordinate. Its original animation selects an OBJ frame with zero parts. */
typedef struct {uint32_t z;} PairFrame;
static bool pair_in_frame(HNMemory *m,PairFrame *frame,uint8_t slot,uint8_t parent,uint8_t category,uint8_t variation,uint8_t bank)
{
    if(category!=255)frame->z=position(m,category,0).z;
    return hn_group3_pair_slot(m,slot,parent,category,variation,bank,frame->z);
}
static bool render_pair_set(HNMemory *m,bool stored)
{
    uint32_t slot=control(m,0x61),parent=control(m,0x62);PairFrame frame={0};
    GameAddr record=RECORDS+control(m,0x60)*16;const GameAddr catalogs[]={CATALOG0,CATALOG1,CATALOG2};
    for(unsigned i=0;i<6;i++,slot++){
        if(i==3)parent=(uint8_t)(parent+1);
        GameAddr pair=stored?record+1+i*2:catalogs[i%3]+hn_read8(m,SELECTED+i)*2;
        if(!pair_in_frame(m,&frame,(uint8_t)slot,(uint8_t)parent,hn_read8(m,pair),hn_read8(m,pair+1),(uint8_t)(i/3)))return false;
    }
    return !m->fault;
}
static bool selected_slots(HNMemory *m)
{
    uint32_t slot=control(m,0x61),parent=control(m,0x62);GameAddr selected=SELECTED+(control(m,0x63)?3:0);
    uint8_t categories[3];const GameAddr catalogs[]={CATALOG0,CATALOG1,CATALOG2};
    for(unsigned i=0;i<3;i++)categories[i]=hn_read8(m,catalogs[i]+hn_read8(m,selected+i)*2);
    for(unsigned i=0;i<3;i++,slot++){
        Position p=categories[i]==255?(Position){0,0,0,0}:position(m,categories[i],0);p.y+=(uint32_t)p.y_adjust;
        if(!hn_render_configure_raw(m,(uint8_t)slot,1,0xB0,(int32_t)p.x>>16,(int32_t)p.y>>16,(int32_t)p.z>>16,DEFAULT_ANIMATION))return false;
        priority(m,slot,1);hn_write8(m,hn_render_slot(m,slot)+0x65,(uint8_t)parent);
    }
    return !m->fault;
}
static bool selected_display(HNMemory *m)
{
    uint32_t slot=control(m,0x61);store(m,0x63,0);if(!selected_slots(m))return false;
    store(m,0x61,control(m,0x61)+3);store(m,0x62,control(m,0x62)+1);store(m,0x63,1);
    if(!selected_slots(m))return false;
    const GameAddr catalogs[]={CATALOG0,CATALOG1,CATALOG2};
    for(unsigned i=0;i<6;i++,slot++){
        GameAddr pair=catalogs[i%3]+hn_read8(m,SELECTED+i)*2;unsigned bank=i/3;
        if(!hn_group3_pair_animation(m,slot,hn_read8(m,pair),bank)||!hn_group3_pair_palette(m,hn_read8(m,pair),hn_read8(m,pair+1),(uint8_t)i,bank))return false;
    }
    return !m->fault;
}
static bool render_records(HNMemory *m)
{
    GameAddr buffer=hn_heap_allocate(m,0xD8,false);if(!buffer)return false;bool ok=true;
    GameAddr record=RECORDS;
    for(unsigned index=0;index<6;index++,record+=16){
        uint32_t offset=hn_read16(m,0x086E9396+index*2);GameAddr first=0x0600F000+offset,second=0x0600F800+offset;
        if(!hn_read8(m,record)){
            for(unsigned row=0;row<6;row++,first+=46)for(unsigned column=0;column<9;column++,first+=2)hn_write16(m,first,0xF3A2);
            continue;
        }
        if(!hn_bios_lz77(m,resource(m,0x0200278C,((uint32_t)hn_read8(m,record)-1)*2),buffer,0x40000)||
           !hn_bios_lz77(m,resource(m,0x0200278C,((uint32_t)hn_read8(m,record)-1)*2+1),buffer+108,0x40000)||
           !hn_group3_tile_map(m,first,buffer,(uint16_t)((index*2+1)<<12))||
           !hn_group3_tile_map(m,second,buffer+108,(uint16_t)(index<<13))){ok=false;break;}
        GameAddr palette=0x05000000+index*64,palette_source=resource(m,0x020027CC,(uint32_t)hn_read8(m,record)-1);
        for(unsigned item=0;item<32;item++)hn_write16(m,palette+item*2,hn_read16(m,palette_source+item*2));
        unsigned next=index+1,column=(index<=2?next:index+2)&3;
        uint32_t x=((uint32_t)(column-1)*80+4)<<16,y=(((next&4)>>2)*7<<19)+0x200000;
        uint16_t saved_x=(uint16_t)(x>>16),saved_y=(uint16_t)(y>>16);
        if(!hn_render_configure_raw(m,(uint8_t)(index+25),1,0x74,(int32_t)x>>16,(int32_t)y>>16,0,resource(m,0x020027EC,(uint32_t)hn_read8(m,record)-1))||
           !hn_render_configure_raw(m,(uint8_t)(index+19),1,0x74,(int16_t)(saved_x+32),(int16_t)(saved_y+25),1,0x088FB2DA)){ok=false;break;}
    }
    hn_heap_free(m,buffer);return ok&&!m->fault;
}
static void list_blank(HNMemory *m)
{
    for(unsigned i=0;i<=10;i++){
        hn_write16(m,LIST_BLANK_TILES+i*2,0x82);hn_write16(m,LIST_BLANK_TILES+(i+32)*2,0x82);
    }
}
static bool list_style_slots(HNMemory *m,bool set_priority)
{
    for(unsigned i=0;i<3;i++){
        if(!hn_render_configure_raw(m,(uint8_t)(16+i),1,0x72,192,0,24+(int32_t)i*16,0x088FA621+i*8))return false;
        if(set_priority)priority(m,16+i,1);
    }
    return !m->fault;
}
bool hn_group3_list_text(HNMemory *m,GameAddr buffer)
{
    list_blank(m);unsigned selected=control(m,0x180);store(m,0x60,control(m,0x60)+1);
    GameAddr text=resource(m,LIST_TEXT_BUNDLES,hn_read8(m,LIST+selected));
    if(text!=HN_TASK_DEFAULT_SCRIPT)text=resource(m,text,control(m,0xEB));
    unsigned limit=control(m,0x60),count=0;
    while(count<limit){
        if(count>=19)return false;hn_write8(m,buffer+count,hn_read8(m,text++));
        if(!hn_read8(m,text)){store(m,0x60,255);count++;break;}
        count++;
    }
    hn_write8(m,buffer+count,0);
    GameAddr task=hn_task_allocate(m);hn_write32(m,0x030037A0,task);
    return task&&hn_task_graphics(m,task,13,2,0,1,0,3,24,0x06005000,LIST_BLANK_TILES)&&
           hn_task_set_script(m,hn_read32(m,0x030037A0),buffer)&&!m->fault;
}
static bool list_numbers(HNMemory *m)
{
    uint16_t digits[3][2];unsigned selected=control(m,0x180);
    for(unsigned i=0;i<3;i++){
        uint16_t packed=hn_read16(m,LIST_NUMBER_FIELDS+hn_read8(m,LIST+selected)*24+i*2);
        uint8_t category=(uint8_t)((packed>>7)&127);unsigned number=packed&127;
        digits[i][0]=(uint16_t)(number/10);digits[i][1]=(uint16_t)(number%10);
        if(category){
            if(!hn_render_configure_raw(m,(uint8_t)(13+i),1,0x72,108,0,24+(int32_t)i*16,
                                        resource(m,LIST_CATEGORY_ANIMATIONS,category)))return false;
            priority(m,13+i,1);
        }else hn_write8(m,hn_render_slot(m,13+i)+0x60,0);
    }
    for(unsigned i=0;i<3;i++){
        hn_write16(m,LIST_STYLE_FRAMES+i*14+6,(uint16_t)(digits[i][0]?0xF2D6+digits[i][0]:0xF2D5));
        hn_write16(m,LIST_STYLE_FRAMES+i*14+12,(uint16_t)(0xF2D6+digits[i][1]));
    }
    return list_style_slots(m,true);
}
#define GROUP3_REQUIRE(operation) do {if(!(operation))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_group3_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080C61E0:
        if(control(m,0x60)&1)for(unsigned i=0;i<=31;i++)if(i>control(m,0x180)){
            GameAddr cell=LIST_CELLS+(i*3+2)*2;uint16_t value=(uint16_t)(hn_read16(m,cell)-1);
            hn_write16(m,cell,value<=0x3FFF?0x41FF:value);
        }
        break;
    case 0x080C6270:
        hn_write16(m,LIST_CELLS+(control(m,0x180)*3+3)*2,0xB35C);hn_write8(m,hn_render_slot(m,12)+0x60,0);
        list_blank(m);
        for(unsigned i=0;i<3;i++)GROUP3_REQUIRE(hn_render_configure_raw(m,(uint8_t)(13+i),1,0x72,108,0,24+(int32_t)i*16,LIST_HEADER_ANIMATION));
        for(unsigned i=0;i<3;i++){hn_write16(m,LIST_STYLE_FRAMES+i*14+6,0xF2D5);hn_write16(m,LIST_STYLE_FRAMES+i*14+12,0xF2D5);}
        GROUP3_REQUIRE(list_style_slots(m,false));break;
    case 0x080C6374:{
        GROUP3_REQUIRE(hn_render_configure_raw(m,12,1,0x72,12,0,8,LIST_CURSOR_ANIMATION));priority(m,12,2);
        unsigned selected=control(m,0x180);uint8_t entry=hn_read8(m,LIST+selected);
        GROUP3_REQUIRE(hn_render_configure_raw(m,2,1,0x72,(int32_t)(selected%10)*22+5,0,79+(int32_t)(selected/10)*20,
                       resource(m,LIST_ITEM_ANIMATIONS,hn_read8(m,LIST_DESCRIPTORS+entry*24+4))));break;
    }
    case 0x080C641C:GROUP3_REQUIRE(hn_group3_list_text(m,HN_GROUP3_LIST_TEXT));break;
    case 0x080C6518:GROUP3_REQUIRE(list_numbers(m));break;
    case 0x080C670C:
        for(unsigned i=0;i<30;i++){
            hn_write16(m,LIST_CELLS+(i*3+1)*2,(uint16_t)(i/10*20+79));
            hn_write16(m,LIST_CELLS+(i*3+2)*2,(uint16_t)(i%10*22+0x4005));
            uint8_t entry=hn_read8(m,LIST+i);uint16_t tile=entry?hn_read16(m,LIST_TILE_INDICES+hn_read8(m,LIST_DESCRIPTORS+entry*24+4)*2):0xB35C;
            hn_write16(m,LIST_CELLS+(i*3+3)*2,tile);
        }
        hn_write16(m,LIST_CELLS+182,79);hn_write16(m,LIST_CELLS+184,0x40E1);hn_write16(m,LIST_CELLS+186,0xB35C);
        hn_write16(m,LIST_CELLS+188,99);hn_write16(m,LIST_CELLS+190,0x40E1);hn_write16(m,LIST_CELLS+192,0xB35C);
        GROUP3_REQUIRE(hn_render_configure_raw(m,10,1,0x72,0,-128,128,LIST_GRID_ANIMATION));break;
    case 0x080C9304:{
        GameAddr ends[]={CATALOG0,CATALOG1,CATALOG2};uint8_t counts[3]={0};
        for(unsigned category=0;category<=29;category++)for(unsigned i=0;i<hn_read8(m,DEFAULT_INDICES+category);i++){
            unsigned group=category%3;hn_write8(m,ends[group]++,(uint8_t)category);hn_write8(m,ends[group]++,(uint8_t)i);counts[group]++;
        }
        for(unsigned i=0;i<6;i++)hn_write8(m,SELECTED+i,counts[i%3]);
        for(unsigned i=0;i<3;i++)hn_write8(m,COUNTS+i,counts[i]);break;
    }
    case 0x080C93F8:{
        uint16_t keys=hn_input_query(m,0x300,0);for(unsigned i=0;i<30;i++)hn_write8(m,LIST+i,0);
        if(keys){
            for(unsigned i=0;i<30;i++)hn_write8(m,FIELDS+i*4,hn_read8(m,FIELDS+i*4)|0x80);
            unsigned start=keys==0x200?1:keys==0x100?31:61,count=keys==0x300?20:30;
            for(unsigned i=0;i<count;i++)hn_write8(m,LIST+i,(uint8_t)(start+i));
        }
        break;
    }
    case 0x080C9478:{
        GameAddr pair=RECORDS+1+control(m,0x60)*16;
        for(unsigned i=0;i<6;i++,pair+=2){uint8_t index;GROUP3_REQUIRE(hn_group3_find_pair(m,resource(m,CATALOGS,i%3),hn_read8(m,pair),hn_read8(m,pair+1),&index));hn_write8(m,SELECTED+i,index);}
        break;
    }
    case 0x080C94CC:{
        uint32_t group=(uint8_t)(control(m,0x70)-1);GameAddr pairs=resource(m,CATALOGS,group);
        uint8_t selected=hn_read8(m,SELECTED+control(m,0x72)*3+group);GameAddr pair=pairs+selected*2;
        store(m,0x60,hn_read8(m,pair)+1);store(m,0x61,hn_read8(m,pair+1)+1);store(m,0x62,hn_read8(m,COUNTS+group));
        uint8_t number=0;for(unsigned i=group;i;i--)number=(uint8_t)(number+hn_read8(m,COUNTS+i-1));
        store(m,0,(uint32_t)number+selected+1);break;
    }
    case 0x080C9558:{
        uint8_t slot=hn_read8(m,arguments);GROUP3_REQUIRE(hn_group3_number(m,slot,control(m,0x60),hn_read8(m,arguments+1),hn_read8(m,arguments+2)));
        *consumed=3;break;
    }
    case 0x080C9578:GROUP3_REQUIRE(prepare_pending(m));break;
    case 0x080C9694:GROUP3_REQUIRE(render_pending(m));break;
    case 0x080C9760:GROUP3_REQUIRE(refresh_flags(m));break;
    case 0x080C97C8:GROUP3_REQUIRE(render_pair_set(m,true));break;
    case 0x080C9884:GROUP3_REQUIRE(render_pair_set(m,false));break;
    case 0x080C9990:GROUP3_REQUIRE(selected_display(m));break;
    case 0x080C9AD8:GROUP3_REQUIRE(selected_slots(m));break;
    case 0x080C9BA0:case 0x080C9BE4:
        for(unsigned group=0;group<=2;group++){
            uint8_t end;GROUP3_REQUIRE(hn_group3_find_pair(m,resource(m,CATALOGS,group),255,0,&end));
            if((callback&~1u)==0x080C9BA0){hn_write8(m,SELECTED+group,end);hn_write8(m,SELECTED+group+3,end);}
            else hn_write8(m,SELECTED+control(m,0x72)*3+group,end);
        }
        break;
    case 0x080C9C30:GROUP3_REQUIRE(advance_selection(m));break;
    case 0x080C9EDC:{
        unsigned group=(uint8_t)(control(m,0x70)-1),index=control(m,0x72)*3+group;
        GameAddr pair=resource(m,CATALOGS,group)+hn_read8(m,SELECTED+index)*2;store(m,0x63,hn_read8(m,pair));break;
    }
    case 0x080C9F20:case 0x080C9F58:
        for(unsigned i=0;i<3;i++)GROUP3_REQUIRE(hn_group3_number(m,15+i*2,hn_read8(m,COUNTS+i),0x88,(uint8_t)(0x16+i*24)));
        GROUP3_REQUIRE(hn_group3_selected_count(m));break;
    case 0x080C9F90:GROUP3_REQUIRE(hn_group3_counts(m));GROUP3_REQUIRE(hn_group3_selected_count(m));break;
    case 0x080C9FA0:{
        unsigned group=control(m,0x70),bank=control(m,0x72);GameAddr selected=SELECTED+bank*3+group-1;
        GameAddr pair=resource(m,CATALOGS,group-1)+hn_read8(m,selected)*2;
        if(hn_read8(m,pair)!=255){
            if(hn_read8(m,pair)<=29){hn_group3_field_bit(m,hn_read8(m,pair),hn_read8(m,pair+1),0);hn_write8(m,FIELD_COUNT,(uint8_t)(hn_read8(m,FIELD_COUNT)-1));}
            else GROUP3_REQUIRE(hn_group3_remove_lookup(m,hn_read8(m,pair)));
            GROUP3_REQUIRE(rebuild(m));hn_render_set_animation(m,bank*3+group+8,DEFAULT_ANIMATION);
            if(hn_read8(m,pair)==255)hn_write8(m,selected,0);
        }
        break;
    }
    case 0x080CA030:
        hn_write32(m,hn_render_slot(m,26)+0x18,(uint32_t)hn_read16(m,ROW_X+((uint32_t)control(m,0x71)-1)*2)<<16);break;
    case 0x080CA060:{
        unsigned group=(uint8_t)(control(m,0x70)-1),bank=control(m,0x72);
        place(m,23,(uint32_t)hn_read16(m,INDICATOR_X+bank*2)<<16,(uint32_t)hn_read16(m,INDICATOR_Y+group*2)<<16,0x10000);
        if(group<=2&&flag(m,hn_read16(m,PAIR_FLAGS+(bank*3+group)*2)))group=4;
        hn_render_set_animation(m,23,resource(m,INDICATOR_ANIMATIONS,bank*5+group));break;
    }
    case 0x080CA0E0:GROUP3_REQUIRE(rebuild(m));break;
    case 0x080CA244:{
        uint32_t bits=(uint16_t)lsr(hn_read8(m,KNOWN_MASK),control(m,0x70));store(m,0x70,control(m,0x70)+1);
        if(!bits){bits=hn_read8(m,KNOWN_MASK);store(m,0x70,1);}
        for(unsigned steps=0;steps<=15&&!(bits&1);steps++,bits>>=1)store(m,0x70,control(m,0x70)+1);break;
    }
    case 0x080CA294:{
        uint32_t current=control(m,0x70);current=current<=1?16:current-1;store(m,0x70,current);
        uint32_t mask=hn_read8(m,KNOWN_MASK),bits=(uint16_t)lsl(mask,16u-control(m,0x70));
        if(!bits){bits=mask;store(m,0x70,16);}
        for(unsigned steps=0;steps<=15&&!(bits&0x8000);steps++,bits=(uint16_t)(bits<<1))store(m,0x70,(uint32_t)control(m,0x70)-1);break;
    }
    case 0x080CA2F4:store(m,0x60,(hn_read8(m,KNOWN_MASK)&0xFC)!=(control(m,0xD2)&0xFC));break;
    case 0x080CA32C:store(m,0xD2,control(m,0xD2)|lsl(1,(uint32_t)control(m,0x70)-1));break;
    case 0x080CA348:GROUP3_REQUIRE(render_records(m));break;
    case 0x080CA4DC:{
        int32_t index=control(m,0x181),column=(index<=2?index+1:index+2)&3,x=(column-1)*80+2;
        uint32_t y=(uint32_t)((((index+1)&4)>>2)*7)<<19;y+=0x1D0000;
        place(m,18,((uint32_t)(uint16_t)x<<16)+0x8000,((y>>16)<<16)+0x8000,0x8000);break;
    }
    case 0x080CA528:{
        GameAddr destination=HN_SCENE_CONTROLS+hn_read16(m,arguments+2),selection=HN_SCENE_CONTROLS+hn_read16(m,arguments);
        hn_write8(m,destination,hn_read8(m,RECORDS+hn_read8(m,selection)*16));*consumed=4;break;
    }
    case 0x080CA55C:hn_video_blend_effect(m,0);break;
    case 0x080CA568:{
        unsigned selection=hn_read16(m,arguments);hn_write8(m,0x0300379C,control(m,selection));
        GameAddr record=RECORDS+hn_read8(m,0x0300379C)*16;
        unsigned value_index=hn_read16(m,arguments+2);hn_write8(m,record,control(m,value_index));
        const GameAddr catalogs[]={CATALOG0,CATALOG1,CATALOG2};
        for(unsigned i=0;i<6;i++){
            hn_write8(m,record+1+i*2,hn_read8(m,catalogs[i%3]+hn_read8(m,SELECTED+i)*2));
            hn_write8(m,record+2+i*2,hn_read8(m,catalogs[i%3]+(uint16_t)(hn_read8(m,SELECTED+i)*2+1)));
        }
        *consumed=4;break;
    }
    case 0x080CA644:hn_background_layers_reset(m,hn_read8(m,arguments));*consumed=1;break;
    case 0x080CA654:GROUP3_REQUIRE(pair_text(m));break;
    case 0x080CA790:store(m,0x61,hn_read8(m,COUNTS+(uint32_t)control(m,0x60)-1));break;
    case 0x080CA7AC:GROUP3_REQUIRE(completed_mask(m));break;
    case 0x080CA92C:{
        GameAddr selected=SELECTED+control(m,0x72)*3;uint8_t count=0;const GameAddr catalogs[]={CATALOG0,CATALOG1,CATALOG2};
        for(unsigned i=0;i<3;i++)if(hn_read8(m,catalogs[i]+hn_read8(m,selected+i)*2)!=255)count++;
        store(m,0x60,count!=0);break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_group3_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080CB028:hn_save_default_fields(m);break;
    case 0x080CACB8:if(!hn_group3_counts(m))return HN_SCENE_RESOURCE;break;
    case 0x080CAD5C:if(!hn_group3_selected_count(m))return HN_SCENE_RESOURCE;break;
    case 0x080C61E0:case 0x080C6270:case 0x080C6374:case 0x080C641C:case 0x080C6518:case 0x080C670C:
    case 0x080C9304:case 0x080C93F8:case 0x080C9478:case 0x080C94CC:
    case 0x080C9578:case 0x080C9694:case 0x080C97C8:case 0x080C9884:case 0x080C9990:case 0x080C9AD8:
    case 0x080C9760:case 0x080C9BA0:case 0x080C9BE4:case 0x080C9C30:case 0x080C9EDC:
    case 0x080C9F20:case 0x080C9F58:case 0x080C9F90:case 0x080C9FA0:case 0x080CA030:case 0x080CA060:
    case 0x080CA0E0:case 0x080CA244:case 0x080CA294:case 0x080CA2F4:case 0x080CA32C:case 0x080CA348:case 0x080CA4DC:
    case 0x080CA55C:case 0x080CA654:case 0x080CA790:case 0x080CA7AC:case 0x080CA92C:{
        uint32_t ignored;return hn_group3_callback(m,callback,0,&ignored);
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
