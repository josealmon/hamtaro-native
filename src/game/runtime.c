#include "game/runtime.h"
#include "game/world.h"
#include "game/state.h"
#include "game/heap.h"
#include "game/save.h"
#include "game/input.h"
#include "game/actor.h"
#include "game/motion.h"
#include "game/background.h"
#include "game/video.h"
#include "game/transfer.h"
#include "game/raster.h"
#include "game/random.h"
#include "game/bank5.h"
#include <string.h>

static bool fail(HNGame *g,HNGameFault fault,unsigned detail,unsigned opcode,GameAddr address)
{
    if(g->fault)return false;
    if(g->memory.fault){fault=HN_GAME_MEMORY;address=g->memory.fault_address;}
    g->fault=fault;g->detail=detail;g->opcode=opcode;g->address=address;return false;
}
bool hn_game_hardware_initialize(HNMemory *m)
{
    hn_write16(m,0x04000204,0x45B6);hn_write16(m,0x04000008,8);
    hn_write32(m,0x040000D4,0x080000FC);hn_write32(m,0x040000D8,0x03002280);hn_write32(m,0x040000DC,0x84000200);
    hn_write32(m,0x03007FFC,0x03002280);hn_write16(m,0x04000200,0x2003);hn_write16(m,0x04000004,8);
    hn_video_state_init(m);hn_write8(m,0x03002E30,0);
    hn_write32(m,0x03000000,0x3AC2);hn_write32(m,0x03000004,0x4DF3);hn_write32(m,0x03000008,(hn_read16(m,0x04000006)&255)+1);
    const uint16_t patterns[]={0x1111,0x2222,0x4444,0x8888,0x0101,0x1010,0x5555,0xAAAA,
                               0x1100,0x0011,0x55AA,0xAA55,0x5A5A,0xA5A5,0x0123,0x4567};
    for(unsigned i=0;i<16;i++)hn_write16(m,0x030051B8+i*2,patterns[i]);return !m->fault;
}
static void audio_sample(void *context,int16_t left,int16_t right)
{
    HNGame *g=context;if(g->fault)return;
    if(g->sample_frames>=1024){fail(g,HN_GAME_AUDIO_BUFFER,0,0,0);return;}
    g->samples[g->sample_frames*2]=left;g->samples[g->sample_frames*2+1]=right;g->sample_frames++;
}
static void interrupt(void *context,uint16_t pending)
{
    HNGame *g=context;HNMemory *m=&g->memory;if(g->fault)return;
    unsigned master=hn_read16(m,0x04000208);hn_write16(m,0x04000208,0);
    if(pending&1){
        hn_write16(m,0x04000202,1);hn_audio_dma_vblank(m);
        if(!hn_read32(m,0x03001F98)&&(hn_read32(m,0x03002C5C)||(hn_read16(m,0x04000130)&15)==0))g->reset_requested=true;
        else{
            if(hn_read32(m,0x03001F5C)){
                hn_write32(m,0x040000D4,HN_OAM_SHADOW);hn_write32(m,0x040000D8,0x07000000);hn_write32(m,0x040000DC,0x84000100);
                hn_video_commit(m);
                GameAddr callback=hn_read32(m,0x03002B90);
                if(callback&&hn_bank5_irq(m,callback)!=HN_SCENE_OK)fail(g,HN_GAME_IRQ,0,1,callback);
                if(hn_read8(m,HN_CPU_TRANSFER_READ)!=hn_read8(m,HN_CPU_TRANSFER_WRITE))fail(g,HN_GAME_CPU_TRANSFER,0,0,HN_TRANSFER_B);
                if(!g->fault)hn_transfer_flush(m);
            }
            if(!g->fault&&!hn_audio_mix(&g->audio,4096))fail(g,HN_GAME_AUDIO,g->audio.fault,g->audio.fault_opcode,g->audio.fault_address);
            hn_write16(m,0x03007FF8,hn_read16(m,0x03007FF8)|1);
        }
    }
    /* The ARM dispatcher prioritizes VBlank, HBlank, then VCOUNT. A preceding
       handler may mask an IRQ or acknowledge its pending bit. */
    pending=hn_read16(m,0x04000200)&hn_read16(m,0x04000202);
    if((pending&2)&&!g->fault){
        hn_write16(m,0x04000202,2);
        if(!hn_raster_hblank(m))fail(g,HN_GAME_IRQ,0,2,hn_read8(m,HN_HBLANK_SELECTOR));
    }
    pending=hn_read16(m,0x04000200)&hn_read16(m,0x04000202);
    if((pending&4)&&!g->fault){
        hn_write16(m,0x04000202,4);
        if(!hn_raster_vcount(m))fail(g,HN_GAME_IRQ,0,4,hn_read8(m,HN_VCOUNT_SELECTOR));
    }
    pending=hn_read16(m,0x04000200)&hn_read16(m,0x04000202);
    if((pending&~7u)&&!g->fault)fail(g,HN_GAME_IRQ,0,pending&~7u,0x04000202);
    hn_write16(m,0x04000208,(uint16_t)master);
    if(m->fault)fail(g,HN_GAME_MEMORY,0,0,m->fault_address);
}
bool hn_game_initialize(HNGame *g,const uint8_t *rom,size_t size,const uint8_t *sram)
{
    memset(g,0,sizeof(*g));hn_memory_init(&g->memory,rom,size);HNMemory *m=&g->memory;
    if(!rom||size!=0x1000000)return fail(g,HN_GAME_INITIALIZATION,0,0,HN_ROM_BASE);
    if(sram)memcpy(m->sram,sram,HN_SRAM_SIZE);
    hn_clock_init(&g->clock,m,g->pixels,interrupt,g);hn_apu_init(&g->apu,m,audio_sample,g);
    hn_script_init(&g->script,m,NULL,0);hn_render_init(&g->render,m,NULL,0);
    hn_background_init(&g->background,m);
    hn_graphics_init(&g->graphics,m);hn_tasks_init(&g->tasks,m);hn_audio_init(&g->audio,m);
    hn_write16(m,0x04000208,0);
    if(!hn_runtime_tables_init(m)||!hn_game_hardware_initialize(m)||!hn_audio_driver_init(m))return fail(g,HN_GAME_INITIALIZATION,0,0,HN_AUDIO_CORE_POINTER);
    hn_write16(m,0x04000208,1);hn_transfer_reset(m);hn_input_initialize(m);hn_heap_init(m);
    if(!hn_sram_init(m))return fail(g,HN_GAME_INITIALIZATION,0,0,0x0E000000);
    hn_mosaic_init(m);hn_task_contexts_init(m);
    if(!hn_core_initialize(m))return fail(g,HN_GAME_INITIALIZATION,0,0,HN_MAP_BANKS);
    g->initialized=true;return !m->fault;
}
static bool core_update(HNGame *g)
{
    HNMemory *m=&g->memory;
    if(!hn_main_frame(&g->script,4096))return fail(g,HN_GAME_SCRIPT,g->script.fault,g->script.fault_opcode,g->script.fault_address);
    if(!hn_motion_update_following(m))return fail(g,HN_GAME_MOVEMENT,0,0,HN_ROUTE_POINTS);
    if(!hn_actor_frames(&g->script,4096))return fail(g,HN_GAME_ACTOR,g->script.fault,g->script.fault_opcode,g->script.fault_address);
    if(!hn_tasks_update(&g->tasks,4096))return fail(g,HN_GAME_TASK,g->tasks.fault,g->tasks.fault_opcode,g->tasks.fault_address);
    if(hn_read8(m,0x03002180)&32){
        hn_write8(m,0x03002BF4,1);hn_audio_dma_pause(m);
        if(!hn_transfer_fill_words(m,0x02004720,0,0x2000))return fail(g,HN_GAME_MEMORY,0,0,0x02004720);
        hn_core_reset(m);if(!hn_save_initialize(m))return fail(g,HN_GAME_INITIALIZATION,0,0,0x0E000000);
        hn_write8(m,0x03002180,hn_read8(m,0x03002180)&~32);hn_audio_dma_resume(m);
    }
    return true;
}
bool hn_game_frame(HNGame *g,uint16_t buttons)
{
    if(!g->initialized||g->fault)return false;HNMemory *m=&g->memory;g->sample_frames=0;
    m->io[0x130]=(uint8_t)~buttons;m->io[0x131]=(uint8_t)((~buttons>>8)&3);
    uint32_t cycles=g->frames?HN_CYCLES_PER_FRAME:HN_HEIGHT*HN_CYCLES_PER_LINE;
    hn_clock_advance(&g->clock,m,cycles);if(g->fault)return false;
    if(g->reset_requested){
        uint8_t save[HN_SRAM_SIZE];memcpy(save,m->sram,sizeof(save));const uint8_t *rom=m->rom;size_t size=m->rom_size;
        return hn_game_initialize(g,rom,size,save);
    }
    hn_write32(m,0x03001F5C,0);
    if(!hn_input_update(m,buttons))return fail(g,HN_GAME_MEMORY,0,0,HN_KEY_STATE);
    if(hn_read8(m,0x03002E30))return fail(g,HN_GAME_COMMUNICATION,0,0,0x03002E30);
    if(!core_update(g))return false;
    if(!hn_background_update(&g->background,4096))return fail(g,HN_GAME_BACKGROUND,g->background.fault,g->background.fault_opcode,g->background.fault_address);
    if(!hn_render_update(&g->render,4096))return fail(g,HN_GAME_RENDER,g->render.fault,g->render.fault_opcode,g->render.fault_address);
    if(!hn_graphics_update(&g->graphics,4096))return fail(g,HN_GAME_GRAPHICS,g->graphics.fault,g->graphics.fault_opcode,g->graphics.fault_address);
    hn_mosaic_update(m);hn_random_memory_next(m);hn_write32(m,0x03001F5C,1);
    if(hn_read8(m,0x03002180)&16)hn_write32(m,0x03002C5C,1);
    if(m->fault)return fail(g,HN_GAME_MEMORY,0,0,m->fault_address);
    g->frames++;return true;
}
