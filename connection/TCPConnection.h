#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include "IConnection.h"

class TCPConnection : public IConnection {
public:

    TCPConnection(boost::asio::io_context& io, std::string conn_key) : IConnection(io, conn_key), tcp_socket(io) {

    }

    virtual bool Connect(std::string ip_address, uint16_t port) override {
        auto remote_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port);
        boost::system::error_code ec;
        this->tcp_socket.open(remote_endpoint.protocol(), ec);
        this->tcp_socket.connect(remote_endpoint, ec);
        if (ec) {
            printf("connect failed --> %s\n", ec.message().c_str());
            return false;
        }
        return true;
    }

    virtual bool Bind(std::string ip_address, uint16_t port) override {
        auto remote_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port);
        boost::system::error_code ec;
        this->tcp_socket.open(remote_endpoint.protocol(), ec);
        this->tcp_socket.bind(remote_endpoint, ec);
        if (ec) {
            printf("bind failed --> %s\n", ec.message().c_str());
            return false;
        }
        return true;
    }

    virtual size_t Send(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) override  {
        auto header = (ProtocolHeader*)(this->GetTunBuffer() - ProtocolHeader::ProtocolHeaderSize());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);
        return this->tcp_socket.async_send(boost::asio::buffer((void*)header, payload_len + ProtocolHeader::ProtocolHeaderSize()), yield);
    }

    virtual size_t Receive(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
        auto bytes_read = boost::asio::async_read(this->tcp_socket, boost::asio::buffer(this->GetConnBuffer(), ProtocolHeader::ProtocolHeaderSize()), yield);
        if (bytes_read == 0) return 0;
        auto header = (ProtocolHeader*)this->GetConnBuffer();
        auto payload_len = this->protocol.DecryptHeader(header);
        if (payload_len == 0) return 0;
        bytes_read = boost::asio::async_read(this->tcp_socket, boost::asio::buffer(this->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize(), header->PAYLOAD_LENGTH), yield);
        if (bytes_read == 0) return 0;
        if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
        return 0;
    }

    // send to the last received ep
    // if conn never ReceiveFrom packet before, the sendto will fail
    virtual size_t SendTo(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
        auto header = (ProtocolHeader*)(this->GetTunBuffer() - ProtocolHeader::ProtocolHeaderSize());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);
        return this->udp_socket.async_send_to(boost::asio::buffer((void*)header, payload_len + ProtocolHeader::ProtocolHeaderSize()), last_recv_ep, yield);
    }

    virtual size_t ReceiveFrom(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) override {
        auto bytes_read = this->udp_socket.async_receive_from(buffer, this->last_recv_ep, yield);
        auto header = (ProtocolHeader*)this->GetConnBuffer();
        auto payload_len = this->protocol.DecryptHeader(header);
        if (payload_len == 0) return 0;
        if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
        return 0;
    }

    virtual void Close() override {
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this](boost::asio::yield_context yield) {
            if (this->async_tasks_running > 0)
            {
                this->udp_socket.close();
            }
        });
    }


private:
    boost::asio::ip::tcp::socket tcp_socket;
    boost::asio::ip::tcp::endpoint last_recv_ep;

    size_t ReceiveHead(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {

        return 0;
    };

