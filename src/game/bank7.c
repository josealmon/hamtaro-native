#include "game/bank7.h"
#include "game/actor_presets.h"
#include "game/state.h"
#include "game/render.h"
#include "game/graphics.h"
#include "game/background.h"

enum { ANIMATIONS=0x086CE184,COORDINATES=0x086CE1AC,VECTORS=0x086CE1D8,OFFSETS=0x020018E8 };
static uint8_t control(HNMemory *m,unsigned offset){return hn_read8(m,HN_SCENE_CONTROLS+offset);}
static void store(HNMemory *m,unsigned offset,uint8_t value){hn_write8(m,HN_SCENE_CONTROLS+offset,value);}
static uint8_t cursor(HNMemory *m){return control(m,0x1AA);}
static uint8_t class2(HNMemory *m){return hn_read8(m,0x03002C04);}
static void indicator(HNMemory *m,uint8_t index,GameAddr image)
{hn_write8(m,0x03001FAC,index);hn_write32(m,0x03001F7C,image);}
static void cursor_animation(HNMemory *m,GameAddr image)
{if(hn_read8(m,HN_RENDER_CLASS_FOLLOWER)==10)hn_write8(m,0x03002170,0);hn_render_set_animation(m,10,image);}
static void velocity(HNMemory *m,uint8_t index,int16_t x,int16_t y,int16_t z)
{GameAddr slot=hn_render_slot(m,index);hn_write32(m,slot+0x24,(uint32_t)((int32_t)x*256));hn_write32(m,slot+0x28,(uint32_t)((int32_t)y*256));hn_write32(m,slot+0x2C,(uint32_t)((int32_t)z*256));}
bool hn_bank7_move_tile(HNMemory *m,uint32_t position_word,uint32_t direction_word)
{
    unsigned position=(uint8_t)position_word,direction=(uint8_t)direction_word;
    uint16_t source=(uint16_t)(position+0x1A0),neighbor=(uint16_t)(source+hn_read16(m,OFFSETS+direction*2));
    uint8_t empty=hn_read8(m,HN_SCENE_CONTROLS+(uint32_t)(int32_t)(int16_t)neighbor);
    if(!empty){
        uint8_t tile=control(m,source);int16_t x=(int16_t)hn_read16(m,VECTORS+direction*4),y=(int16_t)hn_read16(m,VECTORS+direction*4+2);
        velocity(m,(uint8_t)(tile+24),x,y,0);
        /* The setter can alias the RAM offset table and the source cell. */
        int32_t target=(int16_t)hn_read16(m,OFFSETS+direction*2)+0x1A0+(int32_t)position;
        hn_write8(m,HN_SCENE_CONTROLS+(uint32_t)target,control(m,position+0x1A0));
        store(m,position+0x1A0,empty);store(m,0x60,1);
    }
    return !m->fault;
}
#define REQUIRE(op) do{if(!(op))return HN_SCENE_RESOURCE;}while(0)
HNSceneResult hn_bank7_callback(HNMemory *m,GameAddr callback,GameAddr arguments,uint32_t *consumed)
{
    *consumed=0;
    switch(callback&~1u){
    case 0x080B91A8:
        if(!(hn_read8(m,0x030021B5)&32)){
            /* These ten assignments are immediate initializer logic in the source. */
            store(m,0x1A0,1);store(m,0x1A1,2);store(m,0x1A2,6);store(m,0x1A3,5);store(m,0x1A4,0);
            store(m,0x1A5,7);store(m,0x1A6,3);store(m,0x1A7,8);store(m,0x1A8,4);store(m,0x1A9,9);
        }else for(unsigned i=0;i<10;i++)store(m,0x1A0+i,(uint8_t)i);
        break;
    case 0x080B9254:
        for(unsigned i=0;i<10;i++){
            uint8_t tile=control(m,0x1A0+i);if(tile)REQUIRE(hn_render_configure_raw(m,(uint8_t)(tile+24),1,0x72,
                (int16_t)hn_read16(m,COORDINATES+i*4),(int16_t)hn_read16(m,COORDINATES+i*4+2),0,hn_read32(m,ANIMATIONS+tile*4)));
        }break;
    case 0x080B92CC:{
        store(m,0,0);store(m,0x60,0);uint8_t direction=hn_read8(m,arguments);
        switch(direction){
        case 0:if(cursor(m)<=6){cursor_animation(m,0x088AC21B);velocity(m,10,0,0,256);store(m,0,1);
            if(!cursor(m))store(m,0x1AA,10);else{REQUIRE(hn_bank7_move_tile(m,cursor(m),0));store(m,0x1AA,(uint8_t)(cursor(m)+3));}}break;
        case 1:if(cursor(m)>3){cursor_animation(m,0x088AC2BA);velocity(m,10,0,0,-256);store(m,0,1);
            if(cursor(m)==10)store(m,0x1AA,0);else{REQUIRE(hn_bank7_move_tile(m,cursor(m),1));store(m,0x1AA,(uint8_t)(cursor(m)-3));}}break;
        case 2:if(cursor(m)&&cursor(m)!=10&&cursor(m)!=7){cursor_animation(m,0x088AC359);velocity(m,10,-256,0,0);store(m,0,1);
            if(cursor(m)==4)store(m,0x1AA,10);else{REQUIRE(hn_bank7_move_tile(m,cursor(m),2));store(m,0x1AA,(uint8_t)(cursor(m)-1));}}break;
        case 3:if(!cursor(m)||cursor(m)%3){cursor_animation(m,0x088AC3F8);velocity(m,10,256,0,0);store(m,0,1);
            if(cursor(m)==10)store(m,0x1AA,4);else{REQUIRE(hn_bank7_move_tile(m,cursor(m),3));store(m,0x1AA,(uint8_t)(cursor(m)+1));}}break;
        }
        *consumed=1;break;
    }
    case 0x080B94B4:
        store(m,0,0);for(unsigned i=0;i<10;i++)if(control(m,0x1A0+i)!=i){store(m,0,1);break;}break;
    case 0x080B94F0:{
        unsigned index=cursor(m);uint16_t x=hn_read16(m,COORDINATES+index*4),y=hn_read16(m,COORDINATES+index*4+2);bool open;
        if(cursor(m)>3){open=!control(m,cursor(m)+0x19D)&&cursor(m)!=10;
            REQUIRE(hn_render_configure_raw(m,34,1,0x72,(int16_t)(x+12),0,(int16_t)(y-16),open?0x088F4899:0x088F4879));}
        if(cursor(m)<=6){open=!control(m,cursor(m)+0x1A3);
            REQUIRE(hn_render_configure_raw(m,35,1,0x72,(int16_t)(x+12),0,(int16_t)(y+40),open?0x088F48A1:0x088F4881));}
        if(cursor(m)&&cursor(m)!=10&&cursor(m)!=7){open=!control(m,cursor(m)+0x19F);
            REQUIRE(hn_render_configure_raw(m,36,1,0x72,(int16_t)(x-16),0,(int16_t)(y+12),open?0x088F48A9:0x088F4889));}
        if(cursor(m)%3||!cursor(m)){open=!control(m,cursor(m)+0x1A1)&&cursor(m)!=10;
            REQUIRE(hn_render_configure_raw(m,37,1,0x72,(int16_t)(x+40),0,(int16_t)(y+12),open?0x088F48B1:0x088F4891));}
        break;
    }
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
static void slot16_position(HNMemory *m)
{GameAddr slot=hn_render_slot(m,16);hn_write32(m,slot+0x18,0x608000);hn_write32(m,slot+0x1C,0x8000);hn_write32(m,slot+0x20,0x408000);}
static bool actor(HNMemory *m,GameAddr address){return hn_actor_presets_apply(m,address)==HN_SCENE_OK;}
static bool class2_neighbor(HNMemory *m,GameAddr image)
{return hn_render_configure_raw(m,(uint8_t)(class2(m)-1),1,0xB0,0,0,0,image)&&hn_render_modes_raw(m,(uint8_t)(class2(m)-1),2,3,class2(m));}
HNSceneResult hn_bank7_task(HNMemory *m,GameAddr callback)
{
    switch(callback&~1u){
    case 0x080B9700:case 0x080B9808:break;
    case 0x080B9704:slot16_position(m);hn_render_set_animation(m,16,0x088C97B7);REQUIRE(hn_render_configure_raw(m,17,1,0xB0,0,0,0,0x088C97D4));REQUIRE(hn_render_modes_raw(m,17,2,3,16));break;
    case 0x080B974C:slot16_position(m);hn_render_set_animation(m,16,0x088C97F5);break;
    case 0x080B976C:slot16_position(m);hn_render_set_animation(m,16,0x088C9809);indicator(m,16,0x088C9809);break;
    case 0x080B9798:slot16_position(m);hn_render_set_animation(m,16,0x088C980C);break;
    case 0x080B97B8:slot16_position(m);hn_render_set_animation(m,16,0x088C9820);break;
    case 0x080B97D8:indicator(m,hn_read8(m,0x03002C00),0x088CABD0);break;
    case 0x080B97F0:indicator(m,hn_read8(m,0x03002C00),0x088CC40C);break;
    case 0x080B980C:REQUIRE(actor(m,0x08013D90)&&actor(m,0x0801A8C0));break;
    case 0x080B981C:REQUIRE(actor(m,0x08013DA8)&&actor(m,0x0801A914));break;
    case 0x080B982C:REQUIRE(actor(m,0x08013E20)&&class2_neighbor(m,0x088C4BA6));break;
    case 0x080B9874:REQUIRE(actor(m,0x0801499C));break;
    case 0x080B9880:REQUIRE(actor(m,0x080149E4)&&actor(m,0x0801A4AC)&&actor(m,0x0801A500));break;
    case 0x080B9894:REQUIRE(actor(m,0x08014A2C)&&actor(m,0x0801A554));break;
    case 0x080B98A4:hn_render_set_animation(m,class2(m),0x088C99F4);break;
    case 0x080B98BC:REQUIRE(hn_graphics_initialize_raw(m,4,1,0x088E135C));hn_render_set_animation(m,class2(m),0x088C9A09);REQUIRE(class2_neighbor(m,0x088C9A23));indicator(m,class2(m),0x088C9A09);break;
    case 0x080B9924:REQUIRE(hn_graphics_initialize_raw(m,4,1,0x088E135C));hn_render_set_animation(m,class2(m),0x088C9A2F);REQUIRE(class2_neighbor(m,0x088C9A49));indicator(m,class2(m),0x088C9A2F);break;
    case 0x080B998C:hn_render_set_animation(m,class2(m),0x088C9A55);REQUIRE(class2_neighbor(m,0x088C9A61));break;
    case 0x080B99DC:REQUIRE(hn_graphics_initialize_raw(m,4,1,0x088E13AB));hn_render_set_animation(m,class2(m),0x088C9A6D);REQUIRE(class2_neighbor(m,0x088C9A85));break;
    case 0x080B9AA4:REQUIRE(actor(m,0x08012EB0)&&actor(m,0x08013474));break;
    case 0x080B9A3C:case 0x080B9A70:case 0x080B9AB4:case 0x080B9AE8:case 0x080B9B34:{
        unsigned which=callback&~1u;GameAddr background=which==0x080B9A70?0x088F4E12:which==0x080B9AB4?0x088F4E06:which==0x080B9B34?0x088F4E44:0x088F4E2F;
        REQUIRE(hn_background_configure_raw(m,4,1,background));hn_render_set_animation(m,30,which==0x080B9AB4?0x088F4D0F:0x088F4BFD);
        hn_render_set_animation(m,24,which==0x080B9A3C?0x088F4A9C:which==0x080B9AE8?0x088F4AE0:which==0x080B9B34?0x088F4AA9:0x088F4A7A);
        if(which==0x080B9AE8)REQUIRE(hn_render_configure_raw(m,25,1,0x71,0,0,0,0x088F4AF5));else hn_write8(m,hn_render_slot(m,25)+0x60,0);break;
    }
    case 0x080B91A8:case 0x080B9254:case 0x080B94B4:case 0x080B94F0:{uint32_t ignored;return hn_bank7_callback(m,callback,0,&ignored);}
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
