#pragma once
#include "app/storage.h"
#include <array>
namespace hn {
inline constexpr std::array<const char *,10> action_names={"A","B","Select","Start","Right","Left","Up","Down","R","L"};
struct Settings {
    std::array<int,10> keys{},buttons{};
    int deadzone=25,volume=80,window_width=960,window_height=720;
    bool fullscreen=false,integer_scale=true,mute=false,wasd=true,pause_unfocused=true;
    int language=0; // 0 Spanish, 1 English
    static Settings defaults();
};
Settings load_settings(const Paths &);
void save_settings(const Paths &,const Settings &);
}
