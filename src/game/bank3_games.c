#include "game/bank3.h"
#include "game/state.h"
#include "game/render.h"
#include "game/background.h"
#include "game/audio.h"
#include "game/random.h"
#include "game/task.h"
#include "game/input.h"
#include "game/scene_actor.h"
#include "gba/bios.h"

enum { CLASSES=0x0201DC40,PHASE_ROWS=0x020015C8,ORBIT_ANIMATIONS=0x02001640,
       SOURCE_ENTRIES=0x03003EA0,SPAWN_ENTRIES=0x03003DE0 };
static uint8_t control(HNMemory *m,unsigned offset)
{return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value)
{hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static bool flag(HNMemory *m,unsigned id)
{return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void set_flag(HNMemory *m,unsigned id,bool value)
{
    GameAddr address=0x03002180+(id>>3);uint8_t mask=(uint8_t)(1u<<(id&7));
    hn_write8(m,address,(uint8_t)((hn_read8(m,address)&~mask)|(value?mask:0)));
}
static bool animation(HNMemory *m,uint8_t index,GameAddr resource)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,resource);return !m->fault;
}
static void priority(HNMemory *m,uint8_t index,uint8_t value)
{hn_render_flags(m,index,12,(uint8_t)(value<<2));}
static GameAddr phase_resource(HNMemory *m,uint8_t selected,unsigned field)
{return hn_read32(m,PHASE_ROWS+selected*20+field*4);}
static uint32_t sine(HNMemory *m,unsigned angle)
{return (uint32_t)(int32_t)(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+angle*2);}

