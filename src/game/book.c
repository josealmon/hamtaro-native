#include "game/book.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/task.h"

static bool flag(HNMemory *m,unsigned index)
{return ((hn_read8(m,0x03002180+index/8)>>(index&7))&1)!=0;}
static void set_flag(HNMemory *m,unsigned index)
{GameAddr address=0x03002180+index/8;hn_write8(m,address,(uint8_t)(hn_read8(m,address)|(1u<<(index&7))));}
static unsigned control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,unsigned value){hn_write8(m,HN_SCENE_CONTROLS+offset,(uint8_t)value);}
static void append_word(HNMemory *m,uint8_t word)
{
    unsigned count=hn_read8(m,HN_BOOK_COUNT);hn_write8(m,HN_BOOK_WORDS+count,word);
    hn_write8(m,HN_BOOK_COUNT,(uint8_t)(count+1));
}
static void sorted_page(HNMemory *m,uint8_t selected,unsigned *category,unsigned *page,unsigned *row)
{
    uint8_t ordered=hn_read8(m,HN_BOOK_ORDER+selected+control(m,0xEB)*86);
    *category=9;GameAddr categories=HN_BOOK_CATEGORIES+control(m,0xEB)*11;
    while(*category&&ordered<hn_read8(m,categories+*category))(*category)--;
    *page=ordered>>2;
    *row=(unsigned)(((int)ordered-(int)hn_read8(m,categories+*category))%4+1);
}
static void acquired_page(HNMemory *m)
{
    unsigned category=0,page=0,row=1;
    if(flag(m,0xF8))sorted_page(m,(uint8_t)(control(m,0x76)-1),&category,&page,&row);
    store(m,0x70,category);store(m,0x71,page);store(m,0x72,row);
}
static bool restore_page(HNMemory *m)
{
    unsigned category=0,page=0,row=1;uint8_t selected=(uint8_t)(control(m,0x22)-1);
    if(control(m,0x72)&&control(m,0x22)){
        if(flag(m,0xF8))sorted_page(m,selected,&category,&page,&row);
        else{
            unsigned count=hn_read8(m,HN_BOOK_COUNT),index;
            for(index=0;index<count;index++)if(hn_read8(m,HN_BOOK_WORDS+index)==selected+1)break;
            if(index==count)return false; /* The source requires a present item. */
            page=(count-index-1)/4;row=(count-index)%4;if(!row)row=4;
        }
    }
    store(m,0x70,category);store(m,0x71,page);store(m,0x72,row);return !m->fault;
}
static void acquire(HNMemory *m)
{
    unsigned selected=control(m,0x76);if(!selected)return;
    if(selected&128){
        if(selected==255){
            /* Word identifiers and flag layout; word content stays in the ROM. */
            static const uint8_t initial_words[]={1,2,3,4,7,8,9,10,13,63};
            for(unsigned i=0;i<sizeof(initial_words);i++){set_flag(m,initial_words[i]+0x46);append_word(m,initial_words[i]);}
            set_flag(m,0x107);set_flag(m,0x108);set_flag(m,0x109);
            store(m,0x76,1);acquired_page(m);store(m,0x76,0);return;
        }
        set_flag(m,0x107);set_flag(m,0x108);store(m,0x76,selected&127);
    }else{
        if(flag(m,selected+0x46))return;
        set_flag(m,selected+0x46);set_flag(m,0x107);append_word(m,(uint8_t)selected);
    }
    acquired_page(m);store(m,0x76,0);
}
static void resolve(HNMemory *m)
{
    store(m,0x22,0);unsigned selected;
    if(flag(m,0xF8)){
        uint8_t ordered=(uint8_t)(control(m,0x71)*4+control(m,0x72)-1);
        for(selected=0;selected<86;selected++)if(ordered==hn_read8(m,HN_BOOK_ORDER+selected+control(m,0xEB)*86))break;
    }else{
        int index=(int)hn_read8(m,HN_BOOK_COUNT)-(int)(control(m,0x71)*4+control(m,0x72));
        selected=(uint8_t)(hn_read8(m,HN_BOOK_WORDS+(uint32_t)index)-1);
    }
    if(flag(m,selected+0x47))store(m,0x22,selected+1);
}
static uint8_t marker_y(HNMemory *m)
{return (uint8_t)((((uint32_t)control(m,0x72)<<28)+0x10000000u)>>24);}
static bool slot(HNMemory *m,uint8_t index,int x,int y,int z,GameAddr animation)
{return hn_render_configure(m,index,1,120,x,y,z,animation);}
static bool page_slots(HNMemory *m)
{
    unsigned page=control(m,0x70);int x=page==9?168:(int)page*16+24;
    GameAddr first=page==9?0x088F77B8:0x088F77AB;
    GameAddr second=page==9?0x088F780D:hn_read32(m,HN_BOOK_PAGE_ANIMATIONS+page*4);
    if(!slot(m,25,x,0,0,first)||!slot(m,32,x,0,0,second))return false;
    hn_render_flags(m,25,12,12);hn_render_flags(m,32,12,12);
    return hn_graphics_configure(m,4,hn_read32(m,HN_BOOK_PAGE_GRAPHICS+page*4));
}
static bool decorations(HNMemory *m)
{
    static const struct {uint8_t index;GameAddr animation;} entries[]={
        {20,0x088F7586},{21,0x088F752F},{16,0x088F75E4},{17,0x088F7603},{18,0x088F7622},{19,0x088F7641},
        {29,0x088F76F6},{30,0x088F76C8},{31,0x088F76AE},{26,0x088F7774},{27,0x088F7741},{28,0x088F7722}};
    unsigned y=marker_y(m);
    for(unsigned i=0;i<sizeof(entries)/sizeof(*entries);i++){
        int x=i<2?168:i<6?0:i<9?16:88,sy=i<2?(int)y-96:i<6?0:(int)y;
        if(!slot(m,entries[i].index,x,sy,0,entries[i].animation))return false;
    }
    return hn_graphics_configure(m,4,0x088F7833);
}
static bool pairs(HNMemory *m,bool second)
{
    GameAddr scripts=second?HN_BOOK_SCRIPTS_SECOND:HN_BOOK_SCRIPTS_FIRST;
    GameAddr graphics=second?HN_BOOK_GRAPHICS_SECOND:HN_BOOK_GRAPHICS_FIRST;
    unsigned first=second?4:0;
    for(unsigned i=0;i<4;i++){
        GameAddr task=hn_task_allocate(m);if(!task)return false;
        hn_write32(m,HN_BOOK_CONTEXTS+(first+i)*4,task);
        if(!hn_task_graphics(m,task,second?10:8,2,255,1,0,3,16,hn_read32(m,graphics+i*4),0x0600CE00))return false;
        unsigned word=0;bool set=false;
        if(flag(m,0xF8)){
            uint8_t wanted=(uint8_t)(control(m,0x71)*4+i);
            for(unsigned j=0;j<86;j++)if(wanted==hn_read8(m,HN_BOOK_ORDER+j+control(m,0xEB)*86)){
                word=flag(m,j+0x47)?j+1:0;set=true;break;
            }
        }else{
            int remaining=(int)hn_read8(m,HN_BOOK_COUNT)-(int)(control(m,0x71)*4+i);
            if(remaining>0){
                word=hn_read8(m,HN_BOOK_WORDS+(unsigned)remaining-1);
                if(flag(m,word+0x46)&&word)set=true;
            }
        }
        if(set){
            GameAddr script=hn_read32(m,scripts+word*4);
            if(script!=HN_TASK_DEFAULT_SCRIPT)script=hn_read32(m,script+control(m,0xEB)*4);
            if(!hn_task_set_script(m,hn_read32(m,HN_BOOK_CONTEXTS+(first+i)*4),script))return false;
        }
    }
    return !m->fault;
}
static bool position24(HNMemory *m,bool page)
{
    if(hn_read8(m,HN_RENDER_INFO+16)<=24||!hn_read32(m,HN_RENDER_SLOTS))return false;
    GameAddr render=hn_render_slot(m,24);
    hn_write32(m,render+0x18,page?((uint32_t)control(m,0x70)<<20)+0x200000:0x100000);
    hn_write32(m,render+0x1C,page?0x190000:((uint32_t)control(m,0x72)<<20)+0x180000);
    hn_write32(m,render+0x20,0);return !m->fault;
}
HNSceneResult hn_book_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080C1D78:{
        GameAddr task=hn_task_allocate(m);if(!task)return HN_SCENE_RESOURCE;
        hn_write32(m,HN_BOOK_CONTEXTS,task);
        if(!hn_task_graphics(m,task,9,2,255,2,0,0,0,0x06004020,0x0600CE00)||!hn_task_set_script(m,hn_read32(m,HN_BOOK_CONTEXTS),HN_TEXT_FIRST))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080C1DC4:case 0x080C1F28:if(!pairs(m,(callback&~1u)==0x080C1F28))return HN_SCENE_RESOURCE;break;
    case 0x080C2090:if(!hn_task_remove(m,hn_read32(m,HN_BOOK_CONTEXTS)))return HN_SCENE_RESOURCE;break;
    case 0x080C20A4:
        for(unsigned i=8;i;i--)if(!hn_task_remove(m,hn_read32(m,HN_BOOK_CONTEXTS+(i-1)*4)))return HN_SCENE_RESOURCE;
        break;
    case 0x080C20C8:case 0x080C20F0:if(!position24(m,(callback&~1u)==0x080C20C8))return HN_SCENE_RESOURCE;break;
    case 0x080C2118:if(!page_slots(m))return HN_SCENE_RESOURCE;break;
    case 0x080C21E8:
        if(flag(m,0xF8)){
            unsigned language=control(m,0xEB);
            if(language<5){static const uint8_t pages[]={24,25,26,25,26};store(m,0x60,pages[language]);}
        }else{unsigned count=hn_read8(m,HN_BOOK_COUNT);store(m,0x60,(count>>2)+((count&3)!=0));}
        break;
    case 0x080C2260:{
        unsigned index=control(m,0x70)+hn_read8(m,arguments)+control(m,0xEB)*11;
        store(m,0x60,control(m,0x71)==((unsigned)hn_read8(m,HN_BOOK_CATEGORIES+index)>>2));*consumed=1;break;
    }
    case 0x080C229C:store(m,0x71,hn_read8(m,HN_BOOK_CATEGORIES+control(m,0xEB)*11+control(m,0x70))>>2);break;
    case 0x080C22CC:{
        unsigned value=control(m,0x60),index=hn_read8(m,arguments),x=hn_read8(m,arguments+1),y=hn_read8(m,arguments+2);
        if(!slot(m,(uint8_t)index,(int)x,(int)y,0,0x0889F620))return HN_SCENE_RESOURCE;
        if(value/10)hn_render_set_animation(m,index,hn_read32(m,HN_BOOK_DIGITS+(value/10)*4));
        if(!slot(m,(uint8_t)(index+1),(int)x+5,(int)y,0,hn_read32(m,HN_BOOK_DIGITS+(value%10)*4)))return HN_SCENE_RESOURCE;
        hn_render_flags(m,index,12,8);hn_render_flags(m,index+1,12,8);*consumed=3;break;
    }
    case 0x080C2384:if(!restore_page(m))return HN_SCENE_RESOURCE;break;
    case 0x080C24B0:acquired_page(m);break;
    case 0x080C2558:{
        unsigned y=marker_y(m);
        if(!slot(m,22,88,32,0,0x088F769E)||!slot(m,29,16,(int)y,0,0x088F76C2)||!slot(m,26,88,(int)y,0,0x088F773B))return HN_SCENE_RESOURCE;
        break;
    }
    case 0x080C25C8:if(!decorations(m))return HN_SCENE_RESOURCE;break;
    case 0x080C2720:acquire(m);break;
    case 0x080C28DC:resolve(m);break;
    case 0x080C2990:
        resolve(m);
        if(control(m,0x22)){
            for(unsigned i=2;i<=3;i++)if(!slot(m,(uint8_t)(i==2?10:13),(int16_t)hn_read16(m,HN_BOOK_MARKER_POSITIONS+i*2),0,112,
                                            hn_read32(m,HN_BOOK_MARKER_ANIMATIONS+i*4)))return HN_SCENE_RESOURCE;
            set_flag(m,0x112);
        }
        break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
