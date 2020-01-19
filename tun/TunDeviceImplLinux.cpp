#ifdef __linux__

#include "TunDeviceImpl.h"
#include "../state/Context.h"


#include <unistd.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <cstdio>
#include <sys/ioctl.h>
#include <boost/algorithm/string/replace.hpp>
#include "../utils/Shell.h"

int tun_create(char if_name[IFNAMSIZ], const char *wanted_name)
{
    struct ifreq ifr;
    int          fd;
    int          err;

    fd = open("/dev/net/tun", O_RDWR);
    if (fd == -1) {
        return -1;
    }
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", wanted_name == NULL ? "" : wanted_name);
    if (ioctl(fd, TUNSETIFF, &ifr) != 0) {
        (void) close(fd);
        return -1;
    }
    snprintf(if_name, IFNAMSIZ, "%s", ifr.ifr_name);
    return fd;
}

bool tun_setup(Context* context)
{
    Shell shell;

    std::string ifUp = "ip link set dev $IF_NAME up";
    std::string ifUp2 = "iptables -t raw -I PREROUTING ! -i $IF_NAME -d $LOCAL_TUN_IP -m addrtype ! --src-type LOCAL -j DROP";
    std::string ipv4Up = "ip addr add $LOCAL_TUN_IP peer $REMOTE_TUN_IP dev $IF_NAME";
    std::string ipv6Up = "ip -6 addr add $LOCAL_TUN_IP6 peer $REMOTE_TUN_IP6/96 dev $IF_NAME";

    boost::replace_first(ifUp, "$IF_NAME", context->IfName());

    boost::replace_first(ifUp2, "$IF_NAME", context->IfName());
    boost::replace_first(ifUp2, "$LOCAL_TUN_IP", context->LocalTunIP());

    boost::replace_first(ipv4Up, "$LOCAL_TUN_IP", context->LocalTunIP());
    boost::replace_first(ipv4Up, "$REMOTE_TUN_IP", context->RemoteTunIP());
    boost::replace_first(ipv4Up, "$IF_NAME", context->IfName());

    boost::replace_first(ipv6Up, "$LOCAL_TUN_IP6", context->LocalTunIP6());
    boost::replace_first(ipv6Up, "$REMOTE_TUN_IP6", context->RemoteTunIP6());
    boost::replace_first(ipv6Up, "$IF_NAME", context->IfName());

    shell.Run(ifUp);
    shell.Run(ifUp2);
    shell.Run(ipv4Up);
    shell.Run(ipv6Up);
    return true;
}
#endif