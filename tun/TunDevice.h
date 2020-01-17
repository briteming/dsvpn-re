#pragma once

#include <string>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>

#ifdef __WIN32
#define TUN_FD HANDLE
#include <boost/asio/windows/stream_handle.hpp>
#define ASIO_FD boost::asio::windows::stream_handle
#else
#define TUN_FD int
#include <boost/asio/posix/stream_descriptor.hpp>
#define ASIO_FD boost::asio::posix::stream_descriptor
#endif


class Context;
class TunDevice {
public:
    TunDevice(boost::asio::io_context& io);

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
     * @Return false if the operation failed
     * @Return true if the operation success
     *
     * set MTU for the tun device
     */
    bool SetMTU(int mtu);

    /*
     * @Return false if the operation failed
     * @Return true if the operation success
     *
     * setup tun device, setting the ip address
     */
    bool Setup(Context* context);

    // pass ec into yield or the exception will be raised
    size_t Read(const boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield);
    size_t Write(const boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield);

    template <class FunctionType>
    void Spawn(FunctionType func) {
        boost::asio::spawn(this->io_context, func);
    }

    [[nodiscard]] std::string GetTunName() const { return tun_name; }
private:
    std::string tun_name;
    TUN_FD tun_fd;
    boost::asio::io_context& io_context;
    std::unique_ptr<ASIO_FD> asio_fd;
};


