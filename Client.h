#pragma once

#include "connection/IConnection.h"
#include "connection/UDPConnection.h"
#include "connection/TCPConnection.h"
#include "state/Context.h"
#include "utils/IOWorker.h"
#include "route/Router.h"
#include "misc/ip.h"

#ifdef __APPLE__
#define TUN_PACKET_HL 4
#elif __linux__
#define TUN_PACKET_HL 0
#endif

class Client : public boost::enable_shared_from_this<Client> {
public:

    ~Client() {
        printf("client die\n");
    }

    void Run() {
        this->context = boost::make_shared<Context>(IOWorker::GetInstance()->GetContextBy(0));
        auto res = context->InitByFile();
        if (!res) {
            return;
        }

        switch (this->context->ConnProtocol()) {
            case ConnProtocolType::UDP: {
                this->connection = boost::make_shared<UDPConnection>(IOWorker::GetInstance()->GetContextBy(0), this->context->ConnKey());
                break;
            }
            case ConnProtocolType::TCP: {
                this->connection = boost::make_shared<TCPConnection>(IOWorker::GetInstance()->GetContextBy(0), this->context->ConnKey());
                break;
            }
            default: {
                printf("unknow ConnProtocolType\n");
                return;
            }
        }
        Router::SetClientDefaultRoute(context.get());
        Reconnect();
    }

    void Reconnect() {
        std::lock_guard<std::mutex> lg(this->connect_mutex);
        if (!this->connection) return;
        if (!this->context) return;
        bool res = false;
        while (!res) {
            res = this->connection->Connect(context->ServerIPResolved(), context->ServerPort());
            if (!res) {
                printf("Connect failed, retrying in 3s\n");
                sleep(3);
            }else {
                printf("Connected to %s:%d\n",context->ServerIPResolved().c_str(), context->ServerPort());
            }
        }

        auto self(this->shared_from_this());
        // recv from tun and send to remote
        context->GetTunDevice()->Spawn([self, connection = this->connection, this](TunDevice* tun, boost::asio::yield_context& yield){
            while(true) {
                boost::system::error_code ec;
                auto bytes_read = tun->Read(boost::asio::buffer(connection->GetTunBuffer() - TUN_PACKET_HL, DEFAULT_TUN_MTU + TUN_PACKET_HL), yield[ec]);
                if (ec) {
                    printf("read err --> %s\n", ec.message().c_str());
                    return;
                }

                if (((iphdr*)connection->GetTunBuffer())->ip_v != 4 && ((iphdr*)connection->GetTunBuffer())->ip_v != 6)
                    continue;

                auto bytes_send = connection->Send(boost::asio::buffer(connection->GetTunBuffer(), bytes_read - TUN_PACKET_HL), yield[ec]);
                if (ec) {
                    printf("send err --> %s\n", ec.message().c_str());
                    if (this->context && this->context->ConnProtocol() == ConnProtocolType::TCP) {
                        Reconnect();
                        continue;
                    }
                    continue;
                }
            }
        });

        //recv from remote and send to tun
        this->connection->Spawn([self, connection = this->connection, context = this->context, this](boost::asio::yield_context yield){
            while(true) {
                boost::system::error_code ec;
                auto bytes_read = connection->Receive(boost::asio::buffer(connection->GetConnBuffer(), DEFAULT_TUN_MTU + ProtocolHeader::ProtocolHeaderSize()), yield[ec]);
                if (ec) {
                    printf("recv err --> %s\n", ec.message().c_str());
                    if (this->context && this->context->ConnProtocol() == ConnProtocolType::TCP) {
                        Reconnect();
                    }
                    return;
                }

                if (bytes_read == 0) {
                    printf("decrypt error\n");
                    continue;
                }
#ifdef __APPLE__
                auto ip_header = (iphdr*)(connection->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize());
                if (ip_header->ip_v == 4)
                    *(uint32_t*)((char*)ip_header - TUN_PACKET_HL) = 33554432;
                else if (ip_header->ip_v == 6)
                    *(uint32_t*)((char*)ip_header - TUN_PACKET_HL) = 503316480;
#endif
                auto bytes_send = context->GetTunDevice()->Write(boost::asio::buffer((void*)(connection->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize() - TUN_PACKET_HL), bytes_read + TUN_PACKET_HL), yield[ec]);
                if (ec) {
                    printf("recv err --> %s\n", ec.message().c_str());
                    continue;
                }
            }
        });
    }

    void Stop() {
        Router::UnsetClientDefaultRoute(context.get());
        this->connection->Close();
        this->context->Stop();
        this->context.reset();
        this->connection.reset();
    }

private:
    boost::shared_ptr<Context> context;
    boost::shared_ptr<IConnection> connection;
    std::mutex connect_mutex;
};