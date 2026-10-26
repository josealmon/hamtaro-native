#pragma once
#include <SDL3/SDL.h>
#include <span>
#include <cstdint>
#include <atomic>
#include <array>
namespace hn {
/* Host output coupling: remove the hardware DAC's DC level at about 10 Hz.
   Keep state across packets and pauses; only a new session resets it. */
class AudioDcBlocker {
    std::array<float,2> capacitor_{};
public:
    float sample(std::int16_t input,unsigned channel) {
        const float value=input/32768.0f;
        const float output=value-capacitor_[channel];
        capacitor_[channel]=value-output*(511.0f/512.0f);
        return output;
    }
};
class Audio {
    SDL_AudioStream *stream_=nullptr;
    std::atomic<std::uint64_t> shortages_{0};
    AudioDcBlocker filter_;
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
