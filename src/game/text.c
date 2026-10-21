#include "game/text.h"
#include "game/state.h"
#include "game/heap.h"
#include "game/input.h"
#include "game/render.h"
#include "game/graphics.h"

enum { MODE=HN_SCENE_CONTROLS+0x1A2,INDEX=HN_SCENE_CONTROLS+0x1A3,
       POSITION=HN_SCENE_CONTROLS+0x1A4,BANK=HN_SCENE_CONTROLS+0x1A5,RESULT=0x03001FBC };

bool hn_text_blit_glyph(HNMemory *m,GameAddr glyph,GameAddr shadow,uint32_t tile_width,
                        uint32_t horizontal_offset,uint32_t color_group)
{
    if(!shadow)return false;
    unsigned width=tile_width&255,x=horizontal_offset&65535,odd=x&1;
    unsigned leading=(x&7)>>1,trailing=4-leading,column=(x&0xF8)<<2,stride=width*32;
    GameAddr decoded=hn_heap_allocate(m,128,false);if(!decoded)return false;
    for(unsigned row=0;row<16;row++)for(unsigned byte=0;byte<8;byte++){
        unsigned value=0;
        for(unsigned half=0;half<2;half++){
            unsigned pixel=byte*2+half,color;
            if(odd&&!pixel){
                /* This read intentionally precedes width clipping, as on GBA. */
                color=hn_read8(m,shadow+column+leading+(row&7)*4+(row>=8?stride:0))&15;
            }else{
                unsigned source_pixel=pixel-odd;
                unsigned packed=hn_read8(m,glyph+row*4+source_pixel/4);
                color=((color_group&3)<<2)+((packed>>((3-(source_pixel&3))*2))&3);
            }
            value|=color<<(half*4);
        }
        unsigned index=(row>=8?64:0)+(byte>=4?32:0)+(row&7)*4+(byte&3);
        hn_write8(m,decoded+index,(uint8_t)value);
    }
    for(unsigned i=0;i<128;i++){
        unsigned offset=column+((i&3)<trailing?leading:32-trailing)+(i&63);
        unsigned row_base=i>=64?stride:0,limit=i>=64?stride*2:stride;
        offset+=row_base;
        if(offset<limit)hn_write8(m,shadow+offset,hn_read8(m,decoded+i));
    }
    hn_heap_free(m,decoded);return !m->fault;
}
uint8_t hn_scene_text_character(HNMemory *m,uint8_t index,uint8_t value)
{
    GameAddr text=hn_read8(m,HN_SCENE_CONTROLS+0x1A2)==1?HN_TEXT_FIRST:HN_TEXT_SECOND;
    if(value!=255)hn_write8(m,text+index,value);return hn_read8(m,text+index);
}
bool hn_scene_text_render(HNMemory *m)
{
    GameAddr shadow=hn_heap_allocate(m,0x280,false);if(!shadow)return false;
    /* Original DMA fill is 0x1A0 words, independently of the 0x280 allocation.
       Guest EWRAM is addressed explicitly; no host buffer can be overrun. */
    for(unsigned i=0;i<0x1A0;i++)hn_write32(m,shadow+i*4,0xCCCCCCCC);
    uint8_t x=0;
    for(unsigned i=0;i<7;i++){
        unsigned value=hn_scene_text_character(m,(uint8_t)i,255);if(!value)break;value--;
        if(!hn_text_blit_glyph(m,HN_TEXT_GLYPHS+value*64,shadow,7,x,3)){hn_heap_free(m,shadow);return false;}
        x=(uint8_t)(x+1+hn_read8(m,HN_TEXT_ADVANCES+value));
    }
    GameAddr destination=hn_read8(m,HN_SCENE_CONTROLS+0x1A2)<=1?0x06004020:0x060041E0;
    hn_write32(m,0x040000D4,shadow);hn_write32(m,0x040000D8,destination);
    hn_write32(m,0x040000DC,0x84000070);hn_heap_free(m,shadow);return !m->fault;
}
static void reset_name(HNMemory *m,uint8_t mode)
{
    for(unsigned i=0;i<9;i++){
        if(mode<=1){hn_write8(m,HN_TEXT_FIRST+i,hn_read8(m,HN_TEXT_DEFAULT_FIRST+i));if(mode==1)continue;}
        GameAddr source=hn_read8(m,HN_SCENE_CONTROLS+0xEB)==4?HN_TEXT_DEFAULT_SPANISH:HN_TEXT_DEFAULT_SECOND;
        hn_write8(m,HN_TEXT_SECOND+i,hn_read8(m,source+i));
    }
}
static bool caret(HNMemory *m)
{
    if(hn_read8(m,HN_RENDER_INFO+0x10)<=30)return false;
    uint32_t x=4;
    for(unsigned i=0;i<hn_read8(m,POSITION)&&i<=5;i++){
        unsigned code=hn_scene_text_character(m,(uint8_t)i,255);
        if(code)x+=1+hn_read8(m,HN_TEXT_ADVANCES+code-1);
    }
    GameAddr slot=hn_render_slot(m,30);hn_write32(m,slot+0x18,x<<16);
    hn_write32(m,slot+0x1C,0);hn_write32(m,slot+0x20,0x00380000);return !m->fault;
}
static bool grid_value(HNMemory *m,uint8_t *value)
{
    unsigned index=hn_read8(m,INDEX),bank=hn_read8(m,BANK);
    if(index>=77||bank>=3)return false;*value=hn_read8(m,HN_TEXT_GRID+bank*77+index);return !m->fault;
}
static bool grid(HNMemory *m)
{
    if(hn_read8(m,HN_RENDER_INFO+0x10)<36)return false;
    uint8_t value;if(!grid_value(m,&value))return false;
    const GameAddr animations[]={HN_TEXT_CONFIRM,HN_TEXT_BANK_0,HN_TEXT_BANK_1,HN_TEXT_BANK_2,HN_TEXT_ADVANCE,HN_TEXT_RETREAT};
    for(unsigned i=0;i<6;i++){
        unsigned slot=i?30+i:28;hn_render_set_animation(m,slot,animations[i]);hn_render_flags(m,slot,12,8);
    }
    if(value<=239||value==253||value==254){
        hn_render_set_animation(m,29,HN_TEXT_CURSOR);unsigned index=hn_read8(m,INDEX),column=index%11;
        GameAddr slot=hn_render_slot(m,29);hn_write32(m,slot+0x18,(column*11+(column>4?79u:68u))<<16);
        hn_write32(m,slot+0x1C,0);hn_write32(m,slot+0x20,(index/11*16+32)<<16);
    }else{
        hn_render_set_animation(m,29,HN_TEXT_CURSOR_OFF);
        if(value>=0xF0&&value<=0xF5){
            unsigned i=value-0xF0,slot=i?30+i:28;
            hn_render_set_animation(m,slot,animations[i]+9);hn_render_flags(m,slot,12,0);
        }
    }
    return !m->fault;
}
static void select_bank(HNMemory *m,uint8_t bank)
{hn_write8(m,BANK,bank);hn_write16(m,RESULT,(uint16_t)(bank+2));}
static bool move_position(HNMemory *m,bool forward)
{
    unsigned position=hn_read8(m,POSITION);
    if(forward){if(position<=6)position++;}
    else{if(position==7)position--;if(position)position--;}
    hn_write8(m,POSITION,(uint8_t)position);if(!caret(m))return false;
    return hn_graphics_configure(m,forward?5:6,forward?HN_TEXT_ADVANCE_GRAPHICS:HN_TEXT_RETREAT_GRAPHICS);
}
static bool confirm_name(HNMemory *m)
{
    for(unsigned shifted=0;shifted<7&&hn_scene_text_character(m,0,255)==1;shifted++){
        for(unsigned i=1;i<7;i++)hn_scene_text_character(m,(uint8_t)(i-1),hn_scene_text_character(m,(uint8_t)i,255));
        hn_scene_text_character(m,6,1);
    }
    bool any=false;for(unsigned i=0;i<7;i++)if(hn_scene_text_character(m,(uint8_t)i,255)!=1){any=true;break;}
    if(!any)reset_name(m,hn_read8(m,MODE));hn_write16(m,RESULT,1);return hn_scene_text_render(m);
}
static bool move_grid(HNMemory *m)
{
    hn_write8(m,HN_SCENE_CONTROLS+0x60,1);
    unsigned keys=hn_read16(m,0x0201DC40)>>4,direction=0;
    while(direction<4&&!(keys&1)){keys>>=1;direction++;}
    if(direction==4)return true;
    unsigned index=hn_read8(m,INDEX),bank=hn_read8(m,BANK);if(index>=77||bank>=3)return false;
    if(direction<2&&index>66){hn_write8(m,HN_SCENE_CONTROLS+0x60,0);return true;}
    unsigned visits=77;
    do{
        if(direction==0)index=index%11==10?index-10:index+1;
        else if(direction==1)index=index%11==0?index+10:index-1;
        else if(direction==2)index=index/11==0?index+66:index-11;
        else index=index/11>5?index-66:index+11;
        hn_write8(m,INDEX,(uint8_t)index);
        if(hn_read8(m,HN_TEXT_GRID+bank*77+index)!=255)return grid(m);
    }while(--visits&&!m->fault);
    return false;
}
HNSceneResult hn_scene_text_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080C0400:case 0x080C0468:
        hn_write8(m,MODE,(callback&~1u)==0x080C0400?1:2);hn_write8(m,INDEX,0);
        for(unsigned count=7;count;count--){
            unsigned value=hn_scene_text_character(m,(uint8_t)(count-1),255);
            if(value!=0&&value!=1){hn_write8(m,POSITION,(uint8_t)count);break;}
        }
        hn_write8(m,BANK,0);break;
    case 0x080C04D0:{
        uint16_t keys=hn_input_script_mask(m,1),result=0;
        if(keys&1)result=2;else if(keys&2)result=3;else if(keys&0x200)result=4;
        else if(keys&0x100)result=5;else if(keys&8)result=6;else if(keys&4)result=7;
        if(hn_input_script_mask(m,2)&0xF0)result=1;hn_write16(m,RESULT,result);break;
    }
    case 0x080C0560:if(!move_grid(m))return HN_SCENE_RESOURCE;break;
    case 0x080C0714:{
        hn_write16(m,RESULT,0);uint8_t value;if(!grid_value(m,&value))return HN_SCENE_RESOURCE;
        if(value>239){
            if(value==0xF0){if(!confirm_name(m))return HN_SCENE_RESOURCE;}
            else if(value>=0xF1&&value<=0xF3)select_bank(m,(uint8_t)(value-0xF1));
            else if(value==0xF4||value==0xF5){if(!move_position(m,value==0xF4))return HN_SCENE_RESOURCE;}
        }else{
            unsigned position=hn_read8(m,POSITION);if(position>7)return HN_SCENE_RESOURCE;
            if(position==7)position=6;hn_scene_text_character(m,(uint8_t)position,value);
            hn_write8(m,POSITION,(uint8_t)(position+1));if(!caret(m)||!hn_scene_text_render(m))return HN_SCENE_RESOURCE;
        }
        break;
    }
    case 0x080C0804:break;
    case 0x080C0808:hn_write8(m,INDEX,76);if(!grid(m))return HN_SCENE_RESOURCE;break;
    case 0x080C0828:{uint8_t bank=(uint8_t)(hn_read8(m,BANK)+1);select_bank(m,bank>2?0:bank);break;}
    case 0x080C0850:{int8_t bank=(int8_t)(uint8_t)(hn_read8(m,BANK)-1);select_bank(m,bank<0?2:(uint8_t)bank);break;}
    case 0x080C0880:select_bank(m,(uint8_t)arguments);break;
    case 0x080C08A4:case 0x080C08D8:if(!move_position(m,(callback&~1u)==0x080C08A4))return HN_SCENE_RESOURCE;break;
    case 0x080C0914:{
        unsigned position=hn_read8(m,POSITION);if(position>7)return HN_SCENE_RESOURCE;if(position==7)position=6;
        hn_scene_text_character(m,(uint8_t)position,1);if(position)position--;
        hn_write8(m,POSITION,(uint8_t)position);if(!caret(m)||!hn_scene_text_render(m))return HN_SCENE_RESOURCE;break;
    }
    case 0x080C0954:if(!confirm_name(m))return HN_SCENE_RESOURCE;break;
    case 0x080C09F0:if(!grid(m))return HN_SCENE_RESOURCE;break;
    case 0x080C0BA8:if(!caret(m))return HN_SCENE_RESOURCE;break;
    case 0x080C0C5C:
        reset_name(m,0);
        for(unsigned mode=1;mode<=2;mode++){
            hn_write8(m,HN_SCENE_CONTROLS+0x1A2,(uint8_t)mode);if(!hn_scene_text_render(m))return HN_SCENE_RESOURCE;
        }
        hn_write8(m,HN_SCENE_CONTROLS+0x1A2,0);break;
    case 0x080C0C90:reset_name(m,(uint8_t)arguments);break;
    case 0x080C0CF0:
        for(unsigned mode=0;mode<2;mode++){
            hn_write8(m,HN_SCENE_CONTROLS+0x1A2,(uint8_t)mode);
            for(unsigned count=7;count;count--){
                uint8_t index=(uint8_t)(count-1);if(hn_scene_text_character(m,index,255)!=1)break;
                hn_scene_text_character(m,index,0);
            }
        }
        break;
    case 0x080C0D6C:if(!hn_scene_text_render(m))return HN_SCENE_RESOURCE;break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
