#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include "platform/lan.h"
#include "game/state.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <memory>
static unsigned checks;
#define CHECK(c) do{checks++;if(!(c)){std::fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c);std::exit(1);}}while(0)
template<class F> static void until(hn::LanLink &a,HNLink &left,hn::LanLink &b,HNLink &right,F done)
{
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    do{a.poll(left);b.poll(right);if(done())return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<end);
    CHECK(done());
}
static void begin(HNLink &link,unsigned row,unsigned marker)
{
    auto generation=link.generation+1;bool connected=link.connected;link={};link.connected=connected;link.generation=generation;link.active=true;
    link.outgoing.row=static_cast<uint8_t>(row);link.outgoing.bytes[0]=2;link.outgoing.bytes[2]=1;link.outgoing.bytes[5]=static_cast<uint8_t>(marker);
    link.outgoing.bytes[17]=static_cast<uint8_t>(marker);
}
static void wire_failures()
{
    for(unsigned mode=0;mode<3;mode++){
        hn::LanLink host;HNLink mailbox{};host.host(0,true);CHECK(host.state()==hn::LanLink::State::listening);
        SOCKET peer=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(peer!=INVALID_SOCKET);
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(host.port());address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        CHECK(connect(peer,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);
        unsigned char packet[145]{};packet[0]='H';packet[1]='H';packet[2]='L';packet[3]='N';packet[4]=1;packet[5]=1;
        std::memcpy(packet+16,"AH3P",4);if(mode==0)packet[4]=99;
        // Real TCP fragmentation: partial headers never become a valid connection.
        for(unsigned i=0;i<145;i++){
            CHECK(send(peer,reinterpret_cast<char*>(packet+i),1,0)==1);host.poll(mailbox);
            if(i<144)CHECK(!mailbox.connected);
            if(mode==2&&i==22)break;
        }
        if(mode==1){
            for(unsigned i=0;i<200&&!mailbox.connected;i++){host.poll(mailbox);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
            CHECK(mailbox.connected);packet[5]=2;packet[8]=1;packet[16]=10;
            CHECK(send(peer,reinterpret_cast<char*>(packet),sizeof(packet),0)==sizeof(packet));
        }
        closesocket(peer);
        for(unsigned i=0;i<200&&host.state()!=hn::LanLink::State::error;i++){host.poll(mailbox);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        CHECK(host.state()==hn::LanLink::State::error&&!mailbox.connected&&!mailbox.complete);
    }
}
int main()
{
    wire_failures();
    hn::LanLink a,b;HNLink left{},right{};
    b.join("invalid");CHECK(b.state()==hn::LanLink::State::error);
    a.host(0,true);CHECK(a.state()==hn::LanLink::State::listening&&a.port());
    hn::LanLink occupied;occupied.host(a.port(),true);CHECK(occupied.state()==hn::LanLink::State::error);
    b.join("127.0.0.1",a.port());until(a,left,b,right,[&]{return left.connected&&right.connected;});
    // Staggered entry, two-way exchange, and repeated transactions on one connection.
    for(unsigned round=0;round<3;round++){
        begin(left,round,12+round);for(unsigned i=0;i<5;i++){a.poll(left);b.poll(right);}
        begin(right,round+3,22+round);
        until(a,left,b,right,[&]{return left.peer_active&&right.peer_active;});
        CHECK(!left.complete&&!right.complete);left.ready=true;
        until(a,left,b,right,[&]{return right.peer_ready;});CHECK(!left.complete&&!right.complete);
        right.ready=true;until(a,left,b,right,[&]{return left.complete&&right.complete;});
        CHECK(!std::memcmp(&left.incoming,&right.outgoing,sizeof(HNLinkRecord)));
        CHECK(!std::memcmp(&right.incoming,&left.outgoing,sizeof(HNLinkRecord)));
        left.active=right.active=false;until(a,left,b,right,[&]{return !left.peer_active&&!right.peer_active;});
    }
    // No publication to guest memory or SRAM before a complete, validated exchange.
    auto memory=std::make_unique<HNMemory>();hn_memory_init(memory.get(),nullptr,0);memory->link=&left;
    uint32_t consumed=99;hn_write8(memory.get(),0x020046B1,1);
    CHECK(hn_link_callback(memory.get(),0x080C7025,&consumed)==HN_SCENE_OK&&consumed==0&&left.outgoing.row==255);
    CHECK(!memory->sram_dirty);left.peer_active=true;
    CHECK(hn_link_callback(memory.get(),0x080C70B9,&consumed)==HN_SCENE_OK&&hn_read8(memory.get(),0x020046B1)==4);
    hn_write32(memory.get(),0x03002BB0,0x02020000);hn_write8(memory.get(),0x020046B2,1);
    left.complete=true;left.incoming.row=2;left.incoming.bytes[0]=2;left.incoming.bytes[2]=1;left.incoming.bytes[17]=42;
    CHECK(hn_link_callback(memory.get(),0x080C70B8,&consumed)==HN_SCENE_OK&&hn_read8(memory.get(),0x020046B3)==1);
    CHECK(hn_read8(memory.get(),0x03003D30)==3&&hn_read8(memory.get(),0x03003D41)==42&&!memory->sram_dirty);
    CHECK(hn_read8(memory.get(),0x02022B0E)==1&&(hn_read8(memory.get(),0x030021D0)&0x20));
    CHECK(hn_link_callback(memory.get(),0x080C70AC,&consumed)==HN_SCENE_OK&&!left.active);
    a.poll(left);b.poll(right);
    // Cancellation and disconnect retain the record and never report success.
    begin(left,0,7);begin(right,1,8);until(a,left,b,right,[&]{return left.peer_active&&right.peer_active;});
    left.active=false;until(a,left,b,right,[&]{return right.failed;});CHECK(!right.complete);
    a.disconnect();until(a,left,b,right,[&]{return !right.connected;});CHECK(right.failed);
    hn_write8(memory.get(),0x020046B1,0);
    CHECK(hn_link_callback(memory.get(),0x080C70B8,&consumed)==HN_SCENE_OK&&hn_read8(memory.get(),0x020046B3)==2);
    CHECK(hn_read8(memory.get(),0x03003D41)==42&&!memory->sram_dirty);
    HNLinkRecord bad{};bad.row=10;CHECK(!hn_link_record_valid(&bad));bad.row=0;bad.bytes[0]=2;bad.bytes[2]=8;CHECK(!hn_link_record_valid(&bad));
    bad.bytes[2]=1;bad.bytes[17]=255;CHECK(!hn_link_record_valid(&bad));
    std::printf("%u LAN transport and native exchange checks passed.\n",checks);
}
