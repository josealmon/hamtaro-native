#include "game/records.h"
#include "game/state.h"
#include "game/render.h"
#include "game/text.h"
#include "game/transfer.h"
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
enum { CLASSES=0x0201DC40,RESULT=0x03001FBC,VARIABLES=0x03002BB0,
       RECORD_TEXT=0x03003CE0,RECORD_TEXT_TARGET=0x02000AE0,
       RECORD_RENDER_BANKS=0x02000A94,RECORD_RENDER_INDEXES=0x02000AA4,
       RECORD_DESTINATIONS=0x02000AB4,RECORD_REMAINING=0x02000AC0 };
typedef struct {GameAddr data;uint8_t kind,index,maximum,count;} RecordView;
static uint8_t control(HNMemory *m,unsigned index){return hn_read8(m,HN_SCENE_CONTROLS+index);}
static GameAddr resource(HNMemory *m,GameAddr table,uint32_t index){return hn_read32(m,table+index*4);}
static void flag(HNMemory *m,unsigned id,bool value)
{
    GameAddr address=0x03002180+(id>>3);unsigned mask=1u<<(id&7);
    hn_write8(m,address,(uint8_t)((hn_read8(m,address)&~mask)|(value?mask:0)));
}
static RecordView view(HNMemory *m,unsigned descriptor,int32_t count)
{
    RecordView r={0,(uint8_t)(descriptor&3),(uint8_t)((descriptor>>2)&127),
                  (uint8_t)((descriptor>>11)&31),(uint8_t)count};
    r.data=resource(m,resource(m,HN_RECORD_BANKS,r.kind),r.index);return r;
}
bool hn_record_accumulate(HNMemory *m,GameAddr entry,GameAddr data,uint8_t kind,uint8_t index)
{
    for(unsigned i=0;i<64&&!m->fault;i++,entry+=8){
        if(!hn_read32(m,entry)){
            hn_write32(m,entry,data);hn_write8(m,entry+4,kind);hn_write8(m,entry+5,index);
            hn_write8(m,entry+6,(uint8_t)(kind==0&&index==6?10:1));break;
        }
        if(hn_read8(m,entry+4)==kind&&hn_read8(m,entry+5)==index){
            hn_write8(m,entry+6,(uint8_t)(hn_read8(m,entry+6)+(kind==0&&index==6?10:1)));break;
        }
    }
    return !m->fault;
}
static GameAddr localized(HNMemory *m,GameAddr table,uint8_t index)
{
    GameAddr bundle=resource(m,table,index);
    return bundle==HN_TASK_DEFAULT_SCRIPT?bundle:resource(m,bundle,control(m,0xEB));
}
static bool record_text(HNMemory *m,GameAddr text,unsigned length,unsigned adjusted,unsigned word,unsigned mode,GameAddr entry)
{
    while(length<=31&&!m->fault){
        uint8_t code=hn_read8(m,text++);
        if(!code){hn_write8(m,RECORD_TEXT+length,0);break;}
        if(code==1){
            if(mode==0)adjusted=word!=0;
            else if(mode==1)adjusted=control(m,0xEB)==3&&word!=0;
            else adjusted=(control(m,0xEB)==3&&word!=0)||hn_read8(m,entry+4)!=2;
            word=(uint8_t)(word+1);
        }else if(!adjusted&&(uint8_t)(code-0x61)<=0x30){code=(uint8_t)(code-0x55);adjusted=1;}
        else if((uint8_t)(code-0x0C)<=0x30)adjusted=1;
        hn_write8(m,RECORD_TEXT+length++,code);
    }
    hn_write8(m,RECORD_TEXT+31,0);hn_write32(m,VARIABLES,RECORD_TEXT_TARGET);return !m->fault;
}
static bool consume_selected(HNMemory *m)
{
    uint16_t words[6];uint8_t categories[4];
    for(unsigned i=0;i<6;i++)words[i]=hn_read16(m,0x085CBF80+i*2);
    for(unsigned i=0;i<4;i++)categories[i]=hn_read8(m,0x085CBF8C+i);
    uint16_t index=hn_read16(m,CLASSES+66),raw=hn_read16(m,HN_RECORD_SLOTS+index*4);
    unsigned descriptor=hn_read16(m,HN_RECORD_DESCRIPTORS+(raw&127)*4);
    uint8_t comparison=(uint8_t)(hn_read8(m,CLASSES+24)+1);
    bool filled=(hn_read8(m,0x030021B2)&2)||((int)((descriptor>>11)&31)-(int)((raw>>7)&31)<comparison);
    int32_t count=hn_record_consume(m,index,comparison,0);
    /* The failed comparison leaves the source local unwritten. Canonical zero
       replaces those two indeterminate control bytes; no record is consumed. */
    RecordView r=filled?view(m,descriptor,count):(RecordView){0};
    unsigned result=count==0?(r.kind==3&&r.index==2?1:3):count==-1?0:2;
    hn_write16(m,RESULT,(uint16_t)result);hn_write8(m,HN_SCENE_CONTROLS+0x2C,r.kind);
    hn_write8(m,HN_SCENE_CONTROLS+0x2D,r.index);
    if(result!=3)return !m->fault;
    if(!hn_record_accumulate(m,hn_read32(m,VARIABLES+4)+((uint32_t)categories[r.kind]<<9),r.data,r.kind,r.index))return false;
    if(r.kind==0){
        /* Outside 1..6 the original reads beyond its local class-word array. */
        if(!r.index||r.index>6)return false;
        hn_write16(m,CLASSES+12,words[r.index-1]);unsigned length=0,adjusted=control(m,0xEB)!=3,word=0;
        GameAddr text=localized(m,r.index==6?0x0858C6D8:0x0858C6FC,r.index);
        if(r.index==6){
            adjusted=0;word=1;
            if(control(m,0xEB)==3){
                hn_write8(m,RECORD_TEXT,0x0F);hn_write8(m,RECORD_TEXT+1,0x65);
                hn_write8(m,RECORD_TEXT+2,0x73);hn_write8(m,RECORD_TEXT+3,1);length=4;
            }
        }
        return record_text(m,text,length,adjusted,word,0,0);
    }
    if(r.kind==1){flag(m,(unsigned)r.index+0x9D,true);hn_write32(m,VARIABLES,r.data);}
    if(r.kind==2){
        flag(m,0x12D,hn_record_counter(m,r.index)==255);(void)hn_record_adjust_counter(m,r.index,1);
        if(!record_text(m,localized(m,0x0858CA6C,r.index),0,1,0,1,0))return false;
        if(control(m,0xEB)==3){
            unsigned first=hn_read8(m,RECORD_TEXT),next=hn_read8(m,RECORD_TEXT+1);
            if(first==0x75&&next==0x6E)hn_write8(m,RECORD_TEXT,0x20);
            else if(first==0x64&&next==0x75)hn_write8(m,RECORD_TEXT,0x0F);
        }
    }
    return !m->fault;
}
static bool select_record(HNMemory *m,unsigned mode)
{
    GameAddr selected=CLASSES+66;
    if(mode==3)hn_write16(m,selected,63);
    else if(mode==2&&(int16_t)hn_read16(m,selected)>=64){hn_write16(m,selected,65535);return true;}
    for(unsigned iterations=0;iterations<65536&&!m->fault;iterations++){
        if(mode<2)hn_write16(m,selected,(uint16_t)(hn_read16(m,selected)+(mode==0?1:-1)));
        int32_t index=(int16_t)hn_read16(m,selected);
        if((mode==0&&index>63)||((mode==1||mode==3)&&index<0))break;
        GameAddr slot=HN_RECORD_SLOTS+(uint32_t)index*4;
        if((hn_read16(m,slot)>>7)&31){
            unsigned key=hn_read8(m,slot)&127;
            unsigned property=(hn_read8(m,HN_RECORD_DESCRIPTORS+key*4+1)>>1)&3;
            hn_write16(m,RESULT,(uint16_t)property);return !m->fault;
        }
        if(mode>=2){
            hn_write16(m,selected,(uint16_t)(index+(mode==3?-1:1)));
            if(mode==2&&(int16_t)hn_read16(m,selected)>63)break;
        }
    }
    if(m->fault)return false;
    hn_write16(m,selected,65535);return true;
}
static bool present_accumulated(HNMemory *m,GameAddr arguments)
{
    GameAddr category=HN_SCENE_CONTROLS+hn_read16(m,arguments);
    GameAddr entry=hn_read32(m,VARIABLES+4)+((uint32_t)hn_read8(m,category)<<9);
    for(unsigned iterations=0;iterations<65536&&!m->fault;iterations++){
        unsigned index=hn_read8(m,entry+5);
        if(!index){
            uint8_t next=(uint8_t)(hn_read8(m,category)+1);hn_write8(m,category,next);
            if(next>3){hn_write8(m,category,255);return !m->fault;}
            entry=hn_read32(m,VARIABLES+4)+((uint32_t)next<<9);continue;
        }
        if(index==255){entry+=8;continue;}
        GameAddr banks=hn_read8(m,entry+6)==1?0x085CBF90:0x085CBFA0;
        GameAddr text=localized(m,resource(m,banks,hn_read8(m,entry+4)),(uint8_t)index);
        if(!record_text(m,text,0,0,1,2,entry))return false;
        hn_write8(m,HN_SCENE_CONTROLS,hn_read8(m,entry+6));hn_write8(m,entry+5,255);return !m->fault;
    }
    return false;
}
static bool selected_graphics(HNMemory *m)
{
    GameAddr bank=resource(m,RECORD_RENDER_BANKS,control(m,0x2C));
    GameAddr source=resource(m,bank,((uint32_t)control(m,0x2D)-1)*2);
    if(!hn_bios_lz77(m,source,0x0201BE60,0x40000))return false;
    hn_transfer_tiles(m,0x0201BE60,0x06017740,3,3);
    bank=resource(m,RECORD_RENDER_BANKS,control(m,0x2C));
    source=resource(m,bank,(uint32_t)control(m,0x2D)*2-1);
    if(!hn_bios_lz77(m,source,0x0200F720,0x40000))return false;
    bank=resource(m,RECORD_RENDER_INDEXES,control(m,0x2C));
    GameAddr destination=resource(m,RECORD_DESTINATIONS,hn_read8(m,bank+(uint32_t)control(m,0x2D)-1));
    hn_transfer_queue(m,0x0200F720,destination,10);return !m->fault;
}
static bool remaining_graphics(HNMemory *m)
{
    unsigned raw=hn_read16(m,HN_RECORD_SLOTS+(uint8_t)hn_read16(m,CLASSES+66)*4),count=(raw>>7)&31;
    if(!count)return false; /* The original leaves its local result undefined. */
    RecordView r=view(m,hn_read16(m,HN_RECORD_DESCRIPTORS+(raw&127)*4),(int32_t)count);
    GameAddr source=resource(m,RECORD_REMAINING,(uint32_t)((int)r.maximum-(int)r.count));
    if(!hn_bios_lz77(m,source,0x0200F720,0x40000))return false;
    hn_transfer_queue(m,0x0200F720,0x050003AC,10);return !m->fault;
}
static bool display_counter(HNMemory *m,unsigned mode)
{
    hn_write16(m,CLASSES+22,hn_records_count(m));
    if(mode==2){
        hn_write32(m,HN_COUNTER_COUNTDOWN,0);
        bool any=hn_records_count(m)!=0;flag(m,19,any);
        if(any){hn_write32(m,HN_COUNTER_DELTA,UINT32_MAX);hn_write32(m,HN_COUNTER_STEPS,1);hn_write32(m,HN_COUNTER_DELAY,1);}
    }
    if(!hn_text_counter(m,hn_read16(m,CLASSES+22),1))return false;
    hn_write16(m,CLASSES+14,hn_read16(m,CLASSES+22));
    GameAddr image=mode==0?0x0889F713:mode==1?0x0889F6EF:0x0889F735;
    if(!hn_render_configure_raw(m,2,1,0,176,mode==0?-16:8,0,image))return false;
    hn_render_flags(m,2,12,0);if(mode<2)hn_write8(m,HN_SCENE_CONTROLS+12,mode==0?128:0);
    return !m->fault;
}
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
    case 0x0801DEC4:*consumed=0;return display_counter(m,0)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x0801DF1C:*consumed=0;return display_counter(m,1)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x0801DF74:*consumed=0;return display_counter(m,2)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x08022AB0:*consumed=0;return consume_selected(m)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x08022E1C:*consumed=0;return select_record(m,0)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x08022E78:*consumed=0;return select_record(m,1)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x08022ED4:*consumed=0;return select_record(m,2)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x08022F3C:*consumed=0;return select_record(m,3)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x08022F9C:*consumed=0;return selected_graphics(m)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x0802302C:*consumed=2;return present_accumulated(m,arguments)?HN_SCENE_OK:HN_SCENE_RESOURCE;
    case 0x080231B0:*consumed=0;return remaining_graphics(m)?HN_SCENE_OK:HN_SCENE_RESOURCE;
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
HNSceneResult hn_records_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x08023264:return HN_SCENE_OK;
    case 0x0801DEC4:case 0x0801DF1C:case 0x0801DF74:case 0x08022AB0:
    case 0x08022E1C:case 0x08022E78:case 0x08022ED4:case 0x08022F3C:
    case 0x08022F9C:case 0x080231B0:{uint32_t ignored;return hn_records_callback(m,callback,0,&ignored);}
    default:return HN_SCENE_UNKNOWN;
    }
}
