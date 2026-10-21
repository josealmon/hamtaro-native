#include "platform/audio.h"
#include <limits>
#include <algorithm>
namespace hn {
Audio::Audio()
{
    SDL_AudioSpec source{SDL_AUDIO_S16,2,32768};
    stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&source,nullptr,nullptr);
    if(stream_) SDL_ResumeAudioStreamDevice(stream_);
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
void Audio::pause(bool pause)
{
    if(stream_) {
        if(pause) SDL_PauseAudioStreamDevice(stream_);
        else SDL_ResumeAudioStreamDevice(stream_);
    }
}
}
