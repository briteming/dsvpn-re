#pragma once

#include <string>
#include "../route/Router.h"
#include "../tun/TunDevice.h"
#include "../utils/Singleton.h"
#include <boost/asio/io_context.hpp>

class Context {
public:
    Context() : tun_device(io_context), worker(boost::asio::make_work_guard(io_context)) {}

    bool Init();

    TunDevice& TunDevice() { return this->tun_device; }

    auto& IsServer() const { return this->is_server; }
    auto& IfName() const { return this->if_name; }
    auto& LocalTunIP() const { return this->local_tun_ip; }
    auto& RemoteTunIP() const { return this->remote_tun_ip; }
    auto& LocalTunIP6() const { return this->local_tun_ip6; }
    auto& RemoteTunIP6() const { return this->remote_tun_ip6; }
    auto& ServerIPOrName() const { return this->server_ip_or_name; }
    auto& ServerIPResolved() const { return this->server_ip_resolved; }
    auto& ServerPort() const { return this->server_port; }
    void Run() { this->io_context.run(); }

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

    class boost::asio::io_context io_context;
    class boost::asio::executor_work_guard<boost::asio::io_context::executor_type> worker;
    class TunDevice tun_device;

};



