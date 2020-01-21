#pragma once

#include "connection/Connection.h"
#include "state/Context.h"
#include "utils/IOWorker.h"
#include "route/Router.h"

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

        this->connection = boost::make_shared<Connection>(IOWorker::GetInstance()->GetContextBy(0));
        res = connection->Connect(context->ServerIPResolved(), context->ServerPort());
        if (!res) {
            return;
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
                auto bytes_send = connection->Send(boost::asio::buffer(connection->GetTunBuffer(), bytes_read - 4), yield[ec]);
                if (ec.value() == boost::system::errc::operation_canceled || ec.value() == boost::system::errc::bad_file_descriptor) {
                    printf("send err --> %s\n", ec.message().c_str());
                    return;
                }else if (ec) {
                    printf("send err --> %s\n", ec.message().c_str());
                    continue;
                }
            }
        });

        //recv from remote and send to tun
        this->connection->Spawn([self, connection = this->connection, context = this->context, this](Connection* conn, boost::asio::yield_context yield){
            while(true) {
                boost::system::error_code ec;
                auto bytes_read = conn->Receive(boost::asio::buffer(connection->GetConnBuffer(), DEFAULT_TUN_MTU + ProtocolHeader::ProtocolHeaderSize()), yield[ec]);
                if (ec.value() == boost::system::errc::operation_canceled || ec.value() == boost::system::errc::bad_file_descriptor) {
                    printf("recv err --> %s\n", ec.message().c_str());
                    return;
                }else if (ec) {
                    printf("recv err --> %s\n", ec.message().c_str());
                    continue;
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
        Router::SetClientDefaultRoute(context.get());

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
    boost::shared_ptr<Connection> connection;
};