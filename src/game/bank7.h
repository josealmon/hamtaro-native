#ifndef HN_GAME_BANK7_H
#define HN_GAME_BANK7_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_bank7_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_bank7_task(HNMemory *,GameAddr callback);
bool hn_bank7_move_tile(HNMemory *,uint32_t position_word,uint32_t direction_word);
#ifdef __cplusplus
}
#endif
#endif
