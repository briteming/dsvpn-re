#ifdef __linux__

#include "RouterImpl.h"

#include "Router.h"
#include "../state/Context.h"
#include "../utils/Shell.h"
#include <boost/algorithm/string.hpp>

bool route_client_set_default(Context* context) {

    std::string add_default_gw_command = "ip route add default dev $IF_NAME table 42069";
    std::string add_default_gw_command2 = "ip route add $EXT_IP dev $EXT_IF_NAME table 42069";
    std::string add_default_gw_command3 = "ip -6 route add default dev $IF_NAME table 42069";
    std::string add_default_gw_command4 = "ip rule add not fwmark 42069 table 42069";
    std::string add_default_gw_command5 = "ip -6 rule add not fwmark 42069 table 42069";
    std::string add_default_gw_command6 = "ip rule add table main suppress_prefixlength 0";
    std::string add_default_gw_command7 = "ip -6 rule add table main suppress_prefixlength 0";

    boost::replace_first(add_default_gw_command, "$IF_NAME", context->IfName());
    boost::replace_first(add_default_gw_command2, "$EXT_IP", context->ServerIPResolved());
    boost::replace_first(add_default_gw_command2, "$EXT_IF_NAME", Router::GetDefaultInterfaceName());
    boost::replace_first(add_default_gw_command3, "$IF_NAME", context->IfName());

    Shell shell;
    shell.Run(add_default_gw_command);
    shell.Run(add_default_gw_command2);
    shell.Run(add_default_gw_command3);
    shell.Run(add_default_gw_command4);
    shell.Run(add_default_gw_command5);
    shell.Run(add_default_gw_command6);
    shell.Run(add_default_gw_command7);

    return true;
}

bool route_client_unset_default(Context* context) {
    std::string del_default_gw_command = "ip rule delete table 42069";
    std::string del_default_gw_command2 = "ip route del $EXT_IP table 42069";
    std::string del_default_gw_command3 = "ip -6 rule delete table 42069";
    std::string del_default_gw_command4 = "ip rule delete table main suppress_prefixlength 0";
    std::string del_default_gw_command5 = "ip -6 rule delete table main suppress_prefixlength 0";
    std::string del_default_gw_command6 = "iptables -t raw -D PREROUTING ! -i $IF_NAME -d $LOCAL_TUN_IP -m addrtype ! --src-type LOCAL -j DROP";

    boost::replace_first(del_default_gw_command2, "$EXT_IP", context->ServerIPResolved());
    boost::replace_first(del_default_gw_command6, "$IF_NAME", context->IfName());
    boost::replace_first(del_default_gw_command6, "$LOCAL_TUN_IP", context->LocalTunIP());

    Shell shell;
    shell.Run(del_default_gw_command);
    shell.Run(del_default_gw_command2);
    shell.Run(del_default_gw_command3);
    shell.Run(del_default_gw_command4);
    shell.Run(del_default_gw_command5);
    shell.Run(del_default_gw_command6);

    return true;
}

bool route_server_add_client(Context* context, std::string client) {

    return true;
}

bool route_server_remove_client(Context* context) {

    return true;
}

#endif