#pragma once

#include <string>
#include "../route/Router.h"
#include "../tun/TunDevice.h"
#include "../utils/Singleton.h"

struct Context : public Singleton<Context> {

    std::string local_tun_ip;
    std::string remote_tun_ip;
    std::string local_tun_ip6;
    std::string remote_tun_ip6;
    std::string server_ip_or_name;
    std::string server_port;

    TunDevice tun_device;

    bool Init();
};



