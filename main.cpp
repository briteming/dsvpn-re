#include "utils/IOWorker.h"
#include "Client.h"
#include "Server.h"
#include <sodium.h>

int main() {

    sodium_init();
    IOWorker::GetInstance()->AsyncRun();
    {
        auto client = boost::make_shared<Client>();
        client->Run();
        getchar();
        client->Stop();
        getchar();
    }
//    {
//        auto server = boost::make_shared<Server>(DEFAULT_CLIENT_IP, 1800);
//        server->Run();
//        getchar();
//        server->Stop();
//        getchar();
//    }
//
//    printf("outsize\n");
//    getchar();

//    Client client;
//    client.Run();
//    getchar();
//    client.Stop();
//    getchar();
//    getchar();
//    conn->Spawn([](Connection* connection, boost::asio::yield_context yield){
//        char read_buffer[1500];
//        boost::system::error_code ec;
//        auto bytes_read = connection->Send(boost::asio::buffer(read_buffer, 1500), yield[ec]);
//        if (ec) {
//            printf("send err --> %s\n", ec.message().c_str());
//            return;
//        }
//        printf("send %zu bytes\n", bytes_read);
//    });
//
//    conn->Spawn([](Connection* connection, boost::asio::yield_context yield){
//        char read_buffer[1500];
//        boost::system::error_code ec;
//        auto bytes_read = connection->Receive(boost::asio::buffer(read_buffer, 1500), yield[ec]);
//        if (ec) {
//            printf("recv err --> %s\n", ec.message().c_str());
//            return;
//        }
//        printf("recv %zu bytes\n", bytes_read);
//    });
//

}