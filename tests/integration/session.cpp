#include "app/session.h"
#include "platform/audio.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
static unsigned checks;
#define CHECK(c) do{checks++;if(!(c))throw std::runtime_error("Check failed: " #c);}while(0)
template<class F> static void rejects(F action)
{ bool failed=false;try{action();}catch(const std::exception&){failed=true;}CHECK(failed); }
struct Workspace {
    hn::Paths paths{std::filesystem::temp_directory_path()/
        ("hamtaro-session-test-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()))};
    Workspace(){CHECK(std::filesystem::create_directory(paths.root));}
    ~Workspace(){std::error_code ignored;std::filesystem::remove_all(paths.root,ignored);}
};
struct FileLock {
    HANDLE handle;
    explicit FileLock(const std::filesystem::path &path):handle(CreateFileW(path.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr))
    { CHECK(handle!=INVALID_HANDLE_VALUE); }
    ~FileLock(){CloseHandle(handle);}
};
int main(int argc,char **argv)
{
    try{
        Workspace workspace;const auto &paths=workspace.paths;hn::Session session;
        CHECK(!session.present()&&!session.playable()&&session.pixels().empty()&&session.samples().empty());
        session.persist(paths);session.stop(paths);CHECK(!std::filesystem::exists(paths.save()));
        rejects([&]{session.start(paths,{});});CHECK(!session.present());
        if(argc==2||argc==3||argc==4) {
            const auto rom=hn::read_file(hn::from_utf8(argv[1]),hn::rom_size);
            const std::uint8_t damaged[]={1,2,3};hn::atomic_write(paths.save(),damaged);
            rejects([&]{session.start(paths,rom);});CHECK(std::filesystem::file_size(paths.save())==3&&!session.present());
            std::filesystem::remove(paths.save());hn::Save blank;blank.fill(0xFF);hn::write_save(paths,blank);
            CHECK(session.start(paths,rom)&&session.playable()&&!session.save_pending());
            const auto initialized=hn::load_save(paths);CHECK(initialized!=blank);
            CHECK(hn::read_file(paths.backup(),hn::save_size)==std::vector<std::uint8_t>(blank.begin(),blank.end()));
            rejects([&]{session.start(paths,rom);});CHECK(hn::load_save(paths)==initialized);
            /* Test-only state injection: an SRAM write during an otherwise successful update. */
            auto *game=const_cast<HNGame*>(session.game());hn_write8(&game->memory,0x0E007FFF,0x73);
            const auto before=game->frames;
            {
                FileLock lock(paths.save());rejects([&]{session.frame(paths,0);});
                CHECK(session.save_pending()&&game->frames==before+1&&session.playable());
            }
            CHECK(hn::load_save(paths)==initialized);
            session.persist(paths);CHECK(!session.save_pending()&&game->frames==before+1);
            auto committed=hn::load_save(paths);CHECK(committed[0x7FFF]==0x73);
            CHECK(hn::read_file(paths.backup(),hn::save_size)==std::vector<std::uint8_t>(initialized.begin(),initialized.end()));
            CHECK(session.frame(paths,0));CHECK(game->sample_frames>=548&&game->sample_frames<=549);
            /* A failed frame's partial SRAM must not replace the preceding committed snapshot. */
            hn_write8(&game->memory,0x0E007FFE,0x31);
            hn_write16(&game->memory,0x04000200,4);hn_write16(&game->memory,0x04000208,1);game->memory.io[0x202]=4;
            CHECK(!session.frame(paths,0)&&game->fault==HN_GAME_IRQ);
            CHECK(!session.save_pending()&&hn::load_save(paths)==committed);
            session.stop(paths);CHECK(!session.present()&&hn::load_save(paths)==committed);
            if(argc==4) {
                CHECK(std::string(argv[2])=="--replay");Workspace replay;hn::Session playing;
                hn::write_save(replay.paths,blank);CHECK(playing.start(replay.paths,rom));
                std::ifstream input(hn::from_utf8(argv[3]));CHECK(input.is_open());
                unsigned count,buttons;std::uint64_t total=0;
                while(input>>count) {
                    CHECK(static_cast<bool>(input>>buttons));
                    CHECK(count>0&&count<=1000000&&buttons<=1023);
                    total+=count;CHECK(total<=1000000);
                    for(unsigned frame=0;frame<count;frame++)CHECK(playing.frame(replay.paths,static_cast<std::uint16_t>(buttons)));
                }
                CHECK(input.eof()&&total>0&&playing.playable()&&!playing.save_pending());
                auto snapshot=[](const HNGame *current) {
                    auto *memory=const_cast<HNMemory*>(&current->memory);
                    const auto slot=hn_read32(memory,0x03002D98)+hn_read8(memory,0x03002C0C)*104;
                    return std::array<std::uint32_t,6>{hn_read8(memory,0x03002C58),hn_read8(memory,0x03001F68),
                        hn_read32(memory,slot+0x18),hn_read32(memory,slot+0x20),hn_read8(memory,0x03003E30),hn_read16(memory,0x0201DC52)};
                };
                const auto expected=snapshot(playing.game());CHECK(expected[4]>10);
                const auto saved=hn::load_save(replay.paths);
                CHECK(std::equal(saved.begin(),saved.end(),playing.game()->memory.sram));
                CHECK(std::filesystem::file_size(replay.paths.save())==hn::save_size);
                playing.stop(replay.paths);CHECK(!playing.present());
                hn::Session restored;CHECK(restored.start(replay.paths,rom));
                auto advance=[&](unsigned frames,std::uint16_t keys) {
                    for(unsigned frame=0;frame<frames;frame++)CHECK(restored.frame(replay.paths,keys));
                };
                advance(1200,0);advance(1,8);advance(240,0);advance(1,1);advance(480,0);
                CHECK(restored.playable()&&snapshot(restored.game())==expected&&!restored.save_pending());
                restored.stop(replay.paths);
                std::cout<<total<<" input updates persisted to private 32 KiB SRAM; a fresh host session continued with the same map, position, words and seeds.\n";
            }
            if(argc==3) {
                CHECK(std::string(argv[2])=="--audio");CHECK(SDL_Init(SDL_INIT_AUDIO));
                {
                    hn::Audio audio;CHECK(audio.available());CHECK(!audio.submit(std::array<std::int16_t,1>{}));
                    CHECK(session.start(paths,rom));CHECK(audio.pause(false));
                    const Uint64 interval=Uint64{1000000000}*static_cast<Uint64>(HN_CYCLES_PER_FRAME)/static_cast<Uint64>(HN_CLOCK_HZ);
                    const Uint64 start=SDL_GetTicksNS();unsigned minimum=32768,maximum=0;
                    for(unsigned frame=0;frame<600;frame++) {
                        Uint64 deadline=start+(frame+1)*interval,now=SDL_GetTicksNS();
                        if(now<deadline)SDL_DelayPrecise(deadline-now);
                        CHECK(session.frame(paths,0));CHECK(audio.submit(session.samples()));
                        auto queued=audio.queued_frames();minimum=std::min(minimum,queued);maximum=std::max(maximum,queued);
                    }
                    CHECK(audio.pause(true));auto shortages=audio.shortages();
                    std::cout<<"Audio device requests short of samples: "<<shortages<<"; queued stereo frames "<<minimum<<".."<<maximum<<".\n";
                    CHECK(shortages==0);audio.clear();CHECK(audio.queued_frames()==0);session.stop(paths);
                }
                SDL_Quit();
            }
        }else CHECK(argc==1);
        std::cout<<checks<<" native session checks passed"<<(argc>=2?" with the private ROM.\n":" (private ROM checks not requested).\n");return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
