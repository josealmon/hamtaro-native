#include "app/rom.h"
#include "app/settings.h"
#include "app/session.h"
#include "platform/input.h"
#include "platform/audio.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <sstream>
#include <cstring>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <objidl.h>
#include <gdiplus.h>

namespace {
constexpr const char *game_title="Hamtaro: Ham-Ham Heartbreak";
enum class FileAction { rom, import_save };
struct Selection { FileAction action; std::filesystem::path path; std::string error; };
struct DialogState { std::mutex mutex; bool pending=false; std::optional<Selection> result; };
struct DialogRequest { std::shared_ptr<DialogState> state; FileAction action; SDL_DialogFileFilter filter; };
void SDLCALL selected_file(void *user,const char *const *files,int)
{
    std::unique_ptr<DialogRequest> request(static_cast<DialogRequest *>(user));
    std::lock_guard lock(request->state->mutex);
    request->state->pending=false;
    if(!files) request->state->result=Selection{request->action,{},SDL_GetError()};
    else if(files[0]) request->state->result=Selection{request->action,hn::from_utf8(files[0]),{}};
}
void choose_file(SDL_Window *window,const std::shared_ptr<DialogState> &state,FileAction action,int language)
{
    {
        std::lock_guard lock(state->mutex);
        if(state->pending) return;
        state->pending=true;
    }
    auto *request=new DialogRequest{state,action,{hn::translate(language,action==FileAction::rom?"Game Boy Advance ROM":"GBA save"),action==FileAction::rom?"gba":"sav"}};
    SDL_ShowOpenFileDialog(selected_file,request,window,&request->filter,1,nullptr,false);
}
struct SDLSession {
    SDLSession() {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"0");
        if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD)) throw std::runtime_error(SDL_GetError());
        SDL_InitSubSystem(SDL_INIT_AUDIO);
    }
    ~SDLSession(){ SDL_Quit(); }
};
struct Window {
    SDL_Window *value=nullptr;
    ~Window(){ if(value) SDL_DestroyWindow(value); }
};
struct Renderer {
    SDL_Renderer *value=nullptr;
    ~Renderer(){ if(value) SDL_DestroyRenderer(value); }
};
struct Texture {
    SDL_Texture *value=nullptr;
    ~Texture(){if(value)SDL_DestroyTexture(value);}
};
SDL_Texture *load_artwork(SDL_Renderer *renderer,int id)
{
    const auto module=GetModuleHandleW(nullptr);
    const auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10)); // RCDATA
    if(!resource)return nullptr;
    const auto data=LockResource(LoadResource(module,resource));
    const auto size=SizeofResource(module,resource);
    if(!data||!size)throw std::runtime_error("Could not read the embedded launcher image.");
    auto *stream=SDL_IOFromConstMem(data,size);
    if(!stream)throw std::runtime_error(SDL_GetError());
    std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> surface(SDL_LoadPNG_IO(stream,true),SDL_DestroySurface);
    if(!surface)throw std::runtime_error(SDL_GetError());
    auto *texture=SDL_CreateTextureFromSurface(renderer,surface.get());
    if(!texture)throw std::runtime_error(SDL_GetError());
    SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_LINEAR);
    return texture;
}
void draw_artwork(SDL_Texture *texture,float max_width,float max_height,bool centered=false)
{
    if(!texture)return;
    float width=0,height=0;SDL_GetTextureSize(texture,&width,&height);
    const float scale=std::min(max_width/width,max_height/height);
    if(centered)ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.f,(ImGui::GetContentRegionAvail().x-width*scale)/2));
    ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerFromTex,nullptr);
    ImGui::Image(reinterpret_cast<ImTextureID>(texture),ImVec2(width*scale,height*scale));
    ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear,nullptr);
}
struct Animation {
    ULONG_PTR token=0;
    IStream *stream=nullptr;
    std::unique_ptr<Gdiplus::Bitmap> image;
    Texture texture;
    std::vector<Uint32> delays;
    Uint64 duration=0,started=0;
    UINT frame=~0u;
    ~Animation() {
        image.reset();if(stream)stream->Release();
        if(token)Gdiplus::GdiplusShutdown(token);
    }
    void load(SDL_Renderer *renderer) {
        const auto module=GetModuleHandleW(nullptr);
        const auto resource=FindResourceW(module,MAKEINTRESOURCEW(104),MAKEINTRESOURCEW(10));
        if(!resource)return;
        Gdiplus::GdiplusStartupInput options;
        if(Gdiplus::GdiplusStartup(&token,&options,nullptr)!=Gdiplus::Ok)
            throw std::runtime_error("Could not initialize launcher animation.");
        const auto size=SizeofResource(module,resource);
        const auto data=LockResource(LoadResource(module,resource));
        if(!data||!size)throw std::runtime_error("Could not read launcher animation.");
        auto memory=GlobalAlloc(GMEM_MOVEABLE,size);
        if(!memory)throw std::bad_alloc();
        auto *bytes=GlobalLock(memory);
        if(!bytes){GlobalFree(memory);throw std::bad_alloc();}
        std::memcpy(bytes,data,size);GlobalUnlock(memory);
        if(FAILED(CreateStreamOnHGlobal(memory,TRUE,&stream))) {
            GlobalFree(memory);throw std::runtime_error("Could not read launcher animation.");
        }
        image.reset(Gdiplus::Bitmap::FromStream(stream));
        if(!image||image->GetLastStatus()!=Gdiplus::Ok)
            throw std::runtime_error("Invalid launcher animation.");
        const auto count=image->GetFrameCount(&Gdiplus::FrameDimensionTime);
        if(!count)throw std::runtime_error("Launcher animation has no frames.");
        delays.assign(count,100);
        std::vector<std::uint8_t> property(image->GetPropertyItemSize(PropertyTagFrameDelay));
        if(!property.empty()) {
            auto *item=reinterpret_cast<Gdiplus::PropertyItem *>(property.data());
            if(image->GetPropertyItem(PropertyTagFrameDelay,static_cast<UINT>(property.size()),item)==Gdiplus::Ok&&item->type==PropertyTagTypeLong&&item->length/sizeof(UINT)>=count) {
                for(UINT i=0;i<count;i++)delays[i]=std::max(1u,static_cast<const UINT *>(item->value)[i])*10;
            }
        }
        for(auto delay:delays)duration+=delay;
        texture.value=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_BGRA32,SDL_TEXTUREACCESS_STREAMING,image->GetWidth(),image->GetHeight());
        if(!texture.value)throw std::runtime_error(SDL_GetError());
        SDL_SetTextureScaleMode(texture.value,SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(texture.value,SDL_BLENDMODE_BLEND);
        started=SDL_GetTicks();
    }
    SDL_Texture *current() {
        if(!texture.value)return nullptr;
        auto elapsed=(SDL_GetTicks()-started)%duration;
        UINT next=0;
        while(next+1<delays.size()&&elapsed>=delays[next])elapsed-=delays[next++];
        if(next!=frame) {
            if(image->SelectActiveFrame(&Gdiplus::FrameDimensionTime,next)!=Gdiplus::Ok)
                throw std::runtime_error("Could not decode launcher animation frame.");
            Gdiplus::Rect area(0,0,image->GetWidth(),image->GetHeight());Gdiplus::BitmapData pixels{};
            if(image->LockBits(&area,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&pixels)!=Gdiplus::Ok)
                throw std::runtime_error("Could not read launcher animation frame.");
            const bool updated=SDL_UpdateTexture(texture.value,nullptr,pixels.Scan0,pixels.Stride);
            image->UnlockBits(&pixels);
            if(!updated)throw std::runtime_error(SDL_GetError());
            frame=next;
        }
        return texture.value;
    }
};
struct ImGuiSession {
    ImGuiSession(SDL_Window *window,SDL_Renderer *renderer) {
        IMGUI_CHECKVERSION(); ImGui::CreateContext();
        auto &io=ImGui::GetIO();
        io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NavEnableGamepad;
        io.IniFilename=nullptr; io.LogFilename=nullptr;
        wchar_t windows_directory[MAX_PATH]{};
        const auto length=GetWindowsDirectoryW(windows_directory,MAX_PATH);
        if(length&&length<MAX_PATH) {
            const auto font=std::filesystem::path(windows_directory)/L"Fonts"/L"segoeui.ttf";
            if(std::filesystem::exists(font))io.Fonts->AddFontFromFileTTF(hn::utf8(font).c_str(),18.f);
        }
        ImGui::StyleColorsLight();
        auto &s=ImGui::GetStyle();
        s.FontSizeBase=18; s.WindowRounding=0; s.ChildRounding=20; s.FrameRounding=10; s.GrabRounding=8;
        s.WindowPadding=ImVec2(24,20); s.FramePadding=ImVec2(16,9); s.ItemSpacing=ImVec2(12,10);
        s.FrameBorderSize=1; s.ChildBorderSize=1; s.ScrollbarRounding=8;
        s.Colors[ImGuiCol_Text]=ImVec4(0.28f,0.19f,0.12f,1);
        s.Colors[ImGuiCol_TextDisabled]=ImVec4(0.51f,0.43f,0.34f,1);
        s.Colors[ImGuiCol_WindowBg]=ImVec4(0.98f,0.95f,0.88f,1);
        s.Colors[ImGuiCol_ChildBg]=ImVec4(1.f,0.99f,0.95f,1);
        s.Colors[ImGuiCol_PopupBg]=ImVec4(1.f,0.99f,0.95f,1);
        s.Colors[ImGuiCol_Border]=ImVec4(0.85f,0.76f,0.61f,0.65f);
        s.Colors[ImGuiCol_Separator]=ImVec4(0.85f,0.76f,0.61f,0.55f);
        s.Colors[ImGuiCol_FrameBg]=ImVec4(0.96f,0.91f,0.81f,1);
        s.Colors[ImGuiCol_FrameBgHovered]=ImVec4(1.f,0.88f,0.67f,1);
        s.Colors[ImGuiCol_FrameBgActive]=ImVec4(1.f,0.81f,0.52f,1);
        s.Colors[ImGuiCol_Button]=ImVec4(0.97f,0.92f,0.82f,1);
        s.Colors[ImGuiCol_ButtonHovered]=ImVec4(1.f,0.85f,0.61f,1);
        s.Colors[ImGuiCol_ButtonActive]=ImVec4(0.98f,0.76f,0.43f,1);
        s.Colors[ImGuiCol_Header]=ImVec4(0.98f,0.85f,0.64f,1);
        s.Colors[ImGuiCol_HeaderHovered]=s.Colors[ImGuiCol_ButtonHovered];
        s.Colors[ImGuiCol_HeaderActive]=s.Colors[ImGuiCol_ButtonActive];
        s.Colors[ImGuiCol_TableHeaderBg]=ImVec4(0.94f,0.87f,0.74f,1);
        s.Colors[ImGuiCol_CheckMark]=ImVec4(0.76f,0.34f,0.06f,1);
        s.Colors[ImGuiCol_SliderGrab]=ImVec4(0.92f,0.52f,0.14f,1);
        s.Colors[ImGuiCol_SliderGrabActive]=ImVec4(0.79f,0.36f,0.07f,1);
        s.Colors[ImGuiCol_NavCursor]=ImVec4(0.76f,0.34f,0.06f,1);
        ImGui_ImplSDL3_InitForSDLRenderer(window,renderer);
        ImGui_ImplSDLRenderer3_Init(renderer);
    }
    ~ImGuiSession() {
        ImGui_ImplSDLRenderer3_Shutdown(); ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext();
    }
};
const char *tr(const hn::Settings &s,const char *english){ return hn::translate(s.language,english); }
struct App {
    hn::Paths paths;
    hn::Settings settings;
    hn::Input input;
    hn::Audio audio;
    hn::Session session;
    SDL_Texture *texture=nullptr;
    Texture logo,cover;
    Animation mascot;
    std::shared_ptr<DialogState> dialog=std::make_shared<DialogState>();
    hn::Rom rom;
    std::string message;
    bool error=false,paused=false,running=true,settings_writable=true;
    int remap_key=-1,remap_button=-1,page=0;
    bool waiting_release=true,audio_pending=false,frame_ready=false;
    Uint64 last_tick=0;
    double elapsed=0;
    void notice(std::string text,bool failed=false){ message=failed?hn::translate(settings.language,text.c_str()):std::move(text);error=failed; }
    void pause_game() {
        paused=true;input.clear();audio.pause(true);last_tick=0;elapsed=0;
    }
    void game_fault() {
        pause_game();
        notice(tr(settings,"The game reached an unfinished part of the port. You can return to the menu. Your last completed save is preserved."),true);
        if(const auto *g=session.game()) {
            std::ostringstream detail;
            detail<<"Native runtime fault "<<g->fault<<", detail "<<g->detail<<", opcode "<<g->opcode
                  <<", address 0x"<<std::hex<<g->address<<std::dec<<", frame "<<g->frames<<'\n';
            try{hn::atomic_text(paths.root/"runtime-error.log",detail.str());}catch(const std::exception&){}
        }
    }
    void resume_game() {
        try{
            session.persist(paths);
            if(!session.playable())return;
            if(!audio.pause(false))throw std::runtime_error(SDL_GetError());
            paused=false;input.clear();waiting_release=true;last_tick=0;elapsed=0;
            message.clear();error=false;
        }catch(const std::exception&e){pause_game();notice(e.what(),true);}
    }
    void stop_game() {
        pause_game();session.stop(paths);audio.clear();audio_pending=false;frame_ready=false;
    }
    void start_game() {
        try{
            stop_game();
            if(!session.start(paths,rom)){game_fault();return;}
            resume_game();
        }catch(const std::exception&e){pause_game();notice(e.what(),true);}
    }
    void request_quit() {
        try{session.persist(paths);running=false;}
        catch(const std::exception&e){pause_game();notice(e.what(),true);}
    }
    bool advance() {
        if(!running||paused||!session.playable()){last_tick=0;elapsed=0;return false;}
        const Uint64 now=SDL_GetTicksNS();
        if(!last_tick){last_tick=now;return false;}
        constexpr double interval=static_cast<double>(HN_CYCLES_PER_FRAME)/static_cast<double>(HN_CLOCK_HZ);
        elapsed=std::min(elapsed+(now-last_tick)/1e9,interval*4);last_tick=now;
        bool updated=false;
        try{
            for(unsigned catchup=0;elapsed>=interval&&catchup<4;catchup++) {
                if(audio_pending) {
                    if(!audio.submit(session.samples()))throw std::runtime_error(tr(settings,"Audio delivery failed. Check the output device and choose Resume."));
                    audio_pending=false;
                }
                if(audio.available()&&audio.queued_frames()>32768/8){elapsed=std::min(elapsed,interval);break;}
                auto keys=input.sample(settings);
                if(waiting_release){if(!input.buttons(settings))waiting_release=false;keys=0;}
                if(!session.frame(paths,keys)){game_fault();break;}
                if(!SDL_UpdateTexture(texture,nullptr,session.pixels().data(),HN_WIDTH*sizeof(std::uint32_t)))
                    throw std::runtime_error(SDL_GetError());
                frame_ready=true;updated=true;audio_pending=audio.available();elapsed-=interval;
            }
            if(audio_pending&&audio.queued_frames()<=32768/8) {
                if(!audio.submit(session.samples()))throw std::runtime_error(SDL_GetError());
                audio_pending=false;
            }
        }catch(const std::exception&e){pause_game();notice(e.what(),true);}
        return updated;
    }
    void draw_game(SDL_Renderer *renderer) {
        if(!frame_ready)return;
        int width=0,height=0;SDL_GetRenderOutputSize(renderer,&width,&height);
        float scale=std::min(width/static_cast<float>(HN_WIDTH),height/static_cast<float>(HN_HEIGHT));
        if(settings.integer_scale&&scale>=1)scale=static_cast<float>(static_cast<int>(scale));
        const float rendered_width=static_cast<float>(HN_WIDTH)*scale,rendered_height=static_cast<float>(HN_HEIGHT)*scale;
        SDL_FRect destination{(width-rendered_width)/2,(height-rendered_height)/2,rendered_width,rendered_height};
        SDL_RenderTexture(renderer,texture,nullptr,&destination);
    }
    void save_preferences() {
        if(!settings_writable)return;
        try { hn::save_settings(paths,settings); }
        catch(const std::exception &e){notice(e.what(),true);}
    }
    void check_rom() {
        if(!std::filesystem::exists(paths.rom())) return;
        try{rom=hn::load_rom(paths);}
        catch(const std::exception &e){notice(e.what(),true);}
    }
    void poll_dialog() {
        std::optional<Selection> result;
        { std::lock_guard lock(dialog->mutex); result=std::move(dialog->result);dialog->result.reset(); }
        if(!result)return;
        input.clear();
        try {
            if(!result->error.empty()) throw std::runtime_error(result->error);
            if(session.present())
                throw std::runtime_error(tr(settings,"Return to the menu before replacing the ROM or save."));
            switch(result->action) {
            case FileAction::rom:
                hn::import_rom(paths,result->path); rom=hn::load_rom(paths);
                notice(tr(settings,"ROM imported and verified."));break;
            case FileAction::import_save:
                hn::import_save(paths,result->path);
                notice(tr(settings,"Save imported. The previous valid save was backed up."));break;
            }
        }catch(const std::exception &e){notice(e.what(),true);}
    }
    void event(const SDL_Event &e,SDL_Window *window) {
        input.event(e);
        if(e.type==SDL_EVENT_QUIT)request_quit();
        if(e.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)request_quit();
        if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST && settings.pause_unfocused) {
            pause_game();
        }
        if(input.pause_requested) { if(settings.pause_unfocused||e.type!=SDL_EVENT_WINDOW_FOCUS_LOST)pause_game();input.pause_requested=false; }
        if(e.type==SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            if(remap_key>=0) {
                if(e.key.scancode!=SDL_SCANCODE_ESCAPE && e.key.scancode!=SDL_SCANCODE_F11)
                    settings.keys[remap_key]=e.key.scancode;
                remap_key=-1;input.clear();save_preferences();return;
            }
            if(e.key.scancode==SDL_SCANCODE_ESCAPE) {
                if(remap_button>=0){remap_button=-1;input.clear();return;}
                bool dialog_pending;
                {std::lock_guard lock(dialog->mutex);dialog_pending=dialog->pending;}
                if(dialog_pending||ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopup))return;
                if(session.playable()){if(paused)resume_game();else pause_game();}
            }
            if(e.key.scancode==SDL_SCANCODE_F11) {
                settings.fullscreen=!settings.fullscreen;
                SDL_SetWindowFullscreen(window,settings.fullscreen);save_preferences();
            }
        }
        if(e.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
           input.active() && e.gbutton.which==SDL_GetGamepadID(input.active())) {
            if(remap_button>=0) {
                if(e.gbutton.button!=SDL_GAMEPAD_BUTTON_RIGHT_STICK)
                    settings.buttons[remap_button]=e.gbutton.button;
                remap_button=-1;input.clear();save_preferences();return;
            }
            if(e.gbutton.button==SDL_GAMEPAD_BUTTON_RIGHT_STICK)pause_game();
        }
    }
    void home(SDL_Window *window) {
        const auto available=ImGui::GetContentRegionAvail();
        const float card_width=std::min(760.f,available.x);
        const float reserve=message.empty()?0.f:52.f;
        const float card_height=std::clamp(available.y-reserve-8.f,240.f,400.f);
        const float padding=card_width<650.f?20.f:26.f;
        const float footer_height=40.f;
        const float hero_height=card_height-padding*2-footer_height-22.f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+(available.x-card_width)/2);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,(available.y-reserve-card_height)/2)-4.f);
        const auto origin=ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(origin.x+2,origin.y+5),ImVec2(origin.x+card_width+2,origin.y+card_height+5),IM_COL32(108,73,29,15),20.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(padding,padding));
        if(ImGui::BeginChild("home_card",ImVec2(card_width,card_height),ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)) {
            const float inner=card_width-padding*2;
            const float left_width=std::min(260.f,inner*0.42f);
            const float gap=card_width<650.f?20.f:32.f;
            const float cover_size=std::min({220.f,left_width-16.f,hero_height-12.f});
            const ImVec2 cover_pos(padding+(left_width-cover_size)/2,padding+(hero_height-cover_size)/2);
            ImGui::SetCursorPos(cover_pos);
            const auto image_pos=ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(image_pos.x-7,image_pos.y-7),ImVec2(image_pos.x+cover_size+7,image_pos.y+cover_size+7),IM_COL32(231,215,183,255),12.f);
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(image_pos.x-5,image_pos.y-5),ImVec2(image_pos.x+cover_size+5,image_pos.y+cover_size+5),IM_COL32(255,255,250,255),10.f);
            draw_artwork(cover.value,cover_size,cover_size);

            const float right_x=padding+left_width+gap;
            const float right_width=inner-left_width-gap;
            const float button_width=std::min(320.f,right_width);
            const float button_x=right_x+(right_width-button_width)/2;
            const float play_height=card_height<320.f?46.f:54.f;
            const float fixed_height=play_height+44.f+(session.present()?42.f:0.f);
            float gif_height=std::min(124.f,hero_height-fixed_height-12.f);
            if(gif_height<44.f)gif_height=0;
            float y=padding+std::max(0.f,(hero_height-fixed_height-gif_height-(gif_height?12.f:0.f))/2);
            if(gif_height) {
                ImGui::SetCursorPos(ImVec2(right_x,y));
                draw_artwork(mascot.current(),std::min(250.f,right_width),gif_height,true);
                y+=gif_height+12.f;
            }
            ImGui::SetCursorPos(ImVec2(button_x,y));
            ImGui::BeginDisabled(session.present()?!session.playable():!rom);
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1.f,0.64f,0.24f,1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(1.f,0.72f,0.36f,1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.96f,0.52f,0.13f,1));
            ImGui::PushFont(nullptr,21.f);
            if(ImGui::Button(tr(settings,session.present()?"Resume":"Play"),ImVec2(button_width,play_height))) {
                if(session.present())resume_game();else start_game();
            }
            ImGui::PopFont();ImGui::PopStyleColor(3);ImGui::EndDisabled();
            y+=play_height+10.f;
            if(session.present()) {
                ImGui::SetCursorPos(ImVec2(button_x,y));
                const float half=(button_width-8.f)/2;
                if(ImGui::Button(tr(settings,"Restart"),ImVec2(half,32)))start_game();
                ImGui::SameLine(0,8);
                if(ImGui::Button(tr(settings,"Back to menu"),ImVec2(half,32))) {
                    try{stop_game();message.clear();}catch(const std::exception&e){notice(e.what(),true);}
                }
                y+=42.f;
            }
            const char *rom_label=tr(settings,!rom?"Select ROM":"Change ROM");
            const char *status=tr(settings,!rom?"Europe (.gba)":"ROM ready");
            const float rom_width=ImGui::CalcTextSize(rom_label).x+32.f;
            const float row_width=rom_width+12.f+ImGui::CalcTextSize(status).x;
            ImGui::SetCursorPos(ImVec2(right_x+(right_width-row_width)/2,y));
            ImGui::BeginDisabled(session.present());
            if(ImGui::Button(rom_label,ImVec2(rom_width,34)))choose_file(window,dialog,FileAction::rom,settings.language);
            ImGui::EndDisabled();
            ImGui::SameLine(0,12);ImGui::AlignTextToFramePadding();
            ImGui::TextColored(!rom?ImVec4(0.51f,0.43f,0.34f,1):ImVec4(0.32f,0.45f,0.21f,1),"%s",status);

            const float footer_y=card_height-padding-footer_height;
            ImGui::SetCursorPos(ImVec2(padding,footer_y-12.f));ImGui::Separator();
            ImGui::SetCursorPos(ImVec2(padding,footer_y));
            const float half=(inner-12.f)/2;
            ImGui::BeginDisabled(session.present());
            if(ImGui::Button(tr(settings,"Import save"),ImVec2(half,footer_height)))choose_file(window,dialog,FileAction::import_save,settings.language);
            ImGui::EndDisabled();ImGui::SameLine(0,12);
            if(ImGui::Button(tr(settings,"Open save folder"),ImVec2(half,footer_height)))
                ShellExecuteW(nullptr,L"open",paths.save().parent_path().c_str(),nullptr,nullptr,SW_SHOWNORMAL);
        }
        ImGui::EndChild();ImGui::PopStyleVar();
    }
    void controls() {
        ImGui::Spacing();
        const char *name=input.active()?SDL_GetGamepadName(input.active()):tr(settings,"No gamepad connected");
        ImGui::TextUnformatted(tr(settings,"Active gamepad"));
        ImGui::SetNextItemWidth(std::min(360.f,ImGui::GetContentRegionAvail().x));
        if(ImGui::BeginCombo("##active_gamepad",name)) {
            for(auto *pad:input.pads()) {
                bool active=pad==input.active();
                if(ImGui::Selectable(SDL_GetGamepadName(pad),active)) input.select(SDL_GetGamepadID(pad));
            }
            ImGui::EndCombo();
        }
        bool changed=ImGui::Checkbox(tr(settings,"WASD movement alternative"),&settings.wasd);
        ImGui::TextUnformatted(tr(settings,"Stick deadzone"));
        ImGui::SetNextItemWidth(std::min(360.f,ImGui::GetContentRegionAvail().x));
        ImGui::SliderInt("##stick_deadzone",&settings.deadzone,5,80,"%d%%");
        changed|=ImGui::IsItemDeactivatedAfterEdit();
        ImGui::TextDisabled("%s",tr(settings,"Select a binding to change it. Escape cancels."));
        float table_height=std::max(130.0f,ImGui::GetContentRegionAvail().y-110.0f);
        if(ImGui::BeginTable("bindings",3,ImGuiTableFlags_BordersInnerH|ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_ScrollY,ImVec2(0,table_height))) {
            ImGui::TableSetupScrollFreeze(0,1);
            ImGui::TableSetupColumn(tr(settings,"GBA action"),ImGuiTableColumnFlags_WidthFixed,105);
            ImGui::TableSetupColumn(tr(settings,"Keyboard"),ImGuiTableColumnFlags_WidthStretch,1.f);
            ImGui::TableSetupColumn(tr(settings,"Gamepad"),ImGuiTableColumnFlags_WidthStretch,1.f);
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            for(int column=0;column<3;column++) {
                if(!ImGui::TableSetColumnIndex(column))continue;
                const char *heading=ImGui::TableGetColumnName();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.f,
                    (ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(heading).x)/2));
                ImGui::TableHeader(heading);
            }
            for(int i=0;i<10;i++) {
                ImGui::PushID(i);ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);ImGui::TextUnformatted(tr(settings,hn::action_names[i]));
                ImGui::TableSetColumnIndex(1);
                const char *key=remap_key==i?tr(settings,"Press a key..."):tr(settings,settings.keys[i]==SDL_SCANCODE_HOME?"Home key":SDL_GetScancodeName(static_cast<SDL_Scancode>(settings.keys[i])));
                if(ImGui::Button(key,ImVec2(-1,0))) {remap_key=i;remap_button=-1;input.clear();}
                ImGui::TableSetColumnIndex(2);ImGui::BeginDisabled(!input.active());
                const char *button=remap_button==i?tr(settings,"Press a button..."):tr(settings,hn::button_label(input.active(),settings.buttons[i]));
                if(ImGui::Button(button,ImVec2(-1,0))) {remap_button=i;remap_key=-1;input.clear();}
                ImGui::EndDisabled();ImGui::PopID();
            }
            ImGui::EndTable();
        }
        if(ImGui::Button(tr(settings,"Reset controls"))) {
            auto defaults=hn::Settings::defaults();settings.keys=defaults.keys;settings.buttons=defaults.buttons;
            settings.deadzone=defaults.deadzone;settings.wasd=true;remap_key=-1;remap_button=-1;changed=true;
        }
        if(changed)save_preferences();
    }
    void presentation(SDL_Window *window) {
        ImGui::Spacing();
        int choice=settings.language+1;
        const auto automatic=std::string(tr(settings,"Automatic"))+" ("+hn::language_names[hn::system_language()]+")";
        const char *languages[]={automatic.c_str(),hn::language_names[0],hn::language_names[1],hn::language_names[2],hn::language_names[3],hn::language_names[4]};
        ImGui::TextUnformatted(tr(settings,"Application language"));
        ImGui::SetNextItemWidth(std::min(360.f,ImGui::GetContentRegionAvail().x));
        if(ImGui::Combo("##application_language",&choice,languages,6)) {
            settings.language=choice-1;save_preferences();
        }
        ImGui::Spacing();
        bool changed=false;
        if(ImGui::Checkbox(tr(settings,"Fullscreen (F11)"),&settings.fullscreen)) {
            SDL_SetWindowFullscreen(window,settings.fullscreen);changed=true;
        }
        changed|=ImGui::Checkbox(tr(settings,"Integer scaling"),&settings.integer_scale);
        changed|=ImGui::Checkbox(tr(settings,"Pause when switching applications"),&settings.pause_unfocused);
        ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
        const bool volume_changed=ImGui::SliderInt(tr(settings,"Volume"),&settings.volume,0,100,"%d%%");
        changed|=ImGui::IsItemDeactivatedAfterEdit();
        changed|=ImGui::Checkbox(tr(settings,"Mute"),&settings.mute);
        if(!audio.available()) ImGui::TextWrapped("%s",tr(settings,"No audio device is available."));
        if(changed||volume_changed)audio.gain(settings.volume,settings.mute);
        if(changed)save_preferences();
    }
    void draw(SDL_Window *window) {
        if(session.playable()&&!paused)return;
        auto *viewport=ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin(game_title,nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoMove|
                                              ImGuiWindowFlags_NoSavedSettings);
        if(logo.value)draw_artwork(logo.value,300.f,std::clamp(viewport->WorkSize.y*0.14f,60.f,100.f),true);
        else ImGui::TextUnformatted(game_title);
        ImGui::Spacing();
        const char *pages[]={tr(settings,"Home"),tr(settings,"Controls"),tr(settings,"Video and audio")};
        float navigation_width=16.f;
        for(const auto *label:pages)navigation_width+=ImGui::CalcTextSize(label).x+36.f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.f,(ImGui::GetContentRegionAvail().x-navigation_width)/2));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,18.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
        for(int i=0;i<3;i++) {
            if(i)ImGui::SameLine(0,8);
            ImGui::PushStyleColor(ImGuiCol_Button,page==i?ImVec4(0.98f,0.85f,0.64f,1):ImVec4(0,0,0,0));
            if(ImGui::Button(pages[i],ImVec2(ImGui::CalcTextSize(pages[i]).x+36.f,36.f)))page=i;
            ImGui::PopStyleColor();
        }
        ImGui::PopStyleVar(2);ImGui::Spacing();
        if(page==0)home(window);
        else if(page==1)controls();
        else presentation(window);
        if(!message.empty()) {
            ImGui::Spacing();ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Text,error?ImVec4(0.68f,0.19f,0.13f,1):ImVec4(0.29f,0.43f,0.19f,1));
            ImGui::TextWrapped("%s",message.c_str());ImGui::PopStyleColor();
        }
        if(!settings_writable) ImGui::TextWrapped("%s",tr(settings,"Existing settings are preserved. Changes in this session will not be saved."));
        ImGui::End();
    }
};
int run(int argc,char **argv)
{
    bool smoke=false,explicit_profile=false;hn::Paths paths=hn::default_paths();std::filesystem::path verification;
    for(int i=1;i<argc;i++) {
        std::string arg=argv[i];
        if(arg=="--smoke-test")smoke=true;
        else if(arg=="--data-dir"&&i+1<argc) {paths.root=hn::from_utf8(argv[++i]);explicit_profile=true;}
        else if(arg=="--verify-rom"&&i+1<argc)verification=hn::from_utf8(argv[++i]);
        else throw std::runtime_error("Unknown or incomplete command-line option.");
    }
    if(!verification.empty()) {
        auto bytes=hn::read_file(verification,hn::rom_size);
        if(hn::validate_rom(bytes)!=hn::RomStatus::valid) return 2;
        std::cout<<"AH3P revision 0: SHA-256 verified\n";return 0;
    }
    if(smoke&&!explicit_profile) {
        paths.root=std::filesystem::temp_directory_path()/("hamtaro-native-smoke-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
        if(!std::filesystem::create_directory(paths.root)) throw std::runtime_error("Smoke-test directory already exists.");
    }
    int frames=0;
    {
        hn::ProfileLock lock(paths);
        SDLSession session;
        Window window;
        hn::Settings settings=hn::Settings::defaults();
        std::string startup_error;
        try{settings=hn::load_settings(paths);}catch(const std::exception&e){startup_error=e.what();}
        SDL_PropertiesID properties=SDL_CreateProperties();
        SDL_SetStringProperty(properties,SDL_PROP_WINDOW_CREATE_TITLE_STRING,game_title);
        SDL_SetNumberProperty(properties,SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,settings.window_width);
        SDL_SetNumberProperty(properties,SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,settings.window_height);
        SDL_SetBooleanProperty(properties,SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,true);
        SDL_SetBooleanProperty(properties,SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN,true);
        window.value=SDL_CreateWindowWithProperties(properties);SDL_DestroyProperties(properties);
        if(!window.value)throw std::runtime_error(SDL_GetError());
        SDL_Rect work_area{};
        const auto display=SDL_GetDisplayForWindow(window.value);
        const bool have_work_area=SDL_GetDisplayUsableBounds(display,&work_area);
        int top=0,left=0,bottom=0,right=0;
        SDL_GetWindowBordersSize(window.value,&top,&left,&bottom,&right);
        const auto size=hn::game_window_size(settings.window_width,settings.window_height,
            have_work_area?work_area.w-left-right:settings.window_width,
            have_work_area?work_area.h-top-bottom:settings.window_height);
        settings.window_width=size[0];settings.window_height=size[1];
        if(!SDL_SetWindowSize(window.value,size[0],size[1]))throw std::runtime_error(SDL_GetError());
        SDL_SetWindowMinimumSize(window.value,std::min(640,size[0]),std::min(480,size[1]));
        if(have_work_area)SDL_SetWindowPosition(window.value,
            work_area.x+(work_area.w-size[0]-left-right)/2+left,
            work_area.y+(work_area.h-size[1]-top-bottom)/2+top);
        if(settings.fullscreen&&!smoke)SDL_SetWindowFullscreen(window.value,true);
        Renderer renderer;renderer.value=SDL_CreateRenderer(window.value,nullptr);
        if(!renderer.value)throw std::runtime_error(SDL_GetError());
        /* The original 59.7275 Hz clock must not be quantized to the display refresh. */
        SDL_SetRenderVSync(renderer.value,0);
        ImGuiSession gui(window.value,renderer.value);
        App app{paths,settings};
        app.logo.value=load_artwork(renderer.value,102);
        app.cover.value=load_artwork(renderer.value,103);
        app.mascot.load(renderer.value);
        Texture texture;texture.value=SDL_CreateTexture(renderer.value,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,HN_WIDTH,HN_HEIGHT);
        if(!texture.value)throw std::runtime_error(SDL_GetError());
        SDL_SetTextureScaleMode(texture.value,SDL_SCALEMODE_NEAREST);app.texture=texture.value;
        app.audio.gain(settings.volume,settings.mute);
        if(!startup_error.empty()){app.notice(startup_error,true);app.settings_writable=false;}
        app.check_rom();
        if(!smoke)SDL_ShowWindow(window.value);
        Uint64 next_ui_frame=0;
        bool redraw=true;
        auto dispatch=[&](const SDL_Event &e) {
            ImGui_ImplSDL3_ProcessEvent(&e);app.event(e,window.value);
            if(e.type==SDL_EVENT_WINDOW_EXPOSED||e.type==SDL_EVENT_WINDOW_RESIZED||e.type==SDL_EVENT_WINDOW_RESTORED)
                redraw=true;
        };
        while(app.running) {
            SDL_Event e;
            while(SDL_PollEvent(&e))dispatch(e);
            app.poll_dialog();
            const bool updated=app.advance();
            const bool minimized=(SDL_GetWindowFlags(window.value)&SDL_WINDOW_MINIMIZED)!=0;
            const bool playing=app.session.playable()&&!app.paused;
            const Uint64 now=SDL_GetTicksNS();
            if(!minimized&&(updated||redraw||(!playing&&now>=next_ui_frame))) {
                ImGui_ImplSDLRenderer3_NewFrame();ImGui_ImplSDL3_NewFrame();ImGui::NewFrame();
                app.draw(window.value);
                ImGui::Render();
                SDL_SetRenderDrawColor(renderer.value,0,0,0,255);SDL_RenderClear(renderer.value);
                app.draw_game(renderer.value);
                ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(),renderer.value);
                SDL_RenderPresent(renderer.value);
                redraw=false;next_ui_frame=now+1000000000/60;
                if(smoke&&++frames>=6)app.running=false;
            }
            if(!app.running)break;
            // Wait until the next useful frame, without tying the GBA
            // clock to monitor refresh or presenting the same texture repeatedly.
            Uint64 deadline=minimized?now+100000000:next_ui_frame;
            if(app.session.playable()&&!app.paused) {
                constexpr double interval=static_cast<double>(HN_CYCLES_PER_FRAME)/static_cast<double>(HN_CLOCK_HZ);
                deadline=app.last_tick+static_cast<Uint64>(std::max(0.0,interval-app.elapsed)*1e9);
            }
            const Uint64 after=SDL_GetTicksNS();
            const int wait=deadline>after?static_cast<int>(std::clamp<Uint64>((deadline-after)/1000000,1,100)):1;
            SDL_Delay(wait);
        }
        if(!(SDL_GetWindowFlags(window.value)&SDL_WINDOW_FULLSCREEN)) {
            SDL_GetWindowSize(window.value,&app.settings.window_width,&app.settings.window_height);
        }
        app.save_preferences();
    }
    if(smoke) {
        if(!explicit_profile)std::filesystem::remove_all(paths.root);
        std::cout<<"Application smoke test passed.\n";
    }
    return 0;
}
}
int main(int argc,char **argv)
{
    try{return run(argc,argv);}
    catch(const std::exception &e) {
        std::cerr<<e.what()<<'\n';
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,game_title,hn::translate(hn::automatic_language,e.what()),nullptr);
        return 1;
    }
}
