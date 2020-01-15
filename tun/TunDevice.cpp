//
// Created by System Administrator on 2020/1/14.
//

#include "TunDevice.h"
#include "TunDeviceImpl.h"

TUN_FD TunDevice::TunCreate(const char *wanted_name) {
    char if_name[16];
    auto fd = tun_create(if_name, wanted_name);
    if (fd > 0) {
        this->tun_fd = fd;
        this->tun_name = std::string(if_name);
        return fd;
    }else {
        return -1;
    }
}

bool TunDevice::TunSetup(Context* context) {
    return tun_setup(context);
}

