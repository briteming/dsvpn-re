#pragma once

#include <string>
#include "../tun/TunDevice.h"
#include <boost/asio/io_context.hpp>
#include <boost/make_shared.hpp>
#include "../connection/ProtocolHeader.h"
enum class ConnProtocolType : int {
    UDP,
    TCP,
    UTCP
};

std::string ConnProtocolTypeToString(ConnProtocolType type);

struct context_detail {
    bool is_server = false;
    // this may change, may call Router::GetDefaultInterfaceName() to update
    std::string ext_if_name = "auto";
    std::string gateway_ip = "auto";
    // name of the tun device
    std::string tun_if_name;
    std::string local_tun_ip = "auto";
    std::string remote_tun_ip = "auto";
    std::string local_tun_ip6 = "auto";
    std::string remote_tun_ip6 = "auto";
    std::string server_ip_or_name = "auto";
    std::string server_ip_resolved = "auto";

    uint32_t mtu = 1500 - sizeof(ProtocolHeader);

    std::string conn_key = "12345678";
    uint16_t    server_port = 1800;

    ConnProtocolType conn_protocol = ConnProtocolType::UDP;

    bool ipv6 = false;
};

class Context {
    friend class ContextHelper;
public:

    Context(boost::asio::io_context& io) : tun_device(boost::make_shared<TunDevice>(io)){ }
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
    auto& ConnKey() const { return this->detail.conn_key; }
    auto& IPv6() const { return this->detail.ipv6; }
    ConnProtocolType ConnProtocol() const { return this->detail.conn_protocol; }
    auto& MTU() const { return this->detail.mtu; }
    boost::asio::io_context& GetIO() {
        return this->tun_device->GetIO();
    }

    void Stop() {
        if (tun_device)
            tun_device->Close(this);
    }
private:
    context_detail detail;
    boost::shared_ptr<TunDevice> tun_device;
};



