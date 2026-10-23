#include "game/task.h"
#include "game/presets.h"
#include "game/map_choice.h"
#include "game/bank2.h"
#include "game/bank3.h"
#include "game/bank4.h"
#include "game/bank5.h"
#include "game/bank6.h"
#include "game/bank7.h"
#include "game/scene_selection.h"
#include "game/record_selection.h"
#include "game/presentation.h"
#include "game/save_group3.h"
#include "game/save_group4.h"
#include "game/record_menu.h"
#include "game/wander.h"
#include "game/scene_actor.h"
#include "game/scene_motion.h"
#include "game/records.h"
#include "game/counter_scene.h"
#include "game/raster.h"
#include "game/state.h"
#include "game/render.h"
#include "game/transfer.h"
#include "game/audio.h"
#include "game/graphics.h"

bool hn_task_text_bundle(HNMemory *m,unsigned opcode,unsigned index,GameAddr *text)
{
    index&=255;if(index==255)index=hn_read8(m,HN_SCENE_CONTROLS+9);
    GameAddr table=0;
    switch(opcode){
    case 0xF0:case 0xFA:table=HN_TASK_TEXT_BUNDLES;break;
    case 0xF1:case 0xFB:table=HN_TASK_GROUP_E;break;
    case 0xF6:case 0xFC:table=index<=3?HN_TASK_GROUP_A:HN_TASK_GROUP_B;if(index>3)index-=3;break;
    case 0xF7:case 0xFD:table=HN_TASK_GROUP_C;break;
    case 0xF8:case 0xFE:table=HN_TASK_GROUP_D;break;
    default:return false;
    }
    if(table!=HN_TASK_TEXT_BUNDLES){
        unsigned variant=hn_read8(m,HN_TASK_VARIANT);if(variant>=3)return false;
        table=hn_read32(m,table+variant*4);
    }
    GameAddr bundle=hn_read32(m,table+index*4);*text=bundle;
    if(bundle!=HN_TASK_DEFAULT_SCRIPT){
        unsigned language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);if(language>=5)return false;
        *text=hn_read32(m,bundle+language*4);
    }
    return !m->fault;
}
GameAddr hn_task_text_adjust(HNMemory *m,GameAddr text,uint32_t buffer_index)
{
    GameAddr output=HN_TASK_ADJUSTED_TEXT+(buffer_index<<5);
    for(unsigned i=0;i<8;i++)hn_write32(m,output+i*4,0xE1);
    for(unsigned i=0;i<32;i++){
        uint8_t value=hn_read8(m,text+i);
        hn_write8(m,output+i,(uint8_t)((!i&&(uint8_t)(value-0x61)<=0x2E)?value-0x55:value));
        if((uint8_t)(value+0x20)<=1)break;
    }
    return m->fault?0:output;
}
static bool measure(HNMemory *m,GameAddr text,uint8_t style,bool items,uint32_t *result,unsigned depth,unsigned *budget)
{
    if(depth>=64)return false;uint32_t total=0;
    while(*budget){
        (*budget)--;unsigned opcode=hn_read8(m,text++);if(m->fault)return false;
        if(!opcode||(opcode>=0xE0&&opcode<=0xE4)){*result=total;return true;}
        if(opcode<=0xDF){total+=items?1:hn_read8(m,HN_TEXT_ADVANCES+opcode-1+(style?256:0));continue;}
        GameAddr child=0;
        if(opcode==0xE6)child=HN_TEXT_SECOND;
        else if(opcode==0xE7)child=HN_TEXT_FIRST;
        else if(opcode==0xF2||opcode==0xF3){text+=2;continue;}
        else if(opcode==0xF0||opcode==0xF1||(opcode>=0xF6&&opcode<=0xF8)||(opcode>=0xFA&&opcode<=0xFE)){
            if(!hn_task_text_bundle(m,opcode,hn_read8(m,text++),&child))return false;
            if(opcode>=0xFA){child=hn_task_text_adjust(m,child,0);if(!child)return false;}
        }else{if(opcode>0xF0)text++;continue;}
        uint32_t count;if(!measure(m,child,style,items,&count,depth+1,budget))return false;total+=count;
    }
    return false;
}
bool hn_task_text_measure(HNMemory *m,GameAddr text,uint8_t style,bool items,uint32_t *result)
{unsigned budget=16384;return measure(m,text,style,items,result,0,&budget)&&!m->fault;}
bool hn_task_text_offset(HNMemory *m,GameAddr task,uint32_t offset)
{
    if(!task)return false;offset&=65535;
    if(offset<=254)hn_write16(m,task+0x1C,(uint16_t)offset);
    else{
        uint32_t count,width;GameAddr script=hn_read32(m,task+0x10);
        if(!hn_task_text_measure(m,script,0,true,&count))return false;
        unsigned advance=hn_read8(m,task+4);
        if(advance&128)width=count*(advance&127);
        else{
            if(!hn_task_text_measure(m,script,(hn_read8(m,task+0xB)&8)>>3,false,&width))return false;
            width+=advance*(count-1);
        }
        int32_t space=(int32_t)((uint32_t)hn_read8(m,task+1)*8-width);
        hn_write16(m,task+0x1C,(uint16_t)(space/2));
    }
    return !m->fault;
}
bool hn_task_set_script(HNMemory *m,GameAddr task,GameAddr script)
{
    if(!task)return false;hn_write32(m,task+0x10,script);hn_write8(m,task,3);
    unsigned offset=hn_read16(m,task+0x1C);if(hn_read8(m,task+3)==255&&!offset)offset=255;
    return hn_task_text_offset(m,task,offset);
}
void hn_task_text_variant(HNMemory *m,GameAddr task,uint8_t variant)
{
    unsigned next=hn_read8(m,hn_read32(m,task+0x10));
    bool selected=next==0xF1||(next>=0xF6&&next<=0xF9)||(next>=0xFB&&next<=0xFE);
    hn_write8(m,HN_TASK_VARIANT,selected?variant:0);
}
bool hn_task_text_retarget(HNMemory *m,GameAddr variable)
{
    unsigned selected=hn_read8(m,HN_TASK_VARIANT);if(!selected)return !m->fault;if(selected>=3)return false;
    for(unsigned variant=0;variant<3;variant++){
        if(variant==selected)continue;
        for(unsigned group=0;group<5;group++){
            GameAddr family=hn_read32(m,HN_TASK_VARIANT_FAMILIES+group*4),table=hn_read32(m,family+variant*4);
            for(unsigned index=0;index<=80;index++){
                GameAddr bundle=hn_read32(m,table+index*4);if(bundle==HN_TASK_DEFAULT_SCRIPT)break;
                if(bundle==hn_read32(m,variable)){
                    hn_write32(m,variable,hn_read32(m,hn_read32(m,family+selected*4)+index*4));return !m->fault;
                }
            }
        }
    }
    return !m->fault;
}
void hn_task_text_number(HNMemory *m,uint32_t format_arg,uint32_t value_arg)
{
    const GameAddr output=HN_TASK_NUMBER_TEXT;unsigned format=format_arg&255,value=value_arg&65535;
    uint8_t digits[5];unsigned count=0,divisor=10000,padding=0,width=format&127;
    for(unsigned i=0;i<5;i++)hn_write8(m,output+i,0);
    while(divisor>value&&divisor>1)divisor/=10;
    do{digits[count++]=(uint8_t)(value/divisor+2);value%=divisor;divisor/=10;}while(divisor);
    if(width){
        if((format&128)&&width>count){padding=width-count;for(unsigned i=0;i<padding;i++)hn_write8(m,output+i,1);}
        unsigned first=width<count?count-width:0;count-=first;
        for(unsigned i=0;i<count;i++){
            hn_write8(m,output+padding+i,digits[first+i]);
            if(padding+i+1>5){hn_write8(m,output+6,0);break;}
        }
    }else for(unsigned i=0;i<count;i++)hn_write8(m,output+i,digits[i]);
}
static bool task_icon(HNMemory *m,GameAddr task,unsigned kind)
{
    GameAddr resource=hn_read32(m,task+0x30);
    if(resource){
        unsigned number=0;while(number<5&&hn_read8(m,hn_render_slot(m,number+3)+0x60))number++;
        unsigned index=number+3;if(index>=hn_read8(m,HN_RENDER_INFO+0x10)||!hn_read32(m,HN_RENDER_SLOTS))return false;
        hn_write8(m,task+0xA,(uint8_t)(hn_read8(m,task+0xA)|(1u<<number)));
        hn_render_slot_reset(m,index);GameAddr slot=hn_render_slot(m,index);
        unsigned row=hn_read16(m,task+0x1E),height=hn_read8(m,task+2);
        uint32_t x=(uint32_t)((int16_t)hn_read16(m,resource+0x20)+(int32_t)hn_read16(m,task+0x1C));
        uint32_t y=(uint32_t)(int32_t)(int16_t)hn_read16(m,resource+0x22)+(row*2<height?row:(height>>1)-1)*16;
        hn_write32(m,slot+0x18,(x<<16)+0x8000);hn_write32(m,slot+0x1C,(y<<16)+0x8000);hn_write32(m,slot+0x20,0);
        hn_write8(m,slot+0x60,1);hn_write8(m,slot+0x62,120);
        hn_render_set_animation(m,index,hn_read32(m,HN_TASK_ICON_ANIMATIONS+(kind*5+number)*4));hn_render_flags(m,index,12,0);
    }
    hn_write16(m,task+0x1C,(uint16_t)(hn_read16(m,task+0x1C)+16));return !m->fault;
}
static bool task_sound(HNMemory *m)
{return hn_audio_ready(m)&&hn_audio_song(m,0x145,HN_SONG_START);}
static bool advance_row(HNMemory *m,GameAddr task,bool scroll)
{
    hn_write8(m,task,scroll?2:3);hn_write8(m,task+0xB,hn_read8(m,task+0xB)|4);
    hn_write16(m,task+0x1E,(uint16_t)(hn_read16(m,task+0x1E)+1));hn_write32(m,task+0x10,hn_read32(m,task+0x10)+1);
    if(!hn_task_text_offset(m,task,hn_read8(m,task+3)))return false;
    return !scroll||(hn_task_scroll_map(m,task,false)&&hn_task_render_scroll(m,task,true));
}
/* Void script callbacks use guest addresses only as dispatch identifiers. */
static bool task_callback(HNMemory *m,GameAddr callback)
{
    GameAddr animation=0,secondary=0,graphics=0;bool indicator=false,configure=false;
    switch(callback&~1u){
    case 0x080CB164:animation=0x088BA51E;break;
    case 0x080CB178:animation=0x088BA52A;break;
    case 0x080CB18C:animation=0x088BA536;break;
    case 0x080CB1A0:animation=0x088BA54B;break;
    case 0x080CB1B4:animation=0x088BA557;break;
    case 0x080CB1C8:animation=0x088BA563;break;
    case 0x080CB1DC:animation=0x088BA583;secondary=0x088BA59E;configure=true;break;
    case 0x080CB218:animation=0x088BA5B2;break;
    case 0x080CB22C:animation=0x088BA5EB;break;
    case 0x080CB240:animation=0x088BA456;break;
    case 0x080CB254:animation=0x088BA462;break;
    case 0x080CB268:animation=0x088BA46E;break;
    case 0x080CB27C:animation=0x088BA47A;break;
    case 0x080CB290:animation=0x088BA486;break;
    case 0x080CB2A4:case 0x080CB2A8:case 0x080CB2AC:case 0x080CB2B0:case 0x080CB3C0:return !m->fault;
    case 0x080CB2B4:animation=0x088ABCC8;indicator=true;break;
    case 0x080CB2D4:animation=0x088ABD66;indicator=true;secondary=0x088E16A1;graphics=0x088E1348;break;
    case 0x080CB30C:animation=0x088ABEE8;indicator=true;break;
    case 0x080CB32C:animation=0x088ABFAE;indicator=true;secondary=0x088E16F9;configure=true;graphics=0x088E1340;break;
    case 0x080CB384:animation=0x088BA4A8;break;
    case 0x080CB398:animation=0x088BA4B4;break;
    case 0x080CB3AC:animation=0x088BA4C0;break;
    case 0x080CB3C4:animation=0x088ABE43;indicator=true;break;
    /* These reviewed u32 callbacks ignore their argument register; F4 ignores return. */
    case 0x0801C5F4:case 0x0801C66C:case 0x0801CB7C:case 0x0801CCE4:{
        uint32_t ignored;return hn_wander_callback(m,callback,0,&ignored)==HN_SCENE_OK;}
    case 0x0801E00C:{uint32_t ignored;return hn_records_callback(m,callback,0,&ignored)==HN_SCENE_OK;}
    case 0x0801EBBC:case 0x0801EBDC:case 0x0801EC20:case 0x0801EC3C:
    case 0x0801EC6C:case 0x0801ECAC:case 0x0801ECC4:case 0x0801F118:{
        uint32_t ignored;return hn_scene_motion_callback(m,callback,0,&ignored)==HN_SCENE_OK;}
    case 0x0802E22C:case 0x0802E258:{uint32_t ignored;return hn_raster_callback(m,callback,&ignored)==HN_SCENE_OK;}
    default:{HNSceneResult result=hn_bank3_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_bank4_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_bank5_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_bank6_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_bank7_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_scene_selection_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_record_selection_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_presentation_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_group3_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_group4_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_record_menu_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_records_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_counter_scene_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_scene_actor_task(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        result=hn_bank2_apply(m,callback);if(result!=HN_SCENE_UNKNOWN)return result==HN_SCENE_OK;
        uint32_t ignored;result=hn_map_choice_apply(m,callback,&ignored);
        return result==HN_SCENE_UNKNOWN?hn_preset_apply(m,callback)==HN_SCENE_OK:result==HN_SCENE_OK;}
    }
    if(hn_read8(m,HN_RENDER_INFO+0x10)<(secondary?18:17)||!hn_read32(m,HN_RENDER_SLOTS))return false;
    if(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)==16)hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,16,animation);
    if(indicator){hn_write8(m,0x03001FAC,16);hn_write32(m,0x03001F7C,animation);}
    if(secondary){
        if(configure){
            if(!hn_render_configure(m,17,1,0xB0,0,0,0,secondary))return false;
            GameAddr slot=hn_render_slot(m,17);hn_write8(m,slot+0x65,16);hn_write8(m,slot+0x62,0xB0);
        }else{
            if(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)==17)hn_write8(m,0x03002170,0);
            hn_render_set_animation(m,17,secondary);
        }
    }
    return (!graphics||hn_graphics_configure(m,4,graphics))&&!m->fault;
}
static bool command(HNMemory *m,GameAddr task,unsigned opcode,uint32_t *keep_going)
{
    GameAddr script=hn_read32(m,task+0x10),slot=hn_render_slot(m,0);unsigned keys=hn_read16(m,HN_TASK_KEYS),half=hn_read8(m,task+2)>>1;
    *keep_going=1;
    if(opcode==0xF2||opcode==0xF3){
        unsigned format=hn_read8(m,script),index=hn_read8(m,script+1);
        hn_write32(m,task+0x10,script+2);
        hn_task_text_number(m,format,opcode==0xF2?hn_read8(m,HN_SCENE_CONTROLS+index):hn_read16(m,0x0201DC40+index*2));
        hn_write32(m,task+0x14,hn_read32(m,task+0x10));hn_write32(m,task+0x10,HN_TASK_NUMBER_TEXT);return !m->fault;
    }
    if(opcode==0xF4){
        unsigned index=hn_read8(m,script);hn_write32(m,task+0x10,script+1);
        return task_callback(m,hn_read32(m,0x03001FC0+index*4));
    }
    if(opcode==0xF5){unsigned kind=hn_read8(m,script);hn_write32(m,task+0x10,script+1);return task_icon(m,task,kind);}
    if(opcode==0xF9){
        GameAddr variable=0x03002BB0+(hn_read8(m,script)&15)*4;
        hn_write32(m,task+0x10,script+1);hn_write32(m,task+0x14,script+1);
        if(!hn_task_text_retarget(m,variable))return false;
        GameAddr text=hn_read32(m,variable);
        if(text!=HN_TASK_DEFAULT_SCRIPT){unsigned language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);if(language>=5)return false;text=hn_read32(m,text+language*4);}
        hn_write32(m,task+0x10,text);hn_write8(m,HN_TASK_VARIANT,0);return !m->fault;
    }
    if(opcode==0xFF){hn_task_text_variant(m,task,1);return !m->fault;}
    if(opcode==0xF0||opcode==0xF1||(opcode>=0xF6&&opcode<=0xF8)||(opcode>=0xFA&&opcode<=0xFE)){
        unsigned index=hn_read8(m,script);hn_write32(m,task+0x10,script+1);hn_write32(m,task+0x14,script+1);
        GameAddr text;if(!hn_task_text_bundle(m,opcode,index,&text))return false;
        if(opcode>=0xFA){text=hn_task_text_adjust(m,text,0);if(!text)return false;}
        hn_write32(m,task+0x10,text);if(opcode!=0xF0&&opcode!=0xFA)hn_write8(m,HN_TASK_VARIANT,0);return !m->fault;
    }
    if(opcode>=0xF0)return false;
    switch(opcode){
    case 0xE0:{
        if(!hn_read8(m,slot+0x60)&&!hn_task_indicator(m,task))return false;
        GameAddr saved=hn_read32(m,task+0x14);*keep_going=0;
        if(saved){if(keys){if(!task_sound(m))return false;hn_write32(m,task+0x10,saved);hn_write32(m,task+0x14,0);hn_write8(m,slot+0x60,0);*keep_going=1;}}
        else{if(hn_read8(m,HN_TASK_DIRTY)&&!hn_task_row_transfer(m,task))return false;
            if(keys){if(!task_sound(m))return false;hn_write8(m,task,1);hn_write8(m,slot+0x60,0);}}
        break;
    }
    case 0xE1:{
        GameAddr saved=hn_read32(m,task+0x14);
        if(saved){if(hn_read8(m,HN_TASK_DIRTY)&&!hn_task_row_transfer(m,task))return false;
            hn_write32(m,task+0x10,saved);hn_write32(m,task+0x14,0);}
        else{if(!hn_task_row_transfer(m,task))return false;hn_write8(m,task,1);*keep_going=0;}
        break;
    }
    case 0xE2:{
        *keep_going=0;if(hn_read8(m,HN_TASK_DIRTY)&&!hn_task_row_transfer(m,task))return false;
        unsigned remaining=hn_read8(m,task+8);
        if(hn_read16(m,task+0x1E)<hn_read8(m,task+9)){
            if(!advance_row(m,task,false))return false;if(remaining)hn_write8(m,task+8,(uint8_t)(remaining-1));
        }else if(remaining){hn_write8(m,slot+0x60,0);if(!advance_row(m,task,true))return false;hn_write8(m,task+8,(uint8_t)(hn_read8(m,task+8)-1));}
        else if(keys){if(!task_sound(m))return false;hn_write8(m,slot+0x60,0);if(!advance_row(m,task,true))return false;hn_write8(m,task+8,(uint8_t)(half-1));}
        else if(!hn_read8(m,slot+0x60)&&!hn_task_indicator(m,task))return false;
        break;
    }
    case 0xE3:case 0xE4:{
        *keep_going=0;if((opcode==0xE4||hn_read8(m,HN_TASK_DIRTY))&&!hn_task_row_transfer(m,task))return false;
        if(opcode==0xE4||keys){
            if(opcode==0xE3){if(!task_sound(m))return false;hn_write8(m,slot+0x60,0);hn_write8(m,task+8,(uint8_t)(half-1));}
            if(!advance_row(m,task,hn_read16(m,task+0x1E)>=hn_read8(m,task+9)))return false;
            if(opcode==0xE4){unsigned remaining=hn_read8(m,task+8);if(remaining)hn_write8(m,task+8,(uint8_t)(remaining-1));hn_write8(m,task+0xB,hn_read8(m,task+0xB)|4);}
        }else if(!hn_read8(m,slot+0x60)&&!hn_task_indicator(m,task))return false;
        break;
    }
    case 0xE5:
        *keep_going=0;if(hn_read8(m,HN_TASK_DIRTY)&&!hn_task_row_transfer(m,task))return false;
        if(keys){if(!task_sound(m))return false;hn_write8(m,slot+0x60,0);hn_write8(m,task+8,(uint8_t)(half-1));hn_write32(m,task+0x10,script+1);*keep_going=1;}
        else if(!hn_read8(m,slot+0x60)&&!hn_task_indicator(m,task))return false;break;
    case 0xE6:case 0xE7:
        hn_write32(m,task+0x14,script+1);hn_write32(m,task+0x10,opcode==0xE6?HN_TEXT_SECOND:HN_TEXT_FIRST);break;
    case 0xE8:case 0xE9:case 0xEA:hn_write8(m,task+7,(uint8_t)(0xEB-opcode));hn_write32(m,task+0x10,script+1);break;
    case 0xEB:case 0xEC:case 0xED:
        if(opcode!=0xEB&&!hn_read8(m,task+5)&&!hn_task_row_transfer(m,task))return false;
        hn_write8(m,task+5,opcode==0xEB?0:opcode==0xEC?6:2);hn_write32(m,task+0x10,script+1);break;
    case 0xEE:
        *keep_going=0;if(hn_read8(m,HN_TASK_DIRTY)&&!hn_task_row_transfer(m,task))return false;
        if(keys){
            if(!task_sound(m))return false;uint32_t pattern=(uint32_t)hn_read8(m,task+7)*0x44444444u;
            hn_write8(m,slot+0x60,0);hn_write16(m,task+0x1E,hn_read16(m,task+0x20));hn_write32(m,task+0x10,script+1);
            if(!hn_task_text_offset(m,task,hn_read8(m,task+3)))return false;hn_write8(m,task+0xB,hn_read8(m,task+0xB)|4);
            hn_write8(m,task,hn_read8(m,task)&127);hn_write8(m,task+8,(uint8_t)(half-1));
            hn_write8(m,task+9,(uint8_t)(hn_read16(m,task+0x1E)-1+half));
            hn_transfer_queue_fill(m,pattern,hn_read32(m,task+0xC),hn_read8(m,task+1)*(hn_read8(m,task+2)<<5));
            if(!hn_task_render_disable(m,task))return false;
        }else if(!hn_read8(m,slot+0x60)&&!hn_task_indicator(m,task))return false;
        break;
    case 0xEF:{
        if(hn_read8(m,HN_TASK_DIRTY)&&!hn_task_row_transfer(m,task))return false;
        unsigned index=hn_read8(m,0x03001FAC);if(index>=hn_read8(m,HN_RENDER_INFO+0x10))return false;
        GameAddr current=hn_render_slot(m,index),wanted=hn_read32(m,0x03001F7C),animation=hn_read32(m,current+4);
        if(wanted&&hn_read8(m,current+0x60)&&animation&&animation==wanted)*keep_going=0;
        else hn_write32(m,task+0x10,script+1);break;
    }
    default:return false;
    }
    return !m->fault;
}
bool hn_task_command(HNTasks *tasks,GameAddr task,unsigned opcode,uint32_t *keep_going)
{
    if(tasks->fault)return false;HNMemory *m=tasks->memory;
    if(!task||((opcode<=0xE5||opcode==0xEE||opcode==0xEF)&&(!hn_read8(m,HN_RENDER_INFO+0x10)||!hn_read32(m,HN_RENDER_SLOTS)))){
        tasks->fault=HN_TASK_RESOURCE;tasks->fault_opcode=opcode;tasks->fault_address=task;return false;
    }
    if(!command(m,task,opcode,keep_going)){
        tasks->fault=m->fault?HN_TASK_MEMORY:opcode==0xF4?HN_TASK_CALLBACK:
            (opcode<0xE0||opcode>0xFF)?HN_TASK_OPCODE:HN_TASK_RESOURCE;
        tasks->fault_opcode=opcode;tasks->fault_address=m->fault?m->fault_address:hn_read32(m,task+0x10);
        if(tasks->fault==HN_TASK_CALLBACK)tasks->fault_address=hn_read32(m,0x03001FC0+hn_read8(m,tasks->fault_address-1)*4);
        return false;
    }
    return true;
}
