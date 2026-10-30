#pragma once
#include "game/link.h"
#include <cstdint>
#include <memory>
#include <string>

namespace hn {
class LanLink {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    static constexpr std::uint16_t default_port=24816;
    enum class State { disconnected,listening,connecting,connected,error };
    LanLink();
    ~LanLink();
    LanLink(const LanLink &)=delete;
    LanLink &operator=(const LanLink &)=delete;
    void host(std::uint16_t port=default_port,bool loopback=false);
    void join(const char *ip,std::uint16_t port=default_port);
    void disconnect();
    void poll(HNLink &);
    State state() const;
    const std::string &error() const;
    const std::string &addresses() const;
    std::uint16_t port() const;
};
}
