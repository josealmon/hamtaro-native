#include "platform/input.h"
#include "game/input.h"
#include <iostream>
#include <stdexcept>
#include <string>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
static int checks=0;
#define CHECK(c) do { checks++; if(!(c))throw std::runtime_error("Check failed at line "+std::to_string(__LINE__)+": " #c); }while(0)
static void pump(hn::Input &input){SDL_Event e;SDL_PumpEvents();while(SDL_PollEvent(&e))input.event(e);}
static void key(hn::Input &input,SDL_Scancode code,bool down)
{
    SDL_Event e{};e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;e.key.scancode=code;input.event(e);
}
static void saved_bindings(const hn::Paths &paths)
{
    const auto settings=hn::load_settings(paths);hn::Input input;
    CHECK(!settings.wasd&&settings.deadzone==39);
    CHECK(settings.keys!=hn::Settings::defaults().keys&&settings.buttons!=hn::Settings::defaults().buttons);
    input.select(0);
    for(unsigned action=0;action<10;action++){
        const auto expected=static_cast<std::uint16_t>(1u<<action);
        key(input,static_cast<SDL_Scancode>(settings.keys[action]),true);CHECK(input.buttons(settings)==expected);
        key(input,static_cast<SDL_Scancode>(settings.keys[action]),false);
        CHECK(input.sample(settings)==expected&&!input.sample(settings));
    }
    key(input,SDL_SCANCODE_Z,true);CHECK(!(input.buttons(settings)&HN_A));
    key(input,SDL_SCANCODE_Z,false);input.clear();
    SDL_VirtualJoystickDesc desc{};SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=SDL_GAMEPAD_AXIS_COUNT;desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;
    desc.name="Hamtaro saved-bindings test gamepad";
    desc.button_mask=(1u<<SDL_GAMEPAD_BUTTON_COUNT)-1;desc.axis_mask=(1u<<SDL_GAMEPAD_AXIS_COUNT)-1;
    auto id=SDL_AttachVirtualJoystick(&desc);CHECK(id!=0);
    auto *joystick=SDL_OpenJoystick(id);CHECK(joystick!=nullptr);pump(input);input.select(id);
    for(unsigned action=0;action<10;action++){
        const auto expected=static_cast<std::uint16_t>(1u<<action);
        CHECK(SDL_SetJoystickVirtualButton(joystick,settings.buttons[action],true));pump(input);
        CHECK(input.buttons(settings)==expected);
        CHECK(SDL_SetJoystickVirtualButton(joystick,settings.buttons[action],false));pump(input);
        CHECK(input.sample(settings)==expected&&!input.sample(settings));
    }
    CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,true));pump(input);
    CHECK(!input.buttons(settings));
    CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,false));pump(input);
    CHECK(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,10000));pump(input);
    CHECK(!input.buttons(settings));
    CHECK(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,20000));pump(input);
    CHECK(input.buttons(settings)==HN_RIGHT);
    CHECK(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,0));pump(input);
    CHECK(!input.buttons(settings));
    SDL_CloseJoystick(joystick);CHECK(SDL_DetachVirtualJoystick(id));pump(input);
}
int main()
{
    try{
        int argc=0;auto **wide=CommandLineToArgvW(GetCommandLineW(),&argc);CHECK(wide!=nullptr);
        std::vector<std::filesystem::path> arguments;
        for(int i=0;i<argc;i++)arguments.emplace_back(wide[i]);LocalFree(wide);
        CHECK(SDL_Init(SDL_INIT_GAMEPAD|SDL_INIT_EVENTS));
        if(argc==3&&arguments[1]==L"--settings"){
            saved_bindings(hn::Paths{arguments[2]});
            SDL_Quit();std::cout<<checks<<" persisted keyboard/gamepad checks passed.\n";return 0;
        }
        {
            hn::Input input;auto settings=hn::Settings::defaults();
            // Connected controllers must not influence the generated keyboard checks.
            input.select(0);CHECK(input.active()==nullptr);
            key(input,SDL_SCANCODE_Z,true);CHECK(input.buttons(settings)&HN_A);
            key(input,SDL_SCANCODE_Z,false);CHECK(!(input.buttons(settings)&HN_A));
            CHECK(input.sample(settings)&HN_A);CHECK(!input.sample(settings));
            key(input,SDL_SCANCODE_Z,true);CHECK(input.sample(settings)&HN_A);CHECK(input.sample(settings)&HN_A);
            key(input,SDL_SCANCODE_Z,false);CHECK(!input.sample(settings));
            key(input,SDL_SCANCODE_W,true);key(input,SDL_SCANCODE_D,true);
            CHECK((input.buttons(settings)&(HN_UP|HN_RIGHT))==(HN_UP|HN_RIGHT));
            key(input,SDL_SCANCODE_S,true);CHECK(!(input.buttons(settings)&(HN_UP|HN_DOWN)));
            SDL_Event focus{};focus.type=SDL_EVENT_WINDOW_FOCUS_LOST;input.event(focus);
            CHECK(input.buttons(settings)==0 && input.pause_requested);
            focus.type=SDL_EVENT_WINDOW_FOCUS_GAINED;input.event(focus);
            CHECK(input.buttons(settings)==0);
            CHECK(!input.sample(settings));
            // Every WASD key assigned to any action disables the alternative,
            // so a press produces only the selected action, including short taps.
            const auto defaults=hn::Settings::defaults();
            CHECK(defaults.wasd&&defaults.wasd_available());
            for(auto code:{SDL_SCANCODE_W,SDL_SCANCODE_A,SDL_SCANCODE_S,SDL_SCANCODE_D}) {
                for(unsigned action=0;action<10;action++) {
                    settings=defaults;input.clear();settings.bind_key(action,code);
                    CHECK(!settings.wasd&&!settings.wasd_available());
                    const auto expected=static_cast<std::uint16_t>(1u<<action);
                    key(input,code,true);CHECK(input.buttons(settings)==expected);
                    key(input,code,false);CHECK(input.sample(settings)==expected&&!input.sample(settings));
                    settings.bind_key(action,defaults.keys[action]);
                    CHECK(!settings.wasd&&settings.wasd_available());
                }
            }
            settings.bind_key(0,SDL_SCANCODE_W);settings.bind_key(1,SDL_SCANCODE_A);
            settings.bind_key(0,defaults.keys[0]);CHECK(!settings.wasd_available()&&!settings.wasd);
            settings.bind_key(1,defaults.keys[1]);CHECK(settings.wasd_available()&&!settings.wasd);
            settings.wasd=true;key(input,SDL_SCANCODE_W,true);CHECK(input.sample(settings)==HN_UP);
            key(input,SDL_SCANCODE_W,false);input.clear();settings=defaults;
            SDL_VirtualJoystickDesc desc{};SDL_INIT_INTERFACE(&desc);
            desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=SDL_GAMEPAD_AXIS_COUNT;
            desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;desc.name="Hamtaro synthetic test gamepad";
            desc.button_mask=(1u<<SDL_GAMEPAD_BUTTON_COUNT)-1;
            desc.axis_mask=(1u<<SDL_GAMEPAD_AXIS_COUNT)-1;
            auto id=SDL_AttachVirtualJoystick(&desc);CHECK(id!=0);
            auto *joystick=SDL_OpenJoystick(id);CHECK(joystick!=nullptr);
            pump(input);CHECK(input.active()!=nullptr);input.select(id);
            CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,true));
            pump(input);CHECK(input.buttons(settings)&HN_A);
            CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,false));
            pump(input);CHECK(input.sample(settings)&HN_A);CHECK(!input.sample(settings));
            CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,true));pump(input);
            CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,false));pump(input);
            CHECK(!input.buttons(settings)&&(input.sample(settings)&HN_A));CHECK(!input.sample(settings));
            CHECK(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,1000));
            pump(input);CHECK(input.buttons(settings)==0);
            CHECK(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,30000));
            pump(input);CHECK(input.buttons(settings)&HN_RIGHT);
            settings.buttons[0]=SDL_GAMEPAD_BUTTON_NORTH;
            CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_NORTH,true));
            pump(input);CHECK(input.buttons(settings)&HN_A);
            input.pause_requested=false;
            SDL_CloseJoystick(joystick);CHECK(SDL_DetachVirtualJoystick(id));
            pump(input);CHECK(input.pause_requested);
        }
        SDL_Quit();std::cout<<checks<<" keyboard/gamepad checks passed.\n";return 0;
    }catch(const std::exception&e){SDL_Quit();std::cerr<<e.what()<<'\n';return 1;}
}
