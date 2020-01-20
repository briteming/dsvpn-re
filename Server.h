#pragma once

#include "connection/Connection.h"
#include "state/Context.h"
#include "utils/IOWorker.h"
#include "route/Router.h"

class Server {
public:
    void Run() {
        this->context = boost::make_shared<Context>(IOWorker::GetInstance()->GetRandomContext());
        auto res = context->InitByFile();
        if (!res) {
            return;
        }

        this->connection = boost::make_shared<Connection>(IOWorker::GetInstance()->GetRandomContext());
        res = connection->Connect(context->ServerIPResolved(), context->ServerPort());
        if (!res) {
            return;
        }

        // recv from tun and send to remote
        context->GetTunDevice()->Spawn([this](TunDevice* tun, boost::asio::yield_context& yield){
            char read_buffer[1500];
            while(true) {
                boost::system::error_code ec;
                auto bytes_read = tun->Read(boost::asio::buffer(read_buffer, 1500), yield[ec]);
//                for (int i = 0; i < bytes_read; i++) {
//                    printf("%x ", (unsigned char)read_buffer[i]);
//                }
//                printf("\n");
                if (ec) {
                    printf("read err --> %s\n", ec.message().c_str());
                    return;
                }
                printf("read %zu bytes\n", bytes_read);
                auto bytes_send = this->connection->Send(boost::asio::buffer(read_buffer + 4, bytes_read - 4), yield[ec]);
                if (ec) {
                    printf("send err --> %s\n", ec.message().c_str());
                    return;
                }
                printf("send %zu bytes\n", bytes_send);
            }
        });

        //recv from client and send to tun
        this->connection->Spawn([this](Connection* conn, boost::asio::yield_context yield){
            char read_buffer[1500];
            boost::system::error_code ec;
            auto bytes_read = conn->Receive(boost::asio::buffer(read_buffer, 1500), yield[ec]);
            if (ec) {
                printf("recv err --> %s\n", ec.message().c_str());
                return;
            }
            printf("recv %zu bytes\n", bytes_read);
            auto bytes_send = this->context->GetTunDevice()->Write(boost::asio::buffer(read_buffer, bytes_read), yield[ec]);
            if (ec) {
                printf("send err --> %s\n", ec.message().c_str());
                return;
            }
            printf("send %zu bytes\n", bytes_send);
        });
        Router::SetClientDefaultRoute(context.get());

    }

    void Stop() {
        Router::UnsetClientDefaultRoute(context.get());
        context.reset();
    }

private:
    boost::shared_ptr<Context> context;
    boost::shared_ptr<Connection> connection;
};