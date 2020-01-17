#pragma once

#include <string>
class Context;
class Router {
public:
    static std::string GetDefaultGatewayIp();
    static std::string GetDefaultInterfaceName();

    /*
     * For Server
     */
    static bool SetServerRouteForNewClient(Context* context);
    static bool UnsetServerRouteForNewClient(Context* context);


    /*
     * Client Only
     */
    static bool SetClientDefaultRoute(Context* context);
    static bool UnsetClientDefaultRoute(Context* context);
};

