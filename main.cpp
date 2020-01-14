#include <iostream>
#include "route/Router.h"
#include "tun/TunDevice.h"
int main() {
    TunDevice tun_device;
    auto res = tun_device.TunCreate();
    auto gw = Router::GetDefaultGatewayIp();
    auto ifname = Router::GetDefaultInterfaceName();
    std::cout << gw;
    std::cout << ifname;
    return 0;
}