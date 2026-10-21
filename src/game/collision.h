#ifndef HN_GAME_COLLISION_H
#define HN_GAME_COLLISION_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
uint32_t hn_collision_facing(HNMemory *,GameAddr slot);
GameAddr hn_collision_sample(HNMemory *,GameAddr map,int32_t x,int32_t z,uint32_t direction,uint32_t sample);
GameAddr hn_collision_ahead(HNMemory *,GameAddr map,int32_t x,int32_t z,uint32_t direction);
uint32_t hn_collision_response_direction(uint32_t mask,uint32_t direction);
uint32_t hn_collision_mask(HNMemory *,GameAddr map,int32_t x,int32_t z);
void hn_collision_stage(HNMemory *,uint32_t script);
bool hn_collision_contains(HNMemory *,GameAddr record,int32_t x,int32_t z,uint32_t *hit);
bool hn_collision_query(HNMemory *,uint32_t render,GameAddr record,uint32_t *script);
bool hn_collision_response(HNMemory *,uint32_t render,GameAddr record);
bool hn_collision_has_record(HNMemory *,int32_t x,int32_t z,uint32_t *hit);
bool hn_collision_records(HNMemory *,uint32_t control,uint32_t render,bool resolve);
bool hn_collision_flags(HNMemory *,uint32_t control,uint32_t render);
bool hn_collision_scripts(HNMemory *,uint32_t control,uint32_t render);
#ifdef __cplusplus
}
#endif
#endif
