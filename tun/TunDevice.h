#pragma once

#include <string>

#ifdef __WIN32
#define TUN_FD HANDLE
#else
#define TUN_FD int
#endif

class Context;
class TunDevice {
public:
    /*
     * @Return -1 if the operation failed
     * @Return >0 if the operation success
     *
     * if the creation successful
     *
     * On Linux, this->tun_name will be set to @wanted_name
     * On BSD/Darwin this->tun_name will be set to the name of the tun device
     * On Windows // TODO
     */
    bool Create(const char *wanted_name = "dsvpn");

    /*
     * @Return -1 if the operation failed
     * @Return >0 if the operation success
     *
     * set MTU for the tun device
     */
    bool SetMTU(int mtu);

    bool Setup(Context* context);

    std::string GetTunName() const { return tun_name; }
private:
    std::string tun_name;
    TUN_FD tun_fd;
};


