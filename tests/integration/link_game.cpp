/* Interactive integration worker: normal inputs and read-only observations.
 * Each worker uses a new private profile; no state/ROM patching commands. */
#include "app/session.h"
#include "game/state.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
int main(int argc,char **argv)
{
    try{
        if(argc!=4)throw std::runtime_error("Usage: hn_link_game ROM SAVE NEW_PROFILE");
        hn::Paths paths{hn::from_utf8(argv[3])};
        if(!std::filesystem::create_directory(paths.root))throw std::runtime_error("Use a new private profile.");
        hn::ProfileLock lock(paths);hn::Save save{};
        std::ifstream saved(hn::from_utf8(argv[2]),std::ios::binary);
        if(!saved.read(reinterpret_cast<char*>(save.data()),save.size())||saved.peek()!=EOF)throw std::runtime_error("Invalid save fixture.");
        hn::write_save(paths,save);
        auto rom=std::make_shared<const std::vector<uint8_t>>(hn::read_file(hn::from_utf8(argv[1]),hn::rom_size));
        hn::Session session;if(!session.start(paths,rom))throw std::runtime_error("Initialization failed.");
        auto report=[&]{
            const auto *g=session.game();auto *m=const_cast<HNMemory*>(&g->memory);
            std::cout<<"{\"frame\":"<<g->frames<<",\"fault\":"<<g->fault<<",\"address\":"<<g->address
                <<",\"bank\":"<<unsigned(hn_read8(m,HN_MAP_BANK))<<",\"map\":"<<unsigned(hn_read8(m,HN_MAP_INDEX))
                <<",\"main\":"<<hn_read32(m,0x03002C60)<<",\"opcode\":"<<unsigned(hn_read8(m,0x03000010))
                <<",\"phase\":"<<unsigned(hn_read8(m,0x020046B1))<<",\"ready\":"<<unsigned(hn_read8(m,0x020046B2))
                <<",\"result\":"<<unsigned(hn_read8(m,0x020046B3))<<",\"row\":"<<unsigned(hn_read8(m,HN_SCENE_CONTROLS+0x180))
                <<",\"column\":"<<unsigned(hn_read8(m,HN_SCENE_CONTROLS+0x181))<<",\"net\":"<<static_cast<int>(session.lan().state())
                <<",\"port\":"<<session.lan().port()<<"}"<<std::endl;
        };
        report();std::string line;
        for(;;){
            // Keep the transport alive while the controller inspects a paused worker.
            session.poll_link();DWORD available=0;
            if(!PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE),nullptr,0,nullptr,&available,nullptr))break;
            if(!available){Sleep(5);continue;}
            if(!std::getline(std::cin,line))break;
            std::istringstream command(line);char action=0;command>>action;
            if(action=='Q')break;
            if(action=='H'){unsigned port=0;command>>port;if(port>65535)throw std::runtime_error("Invalid port");session.lan().host(static_cast<uint16_t>(port),true);}
            else if(action=='J'){std::string ip;unsigned port=0;command>>ip>>port;if(!port||port>65535)throw std::runtime_error("Invalid port");session.lan().join(ip.c_str(),static_cast<uint16_t>(port));}
            else if(action=='D')session.lan().disconnect();
            else if(action=='F'){unsigned count=0,keys=0;command>>count>>keys;if(!count||count>10000||keys>1023)throw std::runtime_error("Invalid input");for(unsigned i=0;i<count;i++)if(!session.frame(paths,static_cast<uint16_t>(keys)))break;}
            else if(action=='P'){
                auto pixels=session.pixels();std::ofstream image(paths.root/"frame.rgba",std::ios::binary);image.write(reinterpret_cast<const char*>(pixels.data()),pixels.size_bytes());
            }else if(action=='R'){
                uint32_t address=0;unsigned count=0;command>>std::hex>>address>>std::dec>>count;if(count>16384)throw std::runtime_error("Invalid read");
                std::cout<<"[";auto *memory=const_cast<HNMemory*>(&session.game()->memory);
                for(unsigned i=0;i<count;i++){if(i)std::cout<<",";std::cout<<unsigned(hn_read8(memory,address+i));}std::cout<<"]"<<std::endl;continue;
            }else if(action!='S')throw std::runtime_error("Unknown command");
            session.poll_link();report();
        }
        session.stop(paths);return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<std::endl;return 1;}
}
