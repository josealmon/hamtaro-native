#include "game/actor.h"
#include "game/state.h"

static bool context_transfer(HNMemory *m,GameAddr context,bool save)
{
    /* Explicit 84-byte guest layout; the final three padding bytes stay untouched. */
    static const struct { unsigned offset,bytes;GameAddr global; } fields[]={
        {0x4C,1,0x03000010},{0,4,0x03002C60},{0x4D,1,0x03002BF0},
        {4,64,0x03002C10},{0x44,4,0x03000014},{0x4E,1,0x03000018},
        {0x48,2,0x0300001A},{0x4F,1,0x0300001C},{0x50,1,0x0300001D},{0x4A,2,0x03001FBC}
    };
    for(unsigned i=0;i<sizeof(fields)/sizeof(*fields);i++){
        GameAddr source=save?fields[i].global:context+fields[i].offset;
        GameAddr destination=save?context+fields[i].offset:fields[i].global;
        if(!hn_memory_copy(m,destination,source,fields[i].bytes))return false;
    }
    return true;
}
bool hn_actor_configure(HNMemory *m,uint8_t index,GameAddr script)
{
    if(index>16)return false;if(!index)return !m->fault;
    GameAddr context=HN_ACTOR_CONTEXTS+index*84;
    hn_write32(m,context,script==UINT32_MAX?0:script);
    for(unsigned i=4;i<0x4C;i++)hn_write8(m,context+i,0);
    for(unsigned i=0x4C;i<=0x50;i++)hn_write8(m,context+i,0);
    return !m->fault;
}
bool hn_actor_frames(HNScript *script,unsigned command_budget)
{
    HNMemory *m=script->memory;
    for(unsigned index=1;index<=16;index++){
        GameAddr actor=HN_ACTOR_CONTEXTS+index*84;
        if(!hn_read32(m,actor))continue;
        if(!context_transfer(m,HN_ACTOR_CONTEXTS,true)||!context_transfer(m,actor,false))goto failure;
        bool ok=hn_main_frame(script,command_budget);
        bool stored=context_transfer(m,actor,true),restored=context_transfer(m,HN_ACTOR_CONTEXTS,false);
        if(!ok||!stored||!restored)goto failure;
    }
    if(!m->fault)return true;
failure:
    if(m->fault&&script->fault==HN_SCRIPT_OK){script->fault=HN_SCRIPT_MEMORY;script->fault_address=m->fault_address;}
    return false;
}
