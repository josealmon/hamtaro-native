#include "app/rom.h"
#include "app/settings.h"
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_gamepad.h>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <string_view>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
static int checks=0;
#define CHECK(c) do { checks++; if(!(c))throw std::runtime_error("Check failed: " #c); }while(0)
template<class F> void rejects(F fn){bool failed=false;try{fn();}catch(const std::exception&){failed=true;}CHECK(failed);}
struct Temporary {
    std::filesystem::path path=std::filesystem::temp_directory_path()/
        ("hn-tests-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    Temporary(){if(!std::filesystem::create_directory(path))throw std::runtime_error("Temporary directory exists.");}
    ~Temporary(){std::error_code e;std::filesystem::remove_all(path,e);}
};
static void child(const std::filesystem::path &executable,const std::vector<std::wstring> &arguments)
{
    std::wstring command=L"\""+executable.native()+L"\"";
    for(const auto &argument:arguments)command+=L" \""+argument+L"\"";
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION process{};
    CHECK(CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process));
    CloseHandle(process.hThread);
    const auto waited=WaitForSingleObject(process.hProcess,30000);DWORD code=1;
    if(waited!=WAIT_OBJECT_0){TerminateProcess(process.hProcess,1);WaitForSingleObject(process.hProcess,5000);}
    const auto reported=GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hProcess);
    CHECK(waited==WAIT_OBJECT_0&&reported&&code==0);
}
static std::filesystem::path preferences(const std::filesystem::path &application,const std::filesystem::path &input_test)
{
    Temporary temp;hn::Paths profile{temp.path/hn::from_utf8("preferencias ñ")};
    std::filesystem::create_directory(profile.root);
    auto settings=hn::Settings::defaults();
    settings.language=1;settings.volume=37;settings.mute=true;settings.deadzone=39;
    settings.fullscreen=true;settings.integer_scale=false;settings.pause_unfocused=false;settings.wasd=false;
    settings.window_width=944;settings.window_height=656;
    settings.keys={SDL_SCANCODE_C,SDL_SCANCODE_V,SDL_SCANCODE_LCTRL,SDL_SCANCODE_SPACE,
        SDL_SCANCODE_L,SDL_SCANCODE_K,SDL_SCANCODE_I,SDL_SCANCODE_J,SDL_SCANCODE_P,SDL_SCANCODE_O};
    settings.buttons={SDL_GAMEPAD_BUTTON_NORTH,SDL_GAMEPAD_BUTTON_WEST,SDL_GAMEPAD_BUTTON_START,SDL_GAMEPAD_BUTTON_BACK,
        SDL_GAMEPAD_BUTTON_DPAD_LEFT,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,SDL_GAMEPAD_BUTTON_DPAD_DOWN,SDL_GAMEPAD_BUTTON_DPAD_UP,
        SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER};
    hn::save_settings(profile,settings);
    hn::Save previous,current;previous.fill(0x71);current.fill(0x52);
    hn::write_save(profile,previous);hn::write_save(profile,current);
    const auto saved=hn::read_file(profile.save(),hn::save_size),backup=hn::read_file(profile.backup(),hn::save_size);
    for(int language=-1;language<5;language++){
        settings.language=language;hn::save_settings(profile,settings);
        child(application,{L"--smoke-test",L"--data-dir",profile.root.native()});
        auto loaded=hn::load_settings(profile);
        CHECK(loaded.keys==settings.keys&&loaded.buttons==settings.buttons);
        CHECK(loaded.language==language&&loaded.volume==37&&loaded.mute&&loaded.deadzone==39);
        CHECK(loaded.fullscreen&&!loaded.integer_scale&&!loaded.pause_unfocused&&!loaded.wasd);
        CHECK(loaded.window_width==944&&loaded.window_height==656);
        CHECK(hn::read_file(profile.save(),hn::save_size)==saved&&hn::read_file(profile.backup(),hn::save_size)==backup);
        hn::ProfileLock released(profile);
    }
    child(input_test,{L"--settings",profile.root.native()});
    const std::string future="version=99\r\nvolume=37\r\nfuture.option=734\r\n";
    hn::atomic_text(profile.settings(),future);
    child(application,{L"--smoke-test",L"--data-dir",profile.root.native()});
    CHECK(hn::read_file(profile.settings(),65536)==std::vector<std::uint8_t>(future.begin(),future.end()));
    CHECK(hn::read_file(profile.save(),hn::save_size)==saved&&hn::read_file(profile.backup(),hn::save_size)==backup);
    CHECK(std::filesystem::is_directory(profile.root));
    hn::ProfileLock released(profile);
    return temp.path;
}
int main()
{
    try{
        int argc=0;auto **wide=CommandLineToArgvW(GetCommandLineW(),&argc);CHECK(wide!=nullptr);
        std::vector<std::filesystem::path> arguments;
        for(int i=0;i<argc;i++)arguments.emplace_back(wide[i]);LocalFree(wide);
        if(argc==4&&arguments[1]==L"--preferences"){
            const auto cleaned=preferences(arguments[2],arguments[3]);
            CHECK(!std::filesystem::exists(cleaned));
            std::cout<<checks<<" cross-process application preferences checks passed.\n";return 0;
        }
        Temporary temp;hn::Paths paths{temp.path/hn::from_utf8("datos con acentos ñ")};
        CHECK(hn::utf8(paths.root.filename())=="datos con acentos ñ");
        hn::ProfileLock lock(paths);rejects([&]{hn::ProfileLock second(paths);});
        CHECK(hn::Settings::defaults().language==hn::automatic_language);
        CHECK(!hn::load_settings(paths).pause_unfocused);
        CHECK(hn::language_from_windows_id(MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_MEXICAN))==0);
        CHECK(hn::language_from_windows_id(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_UK))==1);
        CHECK(hn::language_from_windows_id(MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_CANADIAN))==2);
        CHECK(hn::language_from_windows_id(MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_SWISS))==3);
        CHECK(hn::language_from_windows_id(MAKELANGID(LANG_ITALIAN,SUBLANG_ITALIAN_SWISS))==4);
        CHECK(hn::language_from_windows_id(MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE_BRAZILIAN))==1);
        CHECK(hn::language_from_windows_id(0)==1);
        const char *play[]={"Jugar","Play","Jouer","Spielen","Gioca"};
        for(int language=0;language<5;language++)CHECK(std::string_view(hn::translate(language,"Play"))==play[language]);
        CHECK(std::string_view(hn::translate(hn::automatic_language,"Play"))==play[hn::system_language()]);
        CHECK(std::string_view(hn::translate(99,"Play"))=="Play");
        CHECK(std::string_view(hn::translate(3,"Unknown device diagnostic"))=="Unknown device diagnostic");
        for(int language=-1;language<5;language++) {
            auto preference=hn::Settings::defaults();preference.language=language;preference.pause_unfocused=true;
            hn::save_settings(paths,preference);const auto restored=hn::load_settings(paths);
            CHECK(restored.language==language&&restored.pause_unfocused);
        }
        hn::atomic_text(paths.settings(),"version=1\nlanguage=99\n");
        CHECK(hn::load_settings(paths).language==hn::automatic_language);
        CHECK(hn::sha256({})=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        const std::uint8_t abc[]={'a','b','c'};
        CHECK(hn::sha256(abc)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        CHECK(hn::validate_rom({})==hn::RomStatus::wrong_size);
        std::vector<std::uint8_t> fake(hn::rom_size);
        CHECK(hn::validate_rom(fake)==hn::RomStatus::wrong_identity);
        std::copy_n("AH3P",4,fake.data()+0xAC);
        CHECK(hn::validate_rom(fake)==hn::RomStatus::wrong_hash);
        auto bad=temp.path/"bad.gba";hn::atomic_write(bad,fake);
        hn::Save save;save.fill(0x71);hn::write_save(paths,save);
        rejects([&]{hn::import_rom(paths,bad);});
        CHECK(!std::filesystem::exists(paths.rom()));
        CHECK(hn::load_save(paths)==save);
        auto next=save;next[12]=0x19;hn::write_save(paths,next);
        CHECK(hn::load_save(paths)==next);
        CHECK(hn::read_file(paths.backup(),hn::save_size)[12]==0x71);
        hn::restore_save(paths);CHECK(hn::load_save(paths)==save);
        auto exported=temp.path/"export.sav";hn::export_save(paths,exported);
        CHECK(hn::read_file(exported,hn::save_size)==std::vector<std::uint8_t>(save.begin(),save.end()));
        hn::atomic_write(temp.path/"small.sav",abc);
        rejects([&]{hn::import_save(paths,temp.path/"small.sav");});
        CHECK(hn::load_save(paths)==save);
        hn::atomic_write(paths.save(),abc);rejects([&]{hn::load_save(paths);});
        rejects([&]{hn::write_save(paths,save);});
        hn::restore_save(paths);CHECK(hn::load_save(paths)==save);
        auto settings=hn::Settings::defaults();settings.volume=27;settings.keys[0]=SDL_SCANCODE_C;settings.language=1;
        hn::save_settings(paths,settings);auto loaded=hn::load_settings(paths);
        CHECK(loaded.volume==27 && loaded.keys[0]==SDL_SCANCODE_C && loaded.language==1);
        hn::atomic_text(paths.settings(),"version=1\nvolume=-1\nkey.A=9999\ndeadzone=abc\nlanguage=1\r\n");
        loaded=hn::load_settings(paths);
        CHECK(loaded.volume==80 && loaded.keys[0]==SDL_SCANCODE_Z && loaded.deadzone==25 && loaded.language==1);
        hn::atomic_text(paths.settings(),"version=99\n");
        rejects([&]{hn::load_settings(paths);});
        if(argc>1){
            auto original=arguments[1];
            auto reference=hn::read_file(original,hn::rom_size);
            CHECK(hn::validate_rom(reference)==hn::RomStatus::valid);
            hn::import_rom(paths,original);
            CHECK(hn::load_rom(paths)==reference);
            CHECK(hn::load_save(paths)==save);
            reference[0x200]^=1;
            CHECK(hn::validate_rom(reference)==hn::RomStatus::wrong_hash);
            auto modified=temp.path/"changed.gba";hn::atomic_write(modified,reference);
            rejects([&]{hn::import_rom(paths,modified);});
            CHECK(hn::validate_rom(hn::load_rom(paths))==hn::RomStatus::valid);
        }
        std::cout<<checks<<" storage and ROM checks passed.\n";return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
