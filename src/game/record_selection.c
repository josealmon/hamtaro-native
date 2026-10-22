#include "game/record_selection.h"
#include "game/state.h"
#include "game/random.h"
#include "game/render.h"
#include "game/transfer.h"
#include "gba/bios.h"

enum { CLASSES=0x0201DC40,VARIABLES=0x03002BB0,DESCRIPTORS=0x0858D344,
       COUNTER_LIST=0x085CBFD8,SLOT_PAIRS=0x085CC004,LABELS=0x02000B08,LABEL_BUFFER=0x02020060 };
static uint8_t control(HNMemory *m,unsigned index){return hn_read8(m,HN_SCENE_CONTROLS+index);}
static void store(HNMemory *m,unsigned index,uint8_t value){hn_write8(m,HN_SCENE_CONTROLS+index,value);}
static uint16_t class_value(HNMemory *m,unsigned index){return hn_read16(m,CLASSES+index*2);}
static void class_store(HNMemory *m,unsigned index,uint16_t value){hn_write16(m,CLASSES+index*2,value);}
static bool flag(HNMemory *m,unsigned index){return (hn_read8(m,0x03002180+(index>>3))>>(index&7))&1;}
static bool lz(HNMemory *m,GameAddr source,GameAddr destination){return hn_bios_lz77(m,source,destination,0x40000);}
static void visible(HNMemory *m,uint32_t slot,uint8_t value){hn_write8(m,hn_render_slot(m,(uint8_t)slot)+0x60,value);}
static bool counter_slot(HNMemory *m,uint32_t slot,int32_t x,GameAddr animation,bool priority)
{
    if(!hn_render_configure_raw(m,(uint8_t)slot,1,0x71,x,48,0,animation))return false;
    if(priority)hn_render_flags(m,(uint8_t)slot,12,0);return !m->fault;
}
bool hn_record_counter_palette(HNMemory *m,GameAddr destination,uint32_t record_word)
{
    hn_write16(m,destination,0);destination+=2;
    GameAddr requirements=DESCRIPTORS+(uint16_t)record_word*24+10;
    for(unsigned group=0;group<3;group++){
        unsigned requirement=hn_read16(m,requirements+group*2),key=(requirement>>7)&127;
        unsigned value=hn_read8(m,HN_RECORD_COUNTERS+key);
        GameAddr source=key&&(value==255||value<(requirement&127))?0x085CBFC2:0x085CBFBC;
        for(unsigned cell=0;cell<3;cell++){hn_write16(m,destination,hn_read16(m,source+cell*2));destination+=2;}
    }
    for(unsigned cell=0;cell<6;cell++)hn_write16(m,destination+cell*2,hn_read16(m,0x085CBFB0+cell*2));
    return !m->fault;
}
static GameAddr counter_descriptor(HNMemory *m,unsigned key)
{
    GameAddr descriptor=HN_RECORD_COUNTER_DESCRIPTORS+key*20;
    return hn_read32(m,descriptor+12)?descriptor:HN_RECORD_COUNTER_DESCRIPTORS+14*20;
}
bool hn_record_counter_rows(HNMemory *m,uint32_t index_word)
{
    uint8_t index=(uint8_t)index_word;GameAddr list=hn_read32(m,VARIABLES);
    GameAddr descriptor=counter_descriptor(m,hn_read8(m,list+control(m,0x61)));
    if(!hn_transfer_decompress_rows(m,hn_read32(m,descriptor+12),0x02005720,256,8,1024)||
       !lz(m,hn_read32(m,descriptor+16),0x02004720))return false;
    store(m,0x63,hn_read8(m,descriptor+4));
    descriptor=counter_descriptor(m,hn_read8(m,list+index));
    return hn_transfer_decompress_rows(m,hn_read32(m,descriptor+12),0x02005820,256,8,1024)&&
           lz(m,hn_read32(m,descriptor+16),0x02004740)&&!m->fault;
}
static bool initialize_display(HNMemory *m)
{
    GameAddr list=hn_read32(m,VARIABLES);int count=0,index=0;
    for(unsigned counter=0;counter<32;counter++){
        unsigned value=hn_read8(m,HN_RECORD_COUNTERS+counter);
        if(value&&value!=255)hn_write8(m,list+(uint32_t)count++,(uint8_t)counter);
    }
    hn_write8(m,list+(uint32_t)count,0);int last=count-1;
    if(count){unsigned key=hn_read8(m,0x020046C5);
        for(;index<count;index++)if(key<=hn_read8(m,list+(uint32_t)index)){
            store(m,0x61,(uint8_t)index);
            if(!hn_read8(m,list+(uint32_t)index))store(m,0x61,(uint8_t)(index-1));break;
        }
    }
    if(index==count)store(m,0x61,(uint8_t)(index-1));
    unsigned selected=control(m,0x61);
    if((int)selected==last)visible(m,9,0);if(!selected)visible(m,8,0);
    if(!hn_record_counter_rows(m,0))return false;
    index=control(m,0x61)<=3?4-control(m,0x61):0;
    for(;index<=3;index++)if(!counter_slot(m,hn_read32(m,COUNTER_LIST+(uint32_t)index*4),index*16,0x088E5020,false))return false;
    unsigned total=control(m,0x60);
    for(unsigned following=1;selected+following<total&&following<=4;following++)
        if(!counter_slot(m,following+24,(int32_t)following*16+160,0x088E5020,false))return false;
    return !m->fault;
}
static bool navigate_counter(HNMemory *m,bool forward)
{
    uint8_t total=control(m,0x60),previous=control(m,0x61),selected;int last=(int)total-1;
    if(forward?(int)previous>=last:!previous){store(m,0x62,0);return !m->fault;}
    store(m,0x62,1);store(m,0x61,(uint8_t)(previous+(forward?1:-1)));
    if(forward){selected=control(m,0x61);if(control(m,0x61)>=last)visible(m,9,0);visible(m,8,1);}
    else{if(!control(m,0x61))visible(m,8,0);visible(m,9,1);selected=control(m,0x61);}
    if(!hn_record_counter_rows(m,previous))return false;
    for(unsigned i=0;i<11;i++)visible(m,hn_read32(m,COUNTER_LIST+i*4),0);
    unsigned left=forward?previous:selected;int first=left<=3?4-(int)left:0;
    GameAddr animation=forward?0x088E503C:0x088E5028;
    for(int i=first;i<=3;i++)if(!counter_slot(m,hn_read8(m,COUNTER_LIST+(uint32_t)i*4),16*(i-(forward?0:1)),animation,true))return false;
    int end=forward?(selected+5>total?(int)total-((int)selected+1):4):
                    (previous+4>total?(int)total-previous:4);
    for(int i=0;i<end;i++)if(!counter_slot(m,hn_read32(m,COUNTER_LIST+(6u+(unsigned)i)*4),16*i+(forward?192:160),animation,true))return false;
    return !m->fault;
}
static bool unused_candidate(HNMemory *m,uint16_t selected,uint16_t value)
{for(unsigned i=0;i<selected;i++)if(class_value(m,i+35)==value)return false;return true;}
bool hn_record_pick_candidate(HNMemory *m,uint32_t selected,uint32_t count,GameAddr candidates,uint32_t *result)
{
    uint16_t value=hn_read16(m,candidates+hn_random_range(m,0,(uint16_t)count)*2);
    *result=unused_candidate(m,(uint16_t)selected,value)?value:0;return !m->fault;
}
static bool prepare_candidates(HNMemory *m)
{
    unsigned counter=control(m,0x1A5);GameAddr cache=hn_read32(m,VARIABLES+4)+counter*4;
    uint16_t selected=0;
    if(!hn_read8(m,cache)){
        hn_write8(m,cache,1);uint16_t candidates[80];unsigned count=0,reserved=0;
        for(unsigned record=1;record<=80;record++){
            if(class_value(m,34)==3&&hn_read8(m,DESCRIPTORS+record*24+6)==2)continue;
            if(record==40&&!flag(m,0x86))continue;
            for(unsigned field=0;field<3;field++)if((unsigned)((hn_read16(m,DESCRIPTORS+record*24+10+field*2)>>7)&127)==counter){
                if(count>=80)return false; /* Undefined original local-array overrun. */
                candidates[count++]=(uint16_t)record;if(record==40)reserved=1;
            }
        }
        if(reserved){if(flag(m,0xC2))reserved=0;else{class_store(m,35,40);hn_write8(m,cache+1,40);}}
        selected=(uint16_t)reserved;
        while(selected<class_value(m,34)&&!m->fault){
            if(!count)return false;
            uint16_t value=0;unsigned attempts=0;
            do{value=candidates[hn_random_range(m,0,(uint16_t)count)];
                if(!unused_candidate(m,selected,value))value=0;
                /* An impossible selection otherwise never returns in the source. */
                if(++attempts>65536)return false;
            }while(!value&&!m->fault);
            unsigned class_index=selected+35;uint16_t next=(uint16_t)(selected+1);
            class_store(m,class_index,value);hn_write8(m,cache+next,(uint8_t)value);selected=next;
        }
    }else while(selected<class_value(m,34)&&!m->fault){
        class_store(m,selected+35,hn_read8(m,cache+selected+1));selected++;
    }
    return !m->fault;
}
bool hn_record_palette_halve(HNMemory *m,GameAddr colors)
{
    for(unsigned i=0;i<8;i++){uint32_t value=hn_read32(m,colors+i*4);
        hn_write32(m,colors+i*4,(value&0x80008000u)|((value>>1)&0x3DEF3DEFu));}
    return !m->fault;
}
bool hn_record_selection_refresh(HNMemory *m,GameAddr arguments)
{
    GameAddr destination=hn_read32(m,arguments);unsigned selected=control(m,0x1A0);
    unsigned selected_record=class_value(m,selected+35);
    for(unsigned row=0;row<3;row++){
        unsigned record=class_value(m,row+35);
        if(!lz(m,hn_read32(m,DESCRIPTORS+record*24+20),destination))return false;
        if(row!=selected&&!hn_record_palette_halve(m,destination))return false;destination+=32;
        if(row==selected){if(!hn_record_counter_palette(m,destination,selected_record))return false;}
        else if(!lz(m,0x0825BB34,destination))return false;
        destination+=32;
    }
    return !m->fault;
}
static void copy_pattern(HNMemory *m,GameAddr source,GameAddr destination,unsigned words,uint32_t pattern)
{for(unsigned i=0;i<words&&!m->fault;i++)hn_write32(m,destination+i*4,hn_read32(m,source+i*4)+pattern);}
static bool selection_graphics(HNMemory *m,GameAddr arguments)
{
    GameAddr origin=hn_read32(m,arguments),labels=origin+0x1000,numbers=origin+0x800;
    GameAddr destinations[3]={labels,origin+0x4800,origin+0x4900};
    if(!hn_transfer_decompress_rows(m,0x0825A38C,origin+0x41A0,608,2,1024)||
       !hn_transfer_decompress_rows(m,0x0825A594,origin+0x4A00,512,8,1024)||
       !hn_transfer_decompress_rows(m,0x0825A960,origin+0x38C0,64,2,1024)||!lz(m,0x0825A9A8,numbers))return false;
    for(unsigned i=0;i<16;i++){
        const unsigned offsets[4]={0x840,0x880,0x9A0,0x9E0};
        for(unsigned j=0;j<4;j++){GameAddr cell=origin+offsets[j]+i*4;hn_write32(m,cell,hn_read32(m,cell)+(j&1?0x9999999Au:0xCCCCCCCDu));}
    }
    GameAddr cursor=origin+32;unsigned panel=0;
    for(unsigned row=0;row<class_value(m,34)&&!m->fault;row++){
        unsigned record=class_value(m,row+35);GameAddr next_cursor=cursor+64;
        for(unsigned field=0;field<3&&!m->fault;field++,panel++){
            GameAddr graphics=panel<=3?labels+(panel<<11)+0x100:labels+((panel-4)<<11)+0x280;
            unsigned key=(hn_read16(m,DESCRIPTORS+record*24+10+field*2)>>7)&127;
            if(!lz(m,hn_read32(m,LABELS+(key*5+control(m,0xEB))*4),LABEL_BUFFER))return false;
            uint32_t pattern=(field*3)*0x11111111u;
            copy_pattern(m,LABEL_BUFFER,graphics,80,pattern);copy_pattern(m,LABEL_BUFFER+320,graphics+0x400,80,pattern);
            unsigned quantity=hn_read16(m,DESCRIPTORS+record*24+10+field*2)&127;
            for(unsigned half=0;half<2;half++){
                GameAddr destination=graphics+0x140+half*0x400;
                if(quantity){copy_pattern(m,numbers+(quantity/10)*32+half*0x160,destination,8,pattern);
                    copy_pattern(m,numbers+(quantity%10)*32+half*0x160,destination+32,8,pattern);}
                else{copy_pattern(m,numbers+0x140,destination,8,pattern);copy_pattern(m,numbers+0x140,destination+32,8,pattern);}
            }
        }
        if(row){if(!lz(m,0x0825BB34,cursor))return false;}
        else if(!hn_record_counter_palette(m,cursor,record))return false;
        cursor=next_cursor;
    }
    cursor=origin;
    for(unsigned row=0;row<class_value(m,34)&&!m->fault;row++){
        if(row>=3)return false; /* Only three pointers exist in the source frame. */
        unsigned record=class_value(m,row+35),offset=record*24;
        if(!lz(m,hn_read32(m,DESCRIPTORS+offset+20),cursor))return false;
        if(row&&!hn_record_palette_halve(m,cursor))return false;cursor+=64;
        if(!hn_transfer_decompress_rows(m,hn_read32(m,DESCRIPTORS+offset+16),destinations[row],256,8,1024))return false;
    }
    return lz(m,0x0825BB50,cursor)&&!m->fault;
}
static void velocity(HNMemory *m,uint8_t index,int16_t x,int16_t y,int16_t z)
{
    GameAddr slot=hn_render_slot(m,index);
    hn_write32(m,slot+0x24,(uint32_t)((int32_t)x*256));hn_write32(m,slot+0x28,(uint32_t)((int32_t)y*256));hn_write32(m,slot+0x2C,(uint32_t)((int32_t)z*256));
}
static bool selection_motion(HNMemory *m,GameAddr arguments,bool forward,bool select)
{
    unsigned selected=control(m,0x1A0);GameAddr slots=SLOT_PAIRS+selected*6;
    for(unsigned group=0;group<3;group++){
        int16_t x=group==0?(select?(forward?-0xA00:0xA00):(forward?0x1600:-0x1600)):
                  group==1?(forward?(select?0x1600:-0xA00):0xC00):(forward?-0xC00:(select?-0x1600:0xA00));
        int16_t y=group==0?0x200:group==(forward?1u:2u)?-0x200:0;
        uint8_t index=hn_read8(m,slots+group*2);velocity(m,index,x,y,(int16_t)-y);
        index=hn_read8(m,slots+group*2);velocity(m,(uint8_t)(index+1),x,y,(int16_t)-y);
    }
    if(select){
        if(selected==(forward?2u:0u))store(m,0x1A0,forward?0:2);
        else store(m,0x1A0,(uint8_t)(control(m,0x1A0)+(forward?1:-1)));
        return hn_record_selection_refresh(m,arguments);
    }
    return !m->fault;
}
#define REQUIRE(operation) do{if(!(operation))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_record_selection_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x08023528:{GameAddr target=hn_read32(m,VARIABLES+4);for(unsigned i=0;i<40;i++)hn_write32(m,target+i*4,0);break;}
    case 0x080236A0:REQUIRE(initialize_display(m));break;
    case 0x080237DC:{GameAddr list=hn_read32(m,VARIABLES);unsigned key=hn_read8(m,list+control(m,0x61));REQUIRE(lz(m,hn_read32(m,LABELS+(key*5+control(m,0xEB))*4),0x02004720));break;}
    case 0x0802381C:REQUIRE(navigate_counter(m,true));break;
    case 0x08023950:REQUIRE(navigate_counter(m,false));break;
    case 0x08023A84:{GameAddr entry=hn_read32(m,VARIABLES)+control(m,0x61);store(m,9,hn_read8(m,entry));store(m,0x1A5,hn_read8(m,entry));break;}
    case 0x08023AFC:REQUIRE(prepare_candidates(m));break;
    case 0x08023D14:{unsigned record=class_value(m,hn_read8(m,arguments)+35);REQUIRE(hn_record_counter_palette(m,hn_read32(m,arguments+1),record));*consumed=5;break;}
    case 0x08023D48:REQUIRE(selection_graphics(m,arguments));*consumed=4;break;
    case 0x080241F0:REQUIRE(selection_motion(m,arguments,false,false));break;
    case 0x08024290:REQUIRE(selection_motion(m,arguments,false,true));*consumed=4;break;
    case 0x08024354:REQUIRE(selection_motion(m,arguments,true,false));break;
    case 0x080243F8:REQUIRE(selection_motion(m,arguments,true,true));*consumed=4;break;
    case 0x08024574:{GameAddr fields=DESCRIPTORS+class_value(m,224)*24+10;store(m,0x68,1);
        for(unsigned field=0;field<3;field++){unsigned req=hn_read16(m,fields+field*2),key=(req>>7)&127;
            if(key){unsigned value=hn_read8(m,HN_RECORD_COUNTERS+key);if(value==255||value<(req&127)){store(m,0x68,0);break;}}}break;}
    case 0x080245E8:
        for(unsigned index=0;index<30;index++)if(!hn_read8(m,0x0201EE40+index)){store(m,0x1A6,(uint8_t)index);store(m,0,1);return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;}store(m,0,0);break;
    case 0x08024624:{unsigned record=class_value(m,control(m,0x1A0)+35);class_store(m,224,(uint16_t)record);class_store(m,6,hn_read8(m,DESCRIPTORS+record*24+7));store(m,0x1A0,(uint8_t)record);break;}
    case 0x08024668:{GameAddr record=DESCRIPTORS+control(m,0x1A0)*24;unsigned roll=hn_random_range(m,1,5),threshold=hn_read8(m,record+8)/25;
        if(roll>=threshold){store(m,0x60,1);store(m,0x61,0);}
        else{store(m,0x60,3);hn_write8(m,0x0201EE40+control(m,0x1A6),0);
            for(unsigned field=0;field<3;field++){unsigned req=hn_read16(m,record+10+field*2),key=(req>>7)&127;
                if(hn_read8(m,HN_RECORD_COUNTER_DESCRIPTORS+key*20+4)==5){hn_write8(m,HN_RECORD_COUNTERS+key,(uint8_t)(hn_read8(m,HN_RECORD_COUNTERS+key)+(req&127)));store(m,0x60,2);store(m,0x61,(uint8_t)key);}}}break;}
    case 0x08024730:{unsigned index=class_value(m,224);GameAddr fields=DESCRIPTORS+index*24+10;
        for(unsigned field=0;field<3;field++){unsigned req=hn_read16(m,fields+field*2),key=(req>>7)&127;hn_write8(m,HN_RECORD_COUNTERS+key,(uint8_t)(hn_read8(m,HN_RECORD_COUNTERS+key)-(req&127)));}
        hn_write8(m,0x0201EE40+control(m,0x1A6),(uint8_t)index);break;}
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_record_selection_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x08023528:case 0x080236A0:case 0x080237DC:case 0x0802381C:case 0x08023950:case 0x08023A84:
    case 0x08023AFC:case 0x080241F0:case 0x08024354:
    case 0x08024574:case 0x080245E8:case 0x08024624:case 0x08024668:case 0x08024730:{uint32_t ignored;return hn_record_selection_callback(m,callback,0,&ignored);}
    default:return HN_SCENE_UNKNOWN;
    }
}
