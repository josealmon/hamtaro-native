#include "gba/memory.h"
#include "gba/timing.h"
#include "gba/audio.h"
#include "game/script.h"
#include "game/render.h"
#include "game/background.h"
#include "game/graphics.h"
#include "game/audio.h"
#include "game/actor.h"
#include "game/task.h"
#include <stdlib.h>
#include <string.h>
/* Optional private oracle bridge. Never installed or shipped with the app. */
HNMemory *hn_probe_create(const uint8_t *rom,size_t size)
{
    HNMemory *m=malloc(sizeof(*m));
    if(m)hn_memory_init(m,rom,size);
    return m;
}
void hn_probe_destroy(HNMemory *m){free(m);}
/* Raw fixture injection, including read-only IO status; test bridge only. */
int hn_probe_seed(HNMemory *m,GameAddr address,const uint8_t *data,size_t count)
{
    uint8_t *target=NULL;size_t capacity=0;GameAddr base=address&0xFF000000u;
    switch(base){
    case 0x02000000:target=m->ewram;capacity=sizeof(m->ewram);break;
    case 0x03000000:target=m->iwram;capacity=sizeof(m->iwram);break;
    case 0x04000000:target=m->io;capacity=sizeof(m->io);break;
    case 0x05000000:target=m->palette;capacity=sizeof(m->palette);break;
    case 0x06000000:target=m->vram;capacity=sizeof(m->vram);break;
    case 0x07000000:target=m->oam;capacity=sizeof(m->oam);break;
    case 0x0E000000:target=m->sram;capacity=sizeof(m->sram);break;
    default:return 0;
    }
    size_t offset=address-base;
    if(offset>capacity||count>capacity-offset)return 0;
    memcpy(target+offset,data,count);return 1;
}
int hn_probe_read(HNMemory *m,GameAddr address,uint8_t *out,size_t count)
{
    for(size_t i=0;i<count && !m->fault;i++)out[i]=hn_read8(m,address+(GameAddr)i);
    return !m->fault;
}
int hn_probe_command(HNMemory *m,unsigned opcode,GameAddr arguments)
{
    HNScript script;hn_script_init(&script,m,NULL,0);
    return hn_main_command(&script,opcode,arguments);
}
int hn_probe_main_frame(HNMemory *m,unsigned budget,uint32_t report[3])
{
    HNScript script;hn_script_init(&script,m,NULL,0);
    bool ok=hn_main_frame(&script,budget);
    report[0]=script.fault;report[1]=script.fault_opcode;report[2]=script.fault_address;
    return ok;
}
int hn_probe_actor_frames(HNMemory *m,unsigned budget,uint32_t report[3])
{
    HNScript script;hn_script_init(&script,m,NULL,0);bool ok=hn_actor_frames(&script,budget);
    report[0]=script.fault;report[1]=script.fault_opcode;report[2]=script.fault_address;return ok;
}
int hn_probe_tasks(HNMemory *m,unsigned budget,uint32_t report[3])
{
    HNTasks tasks;hn_tasks_init(&tasks,m);bool ok=hn_tasks_update(&tasks,budget);
    report[0]=tasks.fault;report[1]=tasks.fault_opcode;report[2]=tasks.fault_address;return ok;
}
int hn_probe_task_command(HNMemory *m,GameAddr task,unsigned opcode,uint32_t report[4])
{
    HNTasks tasks;hn_tasks_init(&tasks,m);uint32_t keep_going=0;bool ok=hn_task_command(&tasks,task,opcode,&keep_going);
    report[0]=tasks.fault;report[1]=tasks.fault_opcode;report[2]=tasks.fault_address;report[3]=keep_going;return ok;
}
int hn_probe_animation(HNMemory *m,GameAddr slot,unsigned op,GameAddr cursor_field,uint32_t report[4])
{
    HNRender render;hn_render_init(&render,m,NULL,0);GameAddr cursor=hn_read32(m,cursor_field);
    uint32_t yield=0;bool ok=hn_animation_command(&render,slot,op,&cursor,&yield);
    hn_write32(m,cursor_field,cursor);
    report[0]=render.fault;report[1]=render.fault_opcode;report[2]=render.fault_address;report[3]=yield;
    return ok;
}
int hn_probe_render_animation(HNMemory *m,GameAddr slot,unsigned budget,uint32_t report[3])
{
    HNRender render;hn_render_init(&render,m,NULL,0);bool ok=hn_render_animation(&render,slot,budget);
    report[0]=render.fault;report[1]=render.fault_opcode;report[2]=render.fault_address;return ok;
}
void hn_probe_background_offset(HNMemory *m,unsigned layer,int32_t x,int32_t y,GameAddr output)
{
    uint32_t ox,oy;hn_background_offset(m,(uint8_t)layer,x,y,&ox,&oy);
    hn_write32(m,output,ox);hn_write32(m,output+4,oy);
}
int hn_probe_render(HNMemory *m,unsigned budget,unsigned update,uint32_t report[3])
{
    HNRender render;hn_render_init(&render,m,NULL,0);
    bool ok=update?hn_render_update(&render,budget):hn_render_draw(&render);
    report[0]=render.fault;report[1]=render.fault_opcode;report[2]=render.fault_address;return ok;
}
int hn_probe_background_command(HNMemory *m,GameAddr state,unsigned op,GameAddr cursor_field,uint32_t report[4])
{
    HNBackground background;hn_background_init(&background,m);GameAddr cursor=hn_read32(m,cursor_field);
    uint32_t yield=0;bool ok=hn_background_command(&background,state,op,&cursor,&yield);
    hn_write32(m,cursor_field,cursor);
    report[0]=background.fault;report[1]=background.fault_opcode;report[2]=background.fault_address;report[3]=yield;
    return ok;
}
int hn_probe_background(HNMemory *m,unsigned budget,uint32_t report[3])
{
    HNBackground background;hn_background_init(&background,m);bool ok=hn_background_update(&background,budget);
    report[0]=background.fault;report[1]=background.fault_opcode;report[2]=background.fault_address;return ok;
}
int hn_probe_graphics_command(HNMemory *m,GameAddr state,unsigned op,GameAddr arguments,uint32_t report[4])
{
    HNGraphics graphics;hn_graphics_init(&graphics,m);uint32_t running=0;
    bool ok=hn_graphics_command(&graphics,state,op,arguments,&running);
    report[0]=graphics.fault;report[1]=graphics.fault_opcode;report[2]=graphics.fault_address;report[3]=running;return ok;
}
int hn_probe_graphics(HNMemory *m,unsigned budget,uint32_t report[3])
{
    HNGraphics graphics;hn_graphics_init(&graphics,m);bool ok=hn_graphics_update(&graphics,budget);
    report[0]=graphics.fault;report[1]=graphics.fault_opcode;report[2]=graphics.fault_address;return ok;
}
int hn_probe_audio_command(HNMemory *m,GameAddr player,GameAddr track,unsigned opcode,uint32_t report[3])
{
    HNAudio audio;hn_audio_init(&audio,m);bool ok=hn_audio_sequence_command(&audio,player,track,opcode);
    report[0]=audio.fault;report[1]=audio.fault_opcode;report[2]=audio.fault_address;return ok;
}
int hn_probe_audio_note(HNMemory *m,GameAddr player,GameAddr track,unsigned clock_selector,uint32_t report[3])
{
    HNAudio audio;hn_audio_init(&audio,m);bool ok=hn_audio_note(&audio,player,track,clock_selector);
    report[0]=audio.fault;report[1]=audio.fault_opcode;report[2]=audio.fault_address;return ok;
}
int hn_probe_audio_player(HNMemory *m,GameAddr player,unsigned command_budget,uint32_t report[3])
{
    HNAudio audio;hn_audio_init(&audio,m);bool ok=hn_audio_player_update(&audio,player,command_budget);
    report[0]=audio.fault;report[1]=audio.fault_opcode;report[2]=audio.fault_address;return ok;
}
int hn_probe_audio_mix(HNMemory *m,unsigned command_budget,uint32_t report[3])
{
    HNAudio audio;hn_audio_init(&audio,m);bool ok=hn_audio_mix(&audio,command_budget);
    report[0]=audio.fault;report[1]=audio.fault_opcode;report[2]=audio.fault_address;return ok;
}
int hn_probe_audio_psg(HNMemory *m,uint32_t report[3])
{
    HNAudio audio;hn_audio_init(&audio,m);bool ok=hn_audio_psg_update(&audio);
    report[0]=audio.fault;report[1]=audio.fault_opcode;report[2]=audio.fault_address;return ok;
}
typedef struct { HNMemory memory;HNClock clock;HNApu apu; } HNHardwareProbe;
HNHardwareProbe *hn_probe_hardware_create(void)
{
    HNHardwareProbe *p=calloc(1,sizeof(*p));if(!p)return NULL;
    hn_memory_init(&p->memory,NULL,0);hn_clock_init(&p->clock,&p->memory,NULL,NULL,NULL);
    hn_apu_init(&p->apu,&p->memory,NULL,NULL);return p;
}
void hn_probe_hardware_destroy(HNHardwareProbe *p){free(p);}
int hn_probe_hardware_write(HNHardwareProbe *p,unsigned width,GameAddr address,uint32_t value)
{
    if(width==1)hn_write8(&p->memory,address,(uint8_t)value);
    else if(width==2)hn_write16(&p->memory,address,(uint16_t)value);
    else if(width==4)hn_write32(&p->memory,address,value);else return 0;
    return !p->memory.fault;
}
int hn_probe_hardware_advance(HNHardwareProbe *p,uint32_t cycles)
{hn_clock_advance(&p->clock,&p->memory,cycles);return !p->memory.fault;}
void hn_probe_hardware_state(HNHardwareProbe *p,uint32_t state[47])
{
    HNMemory *m=&p->memory;HNApu *a=&p->apu;state[0]=(uint32_t)p->clock.cycles;
    for(unsigned i=0;i<4;i++){
        state[1+i]=hn_read16(m,0x04000100+i*4);state[5+i]=p->clock.timers.channel[i].reload;
        state[9+i]=hn_read16(m,0x04000102+i*4);
    }
    state[13]=hn_read16(m,0x04000202);state[14]=hn_read16(m,0x04000082);
    state[15]=hn_read16(m,0x04000084);state[16]=hn_read16(m,0x04000088);
    for(unsigned i=0;i<2;i++){
        HNAudioFifo *f=&a->fifo[i];unsigned o=17+i*4;
        state[o]=f->read;state[o+1]=f->write;state[o+2]=f->remaining;state[o+3]=f->shifter;
        for(unsigned j=0;j<8;j++)state[25+i*8+j]=f->words[j];
        state[41+i]=m->dma_source[i+1];state[43+i]=(uint32_t)(int32_t)f->sample;
        state[45+i]=(uint32_t)(int32_t)(a->dac[i]/64);
    }
}
void hn_probe_hardware_psg_state(HNHardwareProbe *p,uint32_t state[96])
{
    HNPsg *s=&p->apu.psg;state[0]=(uint32_t)p->clock.cycles;state[1]=hn_psg_status(s);
    state[2]=hn_read16(&p->memory,0x04000080);state[3]=s->frame_step;
    for(unsigned i=0;i<4;i++){
        state[4+i]=s->channel[i].length;state[8+i]=i==3?s->registers[0x1C]:s->channel[i].frequency;
        state[12+i]=i==2?s->registers[0x13]>>5:s->channel[i].volume;
        state[16+i]=hn_psg_sample(s,i);state[20+i]=s->channel[i].active;
    }
    state[24]=s->channel[0].phase;state[25]=s->channel[1].phase;
    state[26]=s->sweep_shadow;state[27]=s->sweep_clock;
    for(unsigned i=0;i<32;i++)state[28+i]=s->wave[i];
    for(unsigned i=0;i<34;i++)state[60+i]=hn_psg_read(s,0x60+i);
    int level[2];hn_psg_levels(s,level);state[94]=(uint32_t)level[0];state[95]=(uint32_t)level[1];
}
