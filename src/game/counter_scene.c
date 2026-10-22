#include "game/counter_scene.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/audio.h"

static unsigned value(HNMemory *m,unsigned index){return hn_read16(m,0x0201DC40+index*2);}
static void store(HNMemory *m,unsigned index,uint32_t word){hn_write16(m,0x0201DC40+index*2,(uint16_t)word);}
static void phase(HNMemory *m,unsigned state){hn_write8(m,HN_SCENE_CONTROLS+0x61,(uint8_t)state);}
static bool configure(HNMemory *m,uint8_t slot,int x,int z,GameAddr resource)
{return hn_render_configure(m,slot,1,0x72,x,0,z,resource);}
static void animation(HNMemory *m,uint8_t slot,GameAddr resource)
{
    if(slot==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_render_set_animation(m,slot,resource);
}
static void hide(HNMemory *m,uint8_t slot)
{
    if(slot==hn_read8(m,HN_RENDER_CLASS_FOLLOWER))hn_write8(m,0x03002170,0);
    hn_write8(m,hn_render_slot(m,slot)+0x60,0);hn_render_set_animation(m,slot,0);
}
static void velocity(HNMemory *m,unsigned slot,int32_t z)
{hn_write32(m,hn_render_slot(m,slot)+0x2C,(uint32_t)z);}
static void vector(HNMemory *m,unsigned slot,int32_t z)
{
    GameAddr target=hn_render_slot(m,slot)+0x24;
    hn_write32(m,target,0);hn_write32(m,target+4,0);hn_write32(m,target+8,(uint32_t)z);
}
static bool draw_counter(HNMemory *m)
{
    unsigned number=value(m,0x22),selector=hn_read8(m,HN_SCENE_CONTROLS+0x60);
    /* The original retains an incoming R6 pointer outside these five selectors. */
    if(selector>4)return false;
    GameAddr tiles=HN_COUNTER_SCENE_TILES+selector*20;
    uint8_t hundreds=(uint8_t)(number/100),tens=(uint8_t)((number-hundreds*100)/10),ones=(uint8_t)(number%10);
    hn_write16(m,tiles+6,0xF262);hn_write16(m,tiles+12,0xF262);
    if(hundreds)hn_write16(m,tiles+6,(uint16_t)(hundreds+0xF140));
    if(hundreds||tens)hn_write16(m,tiles+12,(uint16_t)(tens+0xF140));
    hn_write16(m,tiles+18,(uint16_t)(ones+0xF140));return !m->fault;
}
static bool stage_graphics(HNMemory *m,bool page)
{
    unsigned stage=value(m,12);if(stage>4)return !m->fault;
    GameAddr base=0x088F8781+stage*0xE8;
    if(!hn_graphics_configure(m,5,base+(page?47:32)))return false;
    return page||(hn_graphics_configure(m,3,base)&&hn_graphics_configure(m,4,base+16));
}
static bool stage_pair(HNMemory *m)
{
    static const GameAddr resources[2][5]={
        {0x088F7EB8,0x088F7F73,0x088F7F7D,0x088F8008,0x088F807C},
        {0x088F8319,0x088F83D4,0x088F83DE,0x088F8469,0x088F84DD}};
    uint8_t offset=(uint8_t)(value(m,13)*2);unsigned stage=value(m,12);
    return stage>4||(configure(m,23,156,110-offset,resources[0][stage])&&configure(m,24,204,110-offset,resources[1][stage]));
}
static bool render_slots(HNMemory *m)
{
    for(unsigned slot=1;slot<=7;slot++)if(!configure(m,(uint8_t)slot,0,0,0x088F7B69+(slot-1)*47))return false;
    return !m->fault;
}
static bool indicators(HNMemory *m,uint8_t mode)
{
    uint8_t offset=(uint8_t)(value(m,13)*2);if(offset<=15)return !m->fault;
    if(!mode)return configure(m,0,188,120,0x088F7CB2);
    hide(m,0);unsigned stage=value(m,12);if(stage>4)return !m->fault;
    static const GameAddr resources[]={0x088F7D40,0x088F7D1A,0x088F7CEA,0x088F7CCF};
    unsigned first=stage>=2?23-stage:22;
    for(unsigned slot=first;slot<=22;slot++)if(!configure(m,(uint8_t)slot,188,slot==22?120:80+(int)(slot-19)*16,resources[slot-19]))return false;
    return !m->fault;
}
static bool scroll_display(HNMemory *m)
{
    uint8_t offset=(uint8_t)(value(m,13)*2);if(!stage_pair(m))return false;
    if(value(m,13)==0){hn_write16(m,0x04000046,0);hide(m,25);hide(m,26);hide(m,27);}
    else if(value(m,13)==4){
        hn_write16(m,0x04000046,0);
        if(!configure(m,25,184,128,0x088F7DD9)||!configure(m,27,184,120,0x088F7E24)||!configure(m,26,184,128,0x088F7DAA))return false;
    }else if(value(m,13)>55){
        hn_write16(m,0x04000046,0x2082);if(!configure(m,25,184,24,0x088F7D85))return false;
        hide(m,27);if(!configure(m,26,184,128,0x088F7DB4))return false;
    }else if(value(m,13)>52){
        unsigned stage=value(m,13)-53;
        static const GameAddr first[]={0x088F7DED,0x088F7DF7,0x088F7E01},second[]={0x088F7E52,0x088F7E74,0x088F7E96};
        hn_write16(m,0x04000046,0x2082);
        if(!configure(m,25,184,32,first[stage])||!configure(m,27,184,32,second[stage])||!configure(m,26,184,128,0x088F7DB4))return false;
    }else{
        if(value(m,13)>4)hn_write16(m,0x04000046,(uint16_t)(((uint32_t)(136-offset)<<8)+130));
        if(!configure(m,25,184,(int16_t)(128-offset),0x088F7DE3)||!configure(m,27,184,(int16_t)(128-offset),0x088F7E3B)||!configure(m,26,184,128,0x088F7DB4))return false;
    }
    return indicators(m,0)&&!m->fault;
}
static bool sequence(HNMemory *m)
{
    unsigned stage=value(m,12),scroll=value(m,13);uint8_t step=hn_read8(m,HN_COUNTER_SCENE_STEPS+hn_read8(m,HN_SCENE_CONTROLS+0x79));
    if(scroll+step>56)step=(uint8_t)(56-scroll);
    switch(hn_read8(m,HN_SCENE_CONTROLS+0x61)){
    case 0:
        if(!value(m,0x24)){
            if(scroll>68)phase(m,8);
            else{animation(m,13,0x088F7A36);hn_render_flags(m,13,12,0);store(m,0x24,1);}
        }else{
            store(m,0x24,value(m,0x24)+1);
            if(value(m,0x24)==48){
                unsigned count=value(m,14)+1;store(m,14,count);store(m,0x22,count);hn_write8(m,HN_SCENE_CONTROLS+0x60,0);
                if(!draw_counter(m)||!hn_audio_song(m,332,HN_SONG_START))return false;
            }
            if(value(m,0x24)==96){hn_render_flags(m,13,12,8);phase(m,2);store(m,0x24,0);}
        }
        break;
    case 2:
        if(!indicators(m,1))return false;
        animation(m,23,0x088F81A3);animation(m,24,0x088F8604);store(m,0x22,((18-stage*2)<<11)+138);store(m,0x24,0);
        if(!hn_audio_song(m,327,HN_SONG_START))return false;
        if(!value(m,13))phase(m,9);else if(value(m,13)>51)phase(m,10);
        else{
            if(value(m,13)==4){if(!configure(m,25,184,120,0x088F7DE3))return false;animation(m,27,0x088F7E3B);}
            phase(m,4);
        }
        break;
    case 4:
        if(stage<=4&&value(m,0x24)>=step*4u){
            velocity(m,23,0);velocity(m,24,0);vector(m,25,0);vector(m,27,0);
            if(stage==4)hn_write8(m,hn_render_slot(m,25)+0x60,0);store(m,0x24,0);phase(m,5);
        }else{
            store(m,0x24,value(m,0x24)+1);
            if((((int)value(m,0x24)-1)>>2)+(int)value(m,13)>51)phase(m,10);
            velocity(m,23,-32768);velocity(m,24,-32768);
            hn_write16(m,0x04000046,(uint16_t)(((136u-(scroll*2+(value(m,0x24)>>1)))<<8)+130));
            vector(m,25,-32768);vector(m,27,-32768);
        }
        break;
    case 5:if(hn_read32(m,hn_render_slot(m,23)+4)!=0x088F81A3)phase(m,6);break;
    case 6:
        store(m,13,value(m,13)+step);
        if(value(m,13)>=value(m,12)*14+14){
            store(m,12,value(m,12)+1);uint16_t offset=(uint16_t)(value(m,13)*2);
            if(!configure(m,28,174,(int16_t)(112-offset),0x088F7AF3))return false;
            hn_render_flags(m,28,12,0);animation(m,23,0x088F80F1);animation(m,24,0x088F8552);
            if(!stage_graphics(m,true)||!render_slots(m))return false;phase(m,11);
        }
        /* The original falls through to the same-call pending-animation check. */
    case 7:
        if(hn_read32(m,hn_render_slot(m,28)+4)!=0x088F7AF3){if(!scroll_display(m)||!stage_graphics(m,false))return false;phase(m,8);}
        break;
    case 9:
        if(!value(m,0x24)){
            if(!configure(m,25,184,128,0x088F7DBE)||!configure(m,25,184,128,0x088F7D8F)||!configure(m,27,184,120,0x088F7E0B))return false;
            velocity(m,23,-32768);velocity(m,24,-32768);store(m,0x24,1);
        }else{
            velocity(m,23,-32768);velocity(m,24,-32768);store(m,0x24,value(m,0x24)+1);
            if(value(m,0x24)==16){velocity(m,23,0);velocity(m,24,0);store(m,0x24,0);phase(m,5);}
        }
        break;
    case 10:
        if(stage<=5&&value(m,0x24)>=step*4u){
            velocity(m,23,0);velocity(m,24,0);if(stage==5)hn_write8(m,hn_render_slot(m,25)+0x60,0);
            store(m,0x24,0);phase(m,5);
        }else{
            store(m,0x24,value(m,0x24)+1);velocity(m,23,-32768);velocity(m,24,-32768);
            int position=(((int)value(m,0x24)-1)>>2)+(int)value(m,13);
            if(position>=52&&position<=54){
                static const GameAddr first[]={0x088F7DED,0x088F7DF7,0x088F7E01},second[]={0x088F7E52,0x088F7E74,0x088F7E96};
                if(!configure(m,25,184,32,first[position-52])||!configure(m,27,184,32,second[position-52]))return false;
            }else if(position==55){if(!configure(m,25,184,24,0x088F7D85))return false;hide(m,27);}
        }
        break;
    }
    return !m->fault;
}
HNSceneResult hn_counter_scene_apply(HNMemory *m,GameAddr callback,uint32_t word)
{
    bool ok=true;
    switch(callback&~1u){
    case 0x080C2C38:break;
    case 0x080C2C3C:ok=draw_counter(m);break;
    case 0x080C2D00:{
        bool bright=hn_read8(m,HN_SCENE_CONTROLS+0x60)!=0;
        for(unsigned bank=1;bank<=4;bank++)for(unsigned i=3;i<=9;i+=3){
            GameAddr tile=HN_COUNTER_SCENE_TILES+bank*20+i*2;unsigned old=hn_read16(m,tile);
            hn_write16(m,tile,(uint16_t)(bright?old|0xF000:old&0xFFF));
        }
        break;
    }
    case 0x080C2DFC:
        hn_write16(m,0x04000042,0xB9CF);hn_write8(m,HN_DISPLAY_SHADOW+1,(uint8_t)((hn_read8(m,HN_DISPLAY_SHADOW+1)&~0x20u)|0xC0));
        hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|1);break;
    case 0x080C2E34:
        hn_write8(m,HN_DISPLAY_SHADOW+1,hn_read8(m,HN_DISPLAY_SHADOW+1)|12);hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|1);
        hn_write16(m,0x04000048,0x3C00);hn_write16(m,0x0400004A,0x3C33);hn_write16(m,0x04000042,0xB9CF);break;
    case 0x080C2E84:
        hn_write8(m,HN_DISPLAY_SHADOW+1,31);hn_write32(m,HN_VIDEO_STATE,hn_read32(m,HN_VIDEO_STATE)|1);
        hn_write16(m,0x04000048,0);hn_write16(m,0x0400004A,0);hn_write16(m,0x04000042,0);hn_write16(m,0x04000046,0);break;
    case 0x080C2EC8:ok=stage_graphics(m,false);break;
    case 0x080C2FC0:ok=stage_graphics(m,true);break;
    case 0x080C302C:ok=stage_pair(m);break;
    case 0x080C3164:ok=render_slots(m);break;
    case 0x080C321C:ok=indicators(m,(uint8_t)word);break;
    case 0x080C3304:ok=scroll_display(m);break;
    case 0x080C3528:ok=sequence(m);break;
    default:return HN_SCENE_UNKNOWN;
    }
    return ok&&!m->fault?HN_SCENE_OK:HN_SCENE_RESOURCE;
}
HNSceneResult hn_counter_scene_callback(HNMemory *m,GameAddr callback,uint32_t *consumed)
{
    switch(callback&~1u){
    case 0x080C2C38:case 0x080C2C3C:case 0x080C2D00:case 0x080C2DFC:case 0x080C2E34:case 0x080C2E84:
    case 0x080C2EC8:case 0x080C3304:case 0x080C3528:*consumed=0;return hn_counter_scene_apply(m,callback,0);
    default:return HN_SCENE_UNKNOWN;
    }
}
HNSceneResult hn_counter_scene_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080C2C38:case 0x080C2C3C:case 0x080C2D00:case 0x080C2DFC:case 0x080C2E34:case 0x080C2E84:
    case 0x080C2EC8:case 0x080C2FC0:case 0x080C302C:case 0x080C3164:case 0x080C3304:case 0x080C3528:
        return hn_counter_scene_apply(m,callback,0);
    default:return HN_SCENE_UNKNOWN;
    }
}
