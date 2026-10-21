#ifndef HN_GAME_ACTOR_H
#define HN_GAME_ACTOR_H
#include "game/script.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_actor_configure(HNMemory *,uint8_t index,GameAddr script);
bool hn_actor_frames(HNScript *,unsigned command_budget);
#ifdef __cplusplus
}
#endif
#endif
