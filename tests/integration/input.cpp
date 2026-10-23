#include "platform/input.h"
#include "game/input.h"
#include <iostream>
#include <stdexcept>
#include <string>
static int checks=0;
#define CHECK(c) do { checks++; if(!(c))throw std::runtime_error("Check failed at line "+std::to_string(__LINE__)+": " #c); }while(0)
static void pump(hn::Input &input){SDL_Event e;SDL_PumpEvents();while(SDL_PollEvent(&e))input.event(e);}
static void key(hn::Input &input,SDL_Scancode code,bool down)
{
    SDL_Event e{};e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;e.key.scancode=code;input.event(e);
}
int main()
{
    try{
        CHECK(SDL_Init(SDL_INIT_GAMEPAD|SDL_INIT_EVENTS));
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
