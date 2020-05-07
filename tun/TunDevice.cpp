//
// Created by System Administrator on 2020/1/14.
//
#include "../state/Context.h"
#include "TunDevice.h"
#include "TunDeviceImpl.h"
#include <boost/asio/read.hpp>
#include <spdlog/spdlog.h>

TunDevice::TunDevice(boost::asio::io_context &io) : io_context(io) {
    this->asio_fd = std::make_unique<ASIO_FD>(this->io_context);
}

TunDevice::~TunDevice() {
    SPDLOG_DEBUG("TunDevice die");
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
        SPDLOG_INFO("TunDevice::Create failed");
        return -1;
    }
}

bool TunDevice::Setup(Context* context) {
    return this->SetMTU(context->MTU()) && tun_setup(context);
}

bool TunDevice::Close(Context* context) {
    auto self(this->shared_from_this());
    boost::asio::spawn(this->io_context, [self, this, context](boost::asio::yield_context yield) {
        if (this->async_tasks_running > 0)
        {
            this->asio_fd->close();
            tun_remove(context);
        }
    });
    return true;
}

bool TunDevice::SetMTU(int mtu) {
    struct ifreq ifr;
    int          fd;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        return false;
    }
    ifr.ifr_mtu = mtu;
    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", this->tun_name.c_str());
    if (ioctl(fd, SIOCSIFMTU, &ifr) != 0) {
        close(fd);
        SPDLOG_INFO("TunDevice::SetMTU failed {}",this->tun_name);
        return false;
    }
    close(fd);
    return true;
}

size_t TunDevice::Read(const boost::asio::mutable_buffer &&buffer, boost::asio::yield_context&& yield) {
    return this->asio_fd->async_read_some(buffer, yield);
}

size_t TunDevice::Write(const boost::asio::mutable_buffer &&buffer, boost::asio::yield_context&& yield) {
    return this->asio_fd->async_write_some(buffer, yield);
}
