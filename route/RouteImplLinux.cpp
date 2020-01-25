#ifdef __linux__

#include "RouterImpl.h"

#include "Router.h"
#include "../state/Context.h"
#include "../utils/Shell.h"
#include <boost/algorithm/string.hpp>

bool route_client_set_default(Context* context) {

    std::string add_default_gw_command = "ip route add default dev $IF_NAME table 42069";
    std::string add_default_gw_command2 = "ip route add $EXT_IP via $EXT_GW_IP dev $EXT_IF_NAME";
    std::string add_default_gw_command4 = "ip rule add not fwmark 42069 table 42069";
    std::string add_default_gw_command6 = "ip rule add table main suppress_prefixlength 0";


    boost::replace_first(add_default_gw_command, "$IF_NAME", context->TunIfName());
    boost::replace_first(add_default_gw_command2, "$EXT_IP", context->ServerIPResolved());
    boost::replace_first(add_default_gw_command2, "$EXT_GW_IP", Router::GetDefaultGatewayIp());
    boost::replace_first(add_default_gw_command2, "$EXT_IF_NAME", Router::GetDefaultInterfaceName());

    Shell shell;
    shell.Run(add_default_gw_command);
    shell.Run(add_default_gw_command2);
    shell.Run(add_default_gw_command4);
    shell.Run(add_default_gw_command6);

    if (context->IPv6()) {
        std::string add_default_gw_command3 = "ip -6 route add default dev $IF_NAME table 42069";
        std::string add_default_gw_command5 = "ip -6 rule add not fwmark 42069 table 42069";
        std::string add_default_gw_command7 = "ip -6 rule add table main suppress_prefixlength 0";

        boost::replace_first(add_default_gw_command3, "$IF_NAME", context->TunIfName());

        shell.Run(add_default_gw_command3);
        shell.Run(add_default_gw_command5);
        shell.Run(add_default_gw_command7);
    }

    return true;
}

bool route_client_unset_default(Context* context) {
    std::string del_default_gw_command = "ip rule delete table 42069";
    std::string del_default_gw_command2 = "ip route del $EXT_IP";
    std::string del_default_gw_command4 = "ip rule delete table main suppress_prefixlength 0";


    boost::replace_first(del_default_gw_command2, "$EXT_IP", context->ServerIPResolved());

    Shell shell;
    shell.Run(del_default_gw_command);
    shell.Run(del_default_gw_command2);
    shell.Run(del_default_gw_command4);

    if (context->IPv6()) {
        std::string del_default_gw_command3 = "ip -6 rule delete table 42069";
        std::string del_default_gw_command5 = "ip -6 rule delete table main suppress_prefixlength 0";

        shell.Run(del_default_gw_command3);
        shell.Run(del_default_gw_command5);
    }

    return true;
}

bool route_server_add_client(Context* context) {

    Shell shell;

    //$REMOTE_TUN_IP is the client's tun ip
    //$IF_NAME is the tun device name
    std::string add_forward_command = "iptables -t nat -A POSTROUTING -o $EXT_IF_NAME -s $REMOTE_TUN_IP -j MASQUERADE";
    std::string add_forward_command2 = "iptables -t filter -A FORWARD -i $EXT_IF_NAME -o $IF_NAME -m state --state RELATED,ESTABLISHED -j ACCEPT";
    std::string add_forward_command3 = "iptables -t filter -A FORWARD -i $IF_NAME -o $EXT_IF_NAME -j ACCEPT";
    std::string add_forward_command4 = "ip6tables -t nat -A POSTROUTING -o $EXT_IF_NAME -s $REMOTE_TUN_IP6 -j MASQUERADE";

    boost::replace_first(add_forward_command, "$EXT_IF_NAME", context->ExtIfName());
    boost::replace_first(add_forward_command, "$REMOTE_TUN_IP", context->RemoteTunIP());
    boost::replace_first(add_forward_command2, "$EXT_IF_NAME", context->ExtIfName());
    boost::replace_first(add_forward_command2, "$IF_NAME", context->TunIfName());
    boost::replace_first(add_forward_command3, "$IF_NAME", context->TunIfName());
    boost::replace_first(add_forward_command3, "$EXT_IF_NAME", context->ExtIfName());
    boost::replace_first(add_forward_command4, "$REMOTE_TUN_IP6", context->RemoteTunIP6());
    boost::replace_first(add_forward_command4, "$EXT_IF_NAME", context->ExtIfName());

    shell.Run(add_forward_command);
    shell.Run(add_forward_command2);
    shell.Run(add_forward_command3);
    shell.Run(add_forward_command4);

    return true;
}

bool route_server_remove_client(Context* context) {
    Shell shell;

    //$REMOTE_TUN_IP is the client's tun ip
    //$IF_NAME is the tun device name
    std::string add_forward_command = "iptables -t nat -D POSTROUTING -o $EXT_IF_NAME -s $REMOTE_TUN_IP -j MASQUERADE";
    std::string add_forward_command2 = "iptables -t filter -D FORWARD -i $EXT_IF_NAME -o $IF_NAME -m state --state RELATED,ESTABLISHED -j ACCEPT";
    std::string add_forward_command3 = "iptables -t filter -D FORWARD -i $IF_NAME -o $EXT_IF_NAME -j ACCEPT";
    std::string add_forward_command4 = "ip6tables -t nat -D POSTROUTING -o $EXT_IF_NAME -s $REMOTE_TUN_IP -j MASQUERADE";

    boost::replace_first(add_forward_command, "$EXT_IF_NAME", context->ExtIfName());
    boost::replace_first(add_forward_command, "$REMOTE_TUN_IP", context->RemoteTunIP());
    boost::replace_first(add_forward_command2, "$EXT_IF_NAME", context->ExtIfName());
    boost::replace_first(add_forward_command2, "$IF_NAME", context->TunIfName());
    boost::replace_first(add_forward_command3, "$IF_NAME", context->TunIfName());
    boost::replace_first(add_forward_command3, "$EXT_IF_NAME", context->ExtIfName());
    boost::replace_first(add_forward_command4, "$REMOTE_TUN_IP", context->RemoteTunIP6());
    boost::replace_first(add_forward_command4, "$EXT_IF_NAME", context->ExtIfName());

    shell.Run(add_forward_command);
    shell.Run(add_forward_command2);
    shell.Run(add_forward_command3);
    shell.Run(add_forward_command4);

    return true;
}

#endif