#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "platform/lan.h"
#include <array>
#include <chrono>
#include <cstring>
#include <deque>
#include <vector>

namespace hn {
namespace {
using Clock=std::chrono::steady_clock;
// Fixed-size, bounded messages: magic/version/type, two transaction IDs, record.
constexpr unsigned packet_size=145;
using Packet=std::array<unsigned char,packet_size>;
enum Type : unsigned char { hello=1,offer,ready,commit,end,heartbeat };
void close_socket(SOCKET &socket){if(socket!=INVALID_SOCKET)closesocket(socket);socket=INVALID_SOCKET;}
void put(Packet &p,unsigned at,std::uint32_t value){for(unsigned i=0;i<4;i++)p[at+i]=static_cast<unsigned char>(value>>(8*i));}
std::uint32_t get(const Packet &p,unsigned at){std::uint32_t v=0;for(unsigned i=0;i<4;i++)v|=std::uint32_t(p[at+i])<<(8*i);return v;}
bool nonblocking(SOCKET s){u_long value=1;return ioctlsocket(s,FIONBIO,&value)==0;}
std::string local_addresses()
{
    ULONG bytes=16384;std::vector<unsigned char> buffer(bytes);
    ULONG result=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,nullptr,
        reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&bytes);
    if(result==ERROR_BUFFER_OVERFLOW){buffer.resize(bytes);result=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,nullptr,reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&bytes);}
    std::string text;
    if(result==NO_ERROR)for(auto *adapter=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());adapter;adapter=adapter->Next){
        if(adapter->OperStatus!=IfOperStatusUp||adapter->IfType==IF_TYPE_SOFTWARE_LOOPBACK)continue;
        for(auto *address=adapter->FirstUnicastAddress;address;address=address->Next){
            char ip[INET_ADDRSTRLEN]{};
            auto *v4=reinterpret_cast<sockaddr_in*>(address->Address.lpSockaddr);
            if(v4->sin_family==AF_INET&&inet_ntop(AF_INET,&v4->sin_addr,ip,sizeof(ip))){if(!text.empty())text+=" / ";text+=ip;}
        }
    }
    return text.empty()?"127.0.0.1":text;
}
}
struct LanLink::Impl {
    SOCKET listener=INVALID_SOCKET,socket=INVALID_SOCKET;
    bool winsock=false,welcomed=false,sent_ready=false,sent_commit=false,commit_written=false,peer_ready=false,peer_commit=false,ended=true;
    State state=State::disconnected;
    std::string error,addresses;
    std::uint16_t port=0;
    std::uint32_t generation=0,local_id=0,remote_id=0,next_id=0;
    HNLinkRecord remote{};
    std::deque<Packet> outgoing;
    Packet incoming{};unsigned received=0,sent=0;
    Clock::time_point last_received{},last_sent{},started{};
    ~Impl(){close();if(winsock)WSACleanup();}
    void close(){close_socket(listener);close_socket(socket);outgoing.clear();received=sent=0;}
    void fail(const char *message){error=message;state=State::error;close();}
    bool startup(){if(winsock)return true;WSADATA data{};winsock=WSAStartup(MAKEWORD(2,2),&data)==0;if(!winsock)fail("Network initialization failed.");return winsock;}
    void queue(Type type,std::uint32_t id=0,std::uint32_t peer=0,const HNLinkRecord *record=nullptr){
        if(outgoing.size()>=32){fail("Invalid Cable Link data.");return;}
        Packet p{};p[0]='H';p[1]='H';p[2]='L';p[3]='N';p[4]=1;p[5]=type;put(p,8,id);put(p,12,peer);
        if(record){p[16]=record->row;std::memcpy(p.data()+17,record->bytes,128);}
        // Protocol 1 is specific to the supported European AH3P revision 0 port.
        if(type==hello){p[16]='A';p[17]='H';p[18]='3';p[19]='P';}
        outgoing.push_back(p);
    }
    void accepted(){
        int enabled=1;setsockopt(socket,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&enabled),sizeof(enabled));
        started=last_sent=last_received=Clock::now();queue(hello);
    }
    void handle(HNLink &link){
        const auto &p=incoming;auto type=p[5];auto id=get(p,8),peer=get(p,12);
        if(p[0]!='H'||p[1]!='H'||p[2]!='L'||p[3]!='N'||p[4]!=1||p[6]||p[7]||type<hello||type>heartbeat){fail("Incompatible Cable Link version.");return;}
        if(!welcomed){
            if(type!=hello||std::memcmp(p.data()+16,"AH3P",4)||id||peer){fail("Incompatible Cable Link version.");return;}
            welcomed=true;state=State::connected;return;
        }
        if(type==hello){fail("Invalid Cable Link data.");return;}
        if(type==offer){
            if(!id||peer||remote_id){fail("Invalid Cable Link data.");return;}
            remote.row=p[16];std::memcpy(remote.bytes,p.data()+17,128);
            if(!hn_link_record_valid(&remote)){fail("Invalid Cable Link data.");return;}
            remote_id=id;peer_ready=peer_commit=false;
        }else if(type==ready||type==commit){
            if(!remote_id||id!=remote_id||!local_id||peer!=local_id||ended)return; // Late message from a cancelled exchange.
            if(type==ready)peer_ready=true;
            else if(!sent_ready||!peer_ready){fail("Invalid Cable Link data.");return;}
            else {peer_commit=true;if(commit_written&&link.ready)link.complete=true;}
        }else if(type==end){
            if(id==remote_id){remote_id=0;peer_ready=peer_commit=false;if(link.active&&!link.complete)link.failed=true;}
        }
    }
};
LanLink::LanLink():impl_(std::make_unique<Impl>()){}
LanLink::~LanLink()=default;
void LanLink::disconnect(){impl_=std::make_unique<Impl>();}
void LanLink::host(std::uint16_t port,bool loopback)
{
    disconnect();auto &s=*impl_;if(!s.startup())return;
    s.listener=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    int exclusive=1;
    if(s.listener==INVALID_SOCKET||setsockopt(s.listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&exclusive),sizeof(exclusive))||!nonblocking(s.listener)){s.fail("Could not create the connection.");return;}
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(port);address.sin_addr.s_addr=htonl(loopback?INADDR_LOOPBACK:INADDR_ANY);
    if(bind(s.listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))||listen(s.listener,1)){s.fail("Could not create the connection. The port may be in use.");return;}
    int size=sizeof(address);getsockname(s.listener,reinterpret_cast<sockaddr*>(&address),&size);s.port=ntohs(address.sin_port);
    s.addresses=loopback?"127.0.0.1":local_addresses();s.state=State::listening;
}
void LanLink::join(const char *ip,std::uint16_t port)
{
    disconnect();auto &s=*impl_;if(!s.startup())return;
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(port);
    if(!port||inet_pton(AF_INET,ip,&address.sin_addr)!=1){s.fail("Enter a valid IPv4 address.");return;}
    s.socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s.socket==INVALID_SOCKET||!nonblocking(s.socket)){s.fail("Could not create the connection.");return;}
    s.port=port;s.state=State::connecting;s.started=Clock::now();
    int result=connect(s.socket,reinterpret_cast<sockaddr*>(&address),sizeof(address));
    if(result==0)s.accepted();
    else if(WSAGetLastError()!=WSAEWOULDBLOCK)s.fail("Could not connect. Check the IP and the private-network firewall permission.");
}
void LanLink::poll(HNLink &link)
{
    auto &s=*impl_;
    if(s.state==State::disconnected||s.state==State::error){link.connected=false;if(link.active&&!link.complete)link.failed=true;return;}
    auto now=Clock::now();
    if(s.state==State::listening){
        s.socket=accept(s.listener,nullptr,nullptr);
        if(s.socket!=INVALID_SOCKET){close_socket(s.listener);if(!nonblocking(s.socket))s.fail("Could not create the connection.");else{s.state=State::connecting;s.accepted();}}
        else if(WSAGetLastError()!=WSAEWOULDBLOCK)s.fail("Could not create the connection.");
    }
    if(s.state==State::connecting&&s.socket!=INVALID_SOCKET&&s.last_received==Clock::time_point{}){
        WSAPOLLFD p{s.socket,POLLWRNORM,0};int result=WSAPoll(&p,1,0);
        if(result==SOCKET_ERROR)s.fail("Connection lost.");
        else if(result>0){
            int error=0,size=sizeof(error);getsockopt(s.socket,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&size);
            if(error||!(p.revents&POLLWRNORM))s.fail("Could not connect. Check the IP and the private-network firewall permission.");else s.accepted();
        }
    }
    if(s.socket!=INVALID_SOCKET&&s.last_received!=Clock::time_point{}){
        for(unsigned budget=0;budget<32&&s.socket!=INVALID_SOCKET;budget++){
            int count=recv(s.socket,reinterpret_cast<char*>(s.incoming.data()+s.received),packet_size-s.received,0);
            if(count==0){s.fail("Connection lost.");break;}
            if(count==SOCKET_ERROR){if(WSAGetLastError()!=WSAEWOULDBLOCK)s.fail("Connection lost.");break;}
            s.received+=count;
            if(s.received==packet_size){s.received=0;s.last_received=now;s.handle(link);}
        }
        if(s.state==State::connected){
            if(link.active&&link.generation!=s.generation){
                if(!s.ended)s.queue(end,s.local_id);
                s.generation=link.generation;s.local_id=++s.next_id;s.sent_ready=s.sent_commit=s.commit_written=s.peer_ready=s.peer_commit=false;s.ended=false;
                s.queue(offer,s.local_id,0,&link.outgoing);
            }
            if(!link.active&&!s.ended){s.queue(end,s.local_id);s.ended=true;}
            if(!link.active||!link.complete){link.peer_active=s.remote_id!=0;link.peer_ready=s.peer_ready;link.incoming=s.remote;}
            if(link.active&&!link.failed&&s.remote_id){
                if(link.ready&&!s.sent_ready){s.queue(ready,s.local_id,s.remote_id);s.sent_ready=true;}
                if(s.sent_ready&&s.peer_ready&&!s.sent_commit){s.queue(commit,s.local_id,s.remote_id);s.sent_commit=true;}
                if(s.commit_written&&s.peer_commit)link.complete=true;
            }
        }
        if(now-s.last_sent>std::chrono::seconds(2)&&s.outgoing.empty())s.queue(heartbeat);
        for(unsigned budget=0;budget<32&&!s.outgoing.empty()&&s.socket!=INVALID_SOCKET;budget++){
            auto &packet=s.outgoing.front();int count=send(s.socket,reinterpret_cast<const char*>(packet.data()+s.sent),packet_size-s.sent,0);
            if(count==SOCKET_ERROR){if(WSAGetLastError()!=WSAEWOULDBLOCK)s.fail("Connection lost.");break;}
            if(count<=0){s.fail("Connection lost.");break;}
            s.sent+=count;s.last_sent=now;if(s.sent==packet_size){
                if(packet[5]==commit&&get(packet,8)==s.local_id){s.commit_written=true;if(s.peer_commit&&link.ready)link.complete=true;}
                s.sent=0;s.outgoing.pop_front();
            }
        }
        if(now-s.last_received>std::chrono::seconds(15))s.fail("Connection timed out.");
    }
    if(s.state==State::connecting&&now-s.started>std::chrono::seconds(10))s.fail("Connection timed out.");
    link.connected=s.state==State::connected;
    if(!link.connected&&link.active&&!link.complete)link.failed=true;
}
LanLink::State LanLink::state()const{return impl_->state;}
const std::string &LanLink::error()const{return impl_->error;}
const std::string &LanLink::addresses()const{return impl_->addresses;}
std::uint16_t LanLink::port()const{return impl_->port;}
}
