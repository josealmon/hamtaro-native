#pragma once
#include "app/rom.h"
#include "game/runtime.h"
#include "platform/lan.h"
#include <memory>
#include <optional>
namespace hn {
class Session {
    Rom rom_;
    std::unique_ptr<HNGame> game_;
    Save committed_{};
    std::optional<Save> pending_;
    HNLink link_{};
    LanLink lan_;
    void capture_save();
public:
    LanLink &lan() { return lan_; }
    void poll_link() { lan_.poll(link_); }
    bool start(const Paths &,Rom rom);
    bool frame(const Paths &,std::uint16_t buttons);
    void persist(const Paths &);
    void stop(const Paths &);
    bool present() const { return game_!=nullptr; }
    bool playable() const { return game_&&game_->initialized&&!game_->fault; }
    bool save_pending() const { return pending_.has_value(); }
    const HNGame *game() const { return game_.get(); }
    std::span<const std::uint32_t> pixels() const;
    std::span<const std::int16_t> samples() const;
};
}
