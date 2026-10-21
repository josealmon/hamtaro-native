#include "gba/memory.h"
#include "game/script.h"
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
