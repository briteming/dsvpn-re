//
// Created by System Administrator on 2020/1/14.
//

#include "TunDevice.h"
#include "TunDeviceImpl.h"
#include <boost/asio/read.hpp>

TunDevice::TunDevice(boost::asio::io_context &io) : io_context(io) {
    this->asio_fd = std::make_unique<ASIO_FD>(this->io_context);
}

bool TunDevice::Create(const char *wanted_name) {
    char if_name[16];
    auto fd = tun_create(if_name, wanted_name);
    if (fd > 0) {
        this->tun_fd = fd;
        this->tun_name = std::string(if_name);
        this->asio_fd->assign(this->tun_fd);
        return fd;
    }else {
        return -1;
    }
}

bool TunDevice::Setup(Context* context) {
    return tun_setup(context);
}

bool TunDevice::SetMTU(int mtu) {
    return false;
}

size_t TunDevice::Read(const boost::asio::mutable_buffer &&buffer, boost::asio::yield_context&& yield) {
    return this->asio_fd->async_read_some(buffer, yield);
}

size_t TunDevice::Write(const boost::asio::mutable_buffer &&buffer, boost::asio::yield_context&& yield) {
    return this->asio_fd->async_write_some(buffer, yield);
}
