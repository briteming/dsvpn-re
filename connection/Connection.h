#pragma once

#include <string>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "Protocol.h"

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

    bool Bind(std::string ip_address, uint16_t port) {
        auto remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip_address), port);
        boost::system::error_code ec;
        this->udp_socket.open(remote_endpoint.protocol(), ec);
        this->udp_socket.bind(remote_endpoint, ec);
        if (ec) {
            printf("bind failed --> %s\n", ec.message().c_str());
            return false;
        }
        return true;
    }

    size_t Send(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
        auto header = (ProtocolHeader*)(this->GetTunBuffer() - ProtocolHeader::ProtocolHeaderSize());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);
        return this->udp_socket.async_send(boost::asio::buffer((void*)header, payload_len + ProtocolHeader::ProtocolHeaderSize()), yield);
    }

    size_t Receive(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
        this->udp_socket.async_receive(buffer, yield);
        auto header = (ProtocolHeader*)this->GetConnBuffer();
        auto payload_len = this->protocol.DecryptHeader(header);
        if (payload_len == 0) return 0;
        if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
        return 0;
    }

    // send to the last received ep
    // if conn never ReceiveFrom packet before, the sendto will fail
    size_t SendTo(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
        auto header = (ProtocolHeader*)(this->GetTunBuffer() - ProtocolHeader::ProtocolHeaderSize());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);
        return this->udp_socket.async_send_to(boost::asio::buffer((void*)header, payload_len + ProtocolHeader::ProtocolHeaderSize()), last_recv_ep, yield);
    }

    size_t ReceiveFrom(boost::asio::mutable_buffer&& buffer, boost::asio::ip::udp::endpoint& recv_ep, boost::asio::yield_context&& yield) {
        auto bytes_read = this->udp_socket.async_receive_from(buffer, recv_ep, yield);
        this->last_recv_ep = recv_ep;
        auto header = (ProtocolHeader*)this->GetConnBuffer();
        auto payload_len = this->protocol.DecryptHeader(header);
        if (payload_len == 0) return 0;
        if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
        return 0;
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

    char* GetTunBuffer() {
        return this->tun_recv_buffer + ProtocolHeader::ProtocolHeaderSize();
    }

    char* GetConnBuffer() {
        return this->conn_recv_buffer;
    }

private:
    boost::asio::io_context& io_context;
    boost::asio::ip::udp::socket udp_socket;
    std::atomic_int64_t async_tasks_running = 0;
    boost::asio::ip::udp::endpoint last_recv_ep;
    char tun_recv_buffer[1500];
    char conn_recv_buffer[1500];
    Protocol protocol;
};

