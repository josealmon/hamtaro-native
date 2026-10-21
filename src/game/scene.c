#include "game/scene.h"
#include "game/state.h"
#include "game/input.h"
#include "game/background.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/random.h"

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
static uint16_t masked_keys(HNMemory *m,uint8_t mode)
{
    uint16_t keys=hn_input_query(m,hn_read16(m,0x03001F64),mode);
    hn_write16(m,0x0201DC40,keys);
    hn_write16(m,0x03002180,(uint16_t)((keys<<6)|(hn_read16(m,0x03002180)&63)));return keys;
}
HNSceneResult hn_scene_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
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
        hn_write16(m,0x03001FBC,0);uint16_t keys=masked_keys(m,1);
        GameAddr selected=HN_SCENE_CONTROLS+0x1A1;
        if(keys&1)hn_write16(m,0x03001FBC,(uint16_t)(hn_read8(m,selected)+2));
        else if(keys&2){hn_write8(m,selected,3);hn_write16(m,0x03001FBC,5);}
        else{
            keys=masked_keys(m,2);unsigned current=hn_read8(m,selected);
            if(keys&0x60){hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)current);
                hn_write8(m,selected,(uint8_t)(current?current-1:2));hn_write16(m,0x03001FBC,1);}
            else if(keys&0x90){hn_write8(m,HN_SCENE_CONTROLS+0x60,(uint8_t)current);
                hn_write8(m,selected,(uint8_t)(current<=1?current+1:0));hn_write16(m,0x03001FBC,1);}
        }
        break;
    }
    case 0x0801C760:masked_keys(m,1);break;
    case 0x0801C794:masked_keys(m,2);break;
    case 0x0801C7C8:{
        uint16_t mask=hn_read16(m,0x03001F64);
        uint16_t keys=hn_input_query(m,mask&0xF0,0)|hn_input_query(m,mask&0x30F,1);
        hn_write16(m,0x0201DC40,keys);hn_write16(m,0x03002180,(uint16_t)((keys<<6)|(hn_read16(m,0x03002180)&63)));break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
