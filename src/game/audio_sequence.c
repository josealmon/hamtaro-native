#include "game/audio.h"
#include "game/state.h"
static bool fail(HNAudio *a,HNAudioFault fault,unsigned op,GameAddr address)
{a->fault=fault;a->fault_opcode=op;a->fault_address=address;return false;}
static unsigned read_operand(HNMemory *m,GameAddr track,bool checked)
{
    GameAddr p=hn_read32(m,track+0x40);hn_write32(m,track+0x40,p+1);unsigned value=hn_read8(m,p);
    return checked&&p<0x02000000?0:value;
}
static void jump(HNMemory *m,GameAddr track)
{
    GameAddr p=hn_read32(m,track+0x40);uint32_t target=hn_read32(m,p);
    if(p<0x02000000)target&=0xFFFFFF00u;hn_write32(m,track+0x40,target);
}
static bool memory_command(HNAudio *a,GameAddr player,GameAddr track)
{
    HNMemory *m=a->memory;unsigned op=read_operand(m,track,false),offset=read_operand(m,track,false);
    GameAddr base=hn_read32(m,player+0x18),destination=base+offset;
    unsigned value=read_operand(m,track,false),left=hn_read8(m,destination);
    if(op<6){
        unsigned right=op>=3?hn_read8(m,base+value):value;
        hn_write8(m,destination,(uint8_t)(op%3==0?right:op%3==1?left+right:left-right));
    }else if(op<18){
        unsigned right=op>=12?hn_read8(m,base+value):value,kind=(op-6)%6;
        bool take=kind==0?left==right:kind==1?left!=right:kind==2?left>right:kind==3?left>=right:kind==4?left<=right:left<right;
        if(take){
            GameAddr target=hn_read32(m,HN_AUDIO_DISPATCH+4);
            if((target&~1u)!=(hn_read32(m,HN_AUDIO_INITIAL_DISPATCH+4)&~1u))return fail(a,HN_AUDIO_CALLBACK,0xB9,target);
            jump(m,track);
        }else hn_write32(m,track+0x40,hn_read32(m,track+0x40)+4);
    }
    return !m->fault;
}
static bool extended(HNAudio *a,GameAddr track)
{
    HNMemory *m=a->memory;unsigned command=read_operand(m,track,false);
    if(command>=12)return fail(a,HN_AUDIO_COMMAND,0xCD,hn_read32(m,track+0x40)-1);
    if(!command||command==3)return hn_audio_track_finish(m,track);
    if(command==1){GameAddr p=hn_read32(m,track+0x40);hn_write32(m,track+0x28,hn_read32(m,p));hn_write32(m,track+0x40,p+4);}
    else{
        unsigned offset=command==2?0x24:command>=4&&command<=7?0x2C+command-4:command==8?0x1E:command==9?0x1F:command==10?0x26:0x27;
        hn_write8(m,track+offset,(uint8_t)read_operand(m,track,false));
    }
    return !m->fault;
}
static bool end_tie(HNAudio *a,GameAddr track)
{
    HNMemory *m=a->memory;GameAddr p=hn_read32(m,track+0x40);unsigned key=hn_read8(m,p);
    if(key<128){hn_write8(m,track+5,(uint8_t)key);hn_write32(m,track+0x40,p+1);}else key=hn_read8(m,track+5);
    GameAddr channel=hn_read32(m,track+0x20);unsigned visited=0;
    while(channel&&!m->fault){
        if(visited++==64)return fail(a,HN_AUDIO_BUDGET,0xCE,channel);
        unsigned flags=hn_read8(m,channel);
        if((flags&0x83)&&!(flags&64)&&hn_read8(m,channel+0x11)==key){hn_write8(m,channel,(uint8_t)(flags|64));break;}
        channel=hn_read32(m,channel+0x34);
    }
    return !m->fault;
}
bool hn_audio_sequence_command(HNAudio *a,GameAddr player,GameAddr track,unsigned op)
{
    if(a->fault)return false;HNMemory *m=a->memory;
    if(op<0xB1||op>0xCE)return fail(a,HN_AUDIO_COMMAND,op,hn_read32(m,track+0x40));
    unsigned slot=op-0xB1;GameAddr binding=hn_read32(m,HN_AUDIO_DISPATCH+slot*4);
    GameAddr expected=hn_read32(m,HN_AUDIO_INITIAL_DISPATCH+slot*4);
    GameAddr fine=hn_read32(m,HN_AUDIO_INITIAL_DISPATCH);
    if((binding&~1u)==(fine&~1u)){
        if(!hn_audio_track_finish(m,track))return fail(a,m->fault?HN_AUDIO_MEMORY:HN_AUDIO_BUDGET,op,m->fault?m->fault_address:track);
        return true;
    }
    if(slot==8)expected=0x080EBE3D;if(slot==28)expected=0x080EBF95;
    if((binding&~1u)!=(expected&~1u))return fail(a,HN_AUDIO_CALLBACK,op,binding);
    switch(op){
    case 0xB2:jump(m,track);break;
    case 0xB3: {
        unsigned depth=hn_read8(m,track+2);
        if(depth>=3){if(!hn_audio_track_finish(m,track))return fail(a,HN_AUDIO_RESOURCE,op,track);}
        else{
            hn_write32(m,track+0x44+depth*4,hn_read32(m,track+0x40)+4);hn_write8(m,track+2,(uint8_t)(depth+1));jump(m,track);
        }break;
    }
    case 0xB4: {
        unsigned depth=hn_read8(m,track+2);
        if(depth){depth--;hn_write8(m,track+2,(uint8_t)depth);hn_write32(m,track+0x40,hn_read32(m,track+0x44+depth*4));}
        break;
    }
    case 0xB5: {
        GameAddr p=hn_read32(m,track+0x40);unsigned count=hn_read8(m,p);
        if(!count){hn_write32(m,track+0x40,p+1);jump(m,track);}
        else{
            unsigned iteration=hn_read8(m,track+3)+1;hn_write8(m,track+3,(uint8_t)iteration);count=read_operand(m,track,true);
            if(iteration>=count){hn_write8(m,track+3,0);hn_write32(m,track+0x40,p+5);}else jump(m,track);
        }break;
    }
    case 0xB9:if(!memory_command(a,player,track))return a->fault?false:fail(a,HN_AUDIO_MEMORY,op,m->fault_address);break;
    case 0xBA:hn_write8(m,track+0x1D,(uint8_t)read_operand(m,track,true));break;
    case 0xBB: {
        unsigned tempo=read_operand(m,track,true)*2;hn_write16(m,player+0x1C,(uint16_t)tempo);
        hn_write16(m,player+0x20,(uint16_t)((tempo*hn_read16(m,player+0x1E))>>8));break;
    }
    case 0xBD: {
        unsigned voice=read_operand(m,track,false);GameAddr source=hn_read32(m,player+0x30)+voice*12;
        for(unsigned i=0;i<3;i++){
            uint32_t value=hn_read32(m,source+i*4);hn_write32(m,track+0x24+i*4,source<0x02000000?0:value);
        }break;
    }
    case 0xBC:case 0xBE:case 0xBF:case 0xC0:case 0xC1:case 0xC3:case 0xC8: {
        unsigned offset=op==0xBC?10:op==0xBE?0x12:op==0xBF?0x14:op==0xC0?14:op==0xC1?15:op==0xC3?0x1B:12;
        unsigned value=read_operand(m,track,true);if(op==0xBF||op==0xC0||op==0xC8)value-=64;
        hn_write8(m,track+offset,(uint8_t)value);
        if(op!=0xC3)hn_write8(m,track,(uint8_t)(hn_read8(m,track)|((op==0xBE||op==0xBF)?3:12)));
        break;
    }
    case 0xC2:case 0xC4: {
        unsigned value=read_operand(m,track,false);hn_write8(m,track+(op==0xC2?0x19:0x17),(uint8_t)value);
        if(!value)hn_audio_modulation_reset(m,track);break;
    }
    case 0xC5: {
        unsigned value=read_operand(m,track,true);
        if(value!=hn_read8(m,track+0x18)){hn_write8(m,track+0x18,(uint8_t)value);hn_write8(m,track,hn_read8(m,track)|15);}break;
    }
    case 0xCC: {
        unsigned offset=read_operand(m,track,false),value=read_operand(m,track,true);hn_write8(m,0x04000060+offset,(uint8_t)value);break;
    }
    case 0xCD:if(!extended(a,track))return a->fault?false:fail(a,HN_AUDIO_RESOURCE,op,track);break;
    case 0xCE:if(!end_tie(a,track))return a->fault?false:fail(a,HN_AUDIO_RESOURCE,op,track);break;
    default:return fail(a,HN_AUDIO_COMMAND,op,hn_read32(m,track+0x40));
    }
    return m->fault?fail(a,HN_AUDIO_MEMORY,op,m->fault_address):true;
}
