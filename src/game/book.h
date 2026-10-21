#ifndef HN_GAME_BOOK_H
#define HN_GAME_BOOK_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_book_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
