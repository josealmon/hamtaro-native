#include "platform/audio.h"
#include <limits>
#include <algorithm>
#include <array>
namespace hn {
Audio::Audio()
{
    SDL_AudioSpec source{SDL_AUDIO_S16,2,32768};
    stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&source,
        [](void *user,SDL_AudioStream *,int additional,int) {
            if(additional>0)static_cast<Audio*>(user)->shortages_.fetch_add(1,std::memory_order_relaxed);
        },this);
    /* SDL opens the device paused. Playback starts only when the game resumes. */
}
Audio::~Audio() { if(stream_) SDL_DestroyAudioStream(stream_); }
void Audio::gain(int percent,bool mute)
{ if(stream_) SDL_SetAudioStreamGain(stream_,mute?0.0f:std::clamp(percent,0,100)/100.0f); }
bool Audio::submit(std::span<const std::int16_t> samples)
{
    if(!stream_ || samples.size()%2 || samples.size_bytes()>std::numeric_limits<int>::max()) return false;
    /* Backpressure belongs to the frame scheduler; do not discard queued game audio. */
    if(SDL_GetAudioStreamQueued(stream_)>32768*2*2) return false;
    return SDL_PutAudioStreamData(stream_,samples.data(),static_cast<int>(samples.size_bytes()));
}
bool Audio::pause(bool pause)
{
    if(!stream_)return true;
    if(pause)return SDL_PauseAudioStreamDevice(stream_);
    /* A small fixed lead absorbs device scheduling jitter without changing the
       original game clock or discarding samples. Retain queued audio on pause. */
    if(!queued_frames()) {
        const std::array<std::int16_t,4096> silence{};
        if(!submit(silence))return false;
    }
    return SDL_ResumeAudioStreamDevice(stream_);
}
unsigned Audio::queued_frames() const
{ return stream_?static_cast<unsigned>(std::max(0,SDL_GetAudioStreamQueued(stream_)))/4:0; }
void Audio::clear(){if(stream_)SDL_ClearAudioStream(stream_);}
}
