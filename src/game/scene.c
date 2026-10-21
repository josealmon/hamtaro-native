#include "game/scene.h"
#include "game/state.h"
#include "game/input.h"
#include "game/background.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/random.h"
#include "game/text.h"
#include "game/choice.h"

static unsigned menu_kind(HNMemory *m)
{
    unsigned enabled=hn_read8(m,HN_SCENE_CONTROLS+0x1B);
    return enabled==3?1:enabled==5?2:enabled==1?3:0;
}
static bool menu_update(HNMemory *m,uint8_t selection)
{
    unsigned kind=menu_kind(m),language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);
    if(selection>=6||language>=5)return false;
    unsigned enabled=hn_read8(m,HN_SCENE_CONTROLS+0x1B),chosen=hn_read8(m,HN_SCENE_MENU_CODES+kind*6+selection);
    GameAddr table=hn_read32(m,HN_SCENE_MENU_BANKS+(language*4+kind)*4);
    for(unsigned option=1;option<=5;option++)if(option==5||((enabled>>(option-1))&1)){
        unsigned highlighted=option==chosen?5:0;
        if(!hn_background_configure(m,(uint8_t)(option-1),0x82,hn_read32(m,table+(option+highlighted)*4)))return false;
    }
    if(selection&&!hn_render_configure(m,27,1,0x70,56,56+(int)selection*16,0,
        hn_read32(m,HN_SCENE_MENU_CURSORS+selection*4)))return false;
    return !m->fault;
}
static bool flag(HNMemory *m,unsigned index)
{return ((hn_read8(m,0x03002180+(index>>3))>>(index&7))&1)!=0;}
static void arithmetic_flag(HNMemory *m,uint8_t mask,bool enabled)
{
    uint8_t flags=hn_read8(m,0x03002182);
    hn_write8(m,0x03002182,(uint8_t)(enabled?flags|mask:flags&~mask));
}
static bool indirect_arithmetic(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    bool half=callback==0x0801C3B4||callback==0x0801C410,subtract=callback==0x0801C344||callback==0x0801C410;
    unsigned offset=half?2:3,bytes=8-offset;GameAddr step=0x03001138+offset;
    for(unsigned i=0;i<bytes;i++)hn_write8(m,step+i,hn_read8(m,arguments+i));
    GameAddr target=hn_read32(m,0x0300113C);
    if(half){
        if(subtract){
            arithmetic_flag(m,4,hn_read16(m,target)<hn_read16(m,step));
            hn_write16(m,target,(uint16_t)(hn_read16(m,target)-hn_read16(m,step)));
        }else{
            uint16_t result=(uint16_t)(hn_read16(m,target)+hn_read16(m,step));hn_write16(m,target,result);
            arithmetic_flag(m,4,result<hn_read16(m,step));
        }
        arithmetic_flag(m,2,hn_read16(m,target)==0);
    }else{
        uint8_t value=hn_read8(m,target),amount=hn_read8(m,step);
        hn_write8(m,target,(uint8_t)(subtract?value-amount:value+amount));
        /* Byte callbacks reload their global pointer after each write. */
        arithmetic_flag(m,2,hn_read8(m,hn_read32(m,0x0300113C))==0);
        value=hn_read8(m,hn_read32(m,0x0300113C));amount=hn_read8(m,step);
        arithmetic_flag(m,4,subtract?value>amount:value<amount);hn_write32(m,0x03001134,5);
    }
    *consumed=bytes;return !m->fault;
}
static bool panel_layout(HNMemory *m,unsigned variant,unsigned *row,unsigned *count)
{
    unsigned language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);if(variant>=4||language>=5)return false;
    *row=variant*23+language*92;int n=(int8_t)hn_read8(m,HN_PANEL_SHIFTS+*row);
    *count=n>0?(unsigned)n:0;return *count<=hn_read8(m,HN_RENDER_INFO+0x10)&&!m->fault;
}
static void panel_velocity(HNMemory *m,unsigned i,int32_t x,int32_t y,int32_t z,unsigned offset)
{
    GameAddr slot=hn_render_slot(m,i);hn_write32(m,slot+offset,(uint32_t)x);
    hn_write32(m,slot+offset+4,(uint32_t)y);hn_write32(m,slot+offset+8,(uint32_t)z);
}
static void panel_disable(HNMemory *m,unsigned i)
{
    if(i==hn_read8(m,0x03001F50))hn_write8(m,0x03002170,0);
    hn_write8(m,hn_render_slot(m,i)+0x60,0);hn_render_set_animation(m,i,0);
}
static bool panel_command(HNMemory *m,GameAddr callback,uint8_t variant)
{
    unsigned row,count;if(!panel_layout(m,variant,&row,&count))return false;
    unsigned language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);bool any=false;
    for(unsigned i=0;i<count;i++){
        int shift=(int8_t)hn_read8(m,HN_PANEL_SHIFTS+row+1+i);unsigned magnitude=(unsigned)(shift<0?-shift:shift);
        GameAddr countdown=HN_SCENE_CONTROLS+0x1A6+i;
        if(callback==0x080C0E3C){
            int x=shift*8+hn_read8(m,HN_PANEL_PIXELS+row+1+i);
            unsigned code=hn_read8(m,HN_PANEL_CODES+i+variant*22+language*88);if(code>=52)return false;
            if(!hn_render_configure(m,(uint8_t)i,1,0x7C,x,0,4,hn_read32(m,HN_PANEL_ANIMATIONS+code*4)))return false;
            panel_velocity(m,i,shift>0?-0x80000:0x80000,0,0,0x24);hn_write8(m,countdown,(uint8_t)magnitude);
        }else if(callback==0x080C0FD0){
            hn_write8(m,countdown,(uint8_t)(hn_read8(m,HN_PANEL_SHIFTS+row+1)-magnitude));
        }else if(hn_read8(m,countdown)){
            hn_write8(m,countdown,(uint8_t)(hn_read8(m,countdown)-1));any=true;
        }else if(callback==0x080C10B0){panel_velocity(m,i,0,0,0,0x24);}
        else if(!hn_read32(m,hn_render_slot(m,i)+0x24)){
            panel_velocity(m,i,shift>0?0x80000:-0x80000,0,0,0x24);
            /* The original zero test deliberately uses the language-zero row. */
            if((int8_t)hn_read8(m,HN_PANEL_SHIFTS+variant*23+1+i)==0){
                hn_write8(m,countdown,0);panel_disable(m,i);
            }else hn_write8(m,countdown,(uint8_t)(magnitude-1));
        }else panel_disable(m,i);
    }
    if((callback==0x080C10B0||callback==0x080C1150)&&!any)hn_write8(m,HN_SCENE_CONTROLS+0x1A6,255);
    return !m->fault;
}

