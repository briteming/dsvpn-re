//
// Created by System Administrator on 2020/1/15.
//

#include "Context.h"
#include "../utils/YamlHelper.h"
#include "../utils/Resolver.h"
#include <string>
#include "Constant.h"
#include <boost/config.hpp>

bool Context::Init() {
    YamlHelper h;
    auto res = h.Parse<std::string>("is_server");
    if (res.error) {
        printf("is_server not set\n");
        return false;
    }

    this->is_server = res.value == "true";

    if (this->is_server && BOOST_PLATFORM != "linux") {
        printf("should run server on linux\n");
        return false;
    }

    res = h.Parse<std::string>("tun.local_tun_ip");
    if (res.error || (!res.error && res.value == "auto")) {
        this->local_tun_ip = this->is_server ? DEFAULT_SERVER_IP : DEFAULT_CLIENT_IP;
    }

    res = h.Parse<std::string>("tun.remote_tun_ip");
    if (res.error || (!res.error && res.value == "auto")) {
        this->remote_tun_ip = this->is_server ? DEFAULT_CLIENT_IP : DEFAULT_SERVER_IP;
    }

    {
        char local_tun_ip6[40], remote_tun_ip6[40];
        snprintf(local_tun_ip6, sizeof local_tun_ip6, "64:ff9b::%s", this->local_tun_ip.c_str());
        snprintf(remote_tun_ip6, sizeof remote_tun_ip6, "64:ff9b::%s", this->remote_tun_ip.c_str());
        res = h.Parse<std::string>("tun.local_tun_ip6");
        if (res.error || (!res.error && res.value == "auto")) {
            this->local_tun_ip6 = this->is_server ? std::string(remote_tun_ip6) : std::string(local_tun_ip6);
        }

        res = h.Parse<std::string>("tun.remote_tun_ip6");
        if (res.error || (!res.error && res.value == "auto")) {
            this->remote_tun_ip6 = this->is_server ? std::string(local_tun_ip6) : std::string(remote_tun_ip6);
        }
    }

    res = h.Parse<std::string>("server_ip_or_name");
    if (res.error) {
        printf("server_ip_or_name not set\n");
        return false;
    }

//    boost::system::error_code ec;
//    boost::asio::ip::address::from_string( res.value, ec );
//    if (ec) {
//        printf("server_ip_or_name is not a valid domain or ip address\n");
//        return false;
//    }
    this->server_ip_or_name = res.value;


    auto resolve_res = resolve_ip(this->server_ip_or_name, this->server_ip_resolved);
    if (!resolve_res) {
        printf("resolve_ip error\n");
        return false;
    }

    auto portRes = h.Parse<uint16_t>("server_port");
    if (res.error) {
        printf("server_port not set\n");
        return false;
    }
    this->server_port = portRes.value;


    res = h.Parse<std::string>("tun.if_name");
    if (res.error || (!res.error && res.value == "auto")) {
        this->if_name = DEFAULT_TUN_IFNAME;
    }
    this->if_name = res.value;

    auto tunRes = this->tun_device.Create(this->if_name.c_str());
    if (!tunRes) {
        return false;
    }
    this->if_name = tun_device.GetTunName();

    tunRes = this->tun_device.Setup(this);
    if (!tunRes) {
        return false;
    }
    return true;
}