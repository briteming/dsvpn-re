#pragma once

#include <string>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio/deadline_timer.hpp>

class Connection : public boost::enable_shared_from_this<Connection> {
public:

    Connection(boost::asio::io_context& io) : io_context(io), udp_socket(io) {

    }

    bool Connect(std::string ip_address, uint16_t port) {
        auto remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip_address), port);
        boost::system::error_code ec;
        this->udp_socket.open(remote_endpoint.protocol(), ec);
        this->udp_socket.connect(remote_endpoint, ec);
        if (ec) {
            printf("connect failed --> %s\n", ec.message().c_str());
            return false;
        }
        return true;
    }

    size_t Send(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
        return this->udp_socket.async_send(buffer, yield);
    }

    size_t Receive(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
        return this->udp_socket.async_receive(buffer, yield);
    }

    template <class FunctionType>
    void Spawn(FunctionType&& func) {
        this->async_tasks_running++;
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this, func](boost::asio::yield_context yield) {
            func(this, yield);
            this->async_tasks_running--;
        });
    }

    void Close() {
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this](boost::asio::yield_context yield) {
            if (this->async_tasks_running > 0)
            {
                this->udp_socket.close();
            }
        });
    }
private:
    boost::asio::io_context& io_context;
    boost::asio::ip::udp::socket udp_socket;
    std::atomic_int64_t async_tasks_running = 0;

};

