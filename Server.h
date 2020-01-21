#ifdef __linux__
#pragma once

#include "connection/IConnection.h"
#include "connection/UDPConnection.h"
#include "connection/TCPConnection.h"
#include "state/Context.h"
#include "utils/IOWorker.h"
#include "route/Router.h"
#include "state/ContextHelper.h"
#include "state/Constant.h"
#include <netinet/ip.h>
#include <boost/enable_shared_from_this.hpp>

class Server : public boost::enable_shared_from_this<Server> {
    const std::string vpn_default_if_name = "dsvpn";
public:
    Server(std::string client_tun_ip, uint16_t conn_port, std::string conn_key) {

        context_detail detail = {
                .is_server = true,
                .ext_if_name = "Router::GetDefaultInterfaceName()",
                .gateway_ip = "Router::GetDefaultGatewayIp()",
                .tun_if_name = vpn_default_if_name + std::to_string(conn_port),
                .local_tun_ip = DEFAULT_SERVER_IP,
                .remote_tun_ip = client_tun_ip,
                .local_tun_ip6 = "64:ff9b::" + std::string(DEFAULT_SERVER_IP),
                .remote_tun_ip6 = "64:ff9b::" + std::string(client_tun_ip),
                .server_ip_or_name = "auto",
                .server_ip_resolved = "auto",
                .conn_key = conn_key,
                .server_port = conn_port
        };
        this->io_index = IOWorker::GetInstance()->GetRandomIndex();
        this->context = ContextHelper::CreateContextAtIOIndex(detail, this->io_index);
    }

    Server(context_detail detail) {
        this->context = ContextHelper::CreateContext(detail);
    }

    ~Server() {
        printf("server die\n");
    }

    void Run() {

        auto res = context->Init();
        if (!res) {
            return;
        }
        this->client_tun_ip_integer = inet_addr(this->context->RemoteTunIP().c_str());
        this->connection = boost::make_shared<TCPConnection>(IOWorker::GetInstance()->GetContextBy(this->io_index), this->context->ConnKey());

        res = connection->Bind("0.0.0.0", context->ServerPort());
        if (!res) {
            return;
        }

        this->connection->Accept();
        //recv from client and send to tun
        auto self(this->shared_from_this());
        this->connection->Spawn([self, connection = this->connection, context = this->context](boost::asio::yield_context yield){
            while(true) {
                boost::system::error_code ec;
                auto bytes_read = connection->ReceiveFrom(boost::asio::buffer(connection->GetConnBuffer(), DEFAULT_TUN_MTU + ProtocolHeader::ProtocolHeaderSize()), yield[ec]);
                if (ec) {
                    //printf("recv err --> %s\n", ec.message().c_str());
                    return;
                }
                if (bytes_read == 0) {
                    printf("decrypt error\n");
                    continue;
                }

                auto bytes_send = context->GetTunDevice()->Write(boost::asio::buffer(connection->GetConnBuffer() + ProtocolHeader::ProtocolHeaderSize(), bytes_read), yield[ec]);
                if (ec) {
                    printf("send err --> %s\n", ec.message().c_str());
                    return;
                }
            }
        });


        // recv from tun and send to remote
        context->GetTunDevice()->Spawn([self, this, connection = this->connection](TunDevice* tun, boost::asio::yield_context& yield){
            while(true) {
                boost::system::error_code ec;
                auto bytes_read = tun->Read(boost::asio::buffer(connection->GetTunBuffer(), DEFAULT_TUN_MTU), yield[ec]);
                if (ec) {
                    //printf("read err --> %s\n", ec.message().c_str());
                    return;
                }
                auto ip_hdr = (iphdr*)connection->GetTunBuffer();
                if (ip_hdr->version != 4) {
                    continue;
                }

                if (ip_hdr->daddr != this->client_tun_ip_integer) {
                    continue;
                }

                auto bytes_send = connection->SendTo(boost::asio::buffer(connection->GetTunBuffer(), bytes_read), yield[ec]);
                if (ec.value() != boost::system::errc::bad_file_descriptor) {
                    continue;
                }else {
                    //printf("send conn err --> %s\n", ec.message().c_str());
                    return;
                }
            }
        });

        Router::AddClient(context.get());
        printf("dsvpn add client, tun_ip: %s, listened_port: %d\n",this->context->RemoteTunIP().c_str(), this->context->ServerPort());

    }

    void Stop() {
        Router::DeleteClient(context.get());
        this->connection->Close();
        this->context->Stop();
        this->connection.reset();
        this->context.reset();
    }

private:
    uint8_t io_index = 0;
    uint32_t client_tun_ip_integer;
    boost::shared_ptr<Context> context;
    boost::shared_ptr<IConnection> connection;
};

#include <unordered_map>
std::mutex server_mutex;
static std::unordered_map<uint16_t, boost::shared_ptr<Server>> server_map;

bool CreateServer(std::string client_tun_ip, uint16_t conn_port, std::string conn_key) {
    std::lock_guard<std::mutex> lg(server_mutex);
    if (server_map.find(conn_port) != server_map.end()) {
        printf("server already exist\n");
        return false;
    }
    auto server = boost::make_shared<Server>(client_tun_ip, conn_port, conn_key);
    server_map.insert({conn_port, server});
    server->Run();
    return true;
}

bool DestroyServer(uint16_t conn_port) {
    std::lock_guard<std::mutex> lg(server_mutex);
    auto it = server_map.find(conn_port);
    if ( it == server_map.end()) {
        printf("server not exist\n");
        return false;
    }
    it->second->Stop();
    server_map.erase(it);
    return true;
}
#endif