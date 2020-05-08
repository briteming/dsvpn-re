#pragma once

#include <boost/make_shared.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include "IConnection.h"

class TCPConnection : public IConnection {
public:

    TCPConnection(boost::asio::io_context &io, std::string conn_key) : IConnection(io, conn_key), tcp_acceptor(io), tcp_socket(io), selected_signal(io) {

    }

    virtual bool Connect(std::string ip_address, uint16_t port) override {
        auto remote_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port);
        boost::system::error_code ec;
        this->conn_socket = boost::make_shared<boost::asio::ip::tcp::socket>(this->io_context);
        this->conn_socket->open(remote_endpoint.protocol(), ec);
        this->conn_socket->connect(remote_endpoint, ec);
        if (ec) {
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

    // we might accept non vpn conn, we accept all of them and check it one by one in tryReceive
    void Accept() override {
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this](boost::asio::yield_context yield){
            while(true) {
                auto conn_socket = boost::make_shared<boost::asio::ip::tcp::socket>(this->io_context);
                boost::system::error_code ec;
                this->tcp_acceptor.async_accept(*conn_socket, yield[ec]);
                if (ec) {
                    printf("async_accept err --> %s\n", ec.message().c_str());
                    return;
                }
                tryReceive(conn_socket);
            }
        });
    }

    void tryReceive(const boost::shared_ptr<boost::asio::ip::tcp::socket>& conn_socket) {
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this, conn_socket](boost::asio::yield_context yield){
            ProtocolHeader * header;
            size_t payload_len;
            size_t bytes_read;
            boost::system::error_code ec;
            bytes_read = boost::asio::async_read(*conn_socket, boost::asio::buffer(
                    this->GetConnBuffer(), ProtocolHeader::Size()), yield[ec]);
            if (ec) {
                printf("error --> %s\n", ec.message().c_str());
            }
            if (bytes_read == 0) return;
            header = (ProtocolHeader *) this->GetConnBuffer();
            payload_len = this->protocol.DecryptHeader(header);
            if (payload_len == 0) return;
            bytes_read = boost::asio::async_read(*conn_socket, boost::asio::buffer(
                    this->GetConnBuffer() + ProtocolHeader::Size(), payload_len), yield);
            if (bytes_read == 0) return;
            // if we can decode the payload successfully, the conn_socket is chosen
            if (this->protocol.DecryptPayload(header)) {}
            printf("client dsvpn%d connection from %s:%d\n", this->tcp_acceptor.local_endpoint().port(), conn_socket->remote_endpoint().address().to_string().c_str(), conn_socket->remote_endpoint().port());

            // we are not going to replace the old one
//            if (this->conn_socket) {
//                // close the old socket
//                this->conn_socket->close();
//                this->conn_socket.reset();
//            }
            if (!selected) {
                selected = true;
                this->conn_socket = conn_socket;
                // notify the ReceiveFrom
                this->selected_signal.cancel();
            }else {
                // if conn already exist, we need to replace it
                this->replace_socket = conn_socket;
                this->conn_socket->cancel();
            }
        });
    }

    // for client, Send is called only when the conn is established.
    virtual size_t Send(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) override {
        auto header = (ProtocolHeader *) (this->GetTunBuffer() - ProtocolHeader::Size());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);

        auto bytes_send = boost::asio::async_write(*this->conn_socket,
                boost::asio::buffer((void*)header, payload_len + ProtocolHeader::Size()), yield);
        return bytes_send;
    }

    // for client, Receive is called only when the conn is established.
    virtual size_t Receive(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) override {

        ProtocolHeader * header;
        size_t payload_len;
        size_t bytes_read;

        bytes_read = boost::asio::async_read(*this->conn_socket, boost::asio::buffer(
                this->GetConnBuffer(), ProtocolHeader::Size()), yield);
        if (bytes_read == 0) return 0;
        //recheck chosen socket
        if (!this->conn_socket) return 0;
        header = (ProtocolHeader *) this->GetConnBuffer();
        payload_len = this->protocol.DecryptHeader(header);
        if (payload_len == 0) return 0;
        bytes_read = boost::asio::async_read(*this->conn_socket, boost::asio::buffer(
                this->GetConnBuffer() + ProtocolHeader::Size(), payload_len), yield);
        if (bytes_read == 0) return 0;
        if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
        return 0;
    }

    // send to the last received ep
    // if conn never ReceiveFrom packet before, the sendto will fail
    // return -1 if stopped
    virtual size_t SendTo(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) override {
        // if no conn is selected, we drop the packet with no error
        if (!selected) return 0;
        auto header = (ProtocolHeader *) (this->GetTunBuffer() - ProtocolHeader::Size());
        header->PAYLOAD_LENGTH = buffer.size();
        header->PADDING_LENGTH = 0;
        this->protocol.EncryptPayload(header);
        auto payload_len = this->protocol.EncryptHeader(header);
        auto bytes_send = boost::asio::async_write(*this->conn_socket, boost::asio::buffer((void *) header, payload_len + ProtocolHeader::Size()), yield);
        if (this->stopped) {
            yield.ec_->clear();
            return -1;
        }
        // if something goes wrong with write
        // set flag and notify async_read let it closes the socket
        if (*yield.ec_) {
            boost::system::error_code ec;
            this->conn_socket->cancel(ec);
            this->selected = false;
            yield.ec_->clear();
            return 0;
        }
        return bytes_send;
    }

    // we don't have decrypt err in tcp
    // return -1 if stopped
    // return + if packet could be decrypted
    // if err, we just retry
    virtual size_t ReceiveFrom(boost::asio::mutable_buffer &&buffer, boost::asio::yield_context &&yield) override {
        while (true) {
            // we sleep where there's no selected conn socket
            // no data race here cause all callback runs in same io_context with only 1 thread
            while (!selected) {
                this->selected_signal.expires_from_now(boost::posix_time::pos_infin);
                this->selected_signal.async_wait(yield);
            }
            // as long as selected_signal return , a valid conn is set

            ProtocolHeader * header;
            size_t payload_len;
            size_t bytes_read;

            bytes_read = boost::asio::async_read(*this->conn_socket, boost::asio::buffer(
                    this->GetConnBuffer(), ProtocolHeader::Size()), yield);
            // if we lost conn with the select socket, we close the conn
//            printf("read %zu bytes\n", bytes_read);
            if (*yield.ec_) {
                // if Close is called, stopped would be set to true
                if (this->stopped) {
                    return -1;
                }
                boost::system::error_code ec;
                this->conn_socket->close(ec);
                this->conn_socket.reset();
                this->selected = false;

                if (this->replace_socket) {
                    this->conn_socket.swap(this->replace_socket);
                    this->selected = true;
                }

                continue;
            }

            if (bytes_read == 0) continue;

            header = (ProtocolHeader *) this->GetConnBuffer();
            payload_len = this->protocol.DecryptHeader(header);
            if (payload_len == 0) continue;
            bytes_read = boost::asio::async_read(*this->conn_socket, boost::asio::buffer(
                    this->GetConnBuffer() + ProtocolHeader::Size(), payload_len), yield);
            if (bytes_read == 0) continue;
            if (this->protocol.DecryptPayload(header)) return header->PAYLOAD_LENGTH;
        }
    }

    virtual void Close() override {
        auto self(this->shared_from_this());
        boost::asio::spawn(this->io_context, [self, this](boost::asio::yield_context yield) {
            this->tcp_acceptor.close();
            if (this->async_tasks_running > 0) {
                if (this->selected) {
                    boost::system::error_code ec;
                    this->conn_socket->close(ec);
                    this->conn_socket.reset();
                    this->selected = false;
                    this->stopped = true;
                }
            }
        });
    }


private:
    boost::asio::ip::tcp::acceptor tcp_acceptor;
    boost::asio::ip::tcp::socket tcp_socket;
    boost::shared_ptr<boost::asio::ip::tcp::socket> conn_socket;
    boost::shared_ptr<boost::asio::ip::tcp::socket> replace_socket;
    boost::asio::deadline_timer selected_signal;

    // indicate whether there is a valid conn already
    std::atomic_bool stopped = false;
    std::atomic_bool selected = false;
};