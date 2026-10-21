#include "gba/memory.h"
#include "game/script.h"
#include <stdlib.h>
/* Optional private oracle bridge. Never installed or shipped with the app. */
HNMemory *hn_probe_create(const uint8_t *rom,size_t size)
{
    HNMemory *m=malloc(sizeof(*m));
    if(m)hn_memory_init(m,rom,size);
    return m;
}
void hn_probe_destroy(HNMemory *m){free(m);}
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
