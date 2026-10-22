#ifndef HN_GAME_PARTICLES_H
#define HN_GAME_PARTICLES_H
#include "game/scene.h"
#ifdef __cplusplus
extern "C" {
#endif
bool hn_particles_initialize(HNMemory *);
HNSceneResult hn_particles_callback(HNMemory *,GameAddr callback,uint32_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