/* The source retains its incoming R4 word for modes outside 0..4. */
uint32_t hn_bank3_phase_kind(HNMemory *m,uint32_t mode_word,uint32_t retained)
{
    uint8_t mode=(uint8_t)mode_word,value=(uint8_t)hn_random_range(m,0,16);
    switch(mode){
    case 0:return value<=6?1:value<=8?2:value>14?5:4;
    case 1:return value<=1?1:value<=4?2:value==5?0:value<=10?1:value>13?0:2;
    case 2:return value<=3?1:value>13?0:2;
    case 3:return value<=1?4:value<=4?5:value==5?3:value<=10?4:value>13?3:5;
    case 4:return value<=3?4:value>13?3:5;
    default:return retained;
    }
}
uint32_t hn_bank3_boxes_overlap(uint8_t x0,uint8_t z0,uint8_t x1,uint8_t z1,
                               uint8_t other_x0,uint8_t other_z0,uint8_t other_x1,uint8_t other_z1)
{
    bool x=(x0<other_x0&&x1>other_x0)||(x0<other_x1&&x1>other_x1);
    bool z=(z0<other_z0&&z1>other_z0)||(z0<other_z1&&z1>other_z1);
    return x&&z;
}
static bool initialize_phases(HNMemory *m)
{
    uint8_t mode=0,selected=0;
    for(unsigned i=0;i<3;i++)store(m,0x63+i,0);
    store(m,0x66,0);store(m,0x60,0);
    for(unsigned i=0;i<3;i++){
        selected=(uint8_t)hn_bank3_phase_kind(m,mode,0);store(m,0x63+i,selected);
        mode=selected>2?(mode==3?4:3):(mode==1?2:1);
    }
    if(!hn_render_configure_raw(m,15,1,120,120,0,120,phase_resource(m,selected,0))||
       !hn_render_configure_raw(m,16,1,120,120,0,120,phase_resource(m,selected,1)))return false;
    priority(m,15,1);priority(m,16,1);
    for(unsigned i=0;i<3;i++)
        if(!hn_background_configure_raw(m,(uint8_t)(4+i),(uint8_t)(1u<<i),phase_resource(m,selected,i+2)))return false;
    int32_t direction=control(m,0x63)>2?-1:1;
    const GameAddr first[]={0x088EA03F,0x088EA095,0x088EA06A,0x088EA095,0x088EA03F};
    const GameAddr second[]={0x088EA168,0x088EA112,0x088EA13D,0x088EA168,0x088EA112};
    for(unsigned i=0;i<5;i++)
        if(!hn_render_configure_raw(m,(uint8_t)(17+i),1,116,direction*(int32_t)(i*450),100,0,first[i]))return false;
    for(unsigned i=0;i<5;i++)
        if(!hn_render_configure_raw(m,(uint8_t)(22+i),1,116,direction*(int32_t)(256+i*450),72,0,second[i]))return false;
    for(unsigned i=17;i<=26;i++)priority(m,(uint8_t)i,3);
    return !m->fault;
}
static bool advance_phase(HNMemory *m)
{
    if(!control(m,0x66))return !m->fault;
    store(m,0x66,0);store(m,0x60,control(m,0x60)+1);uint8_t phase=control(m,0x60);
    if(phase==2){
        bool backwards=control(m,0x65)>2,paired=flag(m,0x2BC)||flag(m,0x196);
        GameAddr resource=backwards?(paired?0x088EA1E5:0x088EA1CA):(paired?0x088EA1AF:0x088EA193);
        if(!hn_render_configure_raw(m,17,1,120,backwards?-32:272,110,0,resource))return false;
        if(paired){
            if(!hn_render_configure_raw(m,27,1,176,12,-20,0,0x088EA200)||
               !hn_render_configure_raw(m,28,1,176,-14,-20,0,0x088EA216)||
               !hn_render_modes_raw(m,27,2,3,17)||!hn_render_modes_raw(m,28,2,3,17))return false;
        }
    }
    if(phase>2)phase=2;uint8_t selected=control(m,phase+0x63);
    if(!animation(m,15,phase_resource(m,selected,0))||!animation(m,16,phase_resource(m,selected,1)))return false;
    for(unsigned i=0;i<3;i++)
        if(!hn_background_configure_raw(m,(uint8_t)(4+i),(uint8_t)(128|(1u<<i)),phase_resource(m,selected,i+2)))return false;
    return !m->fault;
}
static bool shuffle_orbit(HNMemory *m)
{
    uint8_t order[6]={255,255,255,255,4,255};
    if(control(m,0x93)==4)order[5]=5;
    for(unsigned index=0;index<6;index++)if(order[index]!=index){
        uint8_t selected;unsigned attempts=0;
        do {if(++attempts>4096)return false;selected=(uint8_t)hn_random_range(m,0,6);}while(order[selected]!=255);
        order[selected]=(uint8_t)index;
    }
    for(unsigned index=0;index<6;index++){
        uint8_t render_index=(uint8_t)(index*3+17);
        if(!hn_render_configure_raw(m,render_index,1,176,0,0,0,hn_read32(m,ORBIT_ANIMATIONS+order[index]*4))||
           !hn_render_modes_raw(m,render_index,2,3,(uint8_t)(render_index-1)))return false;
        priority(m,render_index,1);
    }
    hn_write16(m,CLASSES+0xE3*2,0);return !m->fault;
}
static bool initialize_pair_board(HNMemory *m)
{
    for(unsigned i=0;i<=13;i++)store(m,0x1A5+i,0);
    store(m,0x1E4,0);
    for(unsigned index=0;index<=3;index++)if(!((control(m,0x1A5)>>index)&1)){
        for(unsigned placed=0;placed<2;placed++){
            uint8_t candidate;unsigned attempts=0;
            do {if(++attempts>4096)return false;candidate=(uint8_t)hn_random_range(m,0,9);}while(control(m,0x1A6+candidate));
            store(m,0x1A6+candidate,index+1);
        }
    }
    unsigned index=0;while(index<=8&&control(m,0x1A6+index))index++;
    uint32_t variant=(control(m,0x1A5)&16)?1:(uint8_t)hn_random_range(m,0,2);
    store(m,0x1A6+index,variant+5);return !m->fault;
}
static bool select_pair_position(HNMemory *m)
{
    uint32_t previous=control(m,0x1B2);uint8_t candidate;unsigned attempts=0;
    for(;;){
        if(++attempts>4096)return false;
        candidate=(uint8_t)hn_random_range(m,1,10);uint32_t kind=control(m,candidate+0x1A5);
        if((uint8_t)(kind-1)>3)continue;
        if(control(m,0x1B1)==candidate||previous==candidate||kind==control(m,0x1E4))continue;
        break;
    }
    store(m,0x1B2,candidate);
    hn_write16(m,CLASSES+36*2,hn_read16(m,0x08630E20+(candidate-1)*4));
    hn_write16(m,CLASSES+37*2,hn_read16(m,0x08630E22+(candidate-1)*4));return !m->fault;
}
static uint32_t register_bit(uint32_t shift)
{shift&=255;return shift<32?1u<<shift:0;}
static void score_tiles(HNMemory *m,GameAddr tiles,uint32_t value,uint32_t tile)
{
    hn_write16(m,tiles+6,(uint16_t)(tile+value/1000));
    hn_write16(m,tiles+12,(uint16_t)(tile+(value%1000)/100));
    hn_write16(m,tiles+18,(uint16_t)(tile+(value%100)/10));
    hn_write16(m,tiles+24,(uint16_t)(tile+value%10));
}
static bool record_messages(HNMemory *m)
{
    const GameAddr contexts=0x030037A0;GameAddr task=hn_task_allocate(m);hn_write32(m,contexts,task);
    if(!hn_task_graphics(m,task,10,2,255,4,0,0,0,0x06004020,0x0600B600))return false;
    task=hn_task_allocate(m);hn_write32(m,contexts+4,task);
    if(!hn_task_graphics(m,task,10,2,255,4,0,0,0,0x060042A0,0x0600B600))return false;
    if(!hn_read16(m,CLASSES+66*2)){
        GameAddr first=hn_read32(m,contexts),text=hn_read32(m,0x0862DE8C+control(m,0xEB)*4);
        if(!hn_task_set_script(m,first,text))return false;
        GameAddr second=hn_read32(m,contexts+4);text=hn_read32(m,0x0862DEA0+control(m,0xEB)*4);
        if(!hn_task_set_script(m,second,text))return false;
    }else{
        if(!hn_task_set_script(m,hn_read32(m,contexts),0x03002BA0)||
           !hn_task_set_script(m,hn_read32(m,contexts+4),0x03001F70))return false;
    }
    return !m->fault;
}
static void time_tiles(HNMemory *m)
{
    uint32_t value=(uint16_t)((3600-(int32_t)hn_read16(m,CLASSES+224*2))/60);
    hn_write16(m,0x0200154C+6,(uint16_t)(0x7145+value/60));
    hn_write16(m,0x0200154C+18,(uint16_t)(0x7145+(value%60)/10));
    hn_write16(m,0x0200154C+24,(uint16_t)(0x7145+value%10));
}
static void hide_slot(HNMemory *m,uint8_t index)
{hn_write8(m,hn_render_slot(m,index)+0x60,0);}
bool hn_bank3_entry_configure(HNMemory *m,uint32_t index_word)
{
    uint8_t index=(uint8_t)index_word;GameAddr entry=SOURCE_ENTRIES+index*8;
    uint32_t random=(uint8_t)hn_random_range(m,0,8),offset=hn_read8(m,entry)*2;
    hn_write8(m,entry,(uint8_t)(random<hn_read8(m,0x08630E00+offset)?1:random<hn_read8(m,0x08630E01+offset)?2:3));
    random=(uint8_t)hn_random_range(m,0,16);
    if((control(m,0x1E0)>19||control(m,0x1E1)>4)&&control(m,0x1A0)){
        store(m,0x1E0,0);store(m,0x1E1,0);hn_write8(m,entry+1,4);
    }else{
        offset=control(m,0x1A0)*3;
        hn_write8(m,entry+1,(uint8_t)(random<hn_read8(m,0x08630E08+offset)?1:random<hn_read8(m,0x08630E09+offset)?2:3));
    }
    random=(uint8_t)hn_random_range(m,0,8);offset=hn_read8(m,entry+2)*3;
    uint8_t variation=random<hn_read8(m,0x08630E11+offset)?0:random<hn_read8(m,0x08630E12+offset)?1:random<hn_read8(m,0x08630E13+offset)?2:3;
    hn_write8(m,entry+2,variation);hn_write16(m,entry+6,24);hn_write8(m,entry+3,control(m,0x1A0));hn_write8(m,entry+4,0);
    uint16_t x;unsigned attempts=0;
    do {
        if(++attempts>4096)return false;x=hn_random_range(m,64,200);
    }while((int32_t)x>=(int32_t)hn_read16(m,0x03003DB4)-24&&(int32_t)x<=(int32_t)hn_read16(m,0x03003DB4)+24);
    GameAddr resource=hn_read32(m,0x08630DB8+((uint32_t)hn_read8(m,entry+1)-1)*4);
    if(!hn_render_configure_raw(m,(uint8_t)(index+23),1,114,(int16_t)x,0,-24,resource))return false;
    hn_write32(m,hn_render_slot(m,(uint32_t)index+23)+0x2C,0x10000);return !m->fault;
}
bool hn_bank3_entry_motion(HNMemory *m,uint8_t index)
{
    GameAddr entry=SPAWN_ENTRIES+index*8;hn_write8(m,entry+4,16);
    if(hn_read8(m,entry+1)!=4){
        uint32_t random=(uint8_t)hn_random_range(m,0,8),offset=hn_read8(m,entry+2)*3;
        uint8_t variation=random<hn_read8(m,0x08630E11+offset)?0:random<hn_read8(m,0x08630E12+offset)?1:random<hn_read8(m,0x08630E13+offset)?2:3;
        hn_write8(m,entry+2,variation);GameAddr slot=hn_render_slot(m,(uint32_t)index+26);
        uint32_t velocity=(uint32_t)control(m,0x1A4)<<(variation&1?16:15);
        hn_write32(m,slot+0x24,variation<2?velocity:0u-velocity);
        uint32_t acceleration=(uint32_t)control(m,0x1A4)<<(variation&1?11:10);
        hn_write32(m,slot+0x30,variation<2?0u-acceleration:acceleration);
        GameAddr resource=hn_read32(m,0x08630DE8+((uint32_t)hn_read8(m,entry+1)-1)*8+(variation<2?4:0));
        if(!animation(m,(uint8_t)(index+26),resource))return false;
        random=(uint8_t)hn_random_range(m,0,8);
        bool slow=random<hn_read8(m,0x08630E1D+hn_read8(m,SPAWN_ENTRIES+index*8+3));
        slot=hn_render_slot(m,(uint32_t)index+26);
        hn_write32(m,slot+0x2C,(uint32_t)control(m,0x1A4)<<(slow?15:16));
        hn_write32(m,slot+0x38,0u-((uint32_t)control(m,0x1A4)*(slow?7u:17u)<<8));
    }else hn_write32(m,hn_render_slot(m,(uint32_t)index+26)+0x2C,(uint32_t)control(m,0x1A4)<<16);
    return !m->fault;
}
bool hn_bank3_entry_spawn(HNMemory *m,uint32_t index_word)
{
    uint8_t index=(uint8_t)index_word,available=0;
    if(!hn_read8(m,SPAWN_ENTRIES))hn_write8(m,SPAWN_ENTRIES,1);
    else while(++available<=9)if(!hn_read8(m,SPAWN_ENTRIES+available*8)){hn_write8(m,SPAWN_ENTRIES+available*8,1);break;}
    /* A full pool retains the source's mapped slot-10 alias, without setting
       its stage byte. It is not converted into an allocation failure. */
    GameAddr entry=SPAWN_ENTRIES+available*8,origin=SOURCE_ENTRIES+index*8;
    uint8_t kind=hn_read8(m,origin+1);hn_write8(m,entry+1,kind);
    hn_write8(m,entry+2,hn_read8(m,origin+2));hn_write8(m,entry+3,hn_read8(m,origin+3));
    GameAddr slot=hn_render_slot(m,(uint32_t)index+23);int16_t x=(int16_t)hn_read16(m,slot+0x1A),z=(int16_t)hn_read16(m,slot+0x22);
    GameAddr resource=kind==4?0x088E85C2:hn_read32(m,0x08630DE8+((uint32_t)hn_read8(m,entry+1)-1)*8+(hn_read8(m,entry+2)>>1)*4);
    if(!hn_render_configure_raw(m,(uint8_t)(available+26),1,114,x,0,kind==4?z:(int16_t)(z+16),resource))return false;
    priority(m,(uint8_t)(available+26),1);return hn_bank3_entry_motion(m,available);
}
static bool update_source_entries(HNMemory *m)
{
    for(unsigned index=0;index<=2;index++){
        GameAddr entry=SOURCE_ENTRIES+index*8;hn_write16(m,entry+6,(uint16_t)(hn_read16(m,entry+6)-1));
        if(!hn_read8(m,entry))continue;
        switch(hn_read8(m,entry+4)){
        case 0:
            if(!hn_read16(m,entry+6)){
                hn_write32(m,hn_render_slot(m,index+23)+0x2C,0);
                if(hn_read8(m,entry+1)==4)hn_write16(m,entry+6,30);
                else switch(hn_read8(m,entry+3)){
                case 0:hn_write16(m,entry+6,hn_random_range(m,40,60));break;
                case 1:hn_write16(m,entry+6,45);break;
                case 2:hn_write16(m,entry+6,hn_random_range(m,30,50));break;
                }
                uint32_t random=(uint8_t)hn_random_range(m,0,8),offset=hn_read8(m,entry+3)*2;
                uint32_t speed=random<hn_read8(m,0x0200159A+offset)?0x10000:random<hn_read8(m,0x0200159B+offset)?0x18000:0x20000;
                uint8_t stage=hn_read8(m,entry);
                if(stage==1||stage==2)hn_write32(m,hn_render_slot(m,index+23)+0x24,stage==1?speed:0u-speed);
                hn_write8(m,entry+4,(uint8_t)(hn_read8(m,entry+4)+1));
            }
            break;
        case 1:{
            GameAddr slot=hn_render_slot(m,index+23);
            if(hn_read32(m,slot+0x18)-0x400001u>0x87FFFEu)hn_write32(m,slot+0x24,0u-hn_read32(m,slot+0x24));
            if(!hn_read16(m,entry+6)){
                hn_write32(m,hn_render_slot(m,index+23)+0x24,0);hn_write16(m,entry+6,45);
                hn_write8(m,entry+4,(uint8_t)(hn_read8(m,entry+4)+1));
            }
            break;
        }
        case 2:
            if(hn_read16(m,entry+6)==20){
                if(!animation(m,(uint8_t)(index+23),hn_read32(m,0x08630DC8+((uint32_t)hn_read8(m,entry+1)-1)*4))||
                   !hn_bank3_entry_spawn(m,index))return false;
            }
            if(!hn_read16(m,entry+6)){
                hn_write32(m,hn_render_slot(m,index+23)+0x28,0xFFFF0000);hn_write16(m,entry+6,40);
                hn_write8(m,entry+4,(uint8_t)(hn_read8(m,entry+4)+1));
            }
            break;
        case 3:
            if(!hn_read16(m,entry+6)){
                hide_slot(m,(uint8_t)(index+23));hn_write16(m,entry+6,40);
                hn_write8(m,entry+4,(uint8_t)(hn_read8(m,entry+4)+1));
            }
            break;
        case 4:if(!hn_read16(m,entry+6)&&!hn_bank3_entry_configure(m,index))return false;break;
        }
    }
    return !m->fault;
}
static bool update_spawn_entries(HNMemory *m)
{
    for(unsigned index=0;index<=9;index++){
        GameAddr entry=SPAWN_ENTRIES+index*8;if(!hn_read8(m,entry))continue;
        hn_write8(m,entry+4,(uint8_t)(hn_read8(m,entry+4)-1));
        if(!hn_read8(m,entry+4)&&!hn_bank3_entry_motion(m,(uint8_t)index))return false;
        GameAddr slot=hn_render_slot(m,index+26);int32_t x=(int32_t)hn_read32(m,slot+0x18);
        if((x<=0x2FFFFF&&hn_read8(m,entry+2)>1)||(x>0xD00000&&hn_read8(m,entry+2)<=1)){
            slot=hn_render_slot(m,index+26);hn_write32(m,slot+0x24,0u-hn_read32(m,slot+0x24));
            hn_write8(m,entry+2,hn_read8(m,entry+2)^2);
        }
        GameAddr base=hn_read32(m,HN_RENDER_SLOTS);slot=base+(index+26)*104;
        uint8_t particle_x=hn_read8(m,slot+0x1A),particle_z=hn_read8(m,slot+0x22);
        uint8_t leader_x=hn_read8(m,base+10*104+0x1A),leader_z=hn_read8(m,base+10*104+0x22);
        if((uint8_t)(control(m,0x1A1)-1)<=1&&hn_bank3_boxes_overlap(
            (uint8_t)(leader_x+16),(uint8_t)(leader_z-48),(uint8_t)(leader_x+40),(uint8_t)(leader_z-8),
            (uint8_t)(particle_x+2),(uint8_t)(particle_z+2),(uint8_t)(particle_x+6),(uint8_t)(particle_z+6))){
            hn_write8(m,entry,0);uint8_t kind=hn_read8(m,entry+1);
            if(kind>=1&&kind<=3){
                if(!hn_audio_song(m,315,HN_SONG_START))return false;hide_slot(m,(uint8_t)(index+26));
                hn_write16(m,CLASSES+225*2,(uint16_t)(hn_read16(m,CLASSES+225*2)+(kind==1?50:kind==2?30:10)));
                score_tiles(m,0x02001566,hn_read16(m,CLASSES+225*2),0x7145);
                if(kind==2)store(m,0x1E0,control(m,0x1E0)+1);
                if(kind==3)store(m,0x1E0,control(m,0x1E0)+2);
                store(m,0x1E1,kind==3?control(m,0x1E1)+1:0);
            }else if(kind==4){
                if(!animation(m,(uint8_t)(index+26),0x088E85CA))return false;
                GameAddr velocity=hn_render_slot(m,index+26)+0x24;
                for(unsigned axis=0;axis<3;axis++)hn_write32(m,velocity+axis*4,0);
                hn_write16(m,CLASSES+224*2,(uint16_t)(hn_read16(m,CLASSES+224*2)-600));time_tiles(m);store(m,0x1E1,0);
            }
            store(m,0x1A1,3);
            if(!animation(m,10,0x088B132D)||!hn_render_configure_raw(m,9,1,176,0,0,0,
                hn_read32(m,0x08630DD8+((uint32_t)hn_read8(m,entry+1)-1)*4))||!hn_render_modes_raw(m,9,2,3,10)||
               !hn_render_configure_raw(m,11,1,176,0,0,0,0x088E865C)||!hn_render_modes_raw(m,11,2,3,10)||
               !animation(m,13,0x088B831A))return false;
        }
        if((int32_t)hn_read32(m,hn_render_slot(m,index+26)+0x20)>0xA00000){hide_slot(m,(uint8_t)(index+26));hn_write8(m,entry,0);}
    }
    return !m->fault;
}
static bool controlled_slots(HNMemory *m)
{
    if(control(m,0x1A1)){
        if(hn_read16(m,0x03001FBC)&&control(m,0x1A1)==1){store(m,0x1A1,2);return animation(m,13,0x088B847E);}
        return !m->fault;
    }
    uint16_t input=hn_read16(m,0x03001FBC);
    switch(input){
    case 0:
        if(!animation(m,20,0x088E86BD))return false;
        if(control(m,0x1A2)==1){if(!animation(m,13,0x088B40C3))return false;}
        else if(control(m,0x1A2)==2){if(!animation(m,13,0x088B408C))return false;}
        if(!hn_audio_song(m,59,HN_SONG_STOP))return false;store(m,0x1A2,0);break;
    case 1:{
        GameAddr slot=hn_render_slot(m,10);
        if((int32_t)hn_read32(m,slot+0x18)>0x280000)hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)-((uint32_t)control(m,0x1A3)<<16));
        if(control(m,0x1A2)!=1){
            store(m,0x1A2,input);
            if(!animation(m,20,0x088E86E2)||!animation(m,13,0x088B8340)||!hn_audio_song(m,59,HN_SONG_START))return false;
        }
        break;
    }
    case 2:{
        GameAddr slot=hn_render_slot(m,10);
        if((int32_t)hn_read32(m,slot+0x18)<=0xAFFFFF)hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+((uint32_t)control(m,0x1A3)<<16));
        if(control(m,0x1A2)!=2){
            if(!animation(m,20,0x088E86F9))return false;store(m,0x1A2,input);
            if(!animation(m,13,0x088B83DF)||!hn_audio_song(m,59,HN_SONG_START))return false;
        }
        break;
    }
    }
    hn_input_script_mask(m,1);
    if(hn_read16(m,CLASSES)&1){
        if(!animation(m,10,0x088A4DDA)||!hn_render_configure_raw(m,11,1,176,0,0,0,0x088E85F4)||
           !hn_render_modes_raw(m,11,2,3,10)||!animation(m,20,0x088E86BD)||!hn_audio_song(m,59,HN_SONG_STOP))return false;
        return animation(m,13,0x088B8324);
    }
    return !m->fault;
}
HNSceneResult hn_bank3_games_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x0804B634:{ /* Choose two distinct flag entries from the private table. */
        uint8_t choices[5];for(unsigned i=0;i<5;i++)choices[i]=hn_read8(m,0x08630DB0+i);
        for(unsigned selected=0;selected<2;selected++){
            uint8_t index=(uint8_t)hn_random_range(m,0,(uint16_t)(5-selected));
            if(choices[index]<=4)set_flag(m,0x16B+choices[index],true);
            for(unsigned shift=index;shift<=3;shift++)choices[shift]=choices[shift+1];
        }
        break;
    }
    case 0x0804B860:if(!controlled_slots(m))return HN_SCENE_RESOURCE;break;
    case 0x0804BA24:
        hn_write16(m,CLASSES+224*2,0);hn_write16(m,CLASSES+225*2,0);
        store(m,0x1A0,0);store(m,0x1A2,0);store(m,0x1A1,0);store(m,0x1E0,0);store(m,0x1E1,0);
        store(m,0x1A3,1);store(m,0x1A4,2);
        for(unsigned index=0;index<=2;index++){
            GameAddr entry=SOURCE_ENTRIES+index*8;
            hn_write8(m,entry,0);hn_write8(m,entry+1,0);hn_write8(m,entry+2,0);hn_write16(m,entry+6,0);
        }
        for(unsigned index=0;index<=9;index++)for(unsigned field=0;field<=4;field++)hn_write8(m,SPAWN_ENTRIES+index*8+field,0);
        set_flag(m,0x2A0,false);set_flag(m,0x2A1,false);hn_write16(m,0x03003DB4,0);
        if(!hn_bank3_entry_configure(m,0))return HN_SCENE_RESOURCE;break;
    case 0x0804BAE8:
        for(unsigned index=0;index<=2;index++){
            GameAddr slot=hn_render_slot(m,index+23);hn_write32(m,slot+0x2C,0);hn_write32(m,slot+0x24,0);
        }
        for(unsigned index=0;index<=9;index++){
            GameAddr slot=hn_render_slot(m,index+26);
            hn_write32(m,slot+0x24,0);hn_write32(m,slot+0x2C,0);hn_write32(m,slot+0x30,0);hn_write32(m,slot+0x38,0);
        }
        break;
    case 0x0804BB44:{
        for(unsigned index=23;index<=35;index++)hide_slot(m,(uint8_t)index);
        hide_slot(m,9);
        if(!hn_render_configure_raw(m,10,1,51,232,0,216,0x088AC0C6))return HN_SCENE_RESOURCE;
        hide_slot(m,11);
        if(!hn_render_configure_raw(m,13,1,114,272,0,216,0x088B4055)||
           !hn_render_configure_raw(m,16,1,114,232,0,184,0x088D142E)||
           !hn_render_configure_raw(m,18,1,114,32,0,96,0x088E86C5)||
           !hn_render_configure_raw(m,20,1,114,128,137,0,0x088E86BD)||
           !hn_render_configure_raw(m,21,1,114,104,8,104,0x088E86A1)||
           !hn_render_configure_raw(m,22,1,114,104,0,112,0x088E86AF))return HN_SCENE_RESOURCE;
        uint16_t score=hn_read16(m,CLASSES+225*2);uint32_t previous=hn_read16(m,CLASSES+66*2);
        if(previous<score&&score>859){hn_write16(m,CLASSES+66*2,score);set_flag(m,0x2A3,true);}
        else set_flag(m,0x2A3,false);
        break;
    }
    case 0x0804BC70:
        hn_write16(m,CLASSES+224*2,(uint16_t)(hn_read16(m,CLASSES+224*2)+1));
        if(hn_read16(m,CLASSES+224*2)>3599)set_flag(m,253,true);
        if(!flag(m,0x2A0)&&hn_read16(m,CLASSES+224*2)>79){set_flag(m,0x2A0,true);if(!hn_bank3_entry_configure(m,1))return HN_SCENE_RESOURCE;}
        if(!flag(m,0x2A1)&&hn_read16(m,CLASSES+224*2)>1799){set_flag(m,0x2A1,true);if(!hn_bank3_entry_configure(m,2))return HN_SCENE_RESOURCE;}
        store(m,0x1A0,hn_read16(m,CLASSES+224*2)>3000?2:hn_read16(m,CLASSES+224*2)>1800?1:0);
        if(!(hn_read16(m,CLASSES+224*2)%60))time_tiles(m);break;
    case 0x0804C288:if(!update_source_entries(m))return HN_SCENE_RESOURCE;break;
    case 0x0804C520:if(!update_spawn_entries(m))return HN_SCENE_RESOURCE;break;
    case 0x0804C8C8:time_tiles(m);break;
    case 0x0804C930:score_tiles(m,0x02001566,hn_read16(m,CLASSES+225*2),0x7145);break;
    case 0x0804C9B0:{uint32_t value=hn_read16(m,CLASSES+66*2);score_tiles(m,0x02001580,value?value:860,0x8145);break;}
    case 0x0804CA24:if(!record_messages(m))return HN_SCENE_RESOURCE;break;
    case 0x0804CAFC:
        if(!hn_task_remove(m,hn_read32(m,0x030037A4))||!hn_task_remove(m,hn_read32(m,0x030037A0)))return HN_SCENE_RESOURCE;break;
    case 0x0804CB18:{
        hn_write16(m,CLASSES+41*2,0);uint32_t current=hn_read16(m,CLASSES+11*2);
        uint32_t sum=(uint16_t)(current+hn_read16(m,CLASSES+6*2)),saved=sum;
        if(sum>50){
            if(current<=50){hn_write16(m,CLASSES+6*2,(uint16_t)(50-current));sum-=50;}
            else {hn_write16(m,CLASSES+6*2,0);sum=saved-hn_read16(m,CLASSES+11*2);}
            hn_write16(m,CLASSES+41*2,(uint16_t)(sum*10));
        }
        break;
    }
    case 0x0804D00C:{
        uint8_t used=0;
        for(unsigned index=0;index<4;index++){
            uint8_t choice;unsigned attempts=0;
            do {if(++attempts>4096)return HN_SCENE_RESOURCE;choice=(uint8_t)hn_random_range(m,1,8);}while((used>>choice)&1);
            store(m,index+0x1DC,choice);used|=(uint8_t)(1u<<choice);
        }
        break;
    }
    case 0x0804CCDC:if(!initialize_pair_board(m))return HN_SCENE_RESOURCE;break;
    case 0x0804CDBC:if(!select_pair_position(m))return HN_SCENE_RESOURCE;break;
    case 0x0804CE3C:{
        uint32_t index=control(m,hn_read8(m,arguments)?0x1B2:0x1B1);
        store(m,0x60,control(m,index+0x1A5));*consumed=1;break;
    }
    case 0x0804CE80:{
        uint32_t first_index=control(m,0x1B1),second_index=control(m,0x1B2);
        uint32_t first=control(m,first_index+0x1A5),second=control(m,second_index+0x1A5),result;
        if(first==second||first==5){
            result=1;store(m,7,control(m,7)+1);if(first==5)store(m,0x1E4,second);
        }else result=2;
        store(m,0x60,result);break;
    }
    case 0x0804CEDC:{
        uint32_t kind=control(m,control(m,0x1B1)+0x1A5),bits=register_bit(kind-1)|control(m,0x1A5);
        store(m,0x1A5,bits);
        if(kind==5){bits|=register_bit((uint32_t)control(m,control(m,0x1B2)+0x1A5)-1);store(m,0x1A5,bits);}
        uint32_t first=control(m,0x1B1),second=control(m,0x1B2);
        store(m,first+0x1A5,255);store(m,second+0x1A5,255);
        unsigned removed=0;for(unsigned index=0;index<=8;index++)if(control(m,index+0x1A6)==255)removed++;
        store(m,0x60,removed>7);break;
    }
    case 0x0804CF88:store(m,0x1AF,control(m,0x1AF)+1);store(m,0x60,control(m,0x1AF)>3);break;
    case 0x0804CFB8:{
        uint32_t available=control(m,0x1A5)&15;unsigned index;
        for(index=0;index<=3;index++)if((available>>index)&1){store(m,0x1A5,control(m,0x1A5)&~(1u<<index));break;}
        index=(uint8_t)(index+1);store(m,0x60,index>4?0:index);break;
    }
    case 0x0804D064:{
        uint32_t argument=hn_read8(m,arguments),choice=control(m,argument+0x1DC),offset=choice*2;
        store(m,9,hn_read8(m,0x08630E44+offset));store(m,0,hn_read8(m,0x08630E45+offset));
        store(m,0x60,choice);*consumed=1;break;
    }
    case 0x0804F5B0:{
        uint8_t index=hn_read8(m,arguments);GameAddr base=hn_read32(m,HN_RENDER_SLOTS);
        GameAddr first=base+index*104,second=base+hn_read8(m,0x03002C04)*104;
        *consumed=(int32_t)hn_read32(m,first+0x18)<(int32_t)hn_read32(m,second+0x18)?6:1;break;
    }
    case 0x080502E0:if(!initialize_phases(m))return HN_SCENE_RESOURCE;break;
    case 0x08050590:if(!advance_phase(m))return HN_SCENE_RESOURCE;break;
    case 0x08050ABC:if(!shuffle_orbit(m))return HN_SCENE_RESOURCE;break;
    case 0x08050C24:{
        GameAddr slot=hn_render_slot(m,22);unsigned index;
        for(index=22;index<=31;index++,slot+=104)
            if((int32_t)(hn_read32(m,slot+0x1C)+hn_read32(m,slot+0x20))>-0x180000)break;
        if(index>31){
            set_flag(m,0x2B4,false);
            for(index=22;index<=31;index++)hn_write8(m,hn_render_slot(m,index)+0x60,0);
        }
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank3_games_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x0804B634:case 0x0804D00C:case 0x080502E0:case 0x08050590:case 0x08050ABC:case 0x08050C24:
    case 0x0804CCDC:case 0x0804CDBC:case 0x0804CE80:case 0x0804CEDC:case 0x0804CF88:case 0x0804CFB8:
    case 0x0804C8C8:case 0x0804C930:case 0x0804C9B0:case 0x0804CA24:case 0x0804CAFC:case 0x0804CB18:
    case 0x0804B860:case 0x0804BA24:case 0x0804BAE8:case 0x0804BB44:case 0x0804BC70:case 0x0804C288:case 0x0804C520:{
        uint32_t ignored;return hn_bank3_games_callback(m,callback,0,&ignored);
    }
    default:return HN_SCENE_UNKNOWN;
    }
}
HNSceneResult hn_bank3_games_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,
                                      uint32_t *consumed,uint32_t *yield)
{
    switch(callback&~1u){
    case 0x0804FAF0:case 0x0804FB48:{
        if(flag(m,0x2CE)){if(!hn_audio_song(m,0x37,HN_SONG_STOP))return HN_SCENE_RESOURCE;}
        else if((callback&~1u)==0x0804FAF0){
            if(!control(m,0x1DB)&&!hn_audio_song(m,0x37,HN_SONG_START))return HN_SCENE_RESOURCE;
            store(m,0x1DB,control(m,0x1DB)+6);
            hn_audio_track_parameter(m,0x03004E90,255,control(m,0x1DB)*4,0);
        }else{
            uint32_t next=(uint32_t)control(m,0x1DB)-6;store(m,0x1DB,next);
            if((uint8_t)next)hn_audio_track_parameter(m,0x03004E90,255,control(m,0x1DB)*4,0);
            else if(!hn_audio_song(m,0x37,HN_SONG_STOP))return HN_SCENE_RESOURCE;
        }
        *yield=1;*consumed=0;break;
    }
    case 0x080509A8:{
        uint32_t index=hn_read8(m,arguments);
        if(!control(m,0x1D0)){
            GameAddr packed=CLASSES+(index+0xE3)*2;uint32_t value=hn_read16(m,packed);
            if(!(value&7)){
                uint8_t angle=(uint8_t)(value>>3);
                hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+(sine(m,angle+0x40)<<10));
                hn_write32(m,slot+0x1C,hn_read32(m,slot+0x1C)+(sine(m,angle)<<10));
                if(angle>174){hn_write16(m,packed,648);hn_write32(m,slot+0x18,0x013A8000);hn_write32(m,slot+0x1C,0x8000);hn_write32(m,slot+0x20,0x98000);}
                if(angle==128){store(m,0x1D1,index+1);if(!hn_background_configure_raw(m,8,129,0x088EA7BB))return HN_SCENE_RESOURCE;}
                if(angle==129)store(m,0x1D1,0);
            }
        }
        *yield=1;*consumed=1;break;
    }
    case 0x08050A78:{
        uint32_t index=hn_read8(m,arguments);
        if(!control(m,0x1D0)){
            GameAddr packed=CLASSES+(index+0xE3)*2;uint32_t next=hn_read16(m,packed)+1;
            hn_write16(m,packed,(uint16_t)next);if((next&7)==5)hn_write16(m,packed,(uint16_t)(next|7));
        }
        *yield=0;*consumed=1;break;
    }
    case 0x08050B94:{
        uint8_t index=hn_read8(m,arguments);uint32_t value=hn_read16(m,CLASSES+0xE3*2);
        if(!(value&7)){
            uint8_t angle=(uint8_t)((uint8_t)(value>>3)+index*42);
            hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)+(sine(m,angle+0x40)<<8));
            hn_write32(m,slot+0x1C,hn_read32(m,slot+0x1C)+(sine(m,angle)<<8));
            if(!angle&&!hn_background_configure_raw(m,8,129,0x088EAB9E))return HN_SCENE_RESOURCE;
        }
        *yield=1;*consumed=1;break;
    }
    case 0x08050C08:hn_write16(m,CLASSES+0xE3*2,(uint16_t)(hn_read16(m,CLASSES+0xE3*2)+1));*yield=0;*consumed=0;break;
    case 0x08050D40:{
        uint32_t timer=hn_read16(m,CLASSES+0x57*2);hn_write16(m,CLASSES+0x57*2,(uint16_t)(timer+65535));
        hn_write32(m,hn_render_slot(m,22)+0x18,timer*20000+0xA38000);
        uint32_t root=hn_bios_sqrt(timer);GameAddr target=hn_render_slot(m,22);
        uint32_t wave=(sine(m,(timer*2)&255)<<10)+0x8B8000;hn_write32(m,target+0x20,root+wave);
        if(!timer){if(!animation(m,22,0x088E5291))return HN_SCENE_RESOURCE;hn_write8(m,slot+0x60,0);}
        *yield=1;*consumed=0;break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
