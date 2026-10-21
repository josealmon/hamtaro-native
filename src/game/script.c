#include "game/script.h"
#include <string.h>
enum {
    CURSOR=0x03002C60, CONSUMED=0x03000014, OPCODE=0x03000010, INITIALIZED=0x03000018,
    WAIT=0x0300001A, RUNNING=0x0300001C, YIELDED=0x0300001D, SCRATCH=0x030005E8,
    VARIABLES=0x03002BB0, DEPTH=0x03002BF0, RETURNS=0x03002C10, RESULT=0x03001FBC,
    CONTROLS=0x02004520, CLASSES=0x0201DC40, PRESSED_KEYS=0x03000822
};
void hn_script_init(HNScript *s,HNMemory *m,const HNMainCallback *callbacks,size_t count)
{
    memset(s,0,sizeof(*s));s->memory=m;s->callbacks=callbacks;s->callback_count=count;
}
static bool fail(HNScript *s,HNScriptFault fault,unsigned op,GameAddr address)
{ s->fault=fault;s->fault_opcode=op;s->fault_address=address;return false; }
static void scratch(HNMemory *m,GameAddr args,unsigned offset,unsigned bytes)
{for(unsigned i=0;i<bytes;i++)hn_write8(m,SCRATCH+offset+i,hn_read8(m,args+i));}
static uint32_t resolve_variable(HNMemory *m,unsigned offset)
{
    uint32_t value=hn_read32(m,SCRATCH+offset);
    if(value<=15){value=hn_read32(m,VARIABLES+value*4);hn_write32(m,SCRATCH+offset,value);}
    return value;
}
static void advance(HNMemory *m,unsigned bytes)
{hn_write32(m,CONSUMED,bytes);hn_write8(m,OPCODE,0);}
bool hn_main_command(HNScript *s,unsigned op,GameAddr args)
{
    if(s->fault)return false;
    HNMemory *m=s->memory;
    switch(op) {
    case 0: {
        GameAddr cursor=hn_read32(m,CURSOR)+hn_read32(m,CONSUMED);
        hn_write8(m,OPCODE,hn_read8(m,cursor));
        hn_write8(m,INITIALIZED,0);hn_write32(m,CONSUMED,0);hn_write32(m,CURSOR,cursor+1);
        break;
    }
    case 1:hn_write8(m,RUNNING,0);hn_write8(m,YIELDED,1);break;
    case 2:
        if(!hn_read8(m,INITIALIZED)){
            hn_write16(m,WAIT,hn_read16(m,args));hn_write8(m,INITIALIZED,1);hn_write32(m,CONSUMED,2);
        }
        hn_write16(m,WAIT,(uint16_t)(hn_read16(m,WAIT)-1));
        if(!hn_read16(m,WAIT))hn_write8(m,OPCODE,0);
        hn_write8(m,YIELDED,1);break;
    case 3:
        if(!hn_read8(m,INITIALIZED)){
            hn_write16(m,WAIT,hn_read16(m,args));hn_write8(m,INITIALIZED,1);
        }
        if(hn_read16(m,PRESSED_KEYS)&1)hn_write8(m,OPCODE,0);
        hn_write8(m,YIELDED,1);break;
    case 4:hn_write8(m,RUNNING,1);hn_write8(m,OPCODE,0);break;
    case 5:hn_write8(m,RUNNING,0);hn_write8(m,YIELDED,1);hn_write8(m,OPCODE,0);break;
    case 0x0C:
        scratch(m,args,0,4);hn_write32(m,CURSOR,hn_read32(m,SCRATCH));advance(m,0);break;
    case 0x1C: {
        scratch(m,args,0,4);GameAddr address=hn_read32(m,SCRATCH);
        HNMainFunction function=NULL;
        for(size_t i=0;i<s->callback_count;i++)if((s->callbacks[i].address&~1u)==(address&~1u)){
            function=s->callbacks[i].function;break;
        }
        if(!function)return fail(s,HN_SCRIPT_CALLBACK,op,address);
        uint32_t consumed=function(m,args+4);advance(m,consumed+4);break;
    }
    case 0x1E: {
        scratch(m,args,0,4);GameAddr target=resolve_variable(m,0);
        unsigned depth=hn_read8(m,DEPTH);
        if(depth>=16)return fail(s,HN_SCRIPT_STACK,op,DEPTH);
        hn_write8(m,DEPTH,(uint8_t)(depth+1));hn_write32(m,RETURNS+depth*4,hn_read32(m,CURSOR));
        hn_write32(m,CURSOR,target);advance(m,0);break;
    }
    case 0x1F: {
        unsigned depth=hn_read8(m,DEPTH);
        if(!depth||depth>16)return fail(s,HN_SCRIPT_STACK,op,DEPTH);
        depth--;hn_write8(m,DEPTH,(uint8_t)depth);
        hn_write32(m,CURSOR,hn_read32(m,RETURNS+depth*4));advance(m,4);break;
    }
    case 0x27:
        hn_write8(m,RUNNING,0);hn_write32(m,CURSOR,0);hn_write8(m,OPCODE,1);hn_write8(m,YIELDED,1);break;
    case 0x28:
        scratch(m,args,0,4);hn_write16(m,RESULT,hn_read8(m,resolve_variable(m,0)));advance(m,4);break;
    case 0x29:
        scratch(m,args,0,2);hn_write16(m,RESULT,hn_read8(m,CONTROLS+hn_read16(m,SCRATCH)));advance(m,2);break;
    case 0x2A:
        scratch(m,args,0,2);hn_write16(m,RESULT,hn_read16(m,CLASSES+hn_read16(m,SCRATCH)*2));advance(m,2);break;
    case 0x2B:
        scratch(m,args,0,2);hn_write16(m,RESULT,hn_read16(m,SCRATCH));advance(m,2);break;
    case 0x2D:
        scratch(m,args,0,3);hn_write8(m,CONTROLS+hn_read16(m,SCRATCH),hn_read8(m,SCRATCH+2));advance(m,3);break;
    case 0x2E:
        scratch(m,args,0,4);hn_write16(m,CLASSES+hn_read16(m,SCRATCH)*2,hn_read16(m,SCRATCH+2));advance(m,4);break;
    case 0x53:
        scratch(m,args,2,6);hn_write32(m,VARIABLES+hn_read16(m,SCRATCH+2)*4,hn_read32(m,SCRATCH+4));advance(m,6);break;
    default:return fail(s,HN_SCRIPT_OPCODE,op,args);
    }
    if(m->fault)return fail(s,HN_SCRIPT_MEMORY,op,m->fault_address);
    return true;
}
bool hn_main_frame(HNScript *s,unsigned budget)
{
    HNMemory *m=s->memory;hn_write8(m,YIELDED,0);
    for(unsigned i=0;i<budget;i++){
        unsigned op=hn_read8(m,OPCODE);GameAddr cursor=hn_read32(m,CURSOR);
        if(!hn_main_command(s,op,cursor))return false;
        if(hn_read8(m,YIELDED)&&!hn_read8(m,RUNNING))return true;
    }
    return fail(s,HN_SCRIPT_BUDGET,hn_read8(m,OPCODE),hn_read32(m,CURSOR));
}
