#pragma once

#include <boost/make_shared.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include "IConnection.h"

class TCPConnection : public IConnection {
public:

    TCPConnection(boost::asio::io_context &io, std::string conn_key) : IConnection(io, conn_key), tcp_acceptor(io), tcp_socket(io), chosen_socket_signal(io) {

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
        auto remote_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
        boost::system::error_code ec;
        this->tcp_acceptor.open(remote_endpoint.protocol(), ec);
        this->tcp_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        this->tcp_acceptor.bind(remote_endpoint, ec);
        this->tcp_acceptor.listen();
        if (ec) {
            printf("bind failed --> %s\n", ec.message().c_str());
            return false;
        }
        return true;
    }

    void Accept() override {
        if (this->accepting) return;
        this->accepting = true;
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this](boost::asio::yield_context yield){
            while(true) {
                auto conn_socket = boost::make_shared<boost::asio::ip::tcp::socket>(this->io_context);
                boost::system::error_code ec;
                this->tcp_acceptor.async_accept(*conn_socket, yield[ec]);
                if (ec) {
                    printf("async_accept err --> %s\n", ec.message().c_str());
                    continue;
                }
                this->accepting = false;
                tryReceive(conn_socket);
            }
        });
    }

    void tryReceive(boost::shared_ptr<boost::asio::ip::tcp::socket> conn_socket) {
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this, conn_socket](boost::asio::yield_context yield){
            ProtocolHeader * header;
            size_t payload_len;
            size_t bytes_read;
            boost::system::error_code ec;
            bytes_read = boost::asio::async_read(*conn_socket, boost::asio::buffer(
                    this->GetConnBuffer(), ProtocolHeader::ProtocolHeaderSize()), yield[ec]);
            if (ec) {
                printf("error --> %s\n", ec.message().c_str());
            }
            if (bytes_read == 0) return;
            header = (ProtocolHeader *) this->GetConnBuffer();
            payload_len = this->protocol.DecryptHeader(header);
            if (payload_len == 0) return;
            bytes_read = boost::asio::async_read(*conn_socket, boost::asio::buffer(
                    this->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize(), header->PAYLOAD_LENGTH), yield);
            if (bytes_read == 0) return;
            // if we can decode the payload successfully, the conn_socket is chosen
            if (this->protocol.DecryptPayload(header)) {}
            printf("client dsvpn%d connection from %s:%d\n", this->tcp_acceptor.local_endpoint().port(), conn_socket->remote_endpoint().address().to_string().c_str(), conn_socket->remote_endpoint().port());
            if (this->conn_socket) {
                // close the old socket
                this->conn_socket->close();
                this->conn_socket.reset();
            }
            this->conn_socket_pending = conn_socket;
            this->chosen_socket_signal.cancel();
        });
    }

    virtual size_t Send(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) override {
        if (!conn_socket) {
            this->conn_socket.swap(this->conn_socket_pending);
            this->conn_socket_pending = nullptr;
        }
        if (!conn_socket) return 0;
        auto header = (ProtocolHeader *) (this->GetTunBuffer() - ProtocolHeader::ProtocolHeaderSize());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);
        //auto bytes_read = boost::asio::async_read(this->conn_socket, boost::asio::buffer(
        //        this->GetConnBuffer(), ProtocolHeader::ProtocolHeaderSize()), yield);
        return 0;
    }

    virtual size_t Receive(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) {

        while (true) {
            if (!conn_socket) {
                if (conn_socket_pending)
                    this->conn_socket.swap(this->conn_socket_pending);
                // sleep if there's no conn_socket available
                this->chosen_socket_signal.async_wait(yield);
            }

            ProtocolHeader * header;
            size_t payload_len;
            size_t bytes_read;

            bytes_read = boost::asio::async_read(*this->conn_socket, boost::asio::buffer(
                    this->GetConnBuffer(), ProtocolHeader::ProtocolHeaderSize()), yield);
            if (bytes_read == 0) continue;
            //recheck chosen socket
            if (!this->conn_socket) continue;
            header = (ProtocolHeader *) this->GetConnBuffer();
            payload_len = this->protocol.DecryptHeader(header);
            if (payload_len == 0) continue;
            bytes_read = boost::asio::async_read(*this->conn_socket, boost::asio::buffer(
                    this->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize(), header->PAYLOAD_LENGTH), yield);
            if (bytes_read == 0) continue;
            if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
        }
    }

    // send to the last received ep
    // if conn never ReceiveFrom packet before, the sendto will fail
    virtual size_t SendTo(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) {
        if (!conn_socket) return 0;
        auto header = (ProtocolHeader *) (this->GetTunBuffer() - ProtocolHeader::ProtocolHeaderSize());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);
        return boost::asio::async_write(*this->conn_socket, boost::asio::buffer((void *) header, payload_len + ProtocolHeader::ProtocolHeaderSize()), yield);
    }

    virtual size_t ReceiveFrom(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) override {
        while (true) {
            while (!conn_socket) {
                if (conn_socket_pending) {
                    this->conn_socket.swap(this->conn_socket_pending);
                    break;
                }
                // sleep if there's no conn_socket available
                this->chosen_socket_signal.expires_from_now(boost::posix_time::pos_infin);
                this->chosen_socket_signal.async_wait(yield);
            }

            ProtocolHeader * header;
            size_t payload_len;
            size_t bytes_read;

            bytes_read = boost::asio::async_read(*this->conn_socket, boost::asio::buffer(
                    this->GetConnBuffer(), ProtocolHeader::ProtocolHeaderSize()), yield);
            if (bytes_read == 0) continue;
            //recheck chosen socket
            if (!this->conn_socket) continue;
            header = (ProtocolHeader *) this->GetConnBuffer();
            payload_len = this->protocol.DecryptHeader(header);
            if (payload_len == 0) continue;
            bytes_read = boost::asio::async_read(*this->conn_socket, boost::asio::buffer(
                    this->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize(), header->PAYLOAD_LENGTH), yield);
            if (bytes_read == 0) continue;
            if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
        }
    }

    virtual void Close() override {
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this](boost::asio::yield_context yield) {
            if (this->async_tasks_running > 0) {
                this->tcp_socket.close();
            }
        });
    }


private:
    boost::asio::ip::tcp::acceptor tcp_acceptor;
    boost::asio::ip::tcp::socket tcp_socket;
    boost::shared_ptr<boost::asio::ip::tcp::socket> conn_socket;
    boost::shared_ptr<boost::asio::ip::tcp::socket> conn_socket_pending;
    boost::asio::deadline_timer chosen_socket_signal;
    boost::asio::ip::tcp::endpoint last_recv_ep;
    std::atomic_bool accepting = false;
    std::atomic_bool conn_socket_changed = false;
    size_t ReceiveHead(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) {

        return 0;
    };

};