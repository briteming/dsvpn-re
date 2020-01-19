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
    static bool AddClient(Context* context);
    static bool DeleteClient(Context* context);


    /*
     * Client Only
     */
    static bool SetClientDefaultRoute(Context* context);
    static bool UnsetClientDefaultRoute(Context* context);
};

