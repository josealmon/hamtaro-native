#include "game/save_group4.h"
#include "game/state.h"
#include "game/task.h"
#include "game/render.h"
#include "game/video.h"
#include "game/transfer.h"
#include "game/heap.h"
#include "gba/bios.h"

/* Local Ham-Jam composition shares these layouts with the original link menus.
 * Transport is deliberately outside this adapter. All text/images come from ROM. */
enum { CONTEXT=0x03002BB0,RECEIVED=0x03003D30,RESOURCES=0x086E8CCC,
       ROW_TASKS=0x03003930,FLAG_TASKS=0x03003590,AVAILABLE=0x03003500,
       TILES=0x02013F20,FRAME=0x02010720,SHADOW=0x0200FF20,
       POSITION=0x0201DDC4,COLUMN_POSITION=0x0201DDC6,
       SCROLL=0x03002BC0,MODE=0x020046B1,BGX=HN_VIDEO_STATE+0x9A };
static uint8_t control(HNMemory *m,unsigned i){return hn_read8(m,HN_SCENE_CONTROLS+i);}
static void store(HNMemory *m,unsigned i,unsigned v){hn_write8(m,HN_SCENE_CONTROLS+i,(uint8_t)v);}
static GameAddr resource(HNMemory *m){return RESOURCES+control(m,0x180)*28;}
static GameAddr pointer(HNMemory *m,GameAddr table,unsigned i){return hn_read32(m,table+i*4);}
static bool flag(HNMemory *m,unsigned i){return ((hn_read8(m,0x03002180+(i>>3))>>(i&7))&1)!=0;}
static GameAddr localized(HNMemory *m,GameAddr text)
{return text==HN_TASK_DEFAULT_SCRIPT?text:pointer(m,text,control(m,0xEB));}
static unsigned group(unsigned row){return row>7?row-8:row>3?row-4:row;}

