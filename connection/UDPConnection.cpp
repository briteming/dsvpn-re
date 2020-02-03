//
// Created by System Administrator on 2020-01-21.
//

#include "UDPConnection.h"

UDPConnection::UDPConnection(boost::asio::io_context& io, std::string conn_key) : IConnection(io, conn_key), udp_socket(io) {

}

bool UDPConnection::Connect(std::string ip_address, uint16_t port) {
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

bool UDPConnection::Bind(std::string ip_address, uint16_t port) {
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

size_t UDPConnection::Send(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
    auto header = (ProtocolHeader*)((char*)buffer.data() - ProtocolHeader::Size());
    header->PAYLOAD_LENGTH = buffer.size();
    header->PADDING_LENGTH = 0;
    this->protocol.EncryptPayload(header);
    auto payload_len = this->protocol.EncryptHeader(header);
    return this->udp_socket.async_send(boost::asio::buffer((void*)header, payload_len + ProtocolHeader::Size()), yield);
}

size_t UDPConnection::Receive(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
    this->udp_socket.async_receive(buffer, yield);
    auto header = (ProtocolHeader*)buffer.data();
    auto payload_len = this->protocol.DecryptHeader(header);
    if (payload_len == 0) return 0;
    if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
    return 0;
}

// send to the last received ep
// if conn never ReceiveFrom packet before, the sendto will fail
size_t UDPConnection::SendTo(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
    auto header = (ProtocolHeader*)((char*)buffer.data() - ProtocolHeader::Size());
    header->PAYLOAD_LENGTH = buffer.size();
    header->PADDING_LENGTH = 0;
    this->protocol.EncryptPayload(header);
    auto payload_len = this->protocol.EncryptHeader(header);
    return this->udp_socket.async_send_to(boost::asio::buffer((void*)header, payload_len + ProtocolHeader::Size()), last_recv_ep, yield);
}

size_t UDPConnection::ReceiveFrom(boost::asio::mutable_buffer&& buffer, boost::asio::yield_context&& yield) {
    auto bytes_read = this->udp_socket.async_receive_from(buffer, this->last_recv_ep, yield);
    auto header = (ProtocolHeader*)this->GetConnBuffer();
    auto payload_len = this->protocol.DecryptHeader(header);
    if (payload_len == 0) return 0;
    if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
    return 0;
}

void UDPConnection::Close() {
    auto self(this->shared_from_this());
    boost::asio::spawn(this->io_context, [self, this](boost::asio::yield_context yield) {
        if (this->async_tasks_running > 0)
        {
            this->udp_socket.close();
        }
    });
}

