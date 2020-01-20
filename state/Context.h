#pragma once

#include <string>
#include "../route/Router.h"
#include "../tun/TunDevice.h"
#include "../utils/Singleton.h"
#include <boost/asio/io_context.hpp>
#include <boost/make_shared.hpp>

struct context_detail {
    bool is_server;
    // this may change, may call Router::GetDefaultInterfaceName() to update
    std::string ext_if_name;
    std::string gateway_ip;
    // name of the tun device
    std::string tun_if_name;
    std::string local_tun_ip;
    std::string remote_tun_ip;
    std::string local_tun_ip6;
    std::string remote_tun_ip6;
    std::string server_ip_or_name;
    std::string server_ip_resolved;
    uint16_t    server_port;
};

class Context {
    friend class ContextHelper;
public:
    Context(boost::asio::io_context& io) : tun_device(boost::make_shared<TunDevice>(io)){}
    ~Context();

    bool InitByFile();
    bool Init();

    const boost::shared_ptr<TunDevice>& GetTunDevice() { return this->tun_device; }

    auto& IsServer() const { return this->detail.is_server; }
    auto& TunIfName() const { return this->detail.tun_if_name; }
    auto& ExtIfName() const { return this->detail.ext_if_name; }
    auto& LocalTunIP() const { return this->detail.local_tun_ip; }
    auto& RemoteTunIP() const { return this->detail.remote_tun_ip; }
    auto& LocalTunIP6() const { return this->detail.local_tun_ip6; }
    auto& RemoteTunIP6() const { return this->detail.remote_tun_ip6; }
    auto& ServerIPOrName() const { return this->detail.server_ip_or_name; }
    auto& ServerIPResolved() const { return this->detail.server_ip_resolved; }
    auto& ServerPort() const { return this->detail.server_port; }

    boost::asio::io_context& GetIO() {
        return this->tun_device->GetIO();
    }

private:
    context_detail detail;
    boost::shared_ptr<TunDevice> tun_device;
};



