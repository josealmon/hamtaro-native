#include "game/heap.h"
/* Explicit AH3P 12-byte allocator nodes; no host pointers are stored in game RAM. */
enum {
    SENTINEL=0x030010C8, USED_HEAD=0x03002C84, FREE_HEAD=0x03002E04,
    DESCRIPTORS=0x02022000, DATA=0x02022800, CAPACITY=0x1D7FC,
    NODE_LIMIT=10240
};
static uint32_t size(HNMemory *m,GameAddr node){return hn_read32(m,node);}
static GameAddr data(HNMemory *m,GameAddr node){return hn_read32(m,node+4);}
static GameAddr next(HNMemory *m,GameAddr node){return hn_read32(m,node+8);}
static void node(HNMemory *m,GameAddr a,uint32_t bytes,GameAddr payload,GameAddr link)
{hn_write32(m,a,bytes);hn_write32(m,a+4,payload);hn_write32(m,a+8,link);}
void hn_heap_init(HNMemory *m)
{
    hn_write32(m,USED_HEAD,SENTINEL);
    hn_write32(m,SENTINEL+8,SENTINEL);
    hn_write32(m,FREE_HEAD,DESCRIPTORS);
    node(m,DESCRIPTORS,CAPACITY,DATA,SENTINEL);
}
void hn_heap_coalesce(HNMemory *m)
{
    GameAddr block=hn_read32(m,FREE_HEAD);
    unsigned visits=0;
    while(block!=SENTINEL && !m->fault && visits++<NODE_LIMIT) {
        bool merged=false;
        GameAddr link=block+8,other=next(m,block);
        while(other!=SENTINEL && !m->fault && visits++<NODE_LIMIT) {
            if(data(m,block)+size(m,block)==data(m,other) ||
               data(m,block)==data(m,other)+size(m,other)) {
                uint32_t total=size(m,block)+size(m,other);
                GameAddr begin=data(m,block)<data(m,other)?data(m,block):data(m,other);
                hn_write32(m,block,total);hn_write32(m,block+4,begin);
                hn_write32(m,link,next(m,other));node(m,other,0,0,0);
                merged=true;break;
            }
            link=other+8;other=next(m,other);
        }
        if(!merged)block=next(m,block);
    }
    if(visits>=NODE_LIMIT){m->fault=true;m->fault_address=block;}
}
static GameAddr find_descriptor(HNMemory *m)
{
    for(GameAddr p=DESCRIPTORS;p<=0x020227FF;p+=12)
        if(!size(m,p)&&!data(m,p)&&!next(m,p))return p;
    hn_heap_coalesce(m);
    for(GameAddr p=DESCRIPTORS;p<=0x020227FF;p+=12)
        if(!size(m,p)&&!data(m,p)&&!next(m,p))return p;
    return 0;
}
GameAddr hn_heap_allocate(HNMemory *m,uint32_t requested,bool clear)
{
    if(requested>CAPACITY-12 || m->fault)return 0;
    uint32_t payload=(requested+3)&~3u,total=payload+12;
    GameAddr free_node=hn_read32(m,FREE_HEAD);
    bool collected=false;
    unsigned visits=0;
    while(free_node && free_node!=SENTINEL && total>size(m,free_node) && !m->fault) {
        free_node=next(m,free_node);
        if(free_node==SENTINEL && !collected) {
            hn_heap_coalesce(m);free_node=hn_read32(m,FREE_HEAD);collected=true;
        }
        if(++visits>=NODE_LIMIT)return 0;
    }
    if(!free_node||free_node==SENTINEL||m->fault)return 0;
    GameAddr tail=hn_read32(m,USED_HEAD);
    visits=0;
    while(next(m,tail)!=SENTINEL && !m->fault) {
        tail=next(m,tail);if(++visits>=NODE_LIMIT)return 0;
    }
    GameAddr block=data(m,free_node);
    hn_write32(m,tail+8,block);
    node(m,block,payload,block+12,SENTINEL);
    hn_write32(m,free_node,size(m,free_node)-total);
    hn_write32(m,free_node+4,block+total);
    if(clear)for(uint32_t i=0;i<payload;i++)hn_write8(m,block+12+i,0);
    return m->fault?0:block+12;
}
uint32_t hn_heap_free(HNMemory *m,GameAddr pointer)
{
    if(!pointer || m->fault)return 0;
    GameAddr block=hn_read32(m,USED_HEAD),link=block+8;
    unsigned visits=0;
    while(data(m,block)!=pointer) {
        GameAddr candidate=next(m,block);
        if(candidate==SENTINEL || candidate<DATA || candidate-DATA>CAPACITY-1)return 0;
        link=block+8;block=candidate;
        if(++visits>=NODE_LIMIT)return 0;
    }
    uint32_t bytes=size(m,block),total=bytes+12;
    GameAddr free_node=hn_read32(m,FREE_HEAD);
    visits=0;
    while(free_node!=SENTINEL && !m->fault && visits++<NODE_LIMIT) {
        if(data(m,free_node)+size(m,free_node)==block) {
            hn_write32(m,free_node,size(m,free_node)+total);break;
        }
        if(data(m,free_node)==block+total) {
            hn_write32(m,free_node,size(m,free_node)+total);
            hn_write32(m,free_node+4,block);break;
        }
        if(next(m,free_node)==SENTINEL) {
            GameAddr descriptor=find_descriptor(m);
            if(!descriptor)return 0;
            hn_write32(m,free_node+8,descriptor);node(m,descriptor,total,block,SENTINEL);break;
        }
        free_node=next(m,free_node);
    }
    if(m->fault||visits>=NODE_LIMIT)return 0;
    hn_write32(m,link,next(m,block));hn_write32(m,block+8,SENTINEL);
    return bytes;
}
