#include "RouterImpl.h"

#include "Router.h"
#include "../state/Context.h"
#include "../utils/Shell.h"
#include <boost/algorithm/string.hpp>

bool route_client_set_default(Context* context) {
    std::string add_default_gw_command = "route add $EXT_IP $EXT_GW_IP";
    std::string add_default_gw_command2 = "route add 0/1 $REMOTE_TUN_IP";
    std::string add_default_gw_command3 = "route add 128/1 $REMOTE_TUN_IP";
    std::string add_default_gw_command4 = "route add -inet6 -blackhole 0000::/1 $REMOTE_TUN_IP6";
    std::string add_default_gw_command5 = "route add -inet6 -blackhole 8000::/1 $REMOTE_TUN_IP6";

    boost::replace_first(add_default_gw_command, "$EXT_IP", context->ServerIPResolved());
    boost::replace_first(add_default_gw_command, "$EXT_GW_IP", Router::GetDefaultGatewayIp());
    boost::replace_first(add_default_gw_command2, "$REMOTE_TUN_IP", context->RemoteTunIP());
    boost::replace_first(add_default_gw_command3, "$REMOTE_TUN_IP", context->RemoteTunIP());
    boost::replace_first(add_default_gw_command4, "$REMOTE_TUN_IP6", context->RemoteTunIP6());
    boost::replace_first(add_default_gw_command5, "$REMOTE_TUN_IP6", context->RemoteTunIP6());

    Shell shell;
    shell.Run(add_default_gw_command);
    shell.Run(add_default_gw_command2);
    shell.Run(add_default_gw_command3);
    shell.Run(add_default_gw_command4);
    shell.Run(add_default_gw_command5);

    return true;
}

bool route_client_unset_default(Context* context) {
    std::string add_default_gw_command = "route delete $EXT_IP";
    std::string add_default_gw_command2 = "route delete 0/1";
    std::string add_default_gw_command3 = "route delete 128/1";
    std::string add_default_gw_command4 = "route delete -inet6 0000::/1";
    std::string add_default_gw_command5 = "route delete -inet6 8000::/1";

    boost::replace_first(add_default_gw_command, "$EXT_IP", context->ServerIPResolved());

    Shell shell;
    shell.Run(add_default_gw_command);
    shell.Run(add_default_gw_command2);
    shell.Run(add_default_gw_command3);
    shell.Run(add_default_gw_command4);
    shell.Run(add_default_gw_command5);

    return true;
}

bool route_server_add_client(Context* context, std::string client) {
    std::string add_default_gw_command = "route delete $EXT_IP";
    std::string add_default_gw_command2 = "route delete 0/1";
    std::string add_default_gw_command3 = "route delete 128/1";
    std::string add_default_gw_command4 = "route delete -inet6 0000::/1";
    std::string add_default_gw_command5 = "route delete -inet6 8000::/1";

    return true;
}

bool route_server_remove_client(Context* context) {
    return true;
}