#include "game/expression.h"
typedef struct { HNMemory *m;unsigned budget,depth;bool update,failed; } Expression;
enum { CONTROLS=0x02004520,CLASSES=0x0201DC40,FLAGS=0x03002180 };
static GameAddr cursor_slot(const Expression *e){return e->update?0x0300079C:0x03000798;}
static uint16_t next(Expression *e)
{
    HNMemory *m=e->m;GameAddr slot=cursor_slot(e),p=hn_read32(m,slot);
    uint16_t value=hn_read16(m,p);hn_write32(m,slot,p+2);return value;
}
static uint32_t evaluate(Expression *e,GameAddr address);
static uint32_t right(Expression *e){return evaluate(e,hn_read32(e->m,cursor_slot(e)));}
static uint32_t operand(Expression *e,unsigned tag,unsigned selector)
{
    HNMemory *m=e->m;
    switch(tag){
    case 0x6000:return hn_read8(m,CONTROLS+selector);
    case 0x8000:return hn_read16(m,CLASSES+selector*2);
    case 0xA000:return selector;
    case 0xC000:return next(e);
    case 0x2000:return (hn_read8(m,FLAGS+(selector>>3))>>(selector&7))&1;
    case 0x4000:return 1^((hn_read8(m,FLAGS+(selector>>3))>>(selector&7))&1);
    default:return 0;
    }
}
static uint32_t evaluate(Expression *e,GameAddr address)
{
    if(!e->budget||e->depth>=128||e->m->fault){e->failed=true;return 0;}
    e->budget--;e->depth++;
    HNMemory *m=e->m;unsigned word=hn_read16(m,address);
    unsigned tag=word&0xE000,operation=word&0x1E00,selector=word&0x1FF;
    hn_write32(m,cursor_slot(e),address+2);
    uint32_t value=operand(e,tag,selector);
    while(!e->failed&&!m->fault){
        if(!e->budget){e->failed=true;break;}e->budget--;
        if(operation==0){
            value=right(e);operation=next(e)&0x1E00;continue;
        }
        if(operation==0x1A00)value=1;
        else if(operation==0x1C00)value=0;
        else if(operation==0x0A00)value+=right(e);
        else if(operation==0x0C00)value-=right(e);
        else if(!e->update){
            switch(operation){
            case 0x1000:value=value==right(e);break;
            case 0x1200:value=(int32_t)value<(int32_t)right(e);break;
            case 0x0E00:value=(int32_t)value>=(int32_t)right(e);break;
            case 0x0400:if(value)value=right(e)!=0;break;
            case 0x0600:if(!value)value=right(e)!=0;else value=1;break;
            case 0x0800:{uint32_t other=right(e);value=(value!=0)^(other!=0);break;}
            }
        }else{
            switch(operation){
            case 0x0400:value&=right(e);break;
            case 0x0600:value|=right(e);break;
            case 0x0800:value^=right(e);break;
            case 0x1400:
                value=right(e);
                if(tag==0x6000)hn_write8(m,CONTROLS+selector,(uint8_t)value);
                if(tag==0x8000)hn_write16(m,CLASSES+selector*2,(uint16_t)value);
                break;
            case 0x1600:case 0x1800:
                if(tag==0x6000||tag==0x8000){
                    uint32_t other=right(e);
                    value=tag==0x6000?hn_read8(m,CONTROLS+selector):hn_read16(m,CLASSES+selector*2);
                    value=operation==0x1600?value+other:value-other;
                    if(tag==0x6000)hn_write8(m,CONTROLS+selector,(uint8_t)value);
                    else hn_write16(m,CLASSES+selector*2,(uint16_t)value);
                }
                break;
            default:
                if(tag==0x2000||tag==0x4000){
                    GameAddr p=FLAGS+(selector>>3);unsigned old=hn_read8(m,p),bit=1u<<(selector&7);
                    value=tag==0x2000;hn_write8(m,p,(uint8_t)(value?old|bit:old&~bit));
                }
                break;
            }
        }
        break;
    }
    e->depth--;return value;
}
bool hn_expression_evaluate(HNMemory *m,GameAddr stream,bool update,uint32_t *result)
{
    Expression e={m,4096,0,update,false};uint32_t value=evaluate(&e,stream);
    if(e.failed||m->fault)return false;
    *result=value;return true;
}
