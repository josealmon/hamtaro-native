#ifndef HN_GAME_RECORDS_H
#define HN_GAME_RECORDS_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
uint8_t hn_records_count(HNMemory *);
int32_t hn_record_allocate(HNMemory *,uint16_t key);
int32_t hn_record_query(HNMemory *,uint8_t index,GameAddr result);
int32_t hn_record_consume(HNMemory *,uint16_t index,uint8_t comparison,GameAddr result);
uint32_t hn_record_adjust_counter(HNMemory *,uint8_t index,int32_t delta);
uint8_t hn_record_counter(HNMemory *,uint8_t index);
HNSceneResult hn_records_callback(HNMemory *,GameAddr callback,GameAddr arguments,uint32_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