static bool record_menu(HNMemory *m)
{
    unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x1E);
    if(selected>=5||hn_read8(m,HN_BACKGROUND_COUNT)<7)return false;
    for(unsigned i=0;i<5;i++)if(!hn_background_configure(m,(uint8_t)i,0x82,
        hn_read32(m,HN_SCENE_RECORD_MENU+(i+(i==selected?5:0))*4)))return false;
    if(!hn_background_configure(m,5,0x82,hn_read32(m,HN_SCENE_RECORD_LABELS+selected*4)))return false;
    unsigned mode=hn_read8(m,HN_SCENE_CONTROLS+0xEC)?0:5;
    if(!hn_background_configure(m,6,0x82,hn_read32(m,HN_SCENE_RECORD_MODES+(selected+mode)*4)))return false;
    if(!hn_render_configure(m,27,1,0x70,88,56+(int)selected*16,0,HN_SCENE_RECORD_CURSOR))return false;
    hn_render_flags(m,27,12,0);return !m->fault;
}
HNSceneResult hn_scene_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x08009318:if(!hn_choice_reset(m))return HN_SCENE_RESOURCE;break;
    case 0x0801C2D4:case 0x0801C344:case 0x0801C3B4:case 0x0801C410:
        if(!indirect_arithmetic(m,callback&~1u,arguments,consumed))return HN_SCENE_RESOURCE;break;
    case 0x0801C474:{
        unsigned index=hn_read8(m,arguments);if(index!=255)hn_write8(m,HN_RENDER_CLASS_LEADER,(uint8_t)index);
        index=hn_read8(m,arguments+1);if(index!=255)hn_write8(m,HN_RENDER_CLASS_FOLLOWER,(uint8_t)index);
        hn_write32(m,0x03001134,2);*consumed=2;break;
    }
    case 0x0801C4A4:{
        hn_write32(m,0x03001134,0);unsigned a=hn_read8(m,HN_RENDER_CLASS_LEADER),b=hn_read8(m,HN_RENDER_CLASS_FOLLOWER);
        if(a>=hn_read8(m,HN_RENDER_INFO+0x10)||b>=hn_read8(m,HN_RENDER_INFO+0x10))return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS,hn_read8(m,hn_render_slot(m,a)+0x60)&15);
        hn_write8(m,HN_SCENE_CONTROLS+1,hn_read8(m,hn_render_slot(m,b)+0x60)&15);break;
    }
    case 0x0801C4F8:{
        unsigned index=hn_read8(m,arguments);if(index>=hn_read8(m,HN_RENDER_INFO+0x10))return HN_SCENE_RESOURCE;
        hn_write8(m,HN_SCENE_CONTROLS,hn_read8(m,hn_render_slot(m,index)+0x60)&15);*consumed=1;break;
    }
    case 0x080CB150:
        hn_write16(m,HN_BACKGROUND_BOUNDS+2*12+2,1024);
        hn_write16(m,HN_BACKGROUND_BOUNDS+2*12+6,1024);break;
    case 0x0801C864:case 0x0801C888:case 0x0801C8AC:case 0x0801C8D0:{
        uint16_t mask=hn_read16(m,arguments),keys;
        unsigned mode=((callback&~1u)-0x0801C864)/36;
        if(mode==3)keys=hn_input_query(m,mask&0xF0,0)|hn_input_query(m,mask&0x30F,1);
        else keys=hn_input_query(m,mask,(uint8_t)mode);
        uint16_t result=0,ordinal=1;
        for(unsigned bit=0x200;bit;bit>>=1){
            if(keys&mask&bit){result=ordinal;break;}
            if(mask&bit)ordinal++;
        }
        hn_write16(m,0x03001FBC,result);*consumed=2;break;
    }
    case 0x080C0E3C:case 0x080C0FD0:case 0x080C10B0:case 0x080C1150:
        if(!panel_command(m,callback&~1u,hn_read8(m,arguments)))return HN_SCENE_RESOURCE;*consumed=1;break;
    case 0x080BFD34:{
        unsigned kind=menu_kind(m),selected=hn_read8(m,HN_SCENE_CONTROLS+0x19),next=selected;
        uint16_t keys=hn_input_query(m,0xC0,1);
        if(keys&0x40){
            if(kind==3){if(selected==5)next=3;}else if(selected>(kind?3u:1u))next=selected-1;
        }else if(keys){if(kind==3){if(selected==3)next=5;}else if(selected<=4)next=selected+1;}
        if(next!=selected){
            if(!hn_audio_ready(m))return HN_SCENE_RESOURCE;
            hn_write8(m,HN_SCENE_CONTROLS+0x19,(uint8_t)next);
            if(!hn_audio_song(m,302,HN_SONG_START)||!menu_update(m,(uint8_t)next))return HN_SCENE_RESOURCE;
        }
        break;
    }
    case 0x080BFE10:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x19),enabled,initial;
        if(flag(m,0x47)){bool complete=flag(m,0x10B);enabled=complete?15:3;initial=complete?2:4;}
        else{enabled=1;initial=3;for(unsigned i=0x140;i<=0x149;i++)if(flag(m,i)){enabled=5;break;}}
        hn_write8(m,HN_SCENE_CONTROLS+0x1B,(uint8_t)enabled);
        if(!selected)hn_write8(m,HN_SCENE_CONTROLS+0x19,(uint8_t)initial);
        if(!menu_update(m,0))return HN_SCENE_RESOURCE;break;
    }
    case 0x080BFF00:if(!menu_update(m,hn_read8(m,HN_SCENE_CONTROLS+0x19)))return HN_SCENE_RESOURCE;break;
    case 0x080C000C:{
        unsigned kind=menu_kind(m),selected=hn_read8(m,HN_SCENE_CONTROLS+0x19),language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);
        if(selected>=6||language>=5)return HN_SCENE_RESOURCE;
        unsigned chosen=hn_read8(m,HN_SCENE_MENU_CODES+kind*6+selected);
        GameAddr table=hn_read32(m,HN_SCENE_MENU_BANKS+(language*4+kind)*4);
        if(!hn_background_configure(m,0,0x82,hn_read32(m,table+(chosen+10)*4)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080C005C:{
        unsigned i=0;while(i<6&&!hn_read8(m,0x03003730+i*16))i++;
        if(i==6)hn_write8(m,HN_SCENE_CONTROLS+0x60,0);
        else{
            unsigned attempts=4096;
            do{i=hn_random_range(m,0,6);}while(!hn_read8(m,0x03003730+i*16)&&--attempts);
            if(!attempts)return HN_SCENE_RESOURCE;
            hn_write8(m,HN_SCENE_CONTROLS+0x67,(uint8_t)i);
            hn_write8(m,HN_SCENE_CONTROLS+0x60,hn_read8(m,0x03003730+i*16));
        }
        break;
    }
    case 0x080C00CC:
        if(hn_read8(m,HN_SCENE_CONTROLS+0x1F)!=255&&(int32_t)hn_read32(m,0x03004ED4)<0)
            hn_write8(m,HN_SCENE_CONTROLS+0x1F,1);break;
    case 0x080C00F0:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x1D),language=hn_read8(m,HN_SCENE_CONTROLS+0xEB);
        if(language>4)language=0;if(selected>=5)return HN_SCENE_RESOURCE;
        if(selected&&!hn_background_configure(m,8,0x81,hn_read32(m,HN_SCENE_MENU_STATUS+(language*5+selected)*4)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080C0128:
        hn_write8(m,HN_SCENE_CONTROLS+8,hn_read8(m,0x03002168+hn_read8(m,arguments)));*consumed=1;break;
    case 0x080C0168:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x19);if(selected>=6)return HN_SCENE_RESOURCE;
        hn_write16(m,0x03001FBC,hn_read8(m,HN_SCENE_MENU_CODES+menu_kind(m)*6+selected));break;
    }
    case 0x080C019C:if(!record_menu(m))return HN_SCENE_RESOURCE;break;
    case 0x080C0260:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x1E),next=selected;
        uint16_t keys=hn_input_query(m,0xC0,1);
        if(keys&0x40){if(selected)next=selected-1;}else if(keys&&selected<=3)next=selected+1;
        if(next!=selected){
            if(!hn_audio_ready(m))return HN_SCENE_RESOURCE;
            hn_write8(m,HN_SCENE_CONTROLS+0x1E,(uint8_t)next);
            if(!hn_audio_song(m,302,HN_SONG_START)||!record_menu(m))return HN_SCENE_RESOURCE;
        }
        break;
    }
    case 0x080C02BC:{
        unsigned selected=hn_read8(m,HN_SCENE_CONTROLS+0x1E);if(selected>=5)return HN_SCENE_RESOURCE;
        if(!hn_background_configure(m,0,0x82,hn_read32(m,HN_SCENE_RECORD_MENU+(selected+10)*4)))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080C02E4:case 0x080C0304:
        for(unsigned i=0;i<2;i++){
            GameAddr control=HN_SCENE_CONTROLS+0xEB+i,snapshot=HN_SCENE_RECORD_LANGUAGE+i;
            if((callback&~1u)==0x080C02E4)hn_write8(m,snapshot,hn_read8(m,control));
            else hn_write8(m,control,hn_read8(m,snapshot));
        }
        break;
    case 0x080C0324:hn_input_repeat(m,16,8);break;
    case 0x080C0334:{
        hn_write16(m,0x03001FBC,0);uint16_t keys=hn_input_script_mask(m,1);
        GameAddr selected=HN_SCENE_CONTROLS+0x1A1;
        if(keys&1)hn_write16(m,0x03001FBC,(uint16_t)(hn_read8(m,selected)+2));
        else if(keys&2){hn_write8(m,selected,3);hn_write16(m,0x03001FBC,5);}
        else{
            keys=hn_input_script_mask(m,2);unsigned current=hn_read8(m,selected);
            if(keys&0x60){hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)current);
                hn_write8(m,selected,(uint8_t)(current?current-1:2));hn_write16(m,0x03001FBC,1);}
            else if(keys&0x90){hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)current);
                hn_write8(m,selected,(uint8_t)(current<=1?current+1:0));hn_write16(m,0x03001FBC,1);}
        }
        break;
    }
    case 0x0801C760:hn_input_script_mask(m,1);break;
    case 0x0801C794:hn_input_script_mask(m,2);break;
    case 0x0801C7C8:{
        uint16_t mask=hn_read16(m,0x03001F64);
        uint16_t keys=hn_input_query(m,mask&0xF0,0)|hn_input_query(m,mask&0x30F,1);
        hn_write16(m,0x0201DC40,keys);hn_write16(m,0x03002180,(uint16_t)((keys<<6)|(hn_read16(m,0x03002180)&63)));break;
    }
    default:return hn_scene_text_callback(m,callback,arguments,consumed);
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
HNSceneResult hn_scene_animation_callback(HNMemory *m,GameAddr callback,GameAddr slot,GameAddr arguments,
                                         uint32_t *consumed,uint32_t *yield)
{
    (void)slot;(void)arguments;
    if((callback&~1u)==0x0801E3E8){
        if(!hn_text_counter(m,hn_read16(m,0x0201DC52),0))return HN_SCENE_RESOURCE;
        *consumed=0;*yield=(*yield&0xFFFFFF00u)|1;return HN_SCENE_OK;
    }
    if((callback&~1u)!=0x080C12E4)return HN_SCENE_UNKNOWN;
    unsigned row,count;if(!panel_layout(m,hn_read8(m,HN_SCENE_CONTROLS+0x1B9),&row,&count))return HN_SCENE_RESOURCE;
    uint8_t remaining=(uint8_t)(hn_read8(m,HN_SCENE_CONTROLS+0x1B7)-1);hn_write8(m,HN_SCENE_CONTROLS+0x1B7,remaining);
    if(!remaining){
        hn_write8(m,HN_SCENE_CONTROLS+0x1B7,4);unsigned index=hn_read8(m,HN_SCENE_CONTROLS+0x1B8);
        unsigned target=hn_read8(m,HN_PANEL_ORDER+row+1+index);
        if(target>=hn_read8(m,HN_RENDER_INFO+0x10))return HN_SCENE_RESOURCE;
        hn_write32(m,hn_render_slot(m,target)+0x20,0x40000);
        panel_velocity(m,target,0,0,-0x8000,0x24);panel_velocity(m,target,0,0,0x1000,0x30);
        index=(index+1)&255;hn_write8(m,HN_SCENE_CONTROLS+0x1B8,(uint8_t)index);
        if(index==hn_read8(m,HN_PANEL_ORDER+row)){
            hn_write8(m,HN_SCENE_CONTROLS+0x1B8,0);hn_write8(m,HN_SCENE_CONTROLS+0x1B7,127);
        }
    }
    for(unsigned i=0;i<count;i++)if((int32_t)hn_read32(m,hn_render_slot(m,i)+0x20)>0x47FFF){
        hn_write32(m,hn_render_slot(m,i)+0x20,0x48000);panel_velocity(m,i,0,0,0,0x24);panel_velocity(m,i,0,0,0,0x30);
    }
    *consumed=0;*yield=1;return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
