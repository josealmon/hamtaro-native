#ifndef HN_GAME_RECORD_MENU_H
#define HN_GAME_RECORD_MENU_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_record_menu_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_record_menu_task(HNMemory *,GameAddr callback);
bool hn_record_menu_load_list(HNMemory *);
bool hn_record_tile_pairs(HNMemory *,GameAddr text,GameAddr table,GameAddr upper,GameAddr lower,uint32_t palette);
#ifdef __cplusplus
}
#endif
#endif
