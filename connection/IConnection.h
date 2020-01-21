#pragma once

#include <string>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "Protocol.h"

class IConnection : public boost::enable_shared_from_this<IConnection> {
public:

    IConnection(boost::asio::io_context& io, std::string conn_key) : io_context(io), protocol(conn_key) {}

    virtual bool Connect(std::string ip_address, uint16_t port) = 0;

    virtual bool Bind(std::string ip_address, uint16_t port) = 0;

    virtual void Accept() {};

    virtual size_t Send(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) = 0;

    virtual size_t Receive(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) = 0;

    virtual size_t SendTo(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) = 0;

    virtual size_t ReceiveFrom(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) = 0;

    template <class FunctionType>
    void Spawn(FunctionType&& func) {
        this->async_tasks_running++;
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this, func](boost::asio::yield_context yield) {
            func(yield);
            this->async_tasks_running--;
        });
    }

    virtual void Close() = 0;

    char* GetTunBuffer() {
        return this->tun_recv_buffer + ProtocolHeader::ProtocolHeaderSize();
    }

    char* GetConnBuffer() {
        return this->conn_recv_buffer;
    }

protected:
    boost::asio::io_context& io_context;
    std::atomic_int64_t async_tasks_running = 0;
    char tun_recv_buffer[1500];
    char conn_recv_buffer[1500];
    Protocol protocol;
};

