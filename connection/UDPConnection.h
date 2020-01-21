#pragma once

#include <boost/asio/ip/udp.hpp>

#include "IConnection.h"

class UDPConnection : public IConnection {
public:

    UDPConnection(boost::asio::io_context& io, std::string conn_key) : IConnection(io, conn_key), udp_socket(io) {

    }

    virtual bool Connect(std::string ip_address, uint16_t port) override {
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

    virtual bool Bind(std::string ip_address, uint16_t port) override {
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

    virtual size_t Send(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) override  {
        auto header = (ProtocolHeader*)(this->GetTunBuffer() - ProtocolHeader::ProtocolHeaderSize());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);
        return this->udp_socket.async_send(boost::asio::buffer((void*)header, payload_len + ProtocolHeader::ProtocolHeaderSize()), yield);
    }

    virtual size_t Receive(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
        this->udp_socket.async_receive(buffer, yield);
        auto header = (ProtocolHeader*)this->GetConnBuffer();
        auto payload_len = this->protocol.DecryptHeader(header);
        if (payload_len == 0) return 0;
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
    boost::asio::ip::udp::socket udp_socket;
    boost::asio::ip::udp::endpoint last_recv_ep;
};

