#include "game/link.h"
#include "game/state.h"
#include "game/input.h"
#include <string.h>

enum { STATE=0x020046B1,RECORD=0x03003D30 };
bool hn_link_record_valid(const HNLinkRecord *record)
{
    if(record->row==255)return true;
    if(record->row>=10||record->bytes[0]<1||record->bytes[0]>3||record->bytes[2]>7)return false;
    /* Ham-Chat numbers occupy the 111-byte dance payload. */
    for(unsigned i=17;i<128;i++)if(record->bytes[i]>86)return false;
    return true;
}
static void received_flag(HNMemory *m,bool value)
{
    GameAddr address=0x03002180+(0x285>>3);uint8_t old=hn_read8(m,address),mask=1u<<(0x285&7);
    hn_write8(m,address,value?(uint8_t)(old|mask):(uint8_t)(old&~mask));
}
static void finish(HNMemory *m,unsigned result)
{
    hn_write8(m,STATE,3);hn_write8(m,STATE+1,1);hn_write8(m,STATE+2,(uint8_t)result);
    if(m->link){m->link->result=(uint8_t)result;if(result!=1)m->link->active=false;}
}
HNSceneResult hn_link_callback(HNMemory *m,GameAddr callback,uint32_t *consumed)
{
    HNLink *link=m->link;*consumed=0;
    switch(callback&~1u){
    case 0x080C7024:{ /* InitializeCommunicationExchange: original send/receive selection. */
        bool sending=hn_read8(m,STATE)==0;
        if(link){
            uint32_t generation=link->generation+1;bool connected=link->connected;
            memset(link,0,sizeof(*link));link->generation=generation;link->connected=connected;link->active=true;
            link->outgoing.row=sending?hn_read8(m,0x020046A0):255;
            if(sending)for(unsigned i=0;i<128;i++)link->outgoing.bytes[i]=hn_read8(m,RECORD+i);
            link->failed=!hn_link_record_valid(&link->outgoing);
        }
        hn_write8(m,STATE,0);hn_write8(m,STATE+1,0);hn_write8(m,STATE+2,0);received_flag(m,false);break;
    }
    case 0x080C70B8: /* Preserve the original menu's wait/confirm/result contract. */
        if(hn_read8(m,STATE)==3){if(link&&link->result)hn_write8(m,STATE+2,link->result);break;}
        if(!link||(!link->connected&&!link->complete)||link->failed){finish(m,2);break;}
        if(!link->complete&&hn_input_query(m,HN_B,1)){link->failed=true;finish(m,2);break;}
        if(!link->peer_active)break;
        if(hn_read8(m,STATE)==0){hn_write8(m,STATE,4);break;}
        if(hn_read8(m,STATE+1))link->ready=true;
        if(link->peer_ready)hn_write8(m,STATE+1,1);
        if(link->complete&&link->ready){
            if(!hn_link_record_valid(&link->incoming)){finish(m,4);break;}
            if(link->incoming.row!=255){
                unsigned row=link->incoming.row;
                GameAddr context=hn_read32(m,0x03002BB0);
                if(context<0x02000000||context>0x02040000-0x2B80)return HN_SCENE_RESOURCE;
                for(unsigned i=0;i<128;i++)hn_write8(m,RECORD+i,link->incoming.bytes[i]);
                hn_write8(m,RECORD,3);hn_write8(m,HN_SCENE_CONTROLS+0x180,(uint8_t)row);
                hn_write8(m,context+0x2B0C+row,1);received_flag(m,true);
            }
            finish(m,link->outgoing.row==255&&link->incoming.row==255?3:1);
        }
        break;
    case 0x080C70AC:if(link)link->active=false;break;
    default:return HN_SCENE_UNKNOWN;
    }
    return m->fault?HN_SCENE_RESOURCE:HN_SCENE_OK;
}
