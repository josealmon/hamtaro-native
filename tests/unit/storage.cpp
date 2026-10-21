#include "app/rom.h"
#include "app/settings.h"
#include <SDL3/SDL_scancode.h>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
static int checks=0;
#define CHECK(c) do { checks++; if(!(c))throw std::runtime_error("Check failed: " #c); }while(0)
template<class F> void rejects(F fn){bool failed=false;try{fn();}catch(const std::exception&){failed=true;}CHECK(failed);}
struct Temporary {
    std::filesystem::path path=std::filesystem::temp_directory_path()/
        ("hn-tests-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    Temporary(){if(!std::filesystem::create_directory(path))throw std::runtime_error("Temporary directory exists.");}
    ~Temporary(){std::error_code e;std::filesystem::remove_all(path,e);}
};
int main(int argc,char **argv)
{
    try{
        Temporary temp;hn::Paths paths{temp.path/hn::from_utf8("datos con acentos ñ")};
        CHECK(hn::utf8(paths.root.filename())=="datos con acentos ñ");
        hn::ProfileLock lock(paths);rejects([&]{hn::ProfileLock second(paths);});
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
            auto original=hn::from_utf8(argv[1]);
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
