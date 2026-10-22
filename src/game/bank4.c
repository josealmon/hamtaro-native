#include "game/bank4.h"
#include "game/state.h"
#include "game/render.h"
#include "game/input.h"
#include "game/random.h"
#include "game/background.h"
#include "game/graphics.h"
#include "game/audio.h"
#include "gba/bios.h"

enum { JUMP_POSITIONS=0x0867039C,COUNTER_KINDS=0x08670410,COUNTER_X=0x0867041A,
       COUNTER_INITIAL=0x0867044C,COUNTER_SELECTED=0x08670460,COUNTER_COMPLETED=0x08670478 };
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint32_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static int32_t signed_word(HNMemory *m,GameAddr address){return (int32_t)hn_read32(m,address);}
static bool animation(HNMemory *m,uint8_t index,GameAddr image)
{
    if(index==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,index,image);return !m->fault;
}
static bool flag(HNMemory *m,unsigned id){return ((hn_read8(m,0x03002180+(id>>3))>>(id&7))&1)!=0;}
static void set_flag(HNMemory *m,unsigned id,bool value)
{GameAddr a=0x03002180+(id>>3);uint8_t bit=(uint8_t)(1u<<(id&7));hn_write8(m,a,(uint8_t)((hn_read8(m,a)&~bit)|(value?bit:0)));}
static void pending_script(HNMemory *m,uint16_t value)
{hn_write8(m,0x030021E0,hn_read8(m,0x030021E0)|4);hn_write16(m,0x03002A80,value);}
static bool trigger_jump(HNMemory *m,bool *triggered)
{
    GameAddr first=hn_render_slot(m,10),second=hn_render_slot(m,13);*triggered=false;
    if(hn_read32(m,first+4)!=0x088B134B&&hn_read32(m,second+4)!=0x088B82A5&&hn_input_query(m,1,1)){
        uint32_t choice=control(m,0x1A5);*triggered=true;
        return choice==1||choice>3?animation(m,10,0x088B134B):animation(m,13,0x088B82A5);
    }
    return !m->fault;
}
static void jump_timer(HNMemory *m,uint8_t limit)
{uint32_t next=control(m,0x60)+1;store(m,0x60,next);store(m,0x61,(uint8_t)next>limit);}
static bool prepare_jump(HNMemory *m)
{
    GameAddr primary=hn_render_slot(m,30),secondary=hn_render_slot(m,31);
    uint32_t old=control(m,0x1A5);int32_t current=control(m,0),previous=control(m,0x1A6);
    uint32_t raw_stage=control(m,0x1A9),next=0;if(raw_stage>11)raw_stage=11;
    uint8_t stage=(uint8_t)(raw_stage>>1);int32_t duration=(uint8_t)(96-stage*8);
    uint32_t old_offset=old*8;
    if(previous!=current){
        unsigned attempts=0;do{if(attempts++>=4096)return false;next=(uint8_t)hn_random_range(m,1,6);}while(next==old);
        if(next<=2)store(m,0x1A7,next|control(m,0x1A7));
        if(current-previous<=2){uint32_t value=control(m,0x1A7),low=value&3;
            if(low!=3){next=(low&1)?2:1;store(m,0x1A7,next|value);}}
    }
    int32_t dx=(int32_t)(hn_read32(m,JUMP_POSITIONS+next*8)-hn_read32(m,JUMP_POSITIONS+old_offset));
    int32_t dz=(int32_t)(hn_read32(m,JUMP_POSITIONS+next*8+4)-hn_read32(m,JUMP_POSITIONS+old_offset+4));
    int32_t adjusted=old==0?(int32_t)((uint32_t)dz-0x1C0000):next==0?(int32_t)((uint32_t)dz+0x180000):dz;
    int32_t speed=hn_runtime_divide(dx,duration);
    hn_write32(m,primary+0x24,(uint32_t)speed);hn_write32(m,primary+0x2C,(uint32_t)hn_runtime_divide(adjusted,duration));
    hn_write32(m,primary+0x28,0xFFFB0000);hn_write32(m,primary+0x34,(uint32_t)(int32_t)((32.0/duration)*20480.0));
    hn_write32(m,secondary+0x24,(uint32_t)speed);hn_write32(m,secondary+0x2C,(uint32_t)hn_runtime_divide(dz,duration));
    store(m,0x1A5,next);store(m,0x1AA,stage);return !m->fault;
}
bool hn_bank4_distance_volume(HNMemory *m,uint32_t player_word,uint32_t song_word,int32_t distance,
                              int32_t limit,uint32_t from_word,uint32_t to_word)
{
    uint8_t player=(uint8_t)player_word;uint16_t volume=(uint16_t)from_word,to=(uint16_t)to_word;
    if(distance>limit)volume=0;
    else{int32_t product=(int32_t)((uint32_t)((int32_t)volume-to)*(uint32_t)distance);
        volume=(uint16_t)((uint32_t)volume-(uint32_t)hn_runtime_divide(product,limit));}
    if(!hn_audio_song(m,(uint16_t)song_word,HN_SONG_CHANGE))return false;
    hn_audio_track_parameter(m,hn_read32(m,0x020016BC+player*4),255,volume,0);return !m->fault;
}
uint32_t hn_bank4_projected_position(HNMemory *m,GameAddr slot)
{
    int32_t x=(int32_t)(hn_read32(m,slot+0x18)+hn_read32(m,slot+0x24));
    int32_t z=(int32_t)(hn_read32(m,slot+0x20)+hn_read32(m,slot+0x2C));
    if(x>0x388000&&z<=0x887FFF)return 1;
    if(x< -0x288000)pending_script(m,7);return 0;
}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_bank4_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x0801F7BC: /* Shared indicator dependency; it does not change animation. */
        hn_write8(m,0x03001FAC,10);hn_write32(m,0x03001F7C,0x088A5834);break;
    case 0x08068B20:REQUIRE(prepare_jump(m));break;
    case 0x08068CB8:case 0x08068D60:{
        uint8_t limit=(callback&~1u)==0x08068D60?hn_read8(m,arguments):(uint8_t)(96-control(m,0x1AA)*8);
        bool triggered;REQUIRE(trigger_jump(m,&triggered));jump_timer(m,limit);
        if((callback&~1u)==0x08068D60)*consumed=1;break;
    }
    case 0x08068DF8:{bool triggered;REQUIRE(trigger_jump(m,&triggered));break;}
    case 0x08068E68:{
        uint8_t limit=(uint8_t)(96-control(m,0x1AA)*8);
        if(!control(m,0x1A4)){bool triggered;REQUIRE(trigger_jump(m,&triggered));
            if(triggered){uint32_t count=control(m,0x60);if((int32_t)count>=(int32_t)limit-16&&count<limit)store(m,0x1A4,1);}}
        jump_timer(m,limit);break;
    }
    case 0x08069E60:
        hn_write16(m,0x03001FBC,signed_word(m,hn_render_slot(m,10)+0x20)>signed_word(m,hn_render_slot(m,16)+0x20));break;
    case 0x08069FE8:
        REQUIRE(hn_bios_lz77(m,0x080EE48C,0x02004720,0x40000));
        for(unsigned i=0;i<26;i++)for(unsigned row=0;row<4;row++)hn_write16(m,0x02004720+0x44+row*64+i*2,0xA3);break;
    case 0x0806A034:{
        uint8_t index=0;
        while(index<control(m,0x1A3)){
            uint8_t slot=(uint8_t)(index+30);uint32_t offset=control(m,0xEB)*10+index;
            REQUIRE(hn_render_configure_raw(m,slot,1,116,hn_read8(m,COUNTER_X+offset),0,176,
                hn_read32(m,COUNTER_INITIAL+hn_read8(m,COUNTER_KINDS+index)*4)));
            hn_render_flags(m,slot,12,0);index++;
        }
        uint8_t slot=(uint8_t)(index+30);uint32_t offset=control(m,0xEB)*10+index;
        REQUIRE(hn_render_configure_raw(m,slot,1,116,hn_read8(m,COUNTER_X+offset),0,224,
            hn_read32(m,COUNTER_SELECTED+control(m,0x69)*4)));hn_render_flags(m,slot,12,0);
        for(index++;index<=9;index++)hn_write8(m,hn_render_slot(m,(uint8_t)(index+30))+0x60,0);
        REQUIRE(hn_background_configure_raw(m,4,0x84,0x088EC605));break;
    }
    case 0x0806A140:{
        uint32_t index=control(m,0x1A3),selected=control(m,0x69);
        if(hn_read8(m,COUNTER_KINDS+index)==selected){uint32_t next=index+1;store(m,0x1A3,next);
            if((uint8_t)next==10){store(m,0x69,255|selected);store(m,0x1A3,0);}}
        else{
            REQUIRE(hn_graphics_initialize_raw(m,4,1,0x088EC709));uint8_t i;
            for(i=0;i<control(m,0x1A3);i++)REQUIRE(animation(m,(uint8_t)(i+30),hn_read32(m,COUNTER_COMPLETED+hn_read8(m,COUNTER_KINDS+i)*4)));
            REQUIRE(animation(m,(uint8_t)(i+30),hn_read32(m,COUNTER_COMPLETED+control(m,0x69)*4)));store(m,0x1A3,0);
        }break;
    }
    case 0x0806AF68:
        hn_audio_track_parameter(m,0x03004E90,3,control(m,0x60)*30,1);
        REQUIRE(hn_audio_song(m,27,HN_SONG_CHANGE));store(m,0x60,control(m,0x60)+1);break;
    case 0x0806AF9C:
        hn_audio_track_parameter(m,0x03004E90,255,(uint16_t)(256-control(m,0x60)*8),0);store(m,0x60,control(m,0x60)+1);break;
    case 0x0806BAA0:
        *consumed=signed_word(m,hn_render_slot(m,hn_read8(m,arguments))+0x20)<signed_word(m,hn_render_slot(m,hn_read8(m,0x03002C04))+0x20)?6:1;break;
    case 0x0806BBE0:{
        /* Snapshot all six bytes before an aliased destination can overwrite them. */
        uint8_t payload[6];for(unsigned i=0;i<6;i++)payload[i]=hn_read8(m,arguments+i);
        GameAddr destination=(uint32_t)payload[0]|((uint32_t)payload[1]<<8)|((uint32_t)payload[2]<<16)|((uint32_t)payload[3]<<24);
        hn_write16(m,destination,(uint16_t)(payload[4]|(payload[5]<<8)));*consumed=6;break;
    }
    case 0x0806BC10:{
        GameAddr target=hn_render_slot(m,hn_read8(m,arguments)),primary=hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER));
        uint32_t offset=(uint8_t)hn_random_range(m,7,20)<<19;int32_t z=(int32_t)(offset+0x8000);hn_write32(m,target+0x20,(uint32_t)z);
        if(signed_word(m,primary+0x18)<=0x287FFF){int32_t primary_z=signed_word(m,primary+0x20);
            if(z<primary_z){if(primary_z<(int32_t)(offset+0x188000))hn_write32(m,target+0x20,offset-0x178000);}
            else if(z<(int32_t)((uint32_t)primary_z+0x180000))hn_write32(m,target+0x20,offset+0x188000);}
        *consumed=1;break;
    }
    case 0x0806BC84:{
        GameAddr target=hn_render_slot(m,hn_read8(m,arguments));uint32_t value=(uint8_t)hn_random_range(m,7,20);
        hn_write32(m,target+0x20,(value<<19)+0x8000);value=(uint8_t)hn_random_range(m,5,value<=16?7:17);
        hn_write32(m,target+0x18,(value<<19)+0x8000);*consumed=1;break;
    }
    case 0x0806BCD8:hn_write32(m,hn_render_slot(m,19)+0x18,0x8000);break;
    case 0x0806BDE8:{GameAddr slot=hn_render_slot(m,10);for(unsigned i=0;i<3;i++)hn_write32(m,slot+0x24+i*4,0);break;}
    case 0x0806BE00:case 0x0806BE94:{
        uint8_t duration=hn_read8(m,arguments);uint32_t x,y;
        if((callback&~1u)==0x0806BE00){x=((uint32_t)hn_read16(m,arguments+1)<<16)+0x8000;y=((uint32_t)hn_read16(m,arguments+3)<<16)+0x8000;
            hn_write32(m,0x03001144,hn_read32(m,0x03002E40));hn_write32(m,0x03001148,hn_read32(m,0x03002E44));*consumed=5;}
        else{x=0;y=0;*consumed=1;}
        REQUIRE(hn_background_configure_raw(m,5,0x83,0x088ED453));
        if((callback&~1u)==0x0806BE94)x=hn_read32(m,0x03001144);
        hn_write32(m,hn_read32(m,0x03002E08)+5*80+12,(uint32_t)hn_runtime_divide((int32_t)(x-hn_read32(m,0x03002E40+44)),duration));
        if((callback&~1u)==0x0806BE94)y=hn_read32(m,0x03001148);
        hn_write32(m,hn_read32(m,0x03002E08)+5*80+4,(uint32_t)hn_runtime_divide((int32_t)(y-hn_read32(m,0x03002E44+44)),duration));break;
    }
    case 0x0806C484:{
        GameAddr first=hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER)),second=hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER));
        uint32_t first_z=hn_read32(m,first+0x20),second_z=hn_read32(m,second+0x20);
        int32_t delta=(int32_t)(0x488000-first_z);uint32_t a=delta>=0?(uint32_t)delta:first_z-0x488000;
        delta=(int32_t)(0x488000-second_z);uint32_t b=delta>=0?(uint32_t)delta:second_z-0x488000;
        uint32_t extent=(b<a?a:b)>>16;hn_write16(m,0x0201DC40+0x2A*2,(uint16_t)extent);hn_write16(m,0x0201DC40+0x2B*2,(uint16_t)extent);break;
    }
    case 0x0806C590:{
        GameAddr primary=hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER)),other=hn_render_slot(m,hn_read8(m,0x03002C00));
        bool after=(int32_t)(hn_read32(m,other+0x20)+0x180000)<=signed_word(m,primary+0x20);
        REQUIRE(animation(m,hn_read8(m,0x03002C00),after?0x088C6CEE:0x088C6D1B));
        REQUIRE(animation(m,22,after?0x088C787E:0x088C7886));break;
    }
    case 0x0806C5F8:
        if(signed_word(m,hn_render_slot(m,29)+0x20)>0x11FFFFF)REQUIRE(animation(m,29,0x088ED80F));break;
    default:return hn_bank4_preset_callback(m,callback,consumed);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank4_task(HNMemory *m,GameAddr callback)
{
    HNSceneResult preset=hn_bank4_presets(m,callback);if(preset!=HN_SCENE_UNKNOWN)return preset;
    switch(callback&~1u){
    case 0x0801F7BC:
    case 0x08068B20:case 0x08068CB8:case 0x08068DF8:case 0x08068E68:case 0x08069E60:
    case 0x08069FE8:case 0x0806A034:case 0x0806A140:case 0x0806AF68:case 0x0806AF9C:
    case 0x0806BCD8:case 0x0806BDE8:case 0x0806C484:case 0x0806C590:case 0x0806C5F8:{
        uint32_t ignored;return hn_bank4_callback(m,callback,0,&ignored);
    }
    case 0x0806E628:
        for(unsigned i=0;i<2;i++){
            uint8_t index=hn_read8(m,HN_RENDER_CLASS_LEADER);
            GameAddr image=hn_read32(m,0x0200152C+((uint32_t)hn_read8(m,hn_render_slot(m,index)+0x60)-1)*4);
            if(!i)hn_render_set_animation(m,index,image);
            else{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,image);}
        }break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_bank4_animation(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,
                                uint32_t *consumed,uint32_t *yield)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x0806A254:{
        GameAddr leader=hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER));REQUIRE(hn_audio_song(m,44,HN_SONG_CHANGE));
        if(signed_word(m,leader+0x20)>0x7FFFFF)hn_audio_track_parameter(m,0x03004E90,255,256,0);*yield=1;break;
    }
    case 0x0806A29C:REQUIRE(hn_audio_song(m,44,HN_SONG_CHANGE));hn_audio_track_parameter(m,0x03004E90,255,128,0);*yield=1;break;
    case 0x0806BAD8:*yield=1;break;
    case 0x0806BAE0:case 0x0806BB60:{
        uint8_t index=hn_read8(m,arguments),player=hn_read8(m,arguments+1);uint16_t song=hn_read16(m,arguments+2);
        int32_t reference=(int32_t)(((uint32_t)hn_read16(m,arguments+4)<<16)+0x8000);
        int32_t limit=(int32_t)(((uint32_t)hn_read16(m,arguments+6)<<16)+0x8000);
        uint16_t from=hn_read16(m,arguments+8),to=hn_read16(m,arguments+10);
        int32_t coordinate=signed_word(m,hn_render_slot(m,index)+((callback&~1u)==0x0806BAE0?0x18:0x20));
        int32_t distance=reference<coordinate?(int32_t)((uint32_t)coordinate-(uint32_t)reference):(int32_t)((uint32_t)reference-(uint32_t)coordinate);
        REQUIRE(hn_bank4_distance_volume(m,player,song,distance,limit,from,to));*yield=1;*consumed=12;break;
    }
    case 0x0806C4F8:{
        GameAddr other=hn_render_slot(m,10);int32_t dx=(int32_t)(hn_read32(m,other+0x18)-hn_read32(m,slot+0x18));
        if(dx<0)dx=(int32_t)(0u-(uint32_t)dx);int32_t dz=(int32_t)(hn_read32(m,other+0x20)-hn_read32(m,slot+0x20));
        /* The second sign test uses the adjusted dx, including INT_MIN. */
        if(dx<0)dz=(int32_t)(0u-(uint32_t)dz);
        if(dz<=0x17FFFF&&dx<=0x27FFFF)hn_write32(m,slot+0x24,0);
        else{int32_t velocity=signed_word(m,slot+0x24),x=signed_word(m,slot+0x18);
            if(!velocity||x<=0x587FFF)hn_write32(m,slot+0x24,0x4000);if(x>0x788000)hn_write32(m,slot+0x24,0xFFFFC000);}
        *yield=0;break;
    }
    case 0x0806C570:hn_write32(m,hn_render_slot(m,hn_read8(m,0x03002C04))+0x24,0);*yield=0;break;
    case 0x0806C628:
        if(signed_word(m,hn_render_slot(m,hn_read8(m,0x03002C04))+0x20)>0x10FFFFF){hn_write16(m,0x0201DC40+0x2C*2,1);REQUIRE(animation(m,11,0x088ED9DF));}break;
    case 0x0806C66C:if(signed_word(m,slot+0x18)>0x1400000)hn_write32(m,slot+0x18,0xFF800000);*yield=0;*consumed=1;break;
    case 0x0806EDE0:set_flag(m,0x2B6,true);hn_audio_track_parameter(m,0x03005120,255,128,0);REQUIRE(hn_audio_song(m,36,HN_SONG_CHANGE));*yield=1;break;
    case 0x0806EE10:set_flag(m,0x2B6,false);*yield=1;break;
    case 0x0806EE2C:case 0x0806EE58:
        if(!flag(m,0x2B6)){hn_audio_track_parameter(m,0x03005120,255,256,0);REQUIRE(hn_audio_song(m,(callback&~1u)==0x0806EE2C?38:39,HN_SONG_CHANGE));}break;
    default:return hn_bank4_path_animation(m,callback,arguments,consumed,yield);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
