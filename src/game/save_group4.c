#include "game/save_group4.h"
#include "game/state.h"

enum { CONTEXT=0x03002BB0,RECEIVED=0x03003D30,RESOURCES=0x086E8CCC,PAYLOAD_FLAGS=0x086E906A };
static unsigned control(HNMemory *m,unsigned index){return hn_read8(m,HN_SCENE_CONTROLS+index);}
static void store(HNMemory *m,unsigned index,unsigned value){hn_write8(m,HN_SCENE_CONTROLS+index,(uint8_t)value);}
static GameAddr resource(unsigned row){return RESOURCES+row*28;}
static void classify(HNMemory *m)
{
    GameAddr entry=resource(control(m,0x180)),data=hn_read32(m,entry+8);
    unsigned mode=control(m,0x60);store(m,0x61,0);
    if(mode==0){
        unsigned count=hn_read8(m,entry+0x16);
        for(unsigned i=0;i<count&&!m->fault;i++){
            unsigned value=hn_read8(m,RECEIVED+17+i),expected=hn_read8(m,data+i);
            if(value!=expected&&value>1){store(m,0x61,1);break;}
        }
    }else if(mode==1){
        unsigned count=hn_read8(m,entry+0x16);
        for(unsigned i=0;i<count&&!m->fault;i++)if(hn_read8(m,RECEIVED+17+i)==0x34){store(m,0x61,1);break;}
    }else if(mode==2){
        unsigned count=0,length=hn_read8(m,entry+0x16);
        for(unsigned row=0;row<10&&!m->fault;row++)for(unsigned i=0;i<length&&!m->fault;i++){
            int value=hn_read8(m,RECEIVED+17+i),expected=(int)hn_read8(m,PAYLOAD_FLAGS+row)-0x45;
            if(value==expected){count++;break;}
        }
        store(m,0x62,count<=4?1:count<=7?2:count<=9?3:0);
        if(count>9)store(m,0x61,1);
    }
}
HNSceneResult hn_group4_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080C8E48:case 0x080C8E70:{
        GameAddr context=hn_read32(m,CONTEXT);unsigned offset=hn_read16(m,arguments);
        if((callback&~1u)==0x080C8E70)store(m,offset,hn_read8(m,context+0x2B18));
        else hn_write8(m,context+0x2B18,(uint8_t)control(m,offset));
        *consumed=2;break;
    }
    case 0x080C8740:{
        unsigned row=control(m,0x180);GameAddr entry=resource(row),records=hn_read32(m,CONTEXT);
        unsigned column=control(m,0x181);
        for(unsigned i=0;i<110&&!m->fault;i++)hn_write8(m,RECEIVED+17+i,0);
        if(column<=3)hn_memory_copy(m,RECEIVED,records+row*1100+column*220,128);
        else if(column==5)hn_memory_copy(m,RECEIVED+17,hn_read32(m,entry+8),hn_read8(m,entry+0x16));
        break;
    }
    case 0x080C8E98:{
        GameAddr entry=resource(control(m,4));store(m,0x60,1);
        unsigned count=hn_read8(m,entry+0x16);
        for(unsigned i=0;i<count&&!m->fault;i++){
            unsigned value=hn_read8(m,RECEIVED+17+i);
            if(value>1){unsigned flag=value+0x45;
                if(!(hn_read8(m,0x03002180+(flag>>3))&(1u<<(flag&7)))){store(m,0x60,0);break;}}
        }
        break;
    }
    case 0x080C6EDC:classify(m);break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
