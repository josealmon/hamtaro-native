#include "game/background.h"
#include "game/state.h"
#include "game/world.h"
#include "game/video.h"
#include "game/transfer.h"
#include "gba/bios.h"
static bool fail(HNBackground *b,HNBackgroundFault fault,unsigned opcode,GameAddr address)
{b->fault=fault;b->fault_opcode=opcode;b->fault_address=address;return false;}
static void commit(HNMemory *m,GameAddr state,GameAddr *cursor,unsigned bytes)
{*cursor+=bytes;hn_write32(m,state+0x14,*cursor);}
void hn_background_init(HNBackground *b,HNMemory *m)
{b->memory=m;b->fault=HN_BACKGROUND_OK;b->fault_opcode=0;b->fault_address=0;}
void hn_background_clamp(HNMemory *m,uint8_t layer,GameAddr coordinate,uint8_t axis)
{
    GameAddr bounds=HN_BACKGROUND_BOUNDS+layer*12;if(!(hn_read8(m,bounds+8)&1))return;
    int value=(int32_t)hn_read32(m,coordinate)>>16;
    unsigned offset=(axis&1)?4:0;int edge=-(int16_t)hn_read16(m,bounds+offset),changed=0;
    if(value>=edge){value=edge;changed=1;}
    else{
        edge=((axis&1)?160:240)-(int16_t)hn_read16(m,bounds+offset+2);
        if(value<=edge){value=edge;changed=1;}
    }
    if(changed)hn_write32(m,coordinate,((uint32_t)value<<16)+0x8000);
}
bool hn_background_apply(HNMemory *m,uint8_t layer)
{
    if(layer>=4)return false;
    GameAddr p=HN_LAYER_OFFSETS+layer*44,v=HN_VIDEO_STATE+0x38+layer*48;
    hn_write16(m,v+2,(uint16_t)((hn_read32(m,p)+hn_read32(m,p+0x10))>>16));
    hn_write16(m,v+4,(uint16_t)((hn_read32(m,p+4)+hn_read32(m,p+0x14))>>16));
    hn_video_mark_backgrounds(m,(uint8_t)(1u<<layer));
    if((hn_read8(m,HN_DISPLAY_SHADOW)&15)&&layer>=2){
        unsigned angle=hn_read16(m,p+0x1C);if(angle>255)return false;
        int sine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+angle*2);
        int cosine=(int16_t)hn_read16(m,HN_RENDER_SINE_TABLE+(angle+64)*2);
        int rx=(int16_t)hn_runtime_divide(65536,(int16_t)hn_read16(m,p+0x18));
        int ry=(int16_t)hn_runtime_divide(65536,(int16_t)hn_read16(m,p+0x1A));
        int pa=(int16_t)(cosine*rx/256),pb=(int16_t)(sine*rx/256);
        int pc=(int16_t)((int16_t)-sine*ry/256),pd=(int16_t)(cosine*ry/256);
        const int matrix[4]={pa,pb,pc,pd};
        for(unsigned i=0;i<4;i++){
            hn_write16(m,p+0x22+i*2,(uint16_t)matrix[i]);hn_write16(m,v+6+i*2,(uint16_t)matrix[i]);
        }
        int ax=(int16_t)hn_read16(m,p+0x1E),ay=(int16_t)hn_read16(m,p+0x20);
        uint32_t x=(uint32_t)((int32_t)(hn_read32(m,HN_VIDEO_STATE+4)-hn_read32(m,p))>>8);
        uint32_t y=(uint32_t)((int32_t)(hn_read32(m,HN_VIDEO_STATE+8)-hn_read32(m,p+4))>>8);
        x-=(uint32_t)ax+(uint32_t)(pa*ax)+(uint32_t)(pb*ay);
        y-=(uint32_t)ay+(uint32_t)(pc*ax)+(uint32_t)(pd*ay);
        hn_write32(m,p+8,x);hn_write32(m,p+12,y);hn_write32(m,v+0x10,x);hn_write32(m,v+0x14,y);
    }
    return !m->fault;
}
bool hn_background_motion(HNMemory *m,GameAddr state)
{
    hn_write16(m,state+0x12,(uint16_t)(hn_read16(m,state+0x12)-1));
    hn_write32(m,state+4,hn_read32(m,state+4)+hn_read32(m,state));
    hn_write32(m,state+12,hn_read32(m,state+12)+hn_read32(m,state+8));
    hn_write8(m,state+0x3F,(uint8_t)(hn_read8(m,state+0x3F)+hn_read8(m,state+0x3E)));
    for(unsigned i=0;i<4;i++)hn_write16(m,state+0x42+i*4,(uint16_t)(hn_read16(m,state+0x42+i*4)+hn_read16(m,state+0x40+i*4)));
    unsigned mode=hn_read8(m,HN_DISPLAY_SHADOW)&15,mask=hn_read8(m,state+0x38);
    for(unsigned layer=0;layer<4;layer++)if(mask&(1u<<layer)){
        GameAddr p=HN_LAYER_OFFSETS+layer*44;
        hn_write32(m,p,hn_read32(m,p)+hn_read32(m,state+12));
        hn_background_clamp(m,(uint8_t)layer,p,0);
        hn_write32(m,p+4,hn_read32(m,p+4)+hn_read32(m,state+4));
        hn_background_clamp(m,(uint8_t)layer,p+4,1);
        if(mode&&!(mode==1&&layer<2)){
            hn_write16(m,p+0x1C,(uint16_t)((hn_read16(m,p+0x1C)+hn_read8(m,state+0x3F))&255));
            hn_write16(m,p+0x18,(uint16_t)(hn_read16(m,p+0x18)+hn_read16(m,state+0x42)));
            hn_write16(m,p+0x1A,(uint16_t)(hn_read16(m,p+0x1A)+hn_read16(m,state+0x46)));
            hn_write16(m,p+0x1E,(uint16_t)(hn_read16(m,p+0x1E)+hn_read16(m,state+0x4A)));
            hn_write16(m,p+0x20,(uint16_t)(hn_read16(m,p+0x20)+hn_read16(m,state+0x4E)));
        }
        if(!hn_background_apply(m,(uint8_t)layer))return false;
    }
    return !m->fault;
}
bool hn_background_configure(HNMemory *m,uint8_t slot,uint8_t mask,GameAddr script)
{
    if(slot>=hn_read8(m,HN_BACKGROUND_COUNT))return false;
    if(mask&128)mask&=127;else hn_background_layers_reset(m,mask);
    GameAddr state=hn_read32(m,HN_RENDER_STATES)+slot*80;
    if(!hn_transfer_fill_words(m,state,0,20))return false;
    hn_write8(m,state+0x38,mask);hn_write32(m,state+0x14,script);return !m->fault;
}
static void dma_halfwords(HNMemory *m,GameAddr source,GameAddr destination,uint32_t halfwords)
{
    hn_write32(m,0x040000D4,source);hn_write32(m,0x040000D8,destination);
    hn_write32(m,0x040000DC,0x80000000u|halfwords);
}
static bool tilemap(HNMemory *m,GameAddr state,GameAddr p)
{
    GameAddr source=hn_read32(m,p);unsigned left=hn_read8(m,p+4),top=hn_read8(m,p+5);
    unsigned right=(hn_read8(m,p+6)+1)&255,bottom=(hn_read8(m,p+7)+1)&255;
    unsigned mask=hn_read8(m,state+0x38)&15,layer=0;
    if(!mask||right<=left||bottom<=top)return false;
    for(unsigned i=0;i<4;i++)if(mask&(1u<<i))layer=i;
    unsigned control=hn_read8(m,HN_VIDEO_STATE+0x39+layer*48),mode=hn_read8(m,HN_DISPLAY_SHADOW)&15;
    GameAddr target=0x06000000+((control&31)<<11);
    if(!mode||(mode==1&&layer<2)){
        if(right>64||bottom>64)return false;
        for(unsigned y=top;y<bottom;y++){
            unsigned x=left;
            do{
                unsigned end=x<32&&right>32?32:right,width=end-x;
                unsigned offset=((y&31)*32+(x&31))*2;
                if(x>=32)offset+=y>=32?0x1800:0x800;
                else if(y>=32)offset+=right>32?0x1000:0x800;
                dma_halfwords(m,source,target+offset,width);source+=width*2;x=end;
            }while(x<right&&!m->fault);
        }
    }else if(mode<=2){
        unsigned stride=16u<<((control>>6)&1),width=(right-left)/2;
        if(!width)return false;
        target+=top*stride+left;
        for(unsigned row=0;row<bottom-top;row++)dma_halfwords(m,source+row*stride,target+row*stride,width);
    }
    return !m->fault;
}
bool hn_background_command(HNBackground *b,GameAddr state,unsigned op,GameAddr *cursor,uint32_t *yield)
{
    if(b->fault)return false;
    HNMemory *m=b->memory;GameAddr p=*cursor;*yield=0;
    switch(op){
    case 0x80: {
        unsigned depth=hn_read8(m,state+0x3D);
        if(depth>4)return fail(b,HN_BACKGROUND_STACK,op,p);
        if(!depth){hn_write8(m,state+0x38,0);*yield=1;}
        else{*cursor=hn_read32(m,state+0x28+(depth-1)*4);hn_write8(m,state+0x3D,(uint8_t)(depth-1));commit(m,state,cursor,0);}
        break;
    }
    case 0x81:hn_write16(m,state+0x12,hn_read16(m,p));commit(m,state,cursor,2);*yield=1;break;
    case 0x82:*cursor=hn_read32(m,p);commit(m,state,cursor,0);break;
    case 0x83:hn_write8(m,state+0x38,hn_read8(m,p));commit(m,state,cursor,1);break;
    case 0x84:case 0x85:case 0x86:case 0x87: {
        unsigned offset=op==0x84?0:op==0x85?8:op==0x86?4:12;
        hn_write32(m,state+offset,hn_read32(m,p));commit(m,state,cursor,4);break;
    }
    case 0x88:case 0x89:case 0x8A:case 0x8B: {
        uint32_t value=(uint32_t)hn_read16(m,p)<<16;unsigned offset=(op&1)?0:4;
        commit(m,state,cursor,2);
        for(unsigned layer=0;layer<4;layer++)if(hn_read8(m,state+0x38)&(1u<<layer)){
            GameAddr target=HN_LAYER_OFFSETS+layer*44+offset;
            if(op<0x8A){target+=0x10;hn_write32(m,target,hn_read32(m,target)+value);}
            else{hn_write32(m,target,value+0x8000);hn_background_clamp(m,(uint8_t)layer,target,(uint8_t)(offset/4));}
            if(!hn_background_apply(m,(uint8_t)layer))return fail(b,HN_BACKGROUND_RESOURCE,op,p);
        }
        break;
    }
    case 0x8C:case 0x8D:case 0x8E:case 0x8F:case 0x96:case 0x98:case 0x9B:*yield=1;break;
    case 0x90: {
        unsigned i=0;while(i<4&&hn_read8(m,state+0x39+i))i++;
        if(i==4)return fail(b,HN_BACKGROUND_STACK,op,p);
        hn_write8(m,state+0x39+i,hn_read8(m,p));hn_write32(m,state+0x18+i*4,p+1);commit(m,state,cursor,1);break;
    }
    case 0x91: {
        int i=3;while(i>=0&&hn_read8(m,state+0x39+i)<=1)i--;
        if(i>=0){hn_write8(m,state+0x39+i,(uint8_t)(hn_read8(m,state+0x39+i)-1));*cursor=hn_read32(m,state+0x18+i*4);}
        else{commit(m,state,cursor,0);hn_write8(m,state+0x39,0);}break;
    }
    case 0x92:hn_write16(m,state+0x10,hn_read16(m,p));commit(m,state,cursor,2);break;
    case 0x93:case 0x94:case 0x9C:case 0x9D: {
        GameAddr target=hn_read32(m,p);unsigned bytes=(op==0x94||op==0x9D)?2:1;
        unsigned value=bytes==2?hn_read16(m,p+4):hn_read8(m,p+4);commit(m,state,cursor,4+bytes);
        if(op>=0x9C){hn_write16(m,HN_BACKGROUND_QUEUED_VALUE,(uint16_t)value);hn_transfer_cpu_queue(m,HN_BACKGROUND_QUEUED_VALUE,target,bytes);}
        else if(bytes==2)hn_write16(m,target,(uint16_t)value);else hn_write8(m,target,(uint8_t)value);
        break;
    }
    case 0x95: {
        GameAddr target=hn_read32(m,p)+hn_read32(m,p+8),source=hn_read32(m,p+4);commit(m,state,cursor,12);
        if(!hn_bios_lz77(m,source,target,0x40000))return fail(b,HN_BACKGROUND_RESOURCE,op,p);break;
    }
    case 0x97: {
        GameAddr source=hn_read32(m,p),target=hn_read32(m,p+4);unsigned width=hn_read8(m,p+8),rows=hn_read8(m,p+9);
        commit(m,state,cursor,10);
        for(unsigned i=0;i<rows&&!m->fault;i++)dma_halfwords(m,source+i*64,target+i*64,width);break;
    }
    case 0x99:case 0x9A:case 0x9E:case 0x9F: {
        bool offset=op==0x9A||op==0x9F;GameAddr source=hn_read32(m,p);if(offset)source+=hn_read32(m,p+4);
        GameAddr target=hn_read32(m,p+4+(offset?4:0));unsigned size=hn_read32(m,p+8+(offset?4:0));
        commit(m,state,cursor,offset?16:12);
        if(op>=0x9E)hn_transfer_cpu_queue(m,source,target,size);
        else if(!hn_bios_cpu_set(m,source,target,(size&3)?((size>>1)&0x1FFFFF):((size>>2)&0x1FFFFF)|0x04000000))
            return fail(b,HN_BACKGROUND_RESOURCE,op,p);break;
    }
    case 0xA0: {
        unsigned depth=hn_read8(m,state+0x3D);if(depth>=4)return fail(b,HN_BACKGROUND_STACK,op,p);
        hn_write32(m,state+0x28+depth*4,p+4);hn_write8(m,state+0x3D,(uint8_t)(depth+1));
        *cursor=hn_read32(m,p);commit(m,state,cursor,0);break;
    }
    case 0xA1:commit(m,state,cursor,8);if(!tilemap(m,state,p))return fail(b,HN_BACKGROUND_RESOURCE,op,p);break;
    case 0xA3:case 0xA4:hn_write8(m,state+0x3E+(op-0xA3),hn_read8(m,p));commit(m,state,cursor,1);break;
    case 0xA8:case 0xA9:case 0xAA:case 0xAB:case 0xB9:case 0xBA:case 0xBB:case 0xBC: {
        unsigned offset=op==0xA8?0x44:op==0xA9?0x40:op==0xAA?0x46:op==0xAB?0x42:0x48+(op-0xB9)*2;
        hn_write16(m,state+offset,hn_read16(m,p));commit(m,state,cursor,2);break;
    }
    case 0xA2:case 0xA5:case 0xA6:case 0xA7:case 0xAC:case 0xAD:case 0xAE:case 0xAF: {
        bool rotation=op==0xA2||op==0xA5;unsigned value=rotation?hn_read8(m,p):hn_read16(m,p);
        unsigned offset=rotation?0x1C:(op==0xA6||op==0xAC)?0x1A:(op==0xAE)?0x1E:(op==0xAF)?0x20:0x18;
        commit(m,state,cursor,rotation?1:2);
        for(unsigned layer=2;layer<4;layer++)if(op==0xA2||(hn_read8(m,state+0x38)&(1u<<layer))){
            GameAddr target=HN_LAYER_OFFSETS+layer*44+offset;unsigned next=value;
            if(op==0xA5||op==0xAC||op==0xAD)next+=hn_read16(m,target);
            if(op==0xA5)next&=255;hn_write16(m,target,(uint16_t)next);
            if(op<0xAE&&!hn_background_apply(m,(uint8_t)layer))return fail(b,HN_BACKGROUND_RESOURCE,op,p);
        }
        break;
    }
    case 0xB0:case 0xB1:case 0xB2: {
        unsigned value=hn_read8(m,hn_read32(m,p)),expected=hn_read8(m,p+4);
        bool take=op==0xB0?value==expected:op==0xB1?value<expected:value>expected;
        if(take){*cursor=p+5;hn_write32(m,state+0x14,hn_read32(m,p+5));*yield=1;}
        else commit(m,state,cursor,9);break;
    }
    case 0xB3:*cursor+=4;break;
    case 0xB4:*cursor+=5;break;
    case 0xB5:case 0xB6:case 0xB7:*cursor+=1;break;
    case 0xB8:*cursor+=2;break;
    default:return fail(b,HN_BACKGROUND_OPCODE,op,p);
    }
    return m->fault?fail(b,HN_BACKGROUND_MEMORY,op,m->fault_address):true;
}
bool hn_background_process(HNBackground *b,GameAddr state,unsigned command_budget)
{
    if(b->fault)return false;HNMemory *m=b->memory;
    unsigned delay=hn_read16(m,state+0x10);
    if(delay)hn_write16(m,state+0x10,(uint16_t)(delay-1));
    else if(hn_read16(m,state+0x12)){
        if(!hn_background_motion(m,state))return fail(b,HN_BACKGROUND_RESOURCE,0,hn_read32(m,state+0x14));
    }else{
        GameAddr cursor=hn_read32(m,state+0x14);
        for(unsigned i=0;i<command_budget;i++){
            unsigned op=hn_read8(m,cursor);
            if(m->fault)return fail(b,HN_BACKGROUND_MEMORY,op,m->fault_address);
            if(op<0x80)return true;
            cursor++;uint32_t yield;
            if(!hn_background_command(b,state,op,&cursor,&yield))return false;
            if(yield)return true;
        }
        return fail(b,HN_BACKGROUND_BUDGET,0,cursor);
    }
    return m->fault?fail(b,HN_BACKGROUND_MEMORY,0,m->fault_address):true;
}
bool hn_background_update(HNBackground *b,unsigned command_budget)
{
    if(b->fault)return false;HNMemory *m=b->memory;
    GameAddr state=hn_read32(m,HN_RENDER_STATES);unsigned count=hn_read8(m,HN_BACKGROUND_COUNT);
    for(unsigned i=0;i<count;i++,state+=80)
        if(hn_read8(m,state+0x38)&&!hn_background_process(b,state,command_budget))return false;
    return m->fault?fail(b,HN_BACKGROUND_MEMORY,0,m->fault_address):true;
}
