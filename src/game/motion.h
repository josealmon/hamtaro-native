#ifndef HN_GAME_MOTION_H
#define HN_GAME_MOTION_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_motion_route_obstacle(HNMemory *,uint32_t x1,uint32_t z1,uint32_t x2,uint32_t z2,uint32_t *obstacle);
bool hn_motion_rebuild_route(HNMemory *);
bool hn_motion_idle_render(HNMemory *,bool follower);
uint32_t hn_motion_direction(HNMemory *,GameAddr slot);
uint32_t hn_motion_classify(HNMemory *,GameAddr slot);
bool hn_motion_steps(HNMemory *,uint16_t *x,uint16_t *z,uint32_t *mode);
bool hn_motion_extend_route(HNMemory *);
bool hn_motion_follower(HNMemory *);
bool hn_motion_update_following(HNMemory *);
void hn_motion_reset(HNMemory *);
void hn_motion_control(HNMemory *);
bool hn_motion_leader(HNMemory *,uint8_t *direction,uint8_t *variant,uint32_t *mode);
bool hn_motion_advance(HNMemory *);
bool hn_motion_configure(HNMemory *,uint32_t leader,uint32_t follower,uint32_t control,uint32_t secondary,
                         uint32_t flags,uint32_t width,uint32_t height,GameAddr resource,GameAddr parameters);
#ifdef __cplusplus
}
#endif
#endif
