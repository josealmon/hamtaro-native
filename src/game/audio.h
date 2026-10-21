#ifndef HN_GAME_AUDIO_H
#define HN_GAME_AUDIO_H
#include "gba/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {HN_AUDIO_IDENT=0x68736D53,HN_AUDIO_PLAYERS=6};
typedef enum {HN_AUDIO_OK,HN_AUDIO_MEMORY,HN_AUDIO_RESOURCE,HN_AUDIO_COMMAND,
              HN_AUDIO_CALLBACK,HN_AUDIO_BUDGET} HNAudioFault;
typedef struct {
    HNMemory *memory;HNAudioFault fault;unsigned fault_opcode;GameAddr fault_address;
} HNAudio;
typedef enum {HN_SONG_START,HN_SONG_CHANGE,HN_SONG_CONTINUE,HN_SONG_STOP,HN_SONG_RESUME} HNSongAction;
void hn_audio_init(HNAudio *,HNMemory *);
bool hn_audio_driver_init(HNMemory *);
bool hn_audio_ready(HNMemory *);
bool hn_audio_core_init(HNMemory *,GameAddr core);
bool hn_audio_psg_init(HNMemory *,GameAddr channels);
bool hn_audio_mode(HNMemory *,uint32_t mode);
bool hn_audio_frequency(HNMemory *,uint32_t mode);
void hn_audio_dma_pause(HNMemory *);
void hn_audio_dma_resume(HNMemory *);
void hn_audio_dma_vblank(HNMemory *);
bool hn_audio_player_configure(HNMemory *,GameAddr player,GameAddr tracks,uint32_t count);
bool hn_audio_player_start(HNMemory *,GameAddr player,GameAddr song);
bool hn_audio_player_stop(HNMemory *,GameAddr player);
void hn_audio_player_resume(HNMemory *,GameAddr player);
bool hn_audio_song(HNMemory *,uint16_t song,HNSongAction action);
bool hn_audio_all(HNMemory *,bool resume);
void hn_audio_fade(HNMemory *,GameAddr player,uint16_t interval,unsigned mode);
bool hn_audio_fade_update(HNMemory *,GameAddr player);
void hn_audio_tempo(HNMemory *,GameAddr player,uint16_t scale);
void hn_audio_track_parameter(HNMemory *,GameAddr player,uint32_t mask,uint32_t value,unsigned parameter);
void hn_audio_modulation_reset(HNMemory *,GameAddr track);
void hn_audio_volume_pitch(HNMemory *,GameAddr track);
bool hn_audio_track_stop(HNMemory *,GameAddr track);
bool hn_audio_track_finish(HNMemory *,GameAddr track);
void hn_audio_channel_unlink(HNMemory *,GameAddr channel);
bool hn_audio_clear_channels(HNMemory *);
void hn_audio_psg_stop(HNMemory *,uint8_t control);
void hn_audio_psg_volume(HNMemory *,GameAddr channel);
uint32_t hn_audio_wave_frequency(HNMemory *,GameAddr wave,uint32_t key,uint32_t fraction);
uint32_t hn_audio_psg_frequency(HNMemory *,uint8_t control,uint8_t key,uint8_t fraction);
bool hn_audio_sequence_command(HNAudio *,GameAddr player,GameAddr track,unsigned opcode);
void hn_audio_channel_volume(HNMemory *,GameAddr channel,GameAddr track);
bool hn_audio_note(HNAudio *,GameAddr player,GameAddr track,unsigned clock_selector);
bool hn_audio_player_update(HNAudio *,GameAddr player,unsigned command_budget);
bool hn_audio_update_players(HNAudio *,unsigned command_budget);
bool hn_audio_psg_update(HNAudio *);
bool hn_audio_mix(HNAudio *,unsigned command_budget);
#ifdef __cplusplus
}
#endif
#endif
