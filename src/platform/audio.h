#pragma once
#include <SDL3/SDL.h>
#include <span>
#include <cstdint>
#include <atomic>
namespace hn {
class Audio {
    SDL_AudioStream *stream_=nullptr;
    std::atomic<std::uint64_t> shortages_{0};
public:
    Audio();
    ~Audio();
    Audio(const Audio &)=delete;
    bool available() const { return stream_!=nullptr; }
    void gain(int percent,bool mute);
    bool submit(std::span<const std::int16_t> stereo_samples);
    unsigned queued_frames() const;
    void clear();
    bool pause(bool);
    std::uint64_t shortages() const { return shortages_.load(std::memory_order_relaxed); }
};
}
