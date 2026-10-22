#include "game/bank3.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/background.h"
#include "game/random.h"
#include "game/input.h"
#include "game/audio.h"

enum { ENTRIES=0x030035D0,FLAGS=0x03003570,AVAILABLE=0x03003B40 };
static uint8_t control(HNMemory *m,unsigned offset)
{return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value)
{hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static bool flag(HNMemory *m,unsigned id)
{return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void set_flag(HNMemory *m,unsigned id,bool value)
{
    GameAddr address=0x03002180+(id>>3);uint8_t bit=(uint8_t)(1u<<(id&7));
    hn_write8(m,address,(uint8_t)((hn_read8(m,address)&~bit)|(value?bit:0)));
}
static GameAddr entry_at(uint8_t index){return ENTRIES+index*8;}
static bool animation(HNMemory *m,uint8_t index,GameAddr resource)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,resource);return !m->fault;
}
static void hide(HNMemory *m,uint8_t index)
{hn_write8(m,hn_render_slot(m,index)+0x60,0);}
static void priority(HNMemory *m,uint8_t index,uint8_t value)
{hn_render_flags(m,index,12,(uint8_t)(value<<2));}
static GameAddr resource(HNMemory *m,GameAddr table,uint32_t index)
{return hn_read32(m,table+index*4);}
static bool find_index(HNMemory *m,uint32_t kind_word,uint8_t *index)
{
    uint8_t kind=(uint8_t)(kind_word-1);
    for(unsigned i=0;i<=8;i++)if(hn_read8(m,AVAILABLE+i)==kind){*index=(uint8_t)i;return true;}
    return false;
}
uint32_t hn_bank3_chain_find(HNMemory *m,uint32_t kind_word,uint32_t retained_r4)
{uint8_t index;return find_index(m,kind_word,&index)?index:retained_r4;}
void hn_bank3_chain_set_flag(HNMemory *m,uint32_t kind_word,uint32_t value_word)
{
    uint8_t kind=(uint8_t)kind_word,value=(uint8_t)value_word;
    /* The source wrapper retains its captured byte value in R4 across Find. */
    uint8_t index=(uint8_t)hn_bank3_chain_find(m,kind,value);hn_write8(m,FLAGS+index,value);
}
void hn_bank3_chain_clear(HNMemory *m,uint32_t side_word,uint32_t index_word)
{
    uint8_t side=(uint8_t)side_word,index=(uint8_t)index_word;
    if(index!=255)hn_write8(m,entry_at(index)+(side?3:4),255);
}
void hn_bank3_chain_prepare(HNMemory *m,uint32_t index_word)
{
    uint8_t index=(uint8_t)index_word;GameAddr entry=entry_at(index);
    hn_write8(m,entry,255);hn_write8(m,entry+1,0);hn_write8(m,entry+2,0);hn_write8(m,entry+3,255);hn_write8(m,entry+4,255);
    if(control(m,0x1C1)==index)store(m,0x1C1,255);
}
void hn_bank3_chain_consume(HNMemory *m,uint32_t index_word)
{
    GameAddr entry=entry_at((uint8_t)index_word);hn_write8(m,entry,0);hn_write8(m,entry+1,0);hn_write8(m,entry+2,0);
}
bool hn_bank3_chain_variant(HNMemory *m,uint32_t index_word,uint32_t *result)
{
    uint8_t index=(uint8_t)index_word,first,second;unsigned attempts=0;
    do {if(++attempts>4096)return false;first=(uint8_t)hn_random_range(m,1,10);second=(uint8_t)hn_random_range(m,1,10);}while(first==second);
    GameAddr entry=entry_at(index);hn_write8(m,entry+1,first<second?first:second);hn_write8(m,entry+2,first<second?second:first);
    uint8_t sum=0,step=0;
    while(step<hn_read8(m,entry+1)-1){sum=(uint8_t)(sum+8-step);step++;}
    uint32_t difference=~(uint32_t)hn_read8(m,entry+1)+hn_read8(m,entry+2);
    *result=(uint8_t)((int8_t)sum+difference);return !m->fault;
}
static bool pick_kind(HNMemory *m,uint8_t *result)
{
    uint8_t kind;unsigned attempts=0;
    for(;;){
        if(++attempts>4096)return false;kind=(uint8_t)hn_random_range(m,0,16);
        uint32_t row=control(m,0x1BD)*7;
        if(kind<hn_read8(m,0x08630E5D+row)){
            kind=(uint8_t)hn_random_range(m,1,10);
            if(control(m,0x1BE)==kind){kind++;if(kind==10)kind=1;}
            break;
        }
        if(kind<hn_read8(m,0x08630E5E+row)){kind=10;break;}
        if(kind<hn_read8(m,0x08630E5F+row)){kind=11;break;}
        if(kind<hn_read8(m,0x08630E60+row)){kind=control(m,0x1BE)==12?10:12;break;}
        if(kind<hn_read8(m,0x08630E61+row)){kind=13;break;}
        if(kind<hn_read8(m,0x08630E62+row)){
            if(control(m,0x1BF))continue;kind=14;store(m,0x1BF,12);break;
        }
        if(kind<hn_read8(m,0x08630E63+row)){
            uint32_t speed=control(m,0x1B5);kind=speed==0?15:speed==2?16:(uint8_t)(hn_random_range(m,0,2)+15);break;
        }
        kind=17;break;
    }
    store(m,0x1BE,kind);uint32_t cooldown=control(m,0x1BF);
    if(cooldown&&cooldown!=255)store(m,0x1BF,cooldown-1);
    *result=kind;return !m->fault;
}
static bool count_display(HNMemory *m)
{
    uint8_t count=control(m,0x1B6);GameAddr data=count<=9?resource(m,0x08630FF0,count):0x088E8E20;
    if(!hn_render_configure_raw(m,39,1,114,140,0,106,data))return false;priority(m,39,0);return !m->fault;
}
static bool lines_display(HNMemory *m)
{
    if(!hn_render_configure_raw(m,14,1,116,8,0,128,resource(m,0x08631018,control(m,0x1C2))))return false;
    priority(m,14,0);return !m->fault;
}
bool hn_bank3_chain_mark(HNMemory *m,uint32_t index_word)
{
    uint8_t index=(uint8_t)index_word;
    return hn_graphics_initialize_raw(m,(uint8_t)(index+22),1,resource(m,0x086310A8,index));
}
bool hn_bank3_chain_flags(HNMemory *m,GameAddr legacy_old_flags)
{
    uint8_t old[9],order[9];
    for(unsigned i=0;i<=8;i++)order[i]=hn_read8(m,0x086310C8+i);
    for(unsigned i=0;i<=8;i++)old[i]=hn_read8(m,FLAGS+i);
    for(unsigned i=0;i<=8;i++){hn_write8(m,AVAILABLE+i,0);hn_write8(m,FLAGS+i,0);}
    for(unsigned i=0;i<=8;i++){
        uint8_t selected=(uint8_t)hn_random_range(m,0,(uint16_t)(9-i));hn_write8(m,AVAILABLE+i,order[selected]);
        for(unsigned item=selected;item<=7;item++)order[item]=order[item+1];
    }
    for(unsigned i=0;i<=8;i++){
        if(!hn_render_configure_raw(m,(uint8_t)(i+1),1,116,16+(int32_t)(i%3)*16,0,88+(int32_t)(i/3)*16,
            resource(m,0x0863103C,hn_read8(m,AVAILABLE+i))))return false;
        priority(m,(uint8_t)(i+1),0);
    }
    for(unsigned i=0;i<=8;i++)if(old[i]){
        uint8_t index;
        if(!find_index(m,old[i],&index)){
            /* The original uses the oldFlags stack-byte address as incoming
               R4 on this invalid-kind path. Explicit probes supply that guest
               frame address; normal play constructs only flag values 1..9. */
            if(!legacy_old_flags)return false;index=(uint8_t)(legacy_old_flags+i);
        }
        hn_write8(m,FLAGS+index,old[i]);
    }
    return !m->fault;
}
static bool flag_graphics(HNMemory *m)
{
    if(!hn_graphics_initialize_raw(m,1,1,0x088E902A)||!hn_graphics_initialize_raw(m,2,1,0x088E9058))return false;
    for(unsigned i=0;i<=8;i++){
        /* This caller retains the loop index in R4 when Find has no match. */
        uint8_t target=(uint8_t)hn_bank3_chain_find(m,i+1,i);
        if(hn_read8(m,FLAGS+target))
            if(!hn_graphics_initialize_raw(m,(uint8_t)(i+3),1,resource(m,0x08631060,target))||
               !hn_graphics_initialize_raw(m,(uint8_t)(i+12),1,resource(m,0x08631084,i)))return false;
    }
    return !m->fault;
}
static bool refresh_lines(HNMemory *m)
{
    uint8_t count=0;if(!flag_graphics(m))return false;
    for(unsigned i=0;i<=2;i++)if(hn_read8(m,FLAGS+i*3)&&hn_read8(m,FLAGS+i*3+1)&&hn_read8(m,FLAGS+i*3+2)){
        count++;if(!hn_bank3_chain_mark(m,i))return false;
    }
    for(unsigned i=0;i<=2;i++)if(hn_read8(m,FLAGS+i)&&hn_read8(m,FLAGS+i+3)&&hn_read8(m,FLAGS+i+6)){
        count++;if(!hn_bank3_chain_mark(m,i+3))return false;
    }
    if(hn_read8(m,FLAGS)&&hn_read8(m,FLAGS+4)&&hn_read8(m,FLAGS+8)){count++;if(!hn_bank3_chain_mark(m,6))return false;}
    if(hn_read8(m,FLAGS+2)&&hn_read8(m,FLAGS+4)&&hn_read8(m,FLAGS+6)){count++;if(!hn_bank3_chain_mark(m,7))return false;}
    if(control(m,0x1C2)!=count)store(m,0x1E7,count==1?1:2);
    store(m,0x1C2,count);return lines_display(m);
}
static bool configure_grid(HNMemory *m)
{
    if(!animation(m,16,0x088D134F)||!hn_background_configure_raw(m,6,132,0x088E8F23)||
       !hn_graphics_initialize_raw(m,1,1,0x088E902A)||!hn_graphics_initialize_raw(m,2,1,0x088E9058))return false;
    for(unsigned i=0;i<=8;i++){
        if(!hn_render_configure_raw(m,(uint8_t)(i+22),1,116,16+(int32_t)(i%3)*16,0,88+(int32_t)(i/3)*16,resource(m,0x0863103C,i)))return false;
        priority(m,(uint8_t)(i+22),0);
    }
    return !m->fault;
}
static void velocities(HNMemory *m,bool exclude_zero)
{
    for(unsigned i=0;i<=15;i++){
        uint8_t kind=hn_read8(m,ENTRIES+i*8);
        if(exclude_zero?kind==0:kind==255)continue;
        bool negative=control(m,0x1B3)==0;GameAddr slot=hn_render_slot(m,i+22);
        uint32_t value=(uint32_t)hn_read16(m,0x08630E54+control(m,0x1B5)*2)<<8;
        hn_write32(m,slot+0x24,negative?0u-value:value);
    }
}
static bool spawn_one(HNMemory *m)
{
    for(unsigned i=0;i<=15;i++){
        GameAddr entry=ENTRIES+i*8;if(hn_read8(m,entry)!=255||hn_read8(m,hn_render_slot(m,i+22)+0x60))continue;
        uint8_t kind;if(!pick_kind(m,&kind))return false;
        hn_write8(m,entry,kind);hn_write8(m,entry+1,0);hn_write8(m,entry+2,0);
        bool negative=control(m,0x1B3)==0;uint8_t tail=control(m,0x1C0);
        if(tail!=255)hn_write8(m,entry_at(tail)+(negative?4:3),(uint8_t)i);
        if(negative){hn_write8(m,entry+3,control(m,0x1C0));hn_write8(m,entry+4,255);}
        else {hn_write8(m,entry+4,control(m,0x1C0));hn_write8(m,entry+3,255);}
        if(!hn_render_configure_raw(m,(uint8_t)(i+22),1,114,negative?240:-24,0,6,resource(m,0x08630E8C,hn_read8(m,entry))))return false;
        GameAddr slot=hn_render_slot(m,i+22);uint32_t speed=(uint32_t)hn_read16(m,0x08630E54+control(m,0x1B5)*2)<<8;
        hn_write32(m,slot+0x24,negative?0u-speed:speed);store(m,0x1C0,i);
        if(kind==10){uint32_t variant;if(!hn_bank3_chain_variant(m,i,&variant)||!animation(m,(uint8_t)(i+22),resource(m,0x08630F40,(uint8_t)variant)))return false;}
        if(kind==0&&!animation(m,(uint8_t)(i+22),0x0889F620))return false;
        return !m->fault;
    }
    return !m->fault;
}
static bool remove_offscreen(HNMemory *m)
{
    for(unsigned i=0;i<=15;i++){
        GameAddr entry=ENTRIES+i*8;if(hn_read8(m,entry)==255)continue;
        int32_t x=(int32_t)hn_read32(m,hn_render_slot(m,i+22)+0x18);
        if((x>0xE87FFF&&control(m,0x1B3)==1)||(x<=-0x100000&&control(m,0x1B3)==0)){
            hide(m,(uint8_t)(i+22));store(m,0x1C1,hn_read8(m,entry+(control(m,0x1B3)==0?4:3)));
            hn_bank3_chain_clear(m,0,hn_read8(m,entry+3));hn_bank3_chain_clear(m,1,hn_read8(m,entry+4));
            hn_bank3_chain_prepare(m,i);
        }
    }
    return !m->fault;
}
static bool initialize_entries(HNMemory *m)
{
    store(m,0x1BD,2);store(m,0x1B4,0);store(m,0x1C2,0);store(m,0x1B3,0);store(m,0x1B5,1);store(m,0x1B6,4);
    store(m,0x1B7,0);store(m,0x1B8,0);store(m,0x1B9,0);store(m,0x1C0,255);store(m,0x1C1,255);store(m,0x1BF,0);
    set_flag(m,0xFD,false);
    if(!hn_bank3_chain_flags(m,0)||!hn_graphics_initialize_raw(m,1,1,0x088E902A)||!hn_graphics_initialize_raw(m,2,1,0x088E9058))return false;
    for(unsigned i=0;i<=8;i++)hn_write8(m,FLAGS+i,0);
    if(!hn_background_configure_raw(m,6,132,0x088E8F23))return false;
    for(unsigned i=0;i<=10;i++){
        uint8_t kind;if(!pick_kind(m,&kind))return false;GameAddr entry=ENTRIES+i*8;
        hn_write8(m,entry,kind);hn_write8(m,entry+1,0);hn_write8(m,entry+2,0);
        uint8_t tail=control(m,0x1C0);if(tail!=255)hn_write8(m,entry_at(tail)+4,(uint8_t)i);
        hn_write8(m,entry+3,control(m,0x1C0));hn_write8(m,entry+4,255);store(m,0x1C0,i);
        GameAddr data;
        if(kind==0)data=0x0889F620;
        else if(kind!=10)data=resource(m,0x08630E8C,hn_read8(m,entry));
        else {uint32_t variant;if(!hn_bank3_chain_variant(m,i,&variant))return false;data=resource(m,0x08630F40,(uint8_t)variant);}
        if(!hn_render_configure_raw(m,(uint8_t)(i+22),1,114,(int16_t)(i*24),0,6,data))return false;
        GameAddr slot=hn_render_slot(m,i+22);hn_write32(m,slot+0x24,0u-((uint32_t)hn_read16(m,0x08630E54+control(m,0x1B5)*2)<<8));
    }
    for(unsigned i=11;i<=15;i++)hn_bank3_chain_prepare(m,i);
    if(!count_display(m)||!lines_display(m))return false;
    if(!flag(m,0x2A5))store(m,0x1C3,hn_random_range(m,60,180));
    return !m->fault;
}
static bool chain_keys(HNMemory *m)
{
    hn_input_script_mask(m,1);
    if(hn_read16(m,0x0201DC40)&2){
        if(!hn_graphics_initialize_raw(m,1,1,0x088E902A)||!hn_graphics_initialize_raw(m,2,1,0x088E9058)||!hn_bank3_chain_flags(m,0))return false;
        for(unsigned i=0;i<=8;i++){uint8_t value=hn_read8(m,FLAGS+i);if(value)hn_bank3_chain_set_flag(m,value,value);}
        if(!refresh_lines(m))return false;
    }
    if(hn_read16(m,0x0201DC40)&0x200){if(control(m,0x1B5)<=1)store(m,0x1B5,control(m,0x1B5)+1);velocities(m,true);}
    if(hn_read16(m,0x0201DC40)&0x100){if(control(m,0x1B5))store(m,0x1B5,control(m,0x1B5)-1);velocities(m,true);}
    return !m->fault;
}
static bool trigger_action(HNMemory *m)
{
    hn_input_script_mask(m,1);
    if(!flag(m,0x2A5)){
        if(!(hn_read16(m,0x0201DC40)&1))return !m->fault;
        if(hn_read32(m,hn_render_slot(m,10)+0x24))return !m->fault;
        if(!animation(m,10,0x088B139E)||!hn_graphics_initialize_raw(m,21,1,0x088E13AB))return false;
        if((int32_t)hn_read32(m,hn_render_slot(m,10)+0x18)<=0x688000){
            if(control(m,0x1B7)!=2)return !m->fault;
            if(!hn_render_configure_raw(m,11,1,176,0,0,0,0x088E16C5)||!hn_render_modes_raw(m,11,2,3,10))return false;
            priority(m,11,0);if(!animation(m,16,0x088E8DE4))return false;store(m,0x1B7,3);
        }else{
            if(!hn_render_configure_raw(m,11,1,176,0,0,0,0x088E16C5)||!hn_render_modes_raw(m,11,2,3,10))return false;priority(m,11,0);
        }
    }else if(control(m,0x1B7)==2){
        store(m,0x1C3,(uint32_t)control(m,0x1C3)-1);
        if(!control(m,0x1C3)){
            if(!animation(m,10,0x088B8503)||!hn_graphics_initialize_raw(m,21,1,0x088E13AB)||
               !hn_render_configure_raw(m,11,1,176,0,0,0,0x088E16C5)||!hn_render_modes_raw(m,11,2,3,10))return false;
            priority(m,11,0);if(!animation(m,16,0x088E8DE4))return false;
            store(m,0x1B7,3);store(m,0x1C3,hn_random_range(m,60,180));
        }
    }
    return !m->fault;
}
static bool update_primary(HNMemory *m)
{
    switch(control(m,0x1B7)){
    case 0:
        if(!hn_render_configure_raw(m,16,1,114,120,0,109,0x088E8DBE))return false;
        store(m,0x1B6,(uint32_t)control(m,0x1B6)-1);if(!count_display(m))return false;store(m,0x1B7,control(m,0x1B7)+1);break;
    case 1:if(hn_read32(m,hn_render_slot(m,16)+4)!=0x088E8DBE)store(m,0x1B7,2);break;
    case 3:
        if((int32_t)hn_read32(m,hn_render_slot(m,16)+0x20)<-0x180000){
            hide(m,16);if(!control(m,0x1B6))set_flag(m,0xFD,true);else store(m,0x1B7,0);
        }
        break;
    }
    return !m->fault;
}
static bool update_side(HNMemory *m,bool right)
{
    unsigned state=right?0x1B9:0x1B8,timer=right?0x1C5:0x1C4;
    uint8_t paddle=right?17:15,actor=right?19:18,overlay=right?21:20;
    switch(control(m,state)){
    case 0:
        if(!hn_render_configure_raw(m,paddle,1,114,right?200:40,0,109,0x088E8DBE)||
           !hn_render_configure_raw(m,actor,1,114,right?168:8,0,120,0x088E8EC1))return false;
        store(m,timer,hn_random_range(m,180,250));store(m,state,control(m,state)+1);break;
    case 1:if(hn_read32(m,hn_render_slot(m,paddle)+4)!=0x088E8DBE)store(m,state,2);break;
    case 2:
        if(!flag(m,0x2A6)){
            store(m,timer,(uint32_t)control(m,timer)-1);
            if(!control(m,timer)){
                if(!animation(m,actor,0x088E8EC9)||!hn_graphics_initialize_raw(m,21,1,0x088E13AB)||
                   !hn_render_configure_raw(m,overlay,1,176,0,0,0,0x088E16C5)||!hn_render_modes_raw(m,overlay,2,3,actor)||
                   !animation(m,paddle,0x088E8DE4))return false;
                store(m,state,3);
            }
        }
        break;
    case 3:
        if((int32_t)hn_read32(m,hn_render_slot(m,paddle)+0x20)<-0x180000){hide(m,paddle);store(m,state,0);}
        break;
    }
    return !m->fault;
}
static bool effect(HNMemory *m,uint8_t index,uint8_t capture,uint8_t kind,unsigned depth)
{
    if(depth>=64)return false;
    switch(kind){
    case 10:{
        if(capture!=1)return !m->fault;GameAddr entry=entry_at(index);
        hn_bank3_chain_set_flag(m,hn_read8(m,entry+1),hn_read8(m,entry+1));
        hn_bank3_chain_set_flag(m,hn_read8(m,entry+2),hn_read8(m,entry+2));break;
    }
    case 11:{
        uint32_t value=(uint8_t)hn_random_range(m,1,10);
        if(capture==1){
            uint8_t available[9],count=0;
            for(unsigned i=0;i<=8;i++)if(!hn_read8(m,FLAGS+i))available[count++]=(uint8_t)(hn_read8(m,AVAILABLE+i)+1);
            if(count)value=available[(uint8_t)hn_random_range(m,0,count)];
            hn_bank3_chain_set_flag(m,value,value);
        }
        if(!animation(m,(uint8_t)(index+22),resource(m,0x08630F1C,value-1)))return false;break;
    }
    case 12:{
        GameAddr entry=entry_at(index);uint8_t neighbor=hn_read8(m,entry+3);
        if(neighbor!=255&&neighbor!=index&&neighbor<=15){
            if(!animation(m,(uint8_t)(neighbor+22),resource(m,0x08630ED4,hn_read8(m,entry_at(hn_read8(m,entry+3))))))return false;
            hn_bank3_chain_clear(m,0,hn_read8(m,entry_at(hn_read8(m,entry+3))+3));
            hn_bank3_chain_clear(m,1,hn_read8(m,entry_at(hn_read8(m,entry+3))+4));
            /* Each call can rewrite this link. Reload before the recursive
               effect and again before consuming, including mapped index 255. */
            if(!effect(m,hn_read8(m,entry+3),capture,hn_read8(m,entry_at(hn_read8(m,entry+3))),depth+1))return false;
            hn_bank3_chain_consume(m,hn_read8(m,entry+3));
        }
        entry=entry_at(index);neighbor=hn_read8(m,entry+4);
        if(neighbor!=255&&neighbor!=index&&neighbor<=15){
            if(!animation(m,(uint8_t)(neighbor+22),resource(m,0x08630ED4,hn_read8(m,entry_at(hn_read8(m,entry+4))))))return false;
            hn_bank3_chain_clear(m,0,hn_read8(m,entry_at(hn_read8(m,entry+4))+3));
            hn_bank3_chain_clear(m,1,hn_read8(m,entry_at(hn_read8(m,entry+4))+4));
            if(!effect(m,hn_read8(m,entry+4),capture,hn_read8(m,entry_at(hn_read8(m,entry+4))),depth+1))return false;
            hn_bank3_chain_consume(m,hn_read8(m,entry+4));
        }
        break;
    }
    case 13:{
        if(capture!=1)return !m->fault;bool found=false;
        for(unsigned i=0;i<=8;i++)if(hn_read8(m,FLAGS+i)){found=true;break;}
        if(hn_random_range(m,0,2)==0&&found){
            uint8_t selected,value;unsigned attempts=0;
            do {if(++attempts>4096)return false;selected=(uint8_t)hn_random_range(m,0,9);value=hn_read8(m,FLAGS+selected);}while(!value);
            hn_bank3_chain_set_flag(m,value,0);
        }else if(!hn_bank3_chain_flags(m,0))return false;
        break;
    }
    case 14:
        if(capture!=1)return !m->fault;store(m,0x1B6,control(m,0x1B6)+1);
        if(!count_display(m))return false;store(m,0x1BF,255);break;
    case 15:if(control(m,0x1B5)<=1){store(m,0x1B5,control(m,0x1B5)+1);velocities(m,false);}break;
    case 16:if(control(m,0x1B5)){store(m,0x1B5,(uint32_t)control(m,0x1B5)-1);velocities(m,false);}break;
    case 17:{
        store(m,0x1B3,control(m,0x1B3)^1);store(m,0x1B4,96u-control(m,0x1B4));
        uint32_t last=control(m,0x1C1);store(m,0x1C1,control(m,0x1C0));store(m,0x1C0,last);
        for(unsigned i=0;i<=15;i++)if(hn_read8(m,ENTRIES+i*8)!=255){
            GameAddr slot=hn_render_slot(m,i+22);hn_write32(m,slot+0x24,0u-hn_read32(m,slot+0x24));
        }
        break;
    }
    default:
        if(capture!=1)return !m->fault;
        if((uint8_t)(kind-1)<=8)hn_bank3_chain_set_flag(m,kind,kind);
        break;
    }
    if(capture==1){
        if(kind==12){if(control(m,0x1D6)<=2)store(m,0x1D6,2);}
        else if(kind==13||kind>14)store(m,0x1D6,3);
        else if(control(m,0x1D6)<=1)store(m,0x1D6,capture);
    }
    return !m->fault;
}
bool hn_bank3_chain_effect(HNMemory *m,uint32_t index,uint32_t capture,uint32_t kind)
{return effect(m,(uint8_t)index,(uint8_t)capture,(uint8_t)kind,0);}
static bool capture_entries(HNMemory *m)
{
    bool any_first=false;store(m,0x1D6,0);uint32_t old_count=control(m,0x1C2);
    for(unsigned i=0;i<=15;i++){
        GameAddr entry=ENTRIES+i*8;uint8_t kind=hn_read8(m,entry);
        if(kind==255||!kind)continue;
        uint8_t capture=0;GameAddr base=hn_read32(m,HN_RENDER_SLOTS),slot=base+(i+22)*104;
        uint8_t x=hn_read8(m,slot+0x1A),z=hn_read8(m,slot+0x22);
        if(control(m,0x1B7)==3){
            GameAddr actor=base+16*104;uint8_t actor_x=hn_read8(m,actor+0x1A),actor_z=hn_read8(m,actor+0x22);
            if(hn_bank3_boxes_overlap(actor_x,(uint8_t)(actor_z+8),(uint8_t)(actor_x+24),(uint8_t)(actor_z+16),
                x,(uint8_t)(z+8),(uint8_t)(x+24),(uint8_t)(z+24))){capture=1;any_first=true;}
        }
        if(control(m,0x1B8)==3){
            GameAddr actor=hn_render_slot(m,15);uint8_t actor_x=hn_read8(m,actor+0x1A),actor_z=hn_read8(m,actor+0x22);
            if(x<=63&&hn_bank3_boxes_overlap(actor_x,(uint8_t)(actor_z+8),(uint8_t)(actor_x+24),(uint8_t)(actor_z+16),
                x,(uint8_t)(z+8),(uint8_t)(x+24),(uint8_t)(z+24)))capture=2;
        }
        if(control(m,0x1B9)==3){
            GameAddr actor=hn_render_slot(m,17);uint8_t actor_x=hn_read8(m,actor+0x1A),actor_z=hn_read8(m,actor+0x22);
            if(x>152&&hn_bank3_boxes_overlap(actor_x,(uint8_t)(actor_z+8),(uint8_t)(actor_x+24),(uint8_t)(actor_z+16),
                x,(uint8_t)(z+8),(uint8_t)(x+24),(uint8_t)(z+24)))capture=2;
        }
        if(capture){
            if(!animation(m,(uint8_t)(i+22),resource(m,0x08630ED4,hn_read8(m,entry))))return false;
            hn_bank3_chain_clear(m,0,hn_read8(m,entry+3));hn_bank3_chain_clear(m,1,hn_read8(m,entry+4));
            if(!hn_bank3_chain_effect(m,i,capture,hn_read8(m,entry)))return false;
            hn_bank3_chain_consume(m,i);
        }
    }
    switch(control(m,0x1D6)){
    case 1:if(!hn_audio_song(m,63,HN_SONG_START))return false;break;
    case 2:if(!hn_audio_song(m,61,HN_SONG_START))return false;break;
    case 3:if(!hn_audio_song(m,345,HN_SONG_START))return false;break;
    }
    if(any_first&&!refresh_lines(m))return false;
    if(old_count<control(m,0x1C2)&&!hn_render_configure_raw(m,12,1,0,0,0,0,0x088E8E28))return false;
    return !m->fault;
}
HNSceneResult hn_bank3_chain_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    (void)arguments;*consumed=0;
    switch(callback&~1u){
    case 0x0804D4E0:if(!initialize_entries(m))return HN_SCENE_RESOURCE;break;
    case 0x0804D708:
        hide(m,0);hide(m,14);store(m,0x1BD,255);
        for(unsigned i=1;i<=9;i++)hide(m,(uint8_t)i);
        for(unsigned i=0;i<=15;i++)hide(m,(uint8_t)(i+22));break;
    case 0x0804D764:if(!trigger_action(m))return HN_SCENE_RESOURCE;break;
    case 0x0804D8E4:if(!chain_keys(m))return HN_SCENE_RESOURCE;break;
    case 0x0804DA88:{
        uint32_t next=hn_read8(m,0x08630E5A+control(m,0x1B5))+control(m,0x1B4);store(m,0x1B4,next);
        if((uint8_t)next>95){store(m,0x1B4,0);if(!remove_offscreen(m)||!spawn_one(m))return HN_SCENE_RESOURCE;}
        uint8_t remainder=control(m,0x1B4)%10;
        if(!remainder){
            store(m,0x1BC,control(m,0x1BC)+1);if(control(m,0x1BC)>8)store(m,0x1BC,remainder);
            hn_write16(m,0x020015A6,(uint16_t)(0x81D7+control(m,0x1BC)));
            hn_write16(m,0x020015BA,(uint16_t)(0x81D7+control(m,0x1BC)));
        }
        break;
    }
    case 0x0804DB14:if(!update_primary(m))return HN_SCENE_RESOURCE;break;
    case 0x0804DC1C:if(!update_side(m,false))return HN_SCENE_RESOURCE;break;
    case 0x0804DD50:if(!update_side(m,true))return HN_SCENE_RESOURCE;break;
    case 0x0804DE90:if(!remove_offscreen(m))return HN_SCENE_RESOURCE;break;
    case 0x0804DF44:if(!capture_entries(m))return HN_SCENE_RESOURCE;break;
    case 0x0804E7E0:{uint8_t kind;if(!pick_kind(m,&kind))return HN_SCENE_RESOURCE;*consumed=kind;break;}
    case 0x0804EA1C:
        store(m,0x1BA,(uint32_t)control(m,0x1BA)-1);
        if(!control(m,0x1BA)){
            if(control(m,0x1B3)==0)store(m,0x1BB,control(m,0x1BB)!=7?control(m,0x1BB)+1:0);
            else store(m,0x1BB,control(m,0x1BB)?control(m,0x1BB)-1:7);
            store(m,0x1BA,hn_read8(m,0x08630E72+control(m,0x1B5)*8+control(m,0x1BB)));
            if(!hn_background_configure_raw(m,5,130,resource(m,0x08630FD0,control(m,0x1BB))))return HN_SCENE_RESOURCE;
        }
        break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank3_chain_task(HNMemory *m,GameAddr callback)
{
    bool ok;
    switch(callback&~1u){
    case 0x0804D4E0:case 0x0804D708:case 0x0804D764:case 0x0804D8E4:case 0x0804DA88:case 0x0804DB14:
    case 0x0804DC1C:case 0x0804DD50:case 0x0804DE90:case 0x0804DF44:case 0x0804E7E0:case 0x0804EA1C:{
        uint32_t ignored;return hn_bank3_chain_callback(m,callback,0,&ignored);
    }
    case 0x0804E62C:ok=spawn_one(m);break;
    case 0x0804EAB8:ok=count_display(m);break;
    case 0x0804EB20:ok=lines_display(m);break;
    case 0x0804EB64:ok=hn_bank3_chain_flags(m,0);break;
    case 0x0804EC8C:ok=flag_graphics(m);break;
    case 0x0804ED68:ok=refresh_lines(m);break;
    case 0x0804EEF0:ok=configure_grid(m);break;
    default:return HN_SCENE_UNKNOWN;
    }
    return ok&&!m->fault?HN_SCENE_OK:HN_SCENE_RESOURCE;
}
HNSceneResult hn_bank3_chain_animation(HNMemory *m,GameAddr callback,uint32_t *consumed,uint32_t *yield)
{
    if((callback&~1u)!=0x0804EE88)return HN_SCENE_UNKNOWN;
    uint8_t remainder=control(m,0x60)%10;
    if(!remainder){
        store(m,0x61,control(m,0x61)+1);if(control(m,0x61)>8)store(m,0x61,remainder);
        hn_write16(m,0x020015A6,(uint16_t)(0x81D7+control(m,0x61)));
        hn_write16(m,0x020015BA,(uint16_t)(0x81D7+control(m,0x61)));
    }
    store(m,0x60,control(m,0x60)+1);*yield=0;*consumed=0;return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
