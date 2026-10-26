#pragma once
#include "app/storage.h"
#include "app/i18n.h"
#include <array>
namespace hn {
inline constexpr std::array<const char *,10> action_names={"A","B","Select","Start","Right","Left","Up","Down","R","L"};
struct Settings {
    std::array<int,10> keys{},buttons{};
    int deadzone=25,volume=80,window_width=960,window_height=640;
    bool fullscreen=false,integer_scale=false,mute=false,wasd=true,pause_unfocused=false;
    int language=automatic_language;
    static Settings defaults();
};
Settings load_settings(const Paths &);
void save_settings(const Paths &,const Settings &);
std::array<int,2> game_window_size(int width,int height,int available_width,int available_height);
}
