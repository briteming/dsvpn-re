#include "Router.h"
#include "../utils/Shell.h"
#include "../utils/Trim.h"
#include "RouterImpl.h"

std::string Router::GetDefaultGatewayIp() {
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__NetBSD__)
    return TrimNewline(Shell::Run("route -n get default 2>/dev/null|awk '/gateway:/{print $2;exit}'"));
#elif defined(__linux__)
    return TrimNewline(Shell::Run("ip route show default 2>/dev/null|awk '/default/{print $3}'"));
#else
    return NULL;
#endif
}

std::string Router::GetDefaultInterfaceName() {
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__NetBSD__)
    return TrimNewline(Shell::Run("route -n get default 2>/dev/null|awk '/interface:/{print $2;exit}'"));
#elif defined(__linux__)
    return TrimNewline(Shell::Run("ip route show default 2>/dev/null|awk '/default/{print $5}'"));
#else
    return NULL;
#endif
}

bool Router::SetClientDefaultRoute(Context* context) {
    return route_client_set_default(context);
}

bool Router::UnsetClientDefaultRoute(Context* context) {
    return route_client_unset_default(context);
}

#ifdef __linux__
bool Router::AddClient(Context *context) {
    route_server_add_client(context);
}

bool Router::DeleteClient(Context *context) {
    route_server_remove_client(context);
}
#endif