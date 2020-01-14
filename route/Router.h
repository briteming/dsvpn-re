#pragma once

#include <string>

class Router {
public:
    static std::string GetDefaultGatewayIp();
    static std::string GetDefaultInterfaceName();
    static bool SetRoute(bool is_server);
    static bool UnSetRoute(bool is_server);
};

