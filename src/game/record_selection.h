#ifndef HN_GAME_RECORD_SELECTION_H
#define HN_GAME_RECORD_SELECTION_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
HNSceneResult hn_record_selection_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
HNSceneResult hn_record_selection_task(HNMemory *,GameAddr callback);
bool hn_record_counter_palette(HNMemory *,GameAddr destination,uint32_t record);
bool hn_record_counter_rows(HNMemory *,uint32_t index);
bool hn_record_palette_halve(HNMemory *,GameAddr colors);
bool hn_record_selection_refresh(HNMemory *,GameAddr arguments);
bool hn_record_pick_candidate(HNMemory *,uint32_t selected,uint32_t count,GameAddr candidates,uint32_t *result);
#ifdef __cplusplus
}
#endif
#endif
