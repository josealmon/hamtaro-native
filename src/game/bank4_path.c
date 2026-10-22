#include "game/bank4.h"
#include "game/state.h"
#include "game/render.h"
#include "game/random.h"

enum { PATH_HALF=0x0300114C,PATH_IMAGES=0x020016D4,OVERLAY_IMAGES=0x0200178C,
       DIRECTIONAL_IMAGES=0x08670540,BLOCKED_IMAGES=0x08670558 };
static bool flag(HNMemory *m,uint16_t index){return ((hn_read8(m,0x03002180+(index>>3))>>(index&7))&1)!=0;}
static void set_flag(HNMemory *m,uint16_t index,bool value)
{GameAddr a=0x03002180+(index>>3);uint8_t bit=(uint8_t)(1u<<(index&7));hn_write8(m,a,(uint8_t)((hn_read8(m,a)&~bit)|(value?bit:0)));}
bool hn_bank4_advance_path(HNMemory *m,GameAddr state,GameAddr path)
{
    hn_write8(m,state,(uint8_t)(hn_read8(m,state)+2));
    if(hn_read16(m,path+hn_read8(m,state)*2)==65535)hn_write8(m,state,0);
    hn_write16(m,PATH_HALF,0x8000);return !m->fault;
}
bool hn_bank4_advance_reversible(HNMemory *m,GameAddr state,GameAddr path,uint16_t reverse_flag)
{
    if(!flag(m,reverse_flag)){
        hn_write8(m,state,(uint8_t)(hn_read8(m,state)+3));
        if(hn_read16(m,path+hn_read8(m,state)*2)==65535)hn_write8(m,state,0);
    }else{
        if(hn_read8(m,state)<=2){unsigned visits=0;
            while(hn_read16(m,path+hn_read8(m,state)*2)!=65535){
                if(visits++>=256)return false;hn_write8(m,state,(uint8_t)(hn_read8(m,state)+3));
            }
        }
        hn_write8(m,state,(uint8_t)(hn_read8(m,state)-3));
    }
    hn_write16(m,PATH_HALF,0x8000);return !m->fault;
}
bool hn_bank4_path_image(HNMemory *m,GameAddr slot,uint8_t animation,uint8_t overlay,uint8_t index)
{
    if(hn_read32(m,slot+4)!=hn_read32(m,PATH_IMAGES+animation*4)){
        hn_render_set_animation_pointer(m,slot,hn_read32(m,PATH_IMAGES+animation*4));
        if(overlay){uint8_t next=(uint8_t)(index+1);
            if(!hn_render_configure_raw(m,next,1,176,0,0,0,hn_read32(m,OVERLAY_IMAGES+(overlay-1)*4))||
               !hn_render_modes_raw(m,next,2,3,index))return false;
        }else hn_write8(m,hn_render_slot(m,(uint8_t)(index+1))+0x60,0);
    }
    return !m->fault;
}
bool hn_bank4_directional_image(HNMemory *m,GameAddr slot,uint8_t animation,uint8_t direction,
                                uint8_t overlay,uint32_t index_word,bool blocked)
{
    uint8_t image=hn_read8(m,(blocked?BLOCKED_IMAGES:DIRECTIONAL_IMAGES)+animation*4+direction);
    return hn_bank4_path_image(m,slot,image,blocked?0:overlay,(uint8_t)index_word);
}
uint16_t hn_bank4_accumulate(HNMemory *m,GameAddr accumulator,uint16_t speed)
{uint32_t value=((uint32_t)speed<<8)+hn_read16(m,accumulator);hn_write16(m,accumulator,(uint16_t)value);return (uint16_t)(value>>16);}
void hn_bank4_path_bounds(HNMemory *m,uint8_t direction,uint16_t x,uint16_t z,
                          GameAddr min_x,GameAddr max_x,GameAddr min_z,GameAddr max_z)
{
    int left,right,top,bottom;
    switch(direction){
    case 0:left=x-28;right=x+28;top=z-36;bottom=z+16;break;
    case 1:left=x-28;right=x+28;top=z+10;bottom=z+26;break;
    case 2:left=x-34;right=x-26;top=z-24;bottom=z+20;break;
    case 3:left=x+26;right=x+34;top=z-24;bottom=z+20;break;
    default:return;
    }
    hn_write16(m,min_x,(uint16_t)left);hn_write16(m,max_x,(uint16_t)right);
    hn_write16(m,min_z,(uint16_t)top);hn_write16(m,max_z,(uint16_t)bottom);
}
uint8_t hn_bank4_path_blocked(uint16_t player_x,uint16_t player_z,uint16_t follower_x,uint16_t follower_z,
                             uint16_t min_x,uint16_t max_x,uint16_t min_z,uint16_t max_z,uint8_t both)
{
    return (player_x>=min_x&&player_x<=max_x&&player_z>=min_z&&player_z<=max_z)||
           (both&&follower_x>=min_x&&follower_x<=max_x&&follower_z>=min_z&&follower_z<=max_z);
}
static bool direct_blocked(HNMemory *m,GameAddr position,uint16_t x,uint16_t z,unsigned direction)
{
    int px=hn_read16(m,position),pz=hn_read16(m,position+2);
    switch(direction){
    case 0:return x>=px-28&&x<=px+28&&z>=pz-32&&z<=pz-24;
    case 1:return x>=px-28&&x<=px+28&&z>=pz+10&&z<=pz+18;
    case 2:return z>=pz-24&&z<=pz+20&&x>=px-34&&x<=px-26;
    default:return z>=pz-24&&z<=pz+20&&x>=px+26&&x<=px+34;
    }
}
static bool advance_path(HNMemory *m,GameAddr state,GameAddr path,bool reversible,uint16_t reverse_flag)
{return reversible?hn_bank4_advance_reversible(m,state,path,reverse_flag):hn_bank4_advance_path(m,state,path);}
static bool command_image(HNMemory *m,GameAddr slot,GameAddr command,GameAddr arguments,bool directional,bool blocked)
{
    uint16_t bits=hn_read16(m,command);uint8_t animation=hn_read8(m,command),overlay=(uint8_t)((bits&0xF00)>>8);
    uint8_t index=hn_read8(m,arguments);
    if(directional)return hn_bank4_directional_image(m,slot,animation,(uint8_t)(hn_read8(m,slot+0x60)-1),overlay,index,blocked);
    return hn_bank4_path_image(m,slot,animation,overlay,index);
}
bool hn_bank4_follow_path(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t retained_sl)
{
    if((callback&~1u)!=0x0806D38C&&(callback&~1u)!=0x0806D794&&(callback&~1u)!=0x0806DD50)return false;
    bool reversible=(callback&~1u)!=0x0806D38C,stepped=(callback&~1u)==0x0806DD50;
    GameAddr slot=hn_render_slot(m,hn_read8(m,arguments)),player=hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_LEADER));
    GameAddr follower=reversible?hn_render_slot(m,hn_read8(m,HN_RENDER_CLASS_FOLLOWER)):0;
    GameAddr state=HN_SCENE_CONTROLS+hn_read16(m,arguments+1),position=0x0201DC40+hn_read16(m,arguments+3)*2;
    GameAddr accumulator=stepped?0x0201DC40+hn_read16(m,arguments+12)*2:0,path=hn_read32(m,arguments+7);
    uint16_t flag_index=hn_read16(m,arguments+5),speed=stepped?hn_read16(m,arguments+14):0;
    uint8_t options=reversible?hn_read8(m,arguments+11):0;bool stop=false,handled=false,reversed=false;
    if(!flag(m,flag_index)){
        set_flag(m,flag_index,true);hn_write8(m,state,0);hn_write8(m,state+1,0);
        hn_write16(m,position,(uint16_t)(hn_read32(m,slot+0x18)>>16));
        hn_write16(m,position+2,(uint16_t)(hn_read32(m,slot+0x20)>>16));hn_write16(m,PATH_HALF,0x8000);
    }
    if(hn_read8(m,state+1)){
        hn_write8(m,state+1,(uint8_t)(hn_read8(m,state+1)-1));
        if(hn_read8(m,state+1))stop=true;
        else if(!advance_path(m,state,path,reversible,(uint16_t)(flag_index+1)))return false;
        if(reversible&&(stop||hn_read8(m,state+1))){handled=retained_sl!=0;goto complete;}
    }
    uint16_t player_x=(uint16_t)(hn_read32(m,player+0x18)>>16),player_z=(uint16_t)(hn_read32(m,player+0x20)>>16);
    uint16_t follower_x=reversible?(uint16_t)(hn_read32(m,follower+0x18)>>16):0;
    uint16_t follower_z=reversible?(uint16_t)(hn_read32(m,follower+0x20)>>16):0;
    for(unsigned budget=0;!stop;budget++){
        if(budget>=4096)return false;
        GameAddr command=path+hn_read8(m,state)*2;unsigned operation=hn_read16(m,command)>>12;
        uint16_t amount=1;if(stepped&&(operation<=3||operation==5||operation==6||operation==8))amount=hn_bank4_accumulate(m,accumulator,speed);
        if(operation<=3){
            if(direct_blocked(m,position,player_x,player_z,operation)){stop=true;continue;}
            GameAddr coordinate=position+(operation<=1?2:0);uint16_t current=hn_read16(m,coordinate),target=hn_read16(m,command+2);
            bool negative=operation==0||operation==2;
            if(negative?current<=target:current>=target){hn_write16(m,coordinate,target);
                if(!advance_path(m,state,path,reversible,(uint16_t)(flag_index+1)))return false;}
            else{hn_write16(m,coordinate,(uint16_t)(negative?current-amount:current+amount));hn_write8(m,slot+0x60,(uint8_t)(operation+1));stop=true;}
        }else if(operation==4){hn_write8(m,state+1,(uint8_t)hn_read16(m,command+2));stop=true;}
        else if(operation==5||operation==6){
            bool negative=operation==5;
            if(hn_read16(m,PATH_HALF)==0x8000){
                uint16_t distance=hn_random_range(m,1,6);uint16_t current=hn_read16(m,position);
                hn_write16(m,PATH_HALF,(uint16_t)(negative?current-(distance<<3):current+(distance<<3)));
                if(negative&&(int16_t)hn_read16(m,PATH_HALF)<0)hn_write16(m,PATH_HALF,0);
                uint16_t target=hn_read16(m,command+2);
                if(negative?hn_read16(m,PATH_HALF)<=target:hn_read16(m,PATH_HALF)>=target)hn_write16(m,PATH_HALF,target);
            }else if(direct_blocked(m,position,player_x,player_z,negative?2:3)){
                if(!advance_path(m,state,path,reversible,(uint16_t)(flag_index+1)))return false;
            }else{
                uint16_t current=hn_read16(m,position),target=hn_read16(m,PATH_HALF);
                if(negative?current<=target:current>=target){hn_write16(m,position,target);
                    if(!advance_path(m,state,path,reversible,(uint16_t)(flag_index+1)))return false;}
                else{hn_write16(m,position,(uint16_t)(negative?current-amount:current+amount));hn_write8(m,slot+0x60,negative?3:4);stop=true;}
            }
        }else if(operation==7){uint16_t pause=hn_random_range(m,0,61);hn_write8(m,state+1,(uint8_t)(pause+hn_read8(m,command+2)));stop=true;}
        else if(operation==8&&reversible){
            uint16_t step_x=0,step_z=0;
            if(hn_read16(m,position)>hn_read16(m,command+2)){step_x=(uint16_t)(0u-amount);hn_write8(m,slot+0x60,3);}
            else if(hn_read16(m,position)<hn_read16(m,command+2)){step_x=amount;hn_write8(m,slot+0x60,4);}
            else if(hn_read16(m,position+2)>hn_read16(m,command+4)){step_z=(uint16_t)(0u-amount);hn_write8(m,slot+0x60,1);}
            else if(hn_read16(m,position+2)<hn_read16(m,command+4)){step_z=amount;hn_write8(m,slot+0x60,2);}
            uint8_t direction=(uint8_t)(hn_read8(m,slot+0x60)-1);if(direction>3)return false;
            uint16_t x=hn_read16(m,position),z=hn_read16(m,position+2),min_x,max_x,min_z,max_z;
            /* Roots require a valid facing; direct bounds helpers preserve invalid outputs. */
            min_x=(uint16_t)(x-(direction==2?34:direction==3?-26:28));
            max_x=(uint16_t)(x+(direction==2?-26:direction==3?34:28));
            min_z=(uint16_t)(z+(direction==0?-36:direction==1?10:-24));
            max_z=(uint16_t)(z+(direction==0?16:direction==1?26:20));
            bool blocked=hn_bank4_path_blocked(player_x,player_z,follower_x,follower_z,min_x,max_x,min_z,max_z,options&128)!=0;
            if(blocked){
                if((options&127)&&!reversed&&(stepped||step_x||step_z)){
                    if(!command_image(m,slot,command,arguments,true,false))return false;
                    uint16_t reverse_flag=(uint16_t)(flag_index+1);set_flag(m,reverse_flag,!flag(m,reverse_flag));
                    if(!hn_bank4_advance_reversible(m,state,path,reverse_flag))return false;reversed=true;
                }else{if(!command_image(m,slot,command,arguments,true,true))return false;stop=true;}
                handled=true;
            }else{
                if(!command_image(m,slot,command,arguments,true,false))return false;handled=true;
                hn_write16(m,position,(uint16_t)(hn_read16(m,position)+step_x));
                uint16_t updated_z=(uint16_t)(hn_read16(m,position+2)+step_z);hn_write16(m,position+2,updated_z);
                if(hn_read16(m,position)==hn_read16(m,command+2)&&(stepped?updated_z:hn_read16(m,position+2))==hn_read16(m,command+4))
                    if(!hn_bank4_advance_reversible(m,state,path,(uint16_t)(flag_index+1)))return false;
                stop=true;
            }
        }
        if(m->fault)return false;
    }
complete:
    if(!handled&&!command_image(m,slot,path+hn_read8(m,state)*2,arguments,false,false))return false;
    hn_write32(m,slot+0x18,((uint32_t)hn_read16(m,position)<<16)|0x8000);
    hn_write32(m,slot+0x20,((uint32_t)hn_read16(m,position+2)<<16)|0x8000);return !m->fault;
}
HNSceneResult hn_bank4_path_animation(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed,uint32_t *yield)
{
    switch(callback&~1u){
    case 0x0806D38C:*consumed=11;break;
    case 0x0806D794:*consumed=12;break;
    case 0x0806DD50:*consumed=16;break;
    default:return HN_SCENE_UNKNOWN;
    }
    /* The native boot uses the comparison baseline's zero initial SL. Direct
       probes cover arbitrary retained SL separately; no BIOS-history claim. */
    if(!hn_bank4_follow_path(m,callback,arguments,0))return HN_SCENE_RESOURCE;
    *yield=1;return HN_SCENE_OK;
}
