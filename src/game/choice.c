#include "game/choice.h"
#include "game/state.h"
#include "game/task.h"
#include "game/background.h"
#include "game/graphics.h"
#include "game/render.h"
#include "game/motion.h"
#include "game/input.h"
#include "game/audio.h"
#include "game/expression.h"

static GameAddr record(unsigned index){return HN_CHOICE_RECORDS+index*12;}
static bool geometry(HNMemory *m,GameAddr descriptor,unsigned *columns,unsigned *rows)
{
    *columns=hn_read8(m,descriptor+16);*rows=hn_read8(m,descriptor+17);
    return *columns&&*rows&&(*columns)*(*rows)<=9&&!m->fault;
}
static bool position(HNMemory *m,GameAddr descriptor,unsigned selected)
{
    unsigned columns,rows;if(!geometry(m,descriptor,&columns,&rows)||selected>=9)return false;
    hn_write8(m,HN_CHOICE_ROW,(uint8_t)(selected/columns));hn_write8(m,HN_CHOICE_COLUMN,(uint8_t)(selected%columns));return !m->fault;
}
bool hn_choice_initial_selection(HNMemory *m,GameAddr descriptor)
{
    unsigned selected=0,index;uint32_t wanted=hn_read32(m,HN_CHOICE_INITIAL);
    for(index=0;index<9;index++)if(hn_read32(m,record(index))==wanted){selected=index;break;}
    return position(m,descriptor,selected);
}
static bool graphics(HNMemory *m,unsigned mode,GameAddr script)
{
    if(!hn_graphics_configure(m,0,script))return false;
    hn_write8(m,hn_read32(m,HN_GRAPHICS_SCRIPTS),(uint8_t)mode);return !m->fault;
}
bool hn_choice_display(HNMemory *m,GameAddr descriptor)
{
    unsigned columns,rows;if(!geometry(m,descriptor,&columns,&rows)||hn_read8(m,HN_RENDER_INFO+16)<2||!hn_read32(m,HN_RENDER_SLOTS))return false;
    unsigned row=hn_read8(m,HN_CHOICE_ROW),column=hn_read8(m,HN_CHOICE_COLUMN),width=hn_read8(m,descriptor+18);
    unsigned selected=row*columns+column;if(row>=rows||column>=columns||selected>=9)return false;
    if(!graphics(m,1,HN_CHOICE_CURSOR_GRAPHICS))return false;
    hn_render_set_animation(m,0,HN_CHOICE_CURSOR_0);hn_render_set_animation(m,1,width==10?HN_CHOICE_CURSOR_10:HN_CHOICE_CURSOR_OTHER);
    uint32_t x=((uint32_t)hn_read8(m,descriptor+20)<<16)+(column*(columns==2?11u:9u)<<19);
    uint32_t y=((uint32_t)hn_read8(m,descriptor+21)<<16)+(row<<20);
    for(unsigned index=0;index<2;index++){
        GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x18,x);hn_write32(m,slot+0x1C,y);hn_write32(m,slot+0x20,0);
    }
    for(unsigned index=0;index<256;index++){
        GameAddr tile=HN_CHOICE_TILES+index*2;unsigned value=hn_read16(m,tile)&0xFFF;
        if(value&&value<(selected+1)*2*width+1&&value>=selected*2*width+1)value|=0x1000;
        hn_write16(m,tile,(uint16_t)value);
    }
    return hn_background_configure(m,0,0x88,hn_read32(m,descriptor+12))&&!m->fault;
}
bool hn_choice_select_identifier(HNMemory *m,GameAddr descriptor,uint32_t identifier)
{
    unsigned count=hn_read8(m,HN_CHOICE_COUNT),index=0;if(count>9)return false;
    while(index<count&&hn_read32(m,record(index))!=identifier)index++;
    if(index>=9)return false;
    return position(m,descriptor,hn_read32(m,record(index)+4)?0:index)&&hn_choice_display(m,descriptor);
}
static bool close_dialog(HNMemory *m,GameAddr background)
{
    if(!graphics(m,0,0)||hn_read8(m,HN_RENDER_INFO+16)<2||!hn_read32(m,HN_RENDER_SLOTS))return false;
    hn_write8(m,hn_render_slot(m,0)+0x60,0);hn_write8(m,hn_render_slot(m,1)+0x60,0);
    if(!hn_background_configure(m,0,0x88,background))return false;
    for(int index=8;index>=0;index--){
        hn_write32(m,record((unsigned)index),0);if(!hn_task_remove(m,hn_read32(m,record((unsigned)index)+8)))return false;
    }
    return !m->fault;
}
static bool sound(HNMemory *m,unsigned song){return hn_audio_ready(m)&&hn_audio_song(m,(uint16_t)song,HN_SONG_START);}
static GameAddr descriptor_for(HNMemory *m,unsigned count)
{
    if(!count||count>9)return 0;
    GameAddr family=hn_read32(m,HN_CHOICE_FAMILIES+hn_read8(m,HN_CHOICE_FAMILY)*4);
    return hn_read32(m,family+(count-1)*4);
}
static bool create_tasks(HNMemory *m,GameAddr descriptor,GameAddr labels)
{
    unsigned width=hn_read8(m,descriptor+18),offset=hn_read8(m,descriptor+19),language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);
    if(!width||language>=5)return false;
    for(unsigned index=0;index<9;index++){
        GameAddr entry=record(index),task=hn_task_allocate(m);if(!task)return false;hn_write32(m,entry+8,task);
        hn_write32(m,task+0x18,0x02004740+width*index*64);
        unsigned identifier=hn_read32(m,entry),fill=identifier&&hn_read32(m,entry+4)?2:3;
        if(!hn_task_graphics(m,task,width,2,offset,1,0,fill,(hn_read8(m,HN_CHOICE_FLAGS)&32)?8:0,
                            0x06000020+width*index*64,0x0600B520))return false;
        GameAddr text=HN_TASK_DEFAULT_SCRIPT;
        if(identifier){
            unsigned code=hn_read8(m,labels+identifier-1);
            text=hn_read32(m,(code<=86?HN_CHOICE_TEXT_PAIRS:HN_TASK_TEXT_BUNDLES)+code*4);
            if(text!=HN_TASK_DEFAULT_SCRIPT)text=hn_read32(m,text+language*4);
            if(code>86){text=hn_task_text_adjust(m,text,index);if(!text)return false;}
        }
        if(!hn_task_set_script(m,task,text))return false;
    }
    return !m->fault;
}
bool hn_choice_update(HNMemory *m,uint32_t count_word,GameAddr labels,GameAddr conditions,uint32_t *selected)
{
    *selected=0;if(m->fault)return false;
    unsigned state=hn_read16(m,HN_CHOICE_STATE);GameAddr background=hn_read32(m,HN_RENDER_STATES);
    if(!background||!hn_read8(m,HN_BACKGROUND_COUNT))return false;
    if(state==0){
        if(hn_read8(m,background+0x38))return !m->fault;
        unsigned count=count_word&255;
        hn_write8(m,HN_CHOICE_COUNT,0);hn_write32(m,0x03000798,conditions);
        for(unsigned index=0;index<count;index++){
            uint32_t allowed;if(!hn_expression_evaluate(m,hn_read32(m,0x03000798),false,&allowed))return false;
            if(allowed){unsigned current=hn_read8(m,HN_CHOICE_COUNT);if(current>=9)return false;hn_write32(m,record(current),index+1);
                hn_write32(m,record(current)+4,index<16?(hn_read16(m,HN_CHOICE_OPTIONS)>>index)&1:0);hn_write8(m,HN_CHOICE_COUNT,(uint8_t)(current+1));}
        }
        unsigned enabled=hn_read8(m,HN_CHOICE_COUNT);for(unsigned index=enabled;index<9;index++)hn_write32(m,record(index),0);
        GameAddr descriptor=descriptor_for(m,enabled);unsigned columns,rows;if(!descriptor||!geometry(m,descriptor,&columns,&rows))return false;
        if(!(hn_read8(m,HN_CHOICE_FLAGS)&8)){
            if(!hn_motion_idle_render(m,false))return false;
            if(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)&&!hn_motion_idle_render(m,true))return false;
        }
        if(!hn_background_configure(m,0,0x88,hn_read32(m,descriptor)))return false;
        GameAddr script=hn_read32(m,descriptor+8);if(!graphics(m,1,script?script:HN_CHOICE_GRAPHICS)||!create_tasks(m,descriptor,labels))return false;
        hn_input_repeat(m,30,8);if(!sound(m,306))return false;hn_write16(m,HN_CHOICE_STATE,1);return !m->fault;
    }
    if(state==1){
        if(hn_read8(m,background+0x38))return !m->fault;
        GameAddr descriptor=descriptor_for(m,hn_read8(m,HN_CHOICE_COUNT));if(!descriptor||!hn_choice_initial_selection(m,descriptor))return false;
        if(hn_read8(m,HN_RENDER_INFO+16)<2||!hn_read32(m,HN_RENDER_SLOTS))return false;
        for(unsigned index=0;index<2;index++){hn_render_slot_reset(m,index);hn_write8(m,hn_render_slot(m,index)+0x60,1);}
        if(!hn_choice_display(m,descriptor))return false;hn_write16(m,HN_CHOICE_STATE,2);return !m->fault;
    }
    if(state==2){
        unsigned keys=hn_input_query(m,3,1)|hn_input_query(m,240,2),columns,rows;
        GameAddr descriptor=descriptor_for(m,hn_read8(m,HN_CHOICE_COUNT));if(!descriptor||!geometry(m,descriptor,&columns,&rows))return false;
        unsigned row=hn_read8(m,HN_CHOICE_ROW),column=hn_read8(m,HN_CHOICE_COLUMN);if(row>=rows||column>=columns||row*columns+column>=9)return false;
        if(keys&1){
            *selected=hn_read32(m,record(row*columns+column));if(!close_dialog(m,hn_read32(m,descriptor+4)))return false;
            hn_input_initialize(m);if(!sound(m,304))return false;hn_write16(m,HN_CHOICE_STATE,3);return !m->fault;
        }
        unsigned flags=hn_read8(m,HN_CHOICE_FLAGS);
        if((flags&16)||(!(flags&1)&&(keys&2))){
            *selected=hn_read32(m,HN_CHOICE_PENDING);
            if(*selected){if(!hn_choice_select_identifier(m,descriptor,*selected))return false;hn_write32(m,HN_CHOICE_DELAY,8);hn_write16(m,HN_CHOICE_STATE,5);}
            else hn_write16(m,HN_CHOICE_STATE,6);
            return sound(m,307)&&!m->fault;
        }
        if((flags&1)&&(keys&2)&&!sound(m,301))return false;
        bool changed=false;
        if(keys&16){if(column<columns-1){column++;changed=true;}}
        else if(keys&32){if(column){column--;changed=true;}}
        else if(keys&128){for(unsigned next=row+1;next<rows;next++)if(!hn_read32(m,record(next*columns+column)+4)){row=next;changed=true;break;}}
        else if(keys&64){for(unsigned distance=1;distance<=row;distance++)if(!hn_read32(m,record((row-distance)*columns+column)+4)){row-=distance;changed=true;break;}}
        hn_write8(m,HN_CHOICE_ROW,(uint8_t)row);hn_write8(m,HN_CHOICE_COLUMN,(uint8_t)column);
        if(changed&&!sound(m,302))return false;
        if(!hn_read32(m,record(row*columns+column))){
            changed=true;do{if(!column)return false;column--;}while(!hn_read32(m,record(row*columns+column)));
            hn_write8(m,HN_CHOICE_COLUMN,(uint8_t)column);
        }
        return (!changed||hn_choice_display(m,descriptor))&&!m->fault;
    }
    if(state==3){if(!hn_read8(m,background+0x38)){hn_write8(m,HN_CHOICE_FLAGS,hn_read8(m,HN_CHOICE_FLAGS)&239);hn_write16(m,HN_CHOICE_STATE,4);}return !m->fault;}
    if(state==5){uint32_t delay=hn_read32(m,HN_CHOICE_DELAY)-1;hn_write32(m,HN_CHOICE_DELAY,delay);if(delay)return !m->fault;}
    if(state==5||state==6){
        GameAddr descriptor=descriptor_for(m,hn_read8(m,HN_CHOICE_COUNT));if(!descriptor||!close_dialog(m,hn_read32(m,descriptor+4)))return false;
        hn_input_initialize(m);hn_write16(m,HN_CHOICE_STATE,3);
    }
    return !m->fault;
}
void hn_choice_request(HNMemory *m,uint32_t identifier)
{hn_write8(m,HN_CHOICE_FLAGS,hn_read8(m,HN_CHOICE_FLAGS)|16);hn_write32(m,HN_CHOICE_PENDING,identifier);}
bool hn_choice_reset(HNMemory *m)
{
    unsigned state=hn_read16(m,HN_CHOICE_STATE);
    if(state==2||state==6){if(!close_dialog(m,HN_CHOICE_RESET_BACKGROUND))return false;hn_write16(m,HN_CHOICE_STATE,0);}
    else if(state==1){hn_write8(m,HN_SCENE_CONTROLS+0x69,0);return !m->fault;}
    hn_write8(m,HN_SCENE_CONTROLS+0x69,1);return !m->fault;
}
bool hn_choice_main_command(HNMemory *m,unsigned opcode,GameAddr arguments)
{
    const GameAddr scratch=0x030005E8;
    if(opcode==0x45){
        hn_write8(m,HN_CHOICE_FAMILY,hn_read8(m,arguments));hn_write8(m,HN_CHOICE_FLAGS,hn_read8(m,arguments+1));hn_write16(m,HN_CHOICE_OPTIONS,0);
        hn_write8(m,scratch+2,hn_read8(m,arguments+2));hn_write8(m,scratch,hn_read8(m,arguments+3));hn_write8(m,scratch+1,hn_read8(m,arguments+4));
        hn_write32(m,HN_CHOICE_PENDING,hn_read8(m,scratch+2));unsigned value=hn_read16(m,scratch);
        if(value>15){if(value>511){value-=512;if(value>15)value=UINT32_MAX;}value=value==UINT32_MAX?0:hn_read8(m,HN_SCENE_CONTROLS+value);}
        hn_write32(m,HN_CHOICE_INITIAL,value+1);hn_write32(m,0x03000014,5);hn_write8(m,0x03000010,0);return !m->fault;
    }
    if(opcode!=0x24)return false;
    unsigned count=hn_read8(m,arguments);for(unsigned index=0;index<8;index++)hn_write8(m,scratch+index,hn_read8(m,arguments+1+index));
    for(unsigned index=0;index<2;index++){unsigned value=hn_read32(m,scratch+index*4);if(value<=15)hn_write32(m,scratch+index*4,hn_read32(m,0x03002BB0+value*4));}
    unsigned state=hn_read16(m,HN_CHOICE_STATE);uint32_t selected;
    if(!hn_choice_update(m,count,hn_read32(m,scratch),hn_read32(m,scratch+4),&selected))return false;
    if(state<=2)hn_write16(m,0x03001FBC,(uint16_t)selected);
    if(hn_read16(m,HN_CHOICE_STATE)==4){hn_write16(m,HN_CHOICE_STATE,0);hn_write32(m,0x03000014,9);hn_write8(m,0x03000010,0);}
    else hn_write8(m,0x0300001D,1);
    return !m->fault;
}
