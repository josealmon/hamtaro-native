#include "game/task.h"
#include "game/state.h"
#include "game/heap.h"
#include "game/render.h"
#include "game/transfer.h"
#include "game/background.h"
#include "game/graphics.h"
#include "game/input.h"
#include "game/text.h"
#include "gba/bios.h"

GameAddr hn_task_allocate(HNMemory *m)
{
    GameAddr link=HN_TASK_HEAD;
    for(unsigned visits=0;visits<2048;visits++){
        GameAddr task=hn_read32(m,link);
        if(task==HN_TASK_SENTINEL){
            task=hn_heap_allocate(m,52,true);if(!task)return 0;
            hn_write32(m,task+0x18,0);hn_write32(m,task+0x10,HN_TASK_DEFAULT_SCRIPT);
            hn_write32(m,task+0x28,link);hn_write32(m,task+0x2C,HN_TASK_SENTINEL);hn_write32(m,link,task);
            return m->fault?0:task;
        }
        if(!task||m->fault)return 0;link=task+0x2C;
    }
    return 0;
}
bool hn_task_remove(HNMemory *m,GameAddr target)
{
    if(!target||target==HN_TASK_SENTINEL)return false;
    GameAddr task=hn_read32(m,HN_TASK_HEAD);
    for(unsigned visits=0;visits<2048;visits++){
        if(!task||task==HN_TASK_SENTINEL||m->fault)return false;
        if(task==target){
            GameAddr link=hn_read32(m,task+0x28),next=hn_read32(m,task+0x2C);
            if(link)hn_write32(m,link,next);
            if(next!=HN_TASK_SENTINEL)hn_write32(m,next+0x28,link);
            hn_write32(m,task+0x28,0);hn_write32(m,task+0x2C,HN_TASK_SENTINEL);
            uint32_t freed=hn_heap_free(m,task);
            if(task==hn_read32(m,HN_TASK_HEAD))hn_write32(m,HN_TASK_HEAD,HN_TASK_SENTINEL);
            return freed==52&&!m->fault;
        }
        task=hn_read32(m,task+0x2C);
    }
    return false;
}
bool hn_task_graphics(HNMemory *m,GameAddr task,uint32_t width_arg,uint32_t height_arg,uint32_t x_arg,
                      uint32_t advance_arg,uint32_t delay_arg,uint32_t fill_arg,uint32_t flags_arg,GameAddr graphics,GameAddr map)
{
    unsigned width=width_arg&255,height=height_arg&255,x=x_arg&255,advance=advance_arg&255;
    unsigned delay=delay_arg&255,fill=fill_arg&255,flags=flags_arg&255;
    if(!task||!width||height<2)return false;
    uint32_t pattern=((fill&3)*0x44)*0x01010101u;
    hn_write8(m,task+0xB,(uint8_t)flags);hn_write8(m,task,(uint8_t)flags);
    hn_write32(m,task+0xC,graphics);hn_write32(m,task+0x24,map);
    hn_write8(m,task+1,(uint8_t)width);hn_write8(m,task+2,(uint8_t)height);hn_write8(m,task+0xA,0);
    hn_write16(m,task+0x1C,(uint16_t)x);hn_write8(m,task+3,(uint8_t)x);hn_write16(m,task+0x1E,0);hn_write16(m,task+0x20,0);
    hn_write8(m,task+4,(uint8_t)advance);hn_write8(m,task+5,(uint8_t)delay);hn_write8(m,task+7,(uint8_t)fill);
    hn_write8(m,task+9,(uint8_t)(height/2-1));hn_write8(m,task+8,(uint8_t)(height/2-1));
    GameAddr shadow=graphics+0xFC004720u;hn_write32(m,task+0x18,shadow);
    if(!hn_transfer_fill_words(m,shadow,pattern,width*16))return false;
    for(unsigned row=0;row<height;row+=2){
        GameAddr destination=graphics+width*(row/2)*64;
        if(flags&16)hn_transfer_queue(m,shadow,destination,width*64);
        else if(!hn_transfer_fill_words(m,destination,pattern,width*16))return false;
    }
    unsigned tile_base=(graphics<<18)>>23;
    for(unsigned row=0;row<height;row++)for(unsigned column=0;column<width;column++){
        GameAddr entry=map+row*64+column*2;
        hn_write16(m,entry,(uint16_t)((hn_read16(m,entry)&0xFC00)+width*row+column+tile_base));
    }
    hn_write8(m,HN_TASK_VARIANT,0);return !m->fault;
}
bool hn_task_render_disable(HNMemory *m,GameAddr task)
{
    if(!task)return false;if(!hn_read32(m,task+0x30))return !m->fault;
    if(hn_read8(m,HN_RENDER_INFO+0x10)<8||!hn_read32(m,HN_RENDER_SLOTS))return false;
    for(unsigned i=0;i<5;i++){
        GameAddr slot=hn_render_slot(m,i+3);unsigned mask=1u<<i;
        if(hn_read8(m,slot+0x60)&&(hn_read8(m,task+0xA)&mask)){
            hn_write8(m,task+0xA,(uint8_t)(hn_read8(m,task+0xA)&~mask));hn_write8(m,slot+0x60,0);
        }
    }
    return !m->fault;
}
bool hn_task_render_scroll(HNMemory *m,GameAddr task,bool clip)
{
    if(!task)return false;GameAddr resource=hn_read32(m,task+0x30);if(!resource)return !m->fault;
    if(hn_read8(m,HN_RENDER_INFO+0x10)<8||!hn_read32(m,HN_RENDER_SLOTS))return false;
    for(unsigned i=0;i<5;i++){
        GameAddr slot=hn_render_slot(m,i+3);unsigned mask=1u<<i;
        if(!hn_read8(m,slot+0x60)||!(hn_read8(m,task+0xA)&mask))continue;
        uint32_t y=hn_read32(m,slot+0x1C)-0x80000;hn_write32(m,slot+0x1C,y);
        if(clip&&(int32_t)y<(int32_t)((uint32_t)hn_read16(m,resource+0x22)<<16)){
            hn_write8(m,task+0xA,(uint8_t)(hn_read8(m,task+0xA)&~mask));hn_write8(m,slot+0x60,0);
        }
    }
    return !m->fault;
}
bool hn_task_indicator(HNMemory *m,GameAddr task)
{
    if(!task||!hn_read8(m,HN_RENDER_INFO+0x10)||!hn_read32(m,HN_RENDER_SLOTS))return false;
    uint32_t x=0xDC,y=0x94;GameAddr animation=HN_TASK_INDICATOR,resource=hn_read32(m,task+0x30);
    if(resource){x=((uint32_t)hn_read16(m,resource+0x24)<<16)+0x8000;y=((uint32_t)hn_read16(m,resource+0x26)<<16)+0x8000;
        animation=hn_read32(m,resource+0x28);}
    hn_render_slot_reset(m,0);GameAddr slot=hn_render_slot(m,0);
    hn_write8(m,slot+0x60,1);
    hn_write8(m,slot+0x62,120);hn_render_flags(m,0,12,0);
    hn_write32(m,slot+0x18,x);hn_write32(m,slot+0x1C,y+0x40000);hn_write32(m,slot+0x20,0);
    hn_render_set_animation(m,0,animation);return !m->fault;
}
bool hn_task_row_transfer(HNMemory *m,GameAddr task)
{
    if(!task)return false;unsigned half=hn_read8(m,task+2)>>1;if(!half)return false;
    unsigned bytes=hn_read8(m,task+1)<<6,row=hn_read16(m,task+0x1E)%half;
    if(((hn_read8(m,task)&128)||!hn_read8(m,task+5))&&bytes)
        hn_transfer_queue(m,hn_read32(m,task+0x18),hn_read32(m,task+0xC)+row*bytes,bytes);
    hn_write8(m,HN_TASK_DIRTY,0);return !m->fault;
}
bool hn_task_scroll_map(HNMemory *m,GameAddr task,bool complete)
{
    if(!task)return false;unsigned height=hn_read8(m,task+2),width=hn_read8(m,task+1);
    if(height==4||height==6){
        unsigned phase=hn_read16(m,task+0x1E)%(height/2),tile_row=phase*2+(complete?1:0);
        GameAddr map=hn_read32(m,task+0x24),graphics=hn_read32(m,task+0xC);
        for(unsigned row=0;row<height-1;row++)if(!hn_bios_cpu_set(m,map+(row+1)*64,map+row*64,width))return false;
        uint32_t pattern=(uint32_t)hn_read8(m,task+7)*0x44444444u;
        for(unsigned i=0;i<width*8;i++)hn_write32(m,graphics+tile_row*width*32+i*4,pattern);
        for(unsigned column=0;column<width;column++){
            GameAddr entry=map+(height-1)*64+column*2;
            hn_write16(m,entry,(uint16_t)((hn_read16(m,entry)&0xFC00)+tile_row*width+column+1));
        }
    }
    hn_write8(m,task+6,complete?0:2);hn_write8(m,HN_TASK_GATE,complete?0:1);
    if(complete){hn_write8(m,task,3);hn_write16(m,task+0x20,(uint16_t)(hn_read16(m,task+0x20)+1));}
    return !m->fault;
}
void hn_tasks_init(HNTasks *tasks,HNMemory *m)
{tasks->memory=m;tasks->fault=HN_TASK_OK;tasks->fault_opcode=0;tasks->fault_address=0;}
static bool task_fail(HNTasks *tasks,HNTaskFault fault,unsigned opcode,GameAddr address)
{
    if(tasks->memory->fault){fault=HN_TASK_MEMORY;address=tasks->memory->fault_address;}
    tasks->fault=fault;tasks->fault_opcode=opcode;tasks->fault_address=address;return false;
}
bool hn_tasks_update(HNTasks *tasks,unsigned command_budget)
{
    if(tasks->fault)return false;HNMemory *m=tasks->memory;
    if(m->fault)return task_fail(tasks,HN_TASK_MEMORY,0,m->fault_address);
    hn_write16(m,HN_TASK_KEYS,hn_input_query(m,3,1));GameAddr task=hn_read32(m,HN_TASK_HEAD);
    for(unsigned visits=0;task!=HN_TASK_SENTINEL;visits++,task=hn_read32(m,task+0x2C)){
        if(!task||visits>=2048)return task_fail(tasks,HN_TASK_RESOURCE,0,task);
        unsigned mode=hn_read8(m,task)&127;
        if(m->fault)return task_fail(tasks,HN_TASK_MEMORY,0,m->fault_address);
        if(mode<=1)continue;
        if(mode==2){
            if(hn_read8(m,HN_TASK_GATE)==1){
                uint8_t count=(uint8_t)(hn_read8(m,task+6)-1);hn_write8(m,task+6,count);
                if(!count&&(!hn_task_scroll_map(m,task,true)||!hn_task_render_scroll(m,task,false)))return task_fail(tasks,HN_TASK_RESOURCE,0,task);
            }
            continue;
        }
        if(mode==4){
            if(!(hn_read8(m,task+0xB)&1)&&hn_input_query(m,2,0)){hn_write8(m,task,0x83);hn_write8(m,task+6,0);}
            else{uint8_t count=(uint8_t)(hn_read8(m,task+6)-1);hn_write8(m,task+6,count);if(!count)hn_write8(m,task,3);continue;}
        }
        unsigned width=hn_read8(m,task+1),half=hn_read8(m,task+2)>>1;if(!width||!half)return task_fail(tasks,HN_TASK_RESOURCE,0,task);
        if(hn_read8(m,task+0xB)&4){
            hn_write8(m,task+0xB,hn_read8(m,task+0xB)&0xFB);
            if(!hn_transfer_fill_words(m,hn_read32(m,task+0x18),(uint32_t)hn_read8(m,task+7)*0x44444444u,width*16))
                return task_fail(tasks,HN_TASK_RESOURCE,0,task);
        }
        bool stopped=false;
        for(unsigned count=0;count<command_budget;count++){
            GameAddr script=hn_read32(m,task+0x10);unsigned opcode=hn_read8(m,script);uint32_t keep_going=1;
            if(!opcode)opcode=0xE1;
            if(opcode==0x5E){hn_write32(m,task+0x10,script+1);hn_task_text_variant(m,task,2);continue;}
            if(opcode<=0xDF){
                unsigned code=(opcode-1)&255,advance=hn_read8(m,task+4)&127,flags=hn_read8(m,task+0xB);
                unsigned glyph=code+((flags&8)?256:0),glyph_width=hn_read8(m,HN_TEXT_ADVANCES+glyph);
                bool centered=(hn_read8(m,task+4)&128)!=0;unsigned x=hn_read16(m,task+0x1C);
                if(centered)x=(uint16_t)(x+(uint8_t)((int32_t)(advance-glyph_width)>>1));
                unsigned offset=(x&0xF8)<<2,row=hn_read16(m,task+0x1E)%half;
                GameAddr shadow=hn_read32(m,task+0x18),destination=hn_read32(m,task+0xC)+width*64*row;
                if(!hn_text_blit_glyph(m,HN_TEXT_GLYPHS+glyph*64,shadow,width,x,hn_read8(m,task+7)))
                    return task_fail(tasks,HN_TASK_RESOURCE,opcode,script);
                hn_write16(m,task+0x1C,(uint16_t)(hn_read16(m,task+0x1C)+advance+(centered?0:glyph_width)));
                unsigned delay=(hn_read8(m,task)&128)?0:hn_read8(m,task+5);hn_write8(m,task+6,(uint8_t)delay);
                hn_write32(m,task+0x10,hn_read32(m,task+0x10)+1);hn_write8(m,HN_TASK_DIRTY,1);
                if(!delay)continue;
                int remaining=(int)(width*32)-(int)offset;
                if(remaining>0){
                    unsigned bytes=remaining>96?96:(unsigned)remaining;
                    hn_transfer_queue(m,shadow+offset,destination+offset,bytes);
                    hn_transfer_queue(m,shadow+width*32+offset,destination+width*32+offset,bytes);
                }
                hn_write8(m,task,4);stopped=true;break;
            }
            if(opcode>=0xF0)hn_write32(m,task+0x10,script+1);
            if(!hn_task_command(tasks,task,opcode,&keep_going))return false;
            if(!keep_going){stopped=true;break;}
        }
        if(!stopped)return task_fail(tasks,HN_TASK_BUDGET,0,hn_read32(m,task+0x10));
        if(m->fault)return task_fail(tasks,HN_TASK_MEMORY,0,m->fault_address);
    }
    return m->fault?task_fail(tasks,HN_TASK_MEMORY,0,m->fault_address):true;
}
static bool task_text_start(HNMemory *m,GameAddr task,GameAddr arguments)
{
    for(unsigned i=0;i<4;i++)hn_write8(m,0x030005E8+i,hn_read8(m,arguments+i));
    GameAddr text=hn_read32(m,0x030005E8);
    if(text<=15){text=hn_read32(m,0x03002BB0+text*4);hn_write32(m,0x030005E8,text);}
    if(text!=HN_TASK_DEFAULT_SCRIPT){
        unsigned language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);if(language>=5)return false;
        text=hn_read32(m,text+language*4);
    }
    return hn_task_set_script(m,task,text);
}
bool hn_task_main_command(HNMemory *m,unsigned opcode,GameAddr arguments)
{
    unsigned slot=hn_read8(m,arguments);if(slot>=4)return false;
    GameAddr link=HN_TASK_SLOTS+slot*4,task=hn_read32(m,link);
    GameAddr resource=HN_TASK_RESOURCES+hn_read8(m,HN_TASK_RESOURCE_INDEXES+slot)*44;
    unsigned initialized=hn_read8(m,0x03000018);
    if(opcode==0x38){
        unsigned index=hn_read8(m,arguments+1);hn_write8(m,HN_TASK_RESOURCE_INDEXES+slot,(uint8_t)index);
        if(task)hn_write32(m,task+0x30,HN_TASK_RESOURCES+index*44);
    }else if(opcode==0x54){if(!task)return false;hn_write8(m,task+5,hn_read8(m,arguments+1));}
    else{
        if(slot>=hn_read8(m,HN_BACKGROUND_COUNT)||!hn_read32(m,HN_RENDER_STATES))return false;
        bool busy=hn_read8(m,hn_read32(m,HN_RENDER_STATES)+slot*80+0x38)!=0;
        if(opcode==0x19){
            if(busy){hn_write8(m,0x0300001D,1);return !m->fault;}
            if(hn_read8(m,arguments+1)){
                if(!task){task=hn_task_allocate(m);if(!task)return false;hn_write32(m,link,task);}
                GameAddr image=hn_read32(m,resource+0x10);
                hn_write32(m,task+0x18,(image&0xFFFFF)+0x02004720);
                uint32_t pattern=hn_read8(m,resource+0x1C);pattern=(pattern<<2)|(pattern<<6);pattern|=pattern<<8;pattern|=pattern<<16;
                hn_transfer_queue_fill(m,pattern,image,(hn_read8(m,resource+0x19)*hn_read8(m,resource+0x18))<<5);
                hn_write32(m,task+0x30,resource);
                GameAddr background=hn_read32(m,resource),graphics=hn_read32(m,resource+8);
                if(background&&!hn_background_configure(m,(uint8_t)slot,0x88,background))return false;
                if(graphics&&!hn_graphics_configure(m,(uint8_t)slot,graphics))return false;
                hn_write32(m,0x03000014,2);hn_write8(m,0x03000010,0);hn_write8(m,0x0300001D,1);hn_write8(m,task,1);
                return !m->fault;
            }
            if(!initialized){
                if(task){
                    if(!hn_task_render_disable(m,task))return false;
                    GameAddr background=hn_read32(m,resource+4),graphics=hn_read32(m,resource+12);
                    if(background&&!hn_background_configure(m,(uint8_t)slot,0x88,background))return false;
                    if(graphics&&!hn_graphics_configure(m,(uint8_t)slot,graphics))return false;
                    hn_write8(m,0x03000018,1);hn_write8(m,0x0300001D,1);return !m->fault;
                }
            }else if(hn_read8(m,HN_TASK_FLAGS+slot)){
                if(!hn_task_remove(m,task))return false;hn_write32(m,link,0);hn_write8(m,HN_TASK_FLAGS+slot,0);
            }
        }else if(opcode==0x1A||opcode==0x1B){
            unsigned bias=opcode==0x1A?2:1;
            if(initialized>=bias){
                unsigned selected=initialized-bias;if(selected>=4)return false;
                GameAddr selected_task=hn_read32(m,HN_TASK_SLOTS+selected*4);if(!selected_task)return false;
                if(hn_read8(m,selected_task)==1){hn_write32(m,0x03000014,5);hn_write8(m,0x03000010,0);return !m->fault;}
            }else if(opcode==0x1A&&!initialized){
                if(!task)return false;if(hn_read8(m,task+0xA)&&!hn_task_render_disable(m,task))return false;
                if(!hn_task_graphics(m,task,hn_read8(m,resource+0x18),hn_read8(m,resource+0x19),hn_read8(m,resource+0x1A),
                    hn_read8(m,resource+0x1B),hn_read8(m,resource+0x1D),hn_read8(m,resource+0x1C),hn_read8(m,resource+0x1E),
                    hn_read32(m,resource+0x10),hn_read32(m,resource+0x14)))return false;
                hn_write8(m,HN_TASK_FLAGS+slot,1);hn_write8(m,0x03000018,1);
            }else{
                /* Scratch is copied even while the background still owns the slot. */
                if(busy){for(unsigned i=0;i<4;i++)hn_write8(m,0x030005E8+i,hn_read8(m,arguments+1+i));}
                else{
                    if(!task_text_start(m,task,arguments+1))return false;
                    hn_write8(m,0x03000018,(uint8_t)(slot+bias));
                }
            }
            hn_write8(m,0x0300001D,1);return !m->fault;
        }else return false;
    }
    hn_write32(m,0x03000014,2);hn_write8(m,0x03000010,0);return !m->fault;
}
