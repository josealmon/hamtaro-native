#include "platform/input.h"
#include "game/input.h"
#include <algorithm>
#include <cmath>
namespace hn {
Input::Input() { refresh(); }
Input::~Input() { for(auto *pad:pads_) SDL_CloseGamepad(pad); }
SDL_Gamepad *Input::active() const
{
    for(auto *pad:pads_) if(SDL_GetGamepadID(pad)==active_) return pad;
    return nullptr;
}
void Input::refresh()
{
    for(auto i=pads_.begin();i!=pads_.end();) {
        if(!SDL_GamepadConnected(*i)) { SDL_CloseGamepad(*i); i=pads_.erase(i); }
        else ++i;
    }
    int count=0; SDL_JoystickID *ids=SDL_GetGamepads(&count);
    for(int i=0;i<count;i++) {
        if(std::none_of(pads_.begin(),pads_.end(),[&](auto *p){return SDL_GetGamepadID(p)==ids[i];}))
            if(auto *pad=SDL_OpenGamepad(ids[i])) pads_.push_back(pad);
    }
    SDL_free(ids);
    if(!active()) active_=pads_.empty()?0:SDL_GetGamepadID(pads_[0]);
}
void Input::clear() { down_.fill(false);pressed_.fill(false);pad_pressed_=0; }
void Input::select(SDL_JoystickID id)
{ active_=id; clear(); }
void Input::event(const SDL_Event &e)
{
    if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST) { focused_=false; clear(); pause_requested=true; }
    if(e.type==SDL_EVENT_WINDOW_FOCUS_GAINED) { focused_=true; clear(); }
    if(e.type==SDL_EVENT_GAMEPAD_ADDED) refresh();
    if(e.type==SDL_EVENT_GAMEPAD_REMOVED) {
        if(e.gdevice.which==active_) { clear(); pause_requested=true; }
        refresh();
    }
    if(e.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN&&focused_&&e.gbutton.which==active_&&e.gbutton.button<SDL_GAMEPAD_BUTTON_COUNT)
        pad_pressed_|=1u<<e.gbutton.button;
    if(e.type==SDL_EVENT_KEY_DOWN || e.type==SDL_EVENT_KEY_UP) {
        auto key=e.key.scancode;
        if(key>SDL_SCANCODE_UNKNOWN && key<SDL_SCANCODE_COUNT) {
            down_[key]=focused_ && e.type==SDL_EVENT_KEY_DOWN;
            if(down_[key]&&!e.key.repeat)pressed_[key]=true;
        }
    }
}
std::uint16_t Input::buttons(const Settings &s) const
{return read_buttons(s,false);}
std::uint16_t Input::sample(const Settings &s)
{
    auto result=read_buttons(s,true);pressed_.fill(false);pad_pressed_=0;return result;
}
std::uint16_t Input::read_buttons(const Settings &s,bool include_taps) const
{
    if(!focused_) return 0;
    std::uint16_t result=0;
    auto *pad=active();
    for(std::size_t i=0;i<10;i++) {
        bool key=s.keys[i]>0&&s.keys[i]<SDL_SCANCODE_COUNT&&(down_[s.keys[i]]||(include_taps&&pressed_[s.keys[i]]));
        bool button=pad&&s.buttons[i]>=0&&s.buttons[i]<SDL_GAMEPAD_BUTTON_COUNT&&
                    (SDL_GetGamepadButton(pad,static_cast<SDL_GamepadButton>(s.buttons[i]))||(include_taps&&(pad_pressed_&(1u<<s.buttons[i]))));
        if(key||button) result|=static_cast<std::uint16_t>(1u<<i);
    }
    if(s.wasd) {
        if(down_[SDL_SCANCODE_W]||(include_taps&&pressed_[SDL_SCANCODE_W])) result|=HN_UP;
        if(down_[SDL_SCANCODE_A]||(include_taps&&pressed_[SDL_SCANCODE_A])) result|=HN_LEFT;
        if(down_[SDL_SCANCODE_S]||(include_taps&&pressed_[SDL_SCANCODE_S])) result|=HN_DOWN;
        if(down_[SDL_SCANCODE_D]||(include_taps&&pressed_[SDL_SCANCODE_D])) result|=HN_RIGHT;
    }
    if(pad) {
        int x=SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_LEFTX);
        int y=SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_LEFTY);
        int limit=static_cast<int>(32767.0*s.deadzone/100.0);
        if(x>limit) result|=HN_RIGHT; if(x<-limit) result|=HN_LEFT;
        if(y>limit) result|=HN_DOWN; if(y<-limit) result|=HN_UP;
    }
    /* Opposite directions cancel per axis; diagonals remain available. */
    if((result&(HN_LEFT|HN_RIGHT))==(HN_LEFT|HN_RIGHT)) result&=~(HN_LEFT|HN_RIGHT);
    if((result&(HN_UP|HN_DOWN))==(HN_UP|HN_DOWN)) result&=~(HN_UP|HN_DOWN);
    return result;
}
const char *button_label(SDL_Gamepad *pad,int button)
{
    if(button<0||button>=SDL_GAMEPAD_BUTTON_COUNT) return "-";
    if(pad) switch(SDL_GetGamepadButtonLabel(pad,static_cast<SDL_GamepadButton>(button))) {
    case SDL_GAMEPAD_BUTTON_LABEL_A:return "A";
    case SDL_GAMEPAD_BUTTON_LABEL_B:return "B";
    case SDL_GAMEPAD_BUTTON_LABEL_X:return "X";
    case SDL_GAMEPAD_BUTTON_LABEL_Y:return "Y";
    case SDL_GAMEPAD_BUTTON_LABEL_CROSS:return "Cross";
    case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:return "Circle";
    case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:return "Square";
    case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE:return "Triangle";
    default:break;
    }
    const char *name=SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
    return name?name:"-";
}
}
