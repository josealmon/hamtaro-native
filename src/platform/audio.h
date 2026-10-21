#pragma once
#include <SDL3/SDL.h>
#include <span>
#include <cstdint>
namespace hn {
class Audio {
    SDL_AudioStream *stream_=nullptr;
public:
    Audio();
    ~Audio();
    Audio(const Audio &)=delete;
    bool available() const { return stream_!=nullptr; }
    void gain(int percent,bool mute);
    bool submit(std::span<const std::int16_t> stereo_samples);
    void pause(bool);
};
}
