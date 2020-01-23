#pragma once

#include <boost/asio/ip/udp.hpp>

#include "IConnection.h"

class UDPConnection : public IConnection {
public:

    UDPConnection(boost::asio::io_context& io, std::string conn_key);

    bool Connect(std::string ip_address, uint16_t port) final;

    bool Bind(std::string ip_address, uint16_t port) final;

    size_t Send(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) final;

    size_t Receive(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) final;

    size_t SendTo(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) final;

    size_t ReceiveFrom(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) final;

    void Close() final;

private:
    boost::asio::ip::udp::socket udp_socket;
    boost::asio::ip::udp::endpoint last_recv_ep;
};

