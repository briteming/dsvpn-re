#ifdef __APPLE__

#include "TunDeviceImpl.h"
#include "../state/Context.h"

#include <unistd.h>
#include <cerrno>
#include <sys/ioctl.h>
#include <cstring>
#include <cstdio>
#include <net/if_utun.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <sys/socket.h>
#include <boost/algorithm/string/replace.hpp>
#include "../utils/Shell.h"


static int tun_create_by_id(char if_name[16], unsigned int id)
{
    struct ctl_info     ci;
    struct sockaddr_ctl sc;
    int                 err;
    int                 fd;

    if ((fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL)) == -1) {
        return -1;
    }
    memset(&ci, 0, sizeof ci);
    snprintf(ci.ctl_name, sizeof ci.ctl_name, "%s", UTUN_CONTROL_NAME);
    if (ioctl(fd, CTLIOCGINFO, &ci)) {
        err = errno;
        (void) close(fd);
        errno = err;
        return -1;
    }
    memset(&sc, 0, sizeof sc);
    sc = (struct sockaddr_ctl){
            .sc_id      = ci.ctl_id,
            .sc_len     = sizeof sc,
            .sc_family  = AF_SYSTEM,
            .ss_sysaddr = AF_SYS_CONTROL,
            .sc_unit    = id + 1,
    };
    if (connect(fd, (struct sockaddr *) &sc, sizeof sc) != 0) {
        err = errno;
        (void) close(fd);
        errno = err;
        return -1;
    }
    snprintf(if_name, 16, "utun%u", id);
    return fd;
}

int tun_create(char if_name[16], const char *wanted_name)
{
    unsigned int id;
    int          fd;
    for (id = 0; id < 32; id++) {
        if ((fd = tun_create_by_id(if_name, id)) != -1) {
            return fd;
        }
    }
    return -1;
}

bool tun_setup(Context* context)
{
    Shell shell;

    std::string ipv4Up = "ifconfig $IF_NAME $LOCAL_TUN_IP $REMOTE_TUN_IP up";

    boost::replace_first(ipv4Up, "$IF_NAME", context->TunIfName());
    boost::replace_first(ipv4Up, "$LOCAL_TUN_IP", context->LocalTunIP());
    boost::replace_first(ipv4Up, "$REMOTE_TUN_IP", context->RemoteTunIP());


    shell.Run(ipv4Up);

    if (context->IPv6()) {
        std::string ipv6Up = "ifconfig $IF_NAME inet6 $LOCAL_TUN_IP6 $REMOTE_TUN_IP6 prefixlen 128 up";

        boost::replace_first(ipv6Up, "$IF_NAME", context->TunIfName());
        boost::replace_first(ipv6Up, "$LOCAL_TUN_IP6", context->LocalTunIP6());
        boost::replace_first(ipv6Up, "$REMOTE_TUN_IP6", context->RemoteTunIP6());

        shell.Run(ipv6Up);
    }
    return true;
}

bool tun_remove(Context* context)
{
    return true;
}
#endif