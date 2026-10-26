#include "app/settings.h"
#include "gba/video.h"
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_gamepad.h>
#include <sstream>
#include <map>
#include <charconv>
#include <algorithm>
#include <stdexcept>
namespace hn {
std::array<int,2> game_window_size(int width,int height,int available_width,int available_height)
{
    const int limit=std::max(1,std::min(available_width/HN_WIDTH,available_height/HN_HEIGHT));
    const int nearest=std::min((width+HN_WIDTH/2)/HN_WIDTH,(height+HN_HEIGHT/2)/HN_HEIGHT);
    // Prefer at least 3x for launcher readability, within the monitor's work area.
    const int scale=std::clamp(nearest,std::min(3,limit),limit);
    return {HN_WIDTH*scale,HN_HEIGHT*scale};
}
Settings Settings::defaults()
{
    Settings s;
    s.keys={SDL_SCANCODE_Z,SDL_SCANCODE_X,SDL_SCANCODE_RSHIFT,SDL_SCANCODE_RETURN,
            SDL_SCANCODE_RIGHT,SDL_SCANCODE_LEFT,SDL_SCANCODE_UP,SDL_SCANCODE_DOWN,
            SDL_SCANCODE_E,SDL_SCANCODE_Q};
    s.buttons={SDL_GAMEPAD_BUTTON_SOUTH,SDL_GAMEPAD_BUTTON_EAST,SDL_GAMEPAD_BUTTON_BACK,
               SDL_GAMEPAD_BUTTON_START,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,SDL_GAMEPAD_BUTTON_DPAD_LEFT,
               SDL_GAMEPAD_BUTTON_DPAD_UP,SDL_GAMEPAD_BUTTON_DPAD_DOWN,
               SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER};
    return s;
}
bool Settings::wasd_available() const
{
    return std::none_of(keys.begin(),keys.end(),[](int key){
        return key==SDL_SCANCODE_W||key==SDL_SCANCODE_A||key==SDL_SCANCODE_S||key==SDL_SCANCODE_D;
    });
}
void Settings::bind_key(std::size_t action,int scancode)
{
    keys.at(action)=scancode;
    if(!wasd_available())wasd=false;
}
Settings load_settings(const Paths &paths)
{
    auto s=Settings::defaults();
    if(!std::filesystem::exists(paths.settings())) return s;
    auto bytes=read_file(paths.settings(),65536);
    std::istringstream lines(std::string(bytes.begin(),bytes.end()));
    std::map<std::string,int> values;
    for(std::string line;std::getline(lines,line);) {
        auto split=line.find('=');
        if(split==std::string::npos) continue;
        auto value=line.substr(split+1); if(!value.empty()&&value.back()=='\r') value.pop_back();
        int number=0; auto r=std::from_chars(value.data(),value.data()+value.size(),number);
        if(r.ec==std::errc{} && r.ptr==value.data()+value.size()) values[line.substr(0,split)]=number;
    }
    if(auto version=values.find("version");version!=values.end() && version->second!=1)
        throw std::runtime_error("Settings were written by an unsupported version.");
    auto integer=[&](const std::string &key,int current,int min,int max) {
        auto i=values.find(key);
        return i==values.end()||i->second<min||i->second>max?current:i->second;
    };
    s.deadzone=integer("deadzone",s.deadzone,5,80);
    s.volume=integer("volume",s.volume,0,100);
    s.window_width=integer("window_width",s.window_width,640,7680);
    s.window_height=integer("window_height",s.window_height,480,4320);
    s.language=integer("language",s.language,automatic_language,static_cast<int>(language_names.size())-1);
    s.fullscreen=integer("fullscreen",s.fullscreen,0,1)!=0;
    s.integer_scale=integer("integer_scale",s.integer_scale,0,1)!=0;
    s.mute=integer("mute",s.mute,0,1)!=0;
    s.wasd=integer("wasd",s.wasd,0,1)!=0;
    s.pause_unfocused=integer("pause_unfocused",s.pause_unfocused,0,1)!=0;
    for(std::size_t i=0;i<10;i++) {
        s.keys[i]=integer("key."+std::string(action_names[i]),s.keys[i],1,SDL_SCANCODE_COUNT-1);
        s.buttons[i]=integer("pad."+std::string(action_names[i]),s.buttons[i],0,SDL_GAMEPAD_BUTTON_COUNT-1);
    }
    if(!s.wasd_available())s.wasd=false;
    return s;
}
void save_settings(const Paths &paths,const Settings &s)
{
    std::ostringstream o;
    o<<"version=1\nlanguage="<<s.language<<"\nvolume="<<s.volume<<"\nmute="<<s.mute
     <<"\ndeadzone="<<s.deadzone<<"\nfullscreen="<<s.fullscreen<<"\ninteger_scale="<<s.integer_scale
     <<"\nwindow_width="<<s.window_width<<"\nwindow_height="<<s.window_height<<"\nwasd="<<s.wasd
     <<"\npause_unfocused="<<s.pause_unfocused<<"\n";
    for(std::size_t i=0;i<10;i++) o<<"key."<<action_names[i]<<"="<<s.keys[i]<<"\npad."<<action_names[i]<<"="<<s.buttons[i]<<"\n";
    atomic_text(paths.settings(),o.str());
}
}
