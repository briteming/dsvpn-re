//
// Created by System Administrator on 2020/1/15.
//

#include "Context.h"
#include "../utils/YamlHelper.h"
#include "../utils/Resolver.h"
#include <string>
#include "Constant.h"
#include <boost/config.hpp>
#include "../route/Router.h"
#include <spdlog/spdlog.h>

std::string ConnProtocolTypeToString(ConnProtocolType type) {
    switch(type) {
        case ConnProtocolType::UDP:
            return "UDP";
        case ConnProtocolType::TCP:
            return "TCP";
        case ConnProtocolType::UTCP:
            return "UTCP";
        default:
            return "ConnProtocolTypeToString: unknown type";
    }
}

Context::~Context() {
    SPDLOG_DEBUG("Context die");
}

bool Context::InitByFile() {
    YamlHelper h;
    auto res = h.Parse<std::string>("is_server");
    if (res.error) {
        SPDLOG_INFO("is_server not set");
        return false;
    }

    this->detail.is_server = res.value == "true";

    if (this->detail.is_server && BOOST_PLATFORM != "linux") {
        SPDLOG_INFO("should only run server on linux");
        return false;
    }

    res = h.Parse<std::string>("ipv6");
    if (res.error || (!res.error && res.value == "true")) {
        this->detail.ipv6 = true;
    }

    res = h.Parse<std::string>("tun.mtu");
    if (res.error || (!res.error && res.value == "auto")) {}
    else {
        this->detail.mtu = std::atoi(res.value.c_str());
    }

    res = h.Parse<std::string>("tun.local_tun_ip");
    if (res.error || (!res.error && res.value == "auto")) {
        this->detail.local_tun_ip = this->detail.is_server ? DEFAULT_SERVER_IP : DEFAULT_CLIENT_IP;
    }else {
        this->detail.local_tun_ip = res.value;
    }

    res = h.Parse<std::string>("tun.remote_tun_ip");
    if (res.error || (!res.error && res.value == "auto")) {
        this->detail.remote_tun_ip = this->detail.is_server ? DEFAULT_CLIENT_IP : DEFAULT_SERVER_IP;
    }

    {
        char local_tun_ip6[40], remote_tun_ip6[40];
        snprintf(local_tun_ip6, sizeof local_tun_ip6, "64:ff9b::%s", this->detail.local_tun_ip.c_str());
        snprintf(remote_tun_ip6, sizeof remote_tun_ip6, "64:ff9b::%s", this->detail.remote_tun_ip.c_str());
        res = h.Parse<std::string>("tun.local_tun_ip6");
        if (res.error || (!res.error && res.value == "auto")) {
            this->detail.local_tun_ip6 = this->detail.is_server ? std::string(remote_tun_ip6) : std::string(local_tun_ip6);
        }

        res = h.Parse<std::string>("tun.remote_tun_ip6");
        if (res.error || (!res.error && res.value == "auto")) {
            this->detail.remote_tun_ip6 = this->detail.is_server ? std::string(local_tun_ip6) : std::string(remote_tun_ip6);
        }
    }

    res = h.Parse<std::string>("conn_key");
    if (res.error) {
        SPDLOG_INFO("conn_key is required");
        return false;
    }

    this->detail.conn_key = res.value;

    res = h.Parse<std::string>("conn_protocol");
    if (res.error) {
        SPDLOG_INFO("conn_protocol is required");
        return false;
    }

    if (res.value == "udp") {
        this->detail.conn_protocol = ConnProtocolType::UDP;
    }
    if (res.value == "tcp") {
        this->detail.conn_protocol = ConnProtocolType::TCP;
    }
    if (res.value == "utcp") {
        this->detail.conn_protocol = ConnProtocolType::UTCP;
        SPDLOG_INFO("utcp protocol is not support yet");
        return false;
    }

    res = h.Parse<std::string>("server_ip_or_name");
    if (res.error) {
        SPDLOG_INFO("server_ip_or_name not set");
        return false;
    }

    this->detail.server_ip_or_name = res.value;

    auto resolve_res = resolve_ip(this->detail.server_ip_or_name, this->detail.server_ip_resolved);
    if (!resolve_res) {
        SPDLOG_INFO("resolve_ip error");
        return false;
    }

    auto portRes = h.Parse<uint16_t>("server_port");
    if (res.error) {
        SPDLOG_INFO("server_port is required");
        return false;
    }
    this->detail.server_port = portRes.value;


    res = h.Parse<std::string>("tun.if_name");
    if (res.error || (!res.error && res.value == "auto")) {
        this->detail.tun_if_name = DEFAULT_TUN_IFNAME;
    }
    this->detail.tun_if_name = res.value;

    auto tunRes = this->tun_device->Create(this->detail.tun_if_name.c_str());
    if (!tunRes) {
        return false;
    }
    this->detail.tun_if_name = tun_device->GetTunName();

    tunRes = this->tun_device->Setup(this);
    if (!tunRes) {
        return false;
    }
    return true;
}

bool Context::Init() {

    if (this->detail.is_server && BOOST_PLATFORM != "linux") {
        printf("should run server on linux\n");
        return false;
    }

    this->detail.ext_if_name = Router::GetDefaultInterfaceName();
    this->detail.gateway_ip = Router::GetDefaultGatewayIp();
//    auto resolve_res = resolve_ip(this->server_ip_or_name, this->server_ip_resolved);
//    if (!resolve_res) {
//        printf("resolve_ip error\n");
//        return false;
//    }

    auto tunRes = this->tun_device->Create(this->detail.tun_if_name.c_str());
    if (!tunRes) {
        return false;
    }
    this->detail.tun_if_name = tun_device->GetTunName();

    tunRes = this->tun_device->Setup(this);
    if (!tunRes) {
        return false;
    }
    return true;
}