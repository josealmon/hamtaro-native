#ifndef HN_GAME_LINK_H
#define HN_GAME_LINK_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint8_t row,bytes[128]; } HNLinkRecord;
/* Host mailbox: never serialized into the game's RAM or save. */
typedef struct HNLink {
    uint32_t generation;
    bool connected,active,peer_active,ready,peer_ready,complete,failed;
    uint8_t result;
    HNLinkRecord outgoing,incoming;
} HNLink;
bool hn_link_record_valid(const HNLinkRecord *);
HNSceneResult hn_link_callback(HNMemory *,GameAddr,uint32_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
