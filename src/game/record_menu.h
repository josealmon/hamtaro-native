#ifndef HN_GAME_RECORD_MENU_H
#define HN_GAME_RECORD_MENU_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_record_menu_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_record_menu_task(HNMemory *,GameAddr callback);
bool hn_record_menu_load_list(HNMemory *);
#ifdef __cplusplus
}
#endif
#endif
