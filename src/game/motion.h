#ifndef HN_GAME_MOTION_H
#define HN_GAME_MOTION_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_motion_route_obstacle(HNMemory *,uint32_t x1,uint32_t z1,uint32_t x2,uint32_t z2,uint32_t *obstacle);
bool hn_motion_rebuild_route(HNMemory *);
bool hn_motion_idle_render(HNMemory *,bool follower);
bool hn_motion_configure(HNMemory *,uint32_t leader,uint32_t follower,uint32_t control,uint32_t secondary,
                         uint32_t flags,uint32_t width,uint32_t height,GameAddr resource,GameAddr parameters);
#ifdef __cplusplus
}
#endif
#endif
