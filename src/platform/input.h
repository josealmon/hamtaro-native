#pragma once
#include "app/settings.h"
#include <SDL3/SDL.h>
#include <array>
#include <vector>
namespace hn {
class Input {
    std::array<bool,SDL_SCANCODE_COUNT> down_{};
    std::vector<SDL_Gamepad *> pads_;
    SDL_JoystickID active_=0;
    bool focused_=true;
public:
    bool pause_requested=false;
    Input();
    ~Input();
    Input(const Input &)=delete;
    Input &operator=(const Input &)=delete;
    void refresh();
    void event(const SDL_Event &);
    void clear();
    std::uint16_t buttons(const Settings &) const;
    const auto &pads() const { return pads_; }
    SDL_Gamepad *active() const;
    void select(SDL_JoystickID);
};
const char *button_label(SDL_Gamepad *,int button);
}