uint8_t hn_group4_boundary(HNMemory *m,uint32_t mode,GameAddr entry,uint32_t position,uint32_t selection)
{
    mode&=255;position&=65535;selection&=255;
    switch(mode){
    case 0:return position!=65535&&(int)position>=(int)hn_read16(m,entry+0x14)-1;
    case 2:return (int)selection==(int)hn_read8(m,entry+0x16)-1;
    case 3:return selection==0;
    case 4:return position==65535;
    default:return 0;
    }
}
bool hn_group4_text_line(HNMemory *m,GameAddr text,GameAddr output,uint32_t palette)
{
    uint16_t attributes=(uint16_t)(palette<<12);
    for(unsigned count=0;count<65536&&!m->fault;count++,text++){
        unsigned code=hn_read8(m,text);if(!code)return true;
        for(unsigned i=0;i<256;i++){
            unsigned replacement=hn_read8(m,0x086E9002+i*2);if(!replacement)break;
            if(code==replacement){code=hn_read8(m,0x086E9003+i*2);break;}
        }
        unsigned top=0,bottom=0;bool found=code==0x37;
        if(found){top=0x1F5;bottom=0x215;}
        else for(unsigned i=0;i<256;i++){
            GameAddr glyph=0x02000D84+i*4;unsigned key=hn_read8(m,glyph);if(!key)break;
            if(code==key){top=(hn_read32(m,glyph)>>8)&1023;bottom=(hn_read16(m,glyph+2)>>2)&1023;found=true;break;}
        }
        if(found){hn_write16(m,output,(uint16_t)(attributes|top));hn_write16(m,output+64,(uint16_t)(attributes|bottom));output+=2;}
    }
    return false;
}
bool hn_group4_text_lines(HNMemory *m,GameAddr text,GameAddr output,uint32_t palette,uint32_t width)
{
    GameAddr buffer=hn_heap_allocate(m,(width&255)+1,true);if(!buffer)return false;
    bool finished=false,ok=true;
    for(unsigned rows=0;rows<65536&&!finished&&ok&&!m->fault;rows++,output+=128){
        GameAddr cursor=buffer;unsigned code=0,count=0;
        while(count++<65536){code=hn_read8(m,text);if(!code||code==0xE0||code==0xE2)break;hn_write8(m,cursor++,(uint8_t)code);text++;}
        if(count>65536){ok=false;break;}
        hn_write8(m,cursor,0);finished=code==0||code==0xE0;if(!finished)text++;
        ok=hn_group4_text_line(m,buffer,output,(uint16_t)palette);
    }
    hn_heap_free(m,buffer);return finished&&ok&&!m->fault;
}
static bool create_context(HNMemory *m,GameAddr table,unsigned i,unsigned width,unsigned height,
                           unsigned x,unsigned advance,unsigned fill,unsigned flags,GameAddr graphics,GameAddr map)
{
    GameAddr task=hn_task_allocate(m);hn_write32(m,table+i*4,task);
    return task&&hn_task_graphics(m,task,width,height,x,advance,0,fill,flags,graphics,map);
}
static bool prepare(HNMemory *m)
{
    if(!hn_scene_save_actor_slots_raw(m)||!create_context(m,ROW_TASKS,0,12,2,0,136,2,0,0x06000020,SHADOW))return false;
    for(unsigned i=0;i<5;i++)if(!create_context(m,ROW_TASKS,i+1,12,2,0,1,2,0,0x06000320+i*0x300,SHADOW))return false;
    hn_write32(m,ROW_TASKS+24,0);
    for(unsigned i=0;i<15;i++){
        GameAddr graphics=i<=10?0x06000320+i*0x180:i<=13?0x06003980+(i-11)*0x180:0x06003680;
        if(!create_context(m,FLAG_TASKS,i,6,2,255,1,2,8,graphics,SHADOW))return false;
    }
    for(unsigned row=0;row<10;row++){
        GameAddr record=hn_read32(m,CONTEXT)+row*1100;if(hn_read8(m,record)!=1)continue;
        for(unsigned i=1;i<=11;i++)hn_write8(m,record+3+i,1);
        hn_write8(m,record+15,0);hn_write8(m,record+16,0);
        GameAddr text=localized(m,0x086E41EC);unsigned i=0;
        while(i<=6){unsigned code=hn_read8(m,text++);if(!code)break;hn_write8(m,record+5+i++,(uint8_t)code);}
        hn_write8(m,record+5+i,0);hn_write8(m,record+2,(uint8_t)i);
    }
    for(unsigned i=0;i<24;i++)hn_write32(m,AVAILABLE+i*4,0);
    unsigned column=0,rows=0;GameAddr out=AVAILABLE;
    for(unsigned i=0;i<86;i++){
        if(!column){hn_write8(m,out++,0x46);column=1;}
        unsigned value=hn_read8(m,0x086E8E54+control(m,0xEB)*86+i);
        if(flag(m,value)){hn_write8(m,out++,(uint8_t)value);if(column==1)rows++;column=(uint8_t)(column+1);if(column>13)column=0;}
    }
    store(m,0x18C,rows);return !m->fault;
}
static bool restore(HNMemory *m)
{
    for(unsigned i=15;i;i--)if(!hn_task_remove(m,pointer(m,FLAG_TASKS,i-1)))return false;
    if(pointer(m,ROW_TASKS,6)&&!hn_task_remove(m,pointer(m,ROW_TASKS,6)))return false;
    for(unsigned i=6;i;i--)if(!hn_task_remove(m,pointer(m,ROW_TASKS,i-1)))return false;
    const GameAddr indices[]={HN_RENDER_CLASS_LEADER,0x03002274,HN_RENDER_CLASS_FOLLOWER,0x03002270};
    uint8_t variant=(uint8_t)((hn_read8(m,HN_ACTOR_RENDER_SNAPSHOT+96)&15)-1);
    for(unsigned i=0;i<4;i++){
        unsigned index=hn_read8(m,indices[i]);GameAddr saved=HN_ACTOR_RENDER_SNAPSHOT+i*104;
        if(i&&(!index||!hn_read8(m,saved+96)))continue;
        if(i==2)variant=(uint8_t)((hn_read8(m,saved+96)&15)-1);
        if(!hn_memory_copy(m,hn_render_slot(m,index),saved,104))return false;
        unsigned family;GameAddr animation;
        if(!(i&1)){family=(i==2?(hn_read8(m,0x03001F60)^1):hn_read8(m,0x03001F60))*2+1;
            animation=pointer(m,pointer(m,HN_PRIMARY_ANIMATION_FAMILIES,family),variant);}
        else{family=hn_read8(m,0x03001F90)*2+(i==3);
            animation=pointer(m,pointer(m,HN_SECONDARY_ANIMATION_FAMILIES,family),variant*2);}
        hn_render_set_animation(m,hn_read8(m,indices[i]),animation);
    }
    return !m->fault;
}
static void columns(HNMemory *m)
{
    GameAddr text=hn_read32(m,resource(m)+4)+hn_read16(m,COLUMN_POSITION);
    for(unsigned i=0;i<15;i++){
        unsigned glyph=(uint8_t)(hn_read8(m,text++)*2),language=control(m,0xEB);
        hn_write16(m,TILES+i*2,hn_read16(m,0x086E8432+language*40+glyph*2));
        hn_write16(m,TILES+(i+15)*2,hn_read16(m,0x086E8432+control(m,0xEB)*40+(glyph+1)*2));
    }
    unsigned col=control(m,0x192)&31;
    if(col>17){
        unsigned remaining=32-col,wrapped=(uint8_t)(col+0xEF),offset=remaining*2;
        hn_transfer_tilemap_rows(m,TILES,0x0600B200+col*2,remaining,1,0);
        hn_transfer_tilemap_rows(m,TILES+offset,0x0600B200,wrapped,1,0);
        hn_transfer_tilemap_rows(m,TILES+30,0x0600B240+col*2,remaining,1,0);
        hn_transfer_tilemap_rows(m,TILES+30+offset,0x0600B240,wrapped,1,0);
    }else hn_transfer_tilemap_rows(m,TILES,0x0600B200+col*2,15,2,0);
}
static void scroll(HNMemory *m)
{
    GameAddr entry=resource(m);unsigned mode=hn_read8(m,MODE),reverse=hn_read8(m,MODE+1);store(m,0x189,0);
    if(mode==1||mode==5){
        if(!reverse&&(int)control(m,0x187)==(int)hn_read8(m,entry+0x16)-1){store(m,0x189,10);return;}
        if(reverse&&!control(m,0x187)&&mode!=5){store(m,0x189,10);return;}
        unsigned marker=hn_read8(m,hn_read32(m,entry+4)+hn_read16(m,POSITION)+7);
        if(mode==5&&(uint8_t)(marker-1)<=7){store(m,0x189,10);return;}
    }else if(hn_group4_boundary(m,mode,entry,hn_read16(m,POSITION),control(m,0x187))){store(m,0x189,10);return;}
    uint32_t old=hn_read32(m,SCROLL),step=mode==0?hn_read32(m,entry+12):mode==1?0x04000000:mode<=4?0x08000000:mode==5?0x02000000:0;
    uint32_t next=old+step;hn_write32(m,SCROLL,next);
    int8_t delta=(int8_t)((next>>24)-(old>>24));
    hn_write16(m,BGX,(uint16_t)(hn_read16(m,BGX)+(reverse?delta:-delta)));hn_video_mark_backgrounds(m,4);
    unsigned advance=(int32_t)hn_read32(m,SCROLL)>0x07FFFFFF;
    unsigned offset=(hn_read16(m,POSITION)+7+advance)&255;
    unsigned marker=hn_read8(m,hn_read32(m,entry+4)+offset);store(m,0x18F,(uint8_t)(marker-1)<=7?marker:0);
    if((int32_t)hn_read32(m,SCROLL)>0x07FFFFFF){
        hn_write32(m,SCROLL,hn_read32(m,SCROLL)+0xF8000000);
        hn_write16(m,POSITION,(uint16_t)(hn_read16(m,POSITION)+(reverse?-1:1)));
        if(hn_read16(m,POSITION)<=65534){
            hn_write16(m,COLUMN_POSITION,hn_read16(m,POSITION));hn_write8(m,MODE+1,(uint8_t)hn_read16(m,POSITION));columns(m);
        }
        unsigned current=hn_read16(m,POSITION);marker=hn_read8(m,hn_read32(m,entry+4)+current+7);store(m,0x189,marker?marker:9);
        if((uint8_t)(marker-1)<=7){
            if(mode!=5)store(m,0x187,control(m,0x187)+(reverse?-1:1));
            if(mode==1||mode==5)store(m,0x189,10);
        }
        if(hn_group4_boundary(m,mode,entry,current,control(m,0x187))){hn_write8(m,hn_render_slot(m,25)+0x60,0);store(m,0x189,10);}
    }
}
static bool row_context(HNMemory *m,GameAddr args,bool received)
{
    unsigned enabled=hn_read8(m,args),row=control(m,0x193),palette=(uint8_t)(control(m,0x192)+2);
    GameAddr task=pointer(m,ROW_TASKS,enabled?(uint8_t)(row+1):0),destination=enabled?0x06000320+row*0x300:0x06000020;
    hn_write8(m,task,0);hn_write8(m,task+0xB,0);hn_write8(m,task+7,(uint8_t)palette);hn_write32(m,task+0xC,destination);
    hn_write32(m,task+0x24,SHADOW);hn_write8(m,task+9,0);hn_write32(m,task+0x18,destination+SHADOW+0xF9FF4800u);
    hn_write8(m,task+1,12);hn_write8(m,task+2,2);hn_write8(m,task+0xA,0);hn_write8(m,task+3,0);
    hn_write16(m,task+0x1C,0);hn_write16(m,task+0x1E,0);hn_write8(m,task+4,0x88);hn_write8(m,task+5,0);
    if(!hn_transfer_fill_words(m,hn_read32(m,task+0x18),(palette&3)*0x44444444u,0xC0))return false;
    GameAddr text=control(m,0x193)<=3?(received?RECEIVED+3:hn_read32(m,CONTEXT)+control(m,0x191)*1100+control(m,0x193)*220+3):0x086E9058;
    return hn_task_set_script(m,task,text);
}
static bool flag_context(HNMemory *m,bool selected)
{
    unsigned index=selected?14:control(m,0x191),palette=selected?3:(uint8_t)(control(m,0x193)+2);
    GameAddr destination=selected?0x06003680:index<=10?0x06000320+index*0x180:0x06003980+(index-11)*0x180;
    GameAddr task=pointer(m,FLAG_TASKS,index);
    hn_write8(m,task,0);hn_write8(m,task+0xB,8);hn_write8(m,task+7,(uint8_t)palette);hn_write32(m,task+0xC,destination);
    hn_write32(m,task+0x24,SHADOW);hn_write8(m,task+9,0);hn_write32(m,task+0x18,destination+SHADOW+0xF9FF4800u);
    hn_write8(m,task+1,6);hn_write8(m,task+2,2);hn_write8(m,task+0xA,0);hn_write8(m,task+3,selected?255:0);
    hn_write16(m,task+0x1C,0);hn_write16(m,task+0x1E,0);hn_write8(m,task+4,1);hn_write8(m,task+5,0);
    if(!hn_transfer_fill_words(m,hn_read32(m,task+0x18),(palette&3)*0x44444444u,0x60))return false;
    unsigned value=selected?control(m,0x191):hn_read8(m,AVAILABLE+control(m,0x192)*14+control(m,0x191));
    GameAddr text=0x086E9061;
    if(selected){if(value>1)text=localized(m,flag(m,value+0x45)?pointer(m,HN_CHOICE_TEXT_PAIRS,value-1):0x086E4214);}
    else if(value){value=(uint8_t)(value-0x46);text=localized(m,value?pointer(m,HN_CHOICE_TEXT_PAIRS,value):0x086E4200);}
    return hn_task_set_script(m,task,text);
}
static bool prepare_row(HNMemory *m,GameAddr args)
{
    unsigned index=control(m,0x191);GameAddr record=hn_read32(m,CONTEXT)+0x2AF8,tiles=TILES;
    if(hn_read8(m,args))tiles+=(index+1)*64;
    if(index<=9&&!flag(m,hn_read16(m,0x086E83C4+index*2))&&!hn_read8(m,record+20+control(m,0x191)))index=12;
    if(!hn_bios_lz77(m,pointer(m,0x020020D0,control(m,0xEB)*14+index),tiles,0x40000))return false;
    if(index==12)for(unsigned i=0;i<4;i++)hn_write16(m,tiles+(i<2?i:i+12)*2,
        hn_read16(m,0x086E8234+control(m,0xEB)*80+control(m,0x191)*8+i*2));
    uint32_t consumed=0;if(hn_group4_callback(m,0x080C8C60,0,&consumed)!=HN_SCENE_OK)return false;
    unsigned palette=0;
    if(control(m,0x192))palette=control(m,0x63)==2&&hn_read8(m,args)?0x3000:0x5000;
    else if(control(m,0x63)==2)palette=0x6000;
    if(palette)for(unsigned i=0;i<28;i++)hn_write16(m,tiles+i*2,(uint16_t)((hn_read16(m,tiles+i*2)&4095)|palette));
    GameAddr destination=pointer(m,0x086E822C,control(m,0x193));unsigned width;
    if(hn_read8(m,args)){
        unsigned row=control(m,0x191),g=group(row);
        if(index<=9&&!hn_read8(m,record+row))hn_write8(m,hn_render_slot(m,g+35)+0x60,1);
        destination+=0x284+g*0x80;width=14;
    }else{for(unsigned i=0;i<13;i++)hn_write16(m,tiles+(i+13)*2,hn_read16(m,tiles+(i+14)*2));destination+=4;width=13;}
    hn_transfer_tilemap_rows(m,tiles,destination,width,2,0);return !m->fault;
}
HNSceneResult hn_group4_ui_callback(HNMemory *m,GameAddr callback,GameAddr args,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080C6A5C:if(!prepare(m))return HN_SCENE_RESOURCE;break;
    case 0x080C6D3C:if(!restore(m))return HN_SCENE_RESOURCE;break;
    case 0x080C750C:{
        GameAddr task=hn_task_allocate(m);hn_write32(m,ROW_TASKS+24,task);
        if(!task||!hn_task_graphics(m,task,24,(uint8_t)(control(m,0x194)<<1),0,1,0,3,0,0x06000020,TILES)||
           !hn_task_set_script(m,pointer(m,ROW_TASKS,6),localized(m,hn_read32(m,args))))return HN_SCENE_RESOURCE;
        *consumed=4;break;
    }
    case 0x080C7594:{
        unsigned offset=hn_read8(m,resource(m)+0x17);store(m,0x187,0);hn_write16(m,POSITION,(uint16_t)offset);
        hn_write16(m,BGX,(uint16_t)(12-offset*8));hn_video_mark_backgrounds(m,4);break;
    }
    case 0x080C75F0:
        if(!hn_read8(m,hn_render_slot(m,25)+0x60)){
            if(!hn_render_configure_raw(m,25,1,116,69,64,0,pointer(m,0x0200209C,control(m,0x191))))return HN_SCENE_RESOURCE;
            hn_write32(m,hn_render_slot(m,25)+0x24,(uint32_t)-((int32_t)hn_read32(m,resource(m)+12)>>8));
            hn_write32(m,hn_render_slot(m,25)+0x28,0);hn_write32(m,hn_render_slot(m,25)+0x2C,0);
            GameAddr slot=hn_render_slot(m,25);hn_write32(m,slot+0x18,hn_read32(m,slot+0x18)&0xFFFF0000u);
        }break;
    case 0x080C7684:
        store(m,0x61,0);if(control(m,0x60)?(int)control(m,0x187)==(int)hn_read8(m,resource(m)+0x16)-1:!control(m,0x187))store(m,0x61,1);break;
    case 0x080C76D4:scroll(m);break;
    case 0x080C7954:columns(m);break;
    case 0x080C7A90:{
        unsigned value=control(m,hn_read16(m,args));
        if(!hn_render_configure_raw(m,22,1,114,(int16_t)(value*24),32,0,pointer(m,0x020020BC,value)))return HN_SCENE_RESOURCE;
        *consumed=2;break;
    }
    case 0x080C7AD8:{
        for(unsigned i=0;i<640;i++)hn_write16(m,FRAME+i*2,0);
        GameAddr row=FRAME+hn_read8(m,args+3)*64+hn_read8(m,args+2)*2,cursor=row;
        hn_write16(m,cursor,0x1B0);cursor+=2;
        for(unsigned i=0;i<hn_read8(m,args);i++,cursor+=2)hn_write16(m,cursor,0x1B1);hn_write16(m,cursor,0x5B0);row+=64;
        for(unsigned i=0;i<hn_read8(m,args+1);i++,row+=64){
            cursor=row;hn_write16(m,cursor,0x1B2);cursor+=2;
            for(unsigned j=0;j<hn_read8(m,args);j++,cursor+=2)hn_write16(m,cursor,0x1B3);hn_write16(m,cursor,0x5B2);
        }
        cursor=row;hn_write16(m,cursor,0x9B0);cursor+=2;
        for(unsigned i=0;i<hn_read8(m,args);i++,cursor+=2)hn_write16(m,cursor,0x9B1);hn_write16(m,cursor,0xDB0);*consumed=4;break;
    }
    case 0x080C7BC0:
        if(!hn_group4_text_lines(m,localized(m,hn_read32(m,args+4)),FRAME+2+(hn_read8(m,args+3)+1)*64+hn_read8(m,args+2)*2,5,hn_read8(m,args)))return HN_SCENE_RESOURCE;
        *consumed=8;break;
    case 0x080C7C20:{
        GameAddr slot=hn_render_slot(m,23);hn_write32(m,slot+0x18,0x780000);hn_write32(m,slot+0x1C,0x500000+(group(control(m,0x180))<<20));hn_write32(m,slot+0x20,0);break;
    }
    case 0x080C7C5C:hn_render_set_animation(m,23,group(control(m,0x180))==control(m,0x181)?0x088FA8C2:0x088FA8CA);break;
    case 0x080C7CB0:if(!prepare_row(m,args))return HN_SCENE_RESOURCE;*consumed=1;break;
    case 0x080C7EBC:case 0x080C7FC4:if(!row_context(m,args,(callback&~1u)==0x080C7FC4))return HN_SCENE_RESOURCE;*consumed=1;break;
    case 0x080C80B4:{
        unsigned row=control(m,0x191);GameAddr tiles=0x02014320+row*8;
        if(!hn_memory_copy(m,tiles,pointer(m,0x02001EDC,control(m,0xEB)*21+hn_read8(m,args)*7+row-1),8))return HN_SCENE_RESOURCE;
        hn_transfer_tilemap_rows(m,tiles,pointer(m,0x02002080,(unsigned)control(m,0x191)-1),2,2,0);*consumed=1;break;
    }
    case 0x080C811C:case 0x080C8260:if(!flag_context(m,(callback&~1u)==0x080C8260))return HN_SCENE_RESOURCE;break;
    case 0x080C8348:hn_write8(m,RECEIVED+17+control(m,0x187),control(m,0x191));break;
    case 0x080C8370:{unsigned value=hn_read8(m,AVAILABLE+control(m,0x18B)*14+control(m,0x18A));store(m,0x191,value==0x46?1:value?value-0x45:0);break;}
    case 0x080C83BC:store(m,0x191,hn_read8(m,RECEIVED+17+control(m,0x187)));break;
    case 0x080C83E4:store(m,0x191,(control(m,0x18A)&1)?1:control(m,0x18B)?2:0);break;
    case 0x080C8420:store(m,0x191,!(control(m,0x18A)&1)?1:hn_read8(m,AVAILABLE+control(m,0x18B)*14+15)?2:0);break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
