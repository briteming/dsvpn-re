#pragma once

#include <string>
#include "../route/Router.h"
#include "../tun/TunDevice.h"
#include "../utils/Singleton.h"
#include <boost/asio/io_context.hpp>

class Context {
public:
    Context(boost::asio::io_context& io) : tun_device(io) {}

    bool Init();

    TunDevice& GetTunDevice() { return this->tun_device; }

    auto& IsServer() const { return this->is_server; }
    auto& IfName() const { return this->if_name; }
    auto& LocalTunIP() const { return this->local_tun_ip; }
    auto& RemoteTunIP() const { return this->remote_tun_ip; }
    auto& LocalTunIP6() const { return this->local_tun_ip6; }
    auto& RemoteTunIP6() const { return this->remote_tun_ip6; }
    auto& ServerIPOrName() const { return this->server_ip_or_name; }
    auto& ServerIPResolved() const { return this->server_ip_resolved; }
    auto& ServerPort() const { return this->server_port; }

private:
    bool is_server;
    std::string if_name;
    std::string local_tun_ip;
    std::string remote_tun_ip;
    std::string local_tun_ip6;
    std::string remote_tun_ip6;
    std::string server_ip_or_name;
    std::string server_ip_resolved;
    uint16_t    server_port;

    TunDevice tun_device;
};



