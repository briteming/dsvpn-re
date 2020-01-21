#pragma once

#include "connection/IConnection.h"
#include "connection/UDPConnection.h"
#include "connection/TCPConnection.h"
#include "state/Context.h"
#include "utils/IOWorker.h"
#include "route/Router.h"
#include "misc/ip.h"

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
        Reconnect();
        Router::SetClientDefaultRoute(context.get());

    }

    void Reconnect() {
        if (reconnecting) return;
        if (!this->connection) return;
        if (!this->context) return;
        reconnecting = true;
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
                auto bytes_read = tun->Read(boost::asio::buffer(connection->GetTunBuffer() - 4, DEFAULT_TUN_MTU + 4), yield[ec]);
                if (ec) {
                    printf("read err --> %s\n", ec.message().c_str());
                    return;
                }

                if (((iphdr*)connection->GetTunBuffer())->ip_v != 4)
                    continue;

                auto bytes_send = connection->Send(boost::asio::buffer(connection->GetTunBuffer(), bytes_read - 4), yield[ec]);
                if (ec) {
                    printf("send err --> %s\n", ec.message().c_str());
                    Reconnect();
                    return;
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
                    Reconnect();
                    return;
                }

                if (bytes_read == 0) {
                    printf("decrypt error\n");
                    continue;
                }
                *(uint32_t*)(connection->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize() - 4) = 33554432;
                auto bytes_send = context->GetTunDevice()->Write(boost::asio::buffer((void*)(connection->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize() - 4), bytes_read + 4), yield[ec]);
                if (ec) {
                    printf("recv err --> %s\n", ec.message().c_str());
                    return;
                }
            }
        });
        reconnecting = false;
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
    std::atomic_bool reconnecting = false;
};