#include "game/script.h"
#include "game/state.h"
#include "game/world.h"
#include "game/video.h"
#include "game/transfer.h"
#include "game/heap.h"
#include "game/expression.h"
#include "game/save.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/scene.h"
#include "game/random.h"
#include "game/input.h"
#include "game/actor.h"
#include "game/motion.h"
#include "game/task.h"
#include "game/choice.h"
#include "gba/bios.h"
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
static GameAddr audio_player(HNMemory *m,uint16_t song,unsigned *index)
{
    *index=hn_read16(m,HN_AUDIO_SONG_RECORDS+song*8+4);
    return *index<HN_AUDIO_PLAYERS?hn_read32(m,HN_AUDIO_PLAYER_RECORDS+*index*12):0;
}
static void advance(HNMemory *m,unsigned bytes)
{hn_write32(m,CONSUMED,bytes);hn_write8(m,OPCODE,0);}
static void transfer16(HNMemory *m,GameAddr source,GameAddr destination,unsigned bytes,bool fill)
{
    hn_write32(m,0x040000D4,source);hn_write32(m,0x040000D8,destination);
    hn_write32(m,0x040000DC,(fill?0x81000000u:0x80000000u)|(bytes>>1));
}
static bool decompress(HNMemory *m,GameAddr source,GameAddr destination)
{return hn_bios_lz77(m,source,destination,0x40000);}
static uint32_t set_key_mask(HNMemory *m,GameAddr args)
{
    uint16_t mask=hn_read16(m,args);hn_write16(m,0x03001138,mask);
    hn_write16(m,0x03001F64,mask&0x3FF);return 2;
}
static uint32_t reset_visuals(HNMemory *m,GameAddr args)
{
    (void)args;hn_render_reset(m);GameAddr p=hn_read32(m,HN_GRAPHICS_SCRIPTS);
    for(unsigned i=0;i<0x160;i++)hn_write16(m,p+i*2,0);
    hn_background_state_init(m);return 0;
}
static uint32_t read_key_mask(HNMemory *m,GameAddr args)
{
    (void)args;hn_input_script_mask(m,0);return 0;
}
static const HNMainCallback builtins[]={
    {0x0801C708,set_key_mask},{0x0801C72C,read_key_mask},{0x0801D398,reset_visuals}
};
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
    case 0x0A:case 0x0B: {
        if(!hn_audio_ready(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        scratch(m,args,0,3);uint16_t song=hn_read16(m,SCRATCH);unsigned mode=hn_read8(m,SCRATCH+2),index;
        if(op==0x0B&&!song){if(!hn_audio_all(m,false))return fail(s,HN_SCRIPT_RESOURCE,op,args);}
        else{
            GameAddr player=audio_player(m,song,&index);if(!player)return fail(s,HN_SCRIPT_RESOURCE,op,args);
            if(op==0x0A){
                if(song>=200&&song<=299)hn_write16(m,CLASSES+0x1E,song);if(mode>2)mode=2;
                unsigned fade=hn_read8(m,HN_AUDIO_FADE_BYTES+index);
                if(!fade||mode<=1){if(!hn_audio_song(m,song,(HNSongAction)mode))return fail(s,HN_SCRIPT_RESOURCE,op,args);}
                else if(song!=hn_read16(m,HN_AUDIO_SONG_IDS+index*2)){
                    if(!hn_audio_song(m,song,HN_SONG_START))return fail(s,HN_SCRIPT_RESOURCE,op,args);
                }else hn_audio_fade(m,player,(uint16_t)fade,2);
                hn_write16(m,HN_AUDIO_SONG_IDS+index*2,hn_read16(m,SCRATCH));hn_write8(m,HN_AUDIO_FADE_BYTES+index,0);
            }else if(mode){hn_write8(m,HN_AUDIO_FADE_BYTES+index,(uint8_t)mode);hn_audio_fade(m,player,(uint16_t)mode,1);}
            else if(!hn_audio_song(m,song,HN_SONG_STOP))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        }
        advance(m,3);break;
    }
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
    case 6: {
        GameAddr cursor=hn_read32(m,CURSOR),source=hn_read32(m,cursor),destination=hn_read32(m,cursor+4);
        unsigned bytes=hn_read32(m,source)>>8;GameAddr temporary=0;
        bool palette=destination-0x05000000u<=0x3FF;
        if(destination>0x05FFFFFF)temporary=destination+0xFC004720u;
        else if(palette)temporary=destination+0xFD01C720u;
        if(temporary){
            if(!decompress(m,source,temporary))return fail(s,HN_SCRIPT_RESOURCE,op,source);
            if(hn_read16(m,0x04000000)&0x80){
                if(palette)transfer16(m,temporary,destination,bytes,false);
                else hn_transfer_copy(m,temporary,destination,bytes,false);
            }else hn_transfer_queue(m,temporary,destination,bytes);
        }
        advance(m,8);break;
    }
    case 7: {
        scratch(m,args,0,8);GameAddr source=hn_read32(m,SCRATCH),target=resolve_variable(m,4);
        if(target-0x02000000u<=0x01007FFFu ||
           (target>0x04FFFFFF && (hn_read16(m,0x04000000)&0x80))){
            if(!decompress(m,source,target))return fail(s,HN_SCRIPT_RESOURCE,op,source);
        }
        advance(m,8);break;
    }
    case 8:case 9: {
        scratch(m,args,0,8);GameAddr source=resolve_variable(m,0);
        GameAddr destination=op==8?resolve_variable(m,4):hn_read32(m,SCRATCH+4);
        unsigned bytes=hn_read16(m,args+8);
        if(op==8)hn_transfer_copy(m,source,destination,bytes,false);
        else{
            hn_write16(m,SCRATCH,(uint16_t)bytes);
            if(hn_read16(m,0x04000000)&0x80)transfer16(m,source,destination,bytes,false);
            else hn_transfer_queue(m,source,destination,bytes);
        }
        advance(m,10);break;
    }
    case 0x0C:
        scratch(m,args,0,4);hn_write32(m,CURSOR,hn_read32(m,SCRATCH));advance(m,0);break;
    case 0x0D: {
        uint8_t bank=hn_read8(m,HN_MAP_BANK),index=hn_read8(m,args);
        if(!hn_map_initialize(m,bank,index))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        if((uint8_t)(bank-1)<=8&&(hn_read8(m,0x03002180)&2)&&!hn_save_write_groups(m))
            return fail(s,HN_SCRIPT_RESOURCE,op,args);
        hn_render_order_init(m);hn_render_priority_init(m);break;
    }
    case 0x0E: {
        uint8_t bank=hn_read8(m,args);
        if(bank!=9&&hn_read8(m,HN_MAP_BANK)!=9)hn_core_storage_clear(m,2);
        if(!hn_map_initialize(m,bank,0))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        hn_write8(m,YIELDED,1);break;
    }
    case 0x0F:case 0x10:case 0x11: {
        unsigned index=hn_read8(m,args);GameAddr slot=hn_render_slot(m,index);
        GameAddr word=args+1;
        if(op==0x0F){
            hn_render_slot_reset(m,index);hn_write8(m,slot+0x60,hn_read8(m,args+1));
            hn_write8(m,slot+0x62,hn_read8(m,args+2));hn_render_flags(m,index,12,8);
            scratch(m,args+3,0,6);
            for(unsigned i=0;i<3;i++)hn_write32(m,slot+0x18+i*4,((uint32_t)hn_read16(m,SCRATCH+i*2)<<16)+0x8000);
            word=args+9;
        }else if(op==0x11){hn_write8(m,slot+0x60,hn_read8(m,args+1));word=args+2;}
        scratch(m,word,0,4);GameAddr animation=resolve_variable(m,0);
        if(animation)hn_render_set_animation(m,index,animation);
        if(index==hn_read8(m,0x03001F50))hn_write8(m,0x03002170,0);
        advance(m,op==0x0F?13:op==0x10?5:6);break;
    }
    case 0x12: {
        unsigned slot=hn_read8(m,args),mask=hn_read8(m,args+1);
        scratch(m,args+2,0,4);
        if(mask&128)mask&=127;else hn_background_layers_reset(m,(uint8_t)mask);
        GameAddr state=hn_read32(m,HN_RENDER_STATES)+slot*80;
        if(!hn_transfer_fill_words(m,state,0,20))return fail(s,HN_SCRIPT_RESOURCE,op,state);
        hn_write8(m,state+0x38,(uint8_t)mask);
        GameAddr script=resolve_variable(m,0);if(script)hn_write32(m,state+0x14,script);
        advance(m,6);break;
    }
    case 0x13: {
        GameAddr state=hn_read32(m,HN_GRAPHICS_SCRIPTS)+hn_read8(m,args)*44;
        scratch(m,args+2,0,4);hn_write8(m,state,hn_read8(m,args+1));
        GameAddr script=resolve_variable(m,0);if(script)hn_write32(m,state+0x18,script);
        advance(m,6);break;
    }
    case 0x14: {
        static const uint8_t shifts[]={0,2,6,7,8,13,14},widths[]={2,2,1,1,5,1,2};
        scratch(m,args,0,8);unsigned layer=hn_read8(m,SCRATCH);
        if(layer>=4)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        GameAddr control=HN_VIDEO_STATE+0x38+layer*0x30;uint16_t value=hn_read16(m,control);
        for(unsigned i=0;i<7;i++){
            unsigned field=hn_read8(m,SCRATCH+1+i),mask=((1u<<widths[i])-1)<<shifts[i];
            if(field!=255)value=(uint16_t)((value&~mask)|((field<<shifts[i])&mask));
        }
        hn_write16(m,control,value);hn_video_mark_backgrounds(m,(uint8_t)(1u<<layer));
        advance(m,8);break;
    }
    case 0x15: {
        unsigned layer=hn_read8(m,args);scratch(m,args+1,0,4);
        if(layer>=4)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        GameAddr state=HN_VIDEO_STATE+0x38+layer*0x30;
        hn_write16(m,state+2,hn_read16(m,SCRATCH));hn_write16(m,state+4,hn_read16(m,SCRATCH+2));
        hn_video_mark_backgrounds(m,(uint8_t)(1u<<layer));advance(m,5);break;
    }
    case 0x16:case 0x17:
        hn_video_set_layers(m,op==0x16,hn_read8(m,args));advance(m,1);break;
    case 0x18: {
        unsigned type=hn_read8(m,args);scratch(m,args+1,0,8);
        GameAddr destination=hn_read32(m,SCRATCH+4);
        if(!type){
            destination=resolve_variable(m,4);uint8_t value=hn_read8(m,SCRATCH);
            for(unsigned i=0;i<hn_read16(m,SCRATCH+2)&&!m->fault;i++)hn_write8(m,destination+i,value);
        }else if(destination>0x05FFFFFF&&!(hn_read16(m,0x04000000)&0x80)){
            uint32_t value=hn_read16(m,SCRATCH);
            hn_transfer_queue_fill(m,value|(value<<16),destination,hn_read16(m,SCRATCH+2));
        }else transfer16(m,SCRATCH,destination,hn_read16(m,SCRATCH+2),true);
        advance(m,9);break;
    }
    case 0x19:case 0x1A:case 0x1B:case 0x38:case 0x54:
        if(!hn_task_main_command(m,op,args))return fail(s,HN_SCRIPT_RESOURCE,op,args);break;
    case 0x1C: {
        scratch(m,args,0,4);GameAddr address=hn_read32(m,SCRATCH);
        HNMainFunction function=NULL;
        for(size_t i=0;i<s->callback_count;i++)if((s->callbacks[i].address&~1u)==(address&~1u)){
            function=s->callbacks[i].function;break;
        }
        if(!function)for(size_t i=0;i<sizeof(builtins)/sizeof(*builtins);i++)
            if(builtins[i].address==(address&~1u)){function=builtins[i].function;break;}
        uint32_t consumed=0;
        if(function)consumed=function(m,args+4);
        else{
            HNSceneResult result=hn_scene_callback(m,address,args+4,&consumed);
            if(result==HN_SCENE_UNKNOWN)return fail(s,HN_SCRIPT_CALLBACK,op,address);
            if(result==HN_SCENE_RESOURCE)return fail(s,HN_SCRIPT_RESOURCE,op,address);
        }
        advance(m,consumed+4);break;
    }
    case 0x1D: {
        unsigned result=hn_read16(m,RESULT),count=hn_read8(m,args);
        if(!result)advance(m,count*4+1);
        else{scratch(m,args+1+(result-1)*4,0,4);hn_write32(m,CURSOR,hn_read32(m,SCRATCH));advance(m,0);}
        break;
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
    case 0x20:{
        unsigned leader=hn_read8(m,args),follower=hn_read8(m,args+1),control=hn_read8(m,args+2);
        unsigned secondary=hn_read8(m,args+3),flags=hn_read8(m,args+4);
        scratch(m,args+5,0,8);unsigned width=hn_read16(m,SCRATCH),height=hn_read16(m,SCRATCH+2);
        GameAddr resource=resolve_variable(m,4);scratch(m,args+13,0,4);GameAddr parameters=resolve_variable(m,0);
        if(!hn_motion_configure(m,leader,follower,control,secondary,flags&31,width,height,resource,parameters))
            return fail(s,HN_SCRIPT_RESOURCE,op,args);
        if(!(flags&64))hn_write8(m,0x030021E0,0);
        if(!(flags&128)&&!hn_motion_rebuild_route(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        advance(m,17);break;
    }
    case 0x21:{
        if(!hn_read8(m,INITIALIZED)){hn_motion_reset(m);hn_write8(m,INITIALIZED,1);}
        if(!hn_motion_advance(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        unsigned status=hn_read8(m,0x03001FA8);
        if(status==16||(status&3)==2){advance(m,0);hn_write8(m,CONTROLS+18,0);}
        else hn_write8(m,YIELDED,1);break;
    }
    case 0x24:case 0x45:
        if(!hn_choice_main_command(m,op,args))return fail(s,HN_SCRIPT_RESOURCE,op,args);break;
    case 0x22:case 0x23: {
        scratch(m,args,0,8);GameAddr expression=resolve_variable(m,0);
        GameAddr target=op==0x22?resolve_variable(m,4):0;uint32_t value;
        if(!hn_expression_evaluate(m,expression,op==0x23,&value))return fail(s,HN_SCRIPT_RESOURCE,op,expression);
        if(op==0x22&&value){hn_write32(m,CURSOR,target);advance(m,0);}
        else advance(m,op==0x22?8:4);
        break;
    }
    case 0x25:
        scratch(m,args+1,0,4);hn_write32(m,HN_MAP_WORDS+hn_read8(m,args)*4,resolve_variable(m,0));
        advance(m,5);break;
    case 0x26:
        scratch(m,args+1,0,4);resolve_variable(m,0);
        if(hn_read32(m,SCRATCH)==UINT32_MAX)hn_write32(m,SCRATCH,0);
        if(!hn_actor_configure(m,hn_read8(m,args),hn_read32(m,SCRATCH)))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        advance(m,5);hn_write8(m,YIELDED,1);break;
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
    case 0x2C: {
        unsigned layer=hn_read8(m,args),flags=hn_read8(m,args+1);scratch(m,args+2,0,8);
        if(layer>=4)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        GameAddr bounds=HN_BACKGROUND_BOUNDS+layer*12;
        hn_write16(m,bounds,hn_read16(m,SCRATCH));hn_write16(m,bounds+4,hn_read16(m,SCRATCH+2));
        hn_write16(m,bounds+2,hn_read16(m,SCRATCH+4));hn_write16(m,bounds+6,hn_read16(m,SCRATCH+6));
        hn_write8(m,bounds+8,(uint8_t)flags);advance(m,10);break;
    }
    case 0x2D:
        scratch(m,args,0,3);hn_write8(m,CONTROLS+hn_read16(m,SCRATCH),hn_read8(m,SCRATCH+2));advance(m,3);break;
    case 0x2E:
        scratch(m,args,0,4);hn_write16(m,CLASSES+hn_read16(m,SCRATCH)*2,hn_read16(m,SCRATCH+2));advance(m,4);break;
    case 0x2F: {
        unsigned width=hn_read8(m,args);scratch(m,args+1,0,6);
        unsigned source=hn_read16(m,SCRATCH),destination=hn_read16(m,SCRATCH+2);
        if(width)hn_bios_cpu_set(m,CLASSES+source*2,CLASSES+destination*2,hn_read16(m,SCRATCH+4));
        else for(unsigned i=0;i<hn_read16(m,SCRATCH+4)&&!m->fault;i++)
            hn_write8(m,CONTROLS+destination+i,hn_read8(m,CONTROLS+source+i));
        advance(m,7);break;
    }
    case 0x30:case 0x31:case 0x32: {
        unsigned width=hn_read8(m,args);scratch(m,args+1,0,8);GameAddr target=resolve_variable(m,4);
        unsigned index=hn_read16(m,SCRATCH);
        unsigned left=width?hn_read16(m,CLASSES+index*2):hn_read8(m,CONTROLS+index);
        unsigned right=width?hn_read16(m,SCRATCH+2):hn_read8(m,SCRATCH+2);
        bool take=op==0x30?left==right:op==0x31?left>=right:left<right;
        if(take){hn_write32(m,CURSOR,target);advance(m,0);}else advance(m,9);
        break;
    }
    case 0x33: {
        unsigned desired=hn_read8(m,args);scratch(m,args+1,2,6);GameAddr target=resolve_variable(m,4);
        unsigned flag=hn_read16(m,SCRATCH+2),value=(hn_read8(m,0x03002180+(flag>>3))>>(flag&7))&1;
        if((value!=0)==(desired!=0)){hn_write32(m,CURSOR,target);advance(m,0);}else advance(m,7);
        break;
    }
    case 0x34:case 0x35: {
        scratch(m,args,0,8);GameAddr source=resolve_variable(m,0),destination=resolve_variable(m,4);
        scratch(m,args+8,0,6);unsigned width=hn_read16(m,SCRATCH),rows=hn_read16(m,SCRATCH+2),stride=hn_read16(m,SCRATCH+4);
        if((uint32_t)width*rows>0x40000u)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        bool allocated=op==0x35||(hn_read16(m,0x04000000)&0x80);
        GameAddr temporary=allocated?hn_heap_allocate(m,hn_read32(m,source)>>8,true):
            destination+(destination>0x05FFFFFF?0xFC004720u:0xFD01C720u);
        if(!temporary||!decompress(m,source,temporary)){
            if(allocated&&temporary)hn_heap_free(m,temporary);
            return fail(s,HN_SCRIPT_RESOURCE,op,source);
        }
        for(unsigned i=0;i<rows&&!m->fault;i++){
            if(op==0x35)hn_bios_cpu_set(m,temporary+i*width,destination+i*stride,(width>>1)&0xFFFFu);
            else if(allocated)transfer16(m,temporary+i*width,destination+i*stride,width,false);
            else hn_transfer_queue(m,temporary+i*width,destination+i*stride,width);
        }
        if(allocated)hn_heap_free(m,temporary);advance(m,14);break;
    }
    case 0x36:
        scratch(m,args,0,3);hn_write32(m,SCRATCH+4,hn_heap_allocate(m,hn_read16(m,SCRATCH),true));
        hn_write32(m,VARIABLES+hn_read8(m,SCRATCH+2)*4,hn_read32(m,SCRATCH+4));advance(m,3);break;
    case 0x37:
        hn_heap_free(m,hn_read32(m,VARIABLES+hn_read8(m,args)*4));advance(m,1);break;
    case 0x39:case 0x3B:case 0x4B:case 0x4F: {
        unsigned index=hn_read8(m,args);uint8_t value=hn_read8(m,args+1);
        if(op==0x3B)hn_write8(m,hn_render_slot(m,index)+0x60,value);
        else hn_render_flags(m,index,op==0x39?12:op==0x4B?3:32,(uint8_t)(value<<(op==0x39?2:op==0x4F?5:0)));
        advance(m,2);break;
    }
    case 0x3A:case 0x44: {
        GameAddr slot=hn_render_slot(m,hn_read8(m,args));scratch(m,args+1,0,6);
        for(unsigned i=0;i<3;i++){
            uint32_t half=hn_read16(m,SCRATCH+i*2),value;
            if(op==0x3A)value=(half<<16)+0x8000;
            else{value=half<<8;if(value&0x00800000u)value|=0xFF000000u;}
            hn_write32(m,slot+(op==0x3A?0x18:0x24)+i*4,value);
        }
        advance(m,7);break;
    }
    case 0x3C: {
        unsigned index=hn_read8(m,args);scratch(m,args+1,0,8);
        GameAddr animation=resolve_variable(m,0),target=resolve_variable(m,4);
        if(hn_read32(m,hn_render_slot(m,index)+4)==animation){hn_write32(m,CURSOR,target);advance(m,0);}
        else advance(m,9);break;
    }
    case 0x3D:
        if(!hn_motion_rebuild_route(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);advance(m,0);break;
    case 0x3E: {
        unsigned index=hn_read8(m,args),kind=hn_read8(m,args+1),shape=hn_read8(m,args+2),value=hn_read8(m,args+3);
        unsigned packed=((kind&3)<<6)|((shape&3)<<4);GameAddr slot=hn_render_slot(m,index);
        if(kind<=1)packed|=value&15;else hn_write8(m,slot+0x65,(uint8_t)value);
        hn_write8(m,slot+0x62,(uint8_t)packed);advance(m,4);break;
    }
    case 0x3F: {
        uint8_t step=hn_read8(m,args),phase=hn_read8(m,HN_BLEND+6);
        if(!hn_read8(m,INITIALIZED)){hn_write8(m,INITIALIZED,1);hn_video_forced_blank(m,0);}
        if(step&128)step=(uint8_t)(0u-step);
        if(phase==128)advance(m,1);
        else{
            if(step==128)phase=128;
            else if(!(phase&128)){phase=(uint8_t)(phase+step);if(phase>128)phase=128;}
            else{phase=(uint8_t)(phase-step);if(!(phase&128))phase=128;}
            hn_video_fade_phase(m,phase);hn_write8(m,YIELDED,1);
        }
        break;
    }
    case 0x40: {
        uint8_t step=hn_read8(m,args),phase=hn_read8(m,HN_BLEND+6);unsigned value;
        if(step&128){
            step=(uint8_t)(0u-step);
            if(phase<=step){advance(m,1);break;}value=phase-step;
        }else{
            value=phase+step;
            if(value>255){hn_video_forced_blank(m,1);advance(m,1);break;}
        }
        hn_video_fade_phase(m,(uint8_t)value);hn_write8(m,YIELDED,1);break;
    }
    case 0x41:case 0x42: {
        scratch(m,args,1,7);GameAddr source=resolve_variable(m,4);
        unsigned width=op==0x41?1:2;GameAddr destination=(width==1?CONTROLS:CLASSES)+hn_read16(m,SCRATCH+2)*width;
        /* Original forward-copy semantics include overlap and the u8 count. */
        for(unsigned i=0;i<hn_read8(m,SCRATCH+1)&&!m->fault;i++){
            if(width==1)hn_write8(m,destination+i,hn_read8(m,source+i));
            else hn_write16(m,destination+i*2,hn_read16(m,source+i*2));
        }
        advance(m,7);break;
    }
    case 0x43:case 0x60: {
        unsigned group=op==0x43?hn_read8(m,args):1;
        bool ok=op==0x43?hn_save_load_group(m,group):hn_save_load_groups(m);
        if(!ok)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        if(group==1){
            if(hn_read8(m,0x03002169)){
                for(unsigned i=0;i<512;i++)hn_write16(m,0x05000000+i*2,0x7FFF);
                if(!hn_map_initialize(m,hn_read8(m,HN_MAP_BANK),hn_read8(m,HN_MAP_INDEX)))
                    return fail(s,HN_SCRIPT_RESOURCE,op,args);
            }else{hn_write32(m,DEPTH,0);advance(m,op==0x43?1:0);}
        }else advance(m,1);
        break;
    }
    case 0x46: {
        scratch(m,args,0,3);unsigned flag=hn_read16(m,SCRATCH),bit=1u<<(flag&7);
        GameAddr byte=0x03002180+(flag>>3);unsigned value=hn_read8(m,byte);
        hn_write8(m,byte,(uint8_t)(hn_read8(m,SCRATCH+2)?value|bit:value&~bit));advance(m,3);break;
    }
    case 0x47:case 0x48:
        hn_mosaic_configure(m,hn_read8(m,args),0);
        if(op==0x47)hn_mosaic_square(m,(uint8_t)(hn_read8(m,args+1)<<4));
        else hn_mosaic_shared(m,(uint8_t)(hn_read8(m,args+1)<<4),(uint8_t)(hn_read8(m,args+2)<<4));
        advance(m,op==0x47?2:3);break;
    case 0x49:
        hn_write8(m,0x03001F60,hn_read8(m,args));hn_write8(m,0x03001F90,hn_read8(m,args+1));advance(m,2);break;
    case 0x4A:
        if(!hn_render_reorder(m,hn_read8(m,args),hn_read8(m,args+1),hn_read8(m,args+2)))
            return fail(s,HN_SCRIPT_RESOURCE,op,args);
        advance(m,3);break;
    case 0x4C: {
        uint8_t mode=hn_read8(m,args);if(mode<=5)hn_video_set_mode(m,mode);advance(m,1);break;
    }
    case 0x4D:
        scratch(m,args+1,0,4);hn_write32(m,0x030021F0+hn_read8(m,args)*4,resolve_variable(m,0));advance(m,5);break;
    case 0x4E:{
        scratch(m,args,0,4);uint32_t callback=resolve_variable(m,0);
        if(callback==UINT32_MAX){callback=0;hn_write32(m,SCRATCH,0);}
        hn_write32(m,0x03002B90,callback);advance(m,4);break;
    }
    case 0x50:{
        for(unsigned i=1;i<=7;i++)hn_write8(m,SCRATCH+i,hn_read8(m,args+i-1));
        unsigned destination=hn_read16(m,SCRATCH+2);
        uint16_t value=hn_random_range(m,hn_read16(m,SCRATCH+4),hn_read16(m,SCRATCH+6));
        if(hn_read8(m,SCRATCH+1))hn_write16(m,CLASSES+destination*2,value);
        else hn_write8(m,CONTROLS+destination,(uint8_t)value);advance(m,7);break;
    }
    case 0x51:{
        scratch(m,args,0,4);GameAddr target=hn_read32(m,SCRATCH);
        if(target<=15){target=VARIABLES+target*4;hn_write32(m,SCRATCH,hn_read32(m,target));}
        /* The original <=15 route calls the slot address itself. It is not a
           second ordinary callback-pointer lookup. Only reviewed ROM leaves
           are accepted here; their void contract does not consume a return. */
        switch(target&~1u){
        case 0x08043118:case 0x0804311C:case 0x08043120:break;
        case 0x0802C874:{
            unsigned count=0;
            /* The source reads index 31 before excluding it from the count. */
            for(unsigned i=0;i<32;i++){
                unsigned value=hn_read8(m,HN_RECORD_COUNTERS+i);
                if(i<31&&value&&value!=255)count++;
            }
            hn_write8(m,CONTROLS+0x60,(uint8_t)count);break;
        }
        default:return fail(s,HN_SCRIPT_CALLBACK,op,target);
        }
        advance(m,4);break;
    }
    case 0x52:
        hn_video_forced_blank(m,hn_read8(m,args));hn_write8(m,YIELDED,1);advance(m,1);break;
    case 0x53:
        scratch(m,args,2,6);hn_write32(m,VARIABLES+hn_read16(m,SCRATCH+2)*4,hn_read32(m,SCRATCH+4));advance(m,6);break;
    case 0x55:
        hn_write8(m,0x03002170,1);hn_write8(m,0x03001FB8,hn_read8(m,args));advance(m,1);break;
    case 0x56: {
        if(!hn_audio_ready(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        scratch(m,args,0,4);uint16_t song=hn_read16(m,SCRATCH);unsigned mode=hn_read8(m,SCRATCH+2),fade=hn_read8(m,SCRATCH+3);
        GameAddr p0=hn_read32(m,HN_AUDIO_PLAYER_POINTERS),p1=hn_read32(m,HN_AUDIO_PLAYER_POINTERS+4);
        if(!p0||!p1)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        bool ok=true;
        if(mode){
            if(fade)hn_audio_fade(m,p0,(uint16_t)fade,0);else ok=hn_audio_player_stop(m,p0);
            if(song)ok=ok&&hn_audio_song(m,song,HN_SONG_CONTINUE);
            else if(fade&&!(hn_read8(m,0x03002180)&8)&&hn_read8(m,HN_AUDIO_FADE_BYTES+1))hn_audio_fade(m,p1,(uint16_t)fade,2);
            else ok=ok&&hn_audio_song(m,hn_read16(m,CLASSES+0x1E),HN_SONG_CONTINUE);
            hn_write8(m,HN_AUDIO_FADE_BYTES+1,0);
        }else if(fade){hn_audio_fade(m,p1,(uint16_t)fade,1);ok=hn_audio_song(m,song,HN_SONG_START);hn_write8(m,HN_AUDIO_FADE_BYTES+1,1);}
        else ok=hn_audio_player_stop(m,p1)&&hn_audio_song(m,song,HN_SONG_START);
        if(!ok)return fail(s,HN_SCRIPT_RESOURCE,op,args);advance(m,4);break;
    }
    case 0x57: {
        if(!hn_audio_ready(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        unsigned mode=hn_read8(m,args),mask=hn_read8(m,args+1);if(mode>1)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        if(mask==255){if(!hn_audio_all(m,!mode))return fail(s,HN_SCRIPT_RESOURCE,op,args);}
        else for(unsigned i=0;i<HN_AUDIO_PLAYERS;i++)if(mask&(1u<<i)){
            GameAddr player=hn_read32(m,HN_AUDIO_PLAYER_POINTERS+i*4);if(!player)return fail(s,HN_SCRIPT_RESOURCE,op,args);
            if(!mode)hn_audio_player_resume(m,player);else if(!hn_audio_player_stop(m,player))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        }
        advance(m,2);break;
    }
    case 0x58:
        if(!hn_save_write_record(m,hn_read8(m,args),hn_read8(m,args+1)))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        advance(m,2);break;
    case 0x59:
        hn_write8(m,0x03002C0C,hn_read8(m,args));hn_write8(m,0x03001F50,hn_read8(m,args+1));
        hn_write8(m,0x03002C04,hn_read8(m,args+2));hn_write8(m,0x03002C00,hn_read8(m,args+3));advance(m,4);break;
    case 0x5C: {
        if(!hn_audio_ready(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        scratch(m,args,0,8);uint16_t song=hn_read16(m,SCRATCH),mask=hn_read16(m,SCRATCH+2),volume=hn_read16(m,SCRATCH+4),tempo=hn_read16(m,SCRATCH+6);
        scratch(m,args+8,0,3);unsigned index;GameAddr player=audio_player(m,song,&index);if(!player)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        hn_audio_tempo(m,player,tempo);hn_audio_track_parameter(m,player,mask,volume,0);
        hn_audio_track_parameter(m,player,mask,(uint32_t)(int16_t)hn_read16(m,SCRATCH),1);
        hn_audio_track_parameter(m,player,mask,(uint32_t)(int8_t)hn_read8(m,SCRATCH+2),2);advance(m,11);break;
    }
    case 0x5D: {
        unsigned value=hn_read8(m,0x030021E0),mask=hn_read8(m,args);
        hn_write8(m,0x030021E0,(uint8_t)(hn_read8(m,args+1)?value|mask:value&~mask));advance(m,2);break;
    }
    case 0x5E: {
        if(!hn_audio_ready(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        scratch(m,args,0,3);uint16_t song=hn_read16(m,SCRATCH);unsigned condition=hn_read8(m,SCRATCH+2),index;
        GameAddr player=audio_player(m,song,&index);if(!player)return fail(s,HN_SCRIPT_RESOURCE,op,args);
        uint32_t state=hn_read32(m,player+4);
        if(condition)condition=state&0x80000000u;else if((state&0x8000FFFFu)==0x80000000u)condition=1;
        if(condition||hn_read32(m,HN_AUDIO_SONG_RECORDS+song*8)!=hn_read32(m,player))hn_write8(m,OPCODE,0);
        else hn_write8(m,YIELDED,1);
        hn_write32(m,CONSUMED,3);break;
    }
    case 0x5A:
        if(!hn_audio_ready(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        hn_audio_dma_pause(m);hn_core_reset(m);hn_audio_dma_resume(m);advance(m,0);break;
    case 0x5B:hn_write16(m,0x030021E4,hn_read16(m,args));advance(m,2);break;
    case 0x5F:
        if(!hn_save_write_groups(m))return fail(s,HN_SCRIPT_RESOURCE,op,args);
        advance(m,0);break;
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
