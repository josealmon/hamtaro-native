#ifndef HN_GAME_RUNTIME_H
#define HN_GAME_RUNTIME_H
#include "gba/timing.h"
#include "gba/audio.h"
#include "game/script.h"
#include "game/render.h"
#include "game/background.h"
#include "game/graphics.h"
#include "game/task.h"
#include "game/audio.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { HN_GAME_OK,HN_GAME_INITIALIZATION,HN_GAME_MEMORY,HN_GAME_SCRIPT,HN_GAME_ACTOR,
               HN_GAME_MOVEMENT,HN_GAME_TASK,HN_GAME_BACKGROUND,HN_GAME_RENDER,HN_GAME_GRAPHICS,
               HN_GAME_AUDIO,HN_GAME_IRQ,HN_GAME_CPU_TRANSFER,HN_GAME_COMMUNICATION,HN_GAME_AUDIO_BUFFER } HNGameFault;
typedef struct {
    HNMemory memory;HNClock clock;HNApu apu;
    HNScript script;HNRender render;HNBackground background;HNGraphics graphics;HNTasks tasks;HNAudio audio;
    uint32_t pixels[HN_WIDTH*HN_HEIGHT];
    int16_t samples[2048];unsigned sample_frames;
    uint64_t frames;HNGameFault fault;unsigned detail,opcode;GameAddr address;
    bool initialized,reset_requested;
} HNGame;
bool hn_game_hardware_initialize(HNMemory *);
bool hn_game_initialize(HNGame *,const uint8_t *rom,size_t size,const uint8_t *sram);
/* One original game update per native VBlank, with native PPU/APU clocking. */
bool hn_game_frame(HNGame *,uint16_t buttons);
#ifdef __cplusplus
}
#endif
#endif
