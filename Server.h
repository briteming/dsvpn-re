#ifdef __linux__
#pragma once

#include "connection/Connection.h"
#include "state/Context.h"
#include "utils/IOWorker.h"
#include "route/Router.h"
#include "state/ContextHelper.h"
#include "state/Constant.h"
#include <netinet/ip.h>
#include <boost/enable_shared_from_this.hpp>

static int vpn_index = 0;

class Server : public boost::enable_shared_from_this<Server> {
    const std::string vpn_default_if_name = "dsvpn";
public:
    Server(std::string client_tun_ip, uint16_t conn_port, std::string conn_key) {
        this->client_tun_ip = client_tun_ip;
        this->client_tun_ip_integer = inet_addr(client_tun_ip.c_str());
        this->conn_port = conn_port;
        this->connection = boost::make_shared<Connection>(IOWorker::GetInstance()->GetRandomContext(), conn_key);
    }

    ~Server() {
        printf("server die\n");
    }

    void Run() {
        context_detail detail = {
            .is_server = true,
            .ext_if_name = "Router::GetDefaultInterfaceName()",
            .gatewReceiveFromay_ip = "Router::GetDefaultGatewayIp()",
            .tun_if_name = vpn_default_if_name + std::to_string(vpn_index++),
            .local_tun_ip = DEFAULT_SERVER_IP,
            .remote_tun_ip = this->client_tun_ip,
            .local_tun_ip6 = "64:ff9b::" + std::string(DEFAULT_SERVER_IP),
            .remote_tun_ip6 = "64:ff9b::" + std::string(this->client_tun_ip),
            .server_ip_or_name = "auto",
            .server_ip_resolved = "auto",
            .conn_key = "12345678",
            .server_port = this->conn_port
        };

        this->context = ContextHelper::CreateContext(detail);
        auto res = context->Init();
        if (!res) {
            return;
        }

        res = connection->Bind("0.0.0.0", context->ServerPort());
        if (!res) {
            return;
        }
        //recv from client and send to tun
        auto self(this->shared_from_this());
        this->connection->Spawn([self, connection = this->connection, context = this->context](Connection* conn, boost::asio::yield_context yield){
            while(true) {
                boost::system::error_code ec;
                boost::asio::ip::udp::endpoint recv_ep;
                auto bytes_read = conn->ReceiveFrom(boost::asio::buffer(connection->GetConnBuffer(), DEFAULT_TUN_MTU + ProtocolHeader::ProtocolHeaderSize()), recv_ep, yield[ec]);
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
        printf("dsvpn add client, tun_ip: %s, listened_port: %d\n",this->client_tun_ip.c_str(), this->conn_port);

    }

    void Stop() {
        Router::DeleteClient(context.get());
        this->connection->Close();
        this->context->Stop();
        this->connection.reset();
        this->context.reset();
    }

private:
    std::string client_tun_ip;
    uint32_t client_tun_ip_integer;
    uint16_t conn_port;
    boost::shared_ptr<Context> context;
    boost::shared_ptr<Connection> connection;
};

#include <unordered_map>
//std::unordered_map<client_tun_ip_integer, boost::shared_ptr<Server>> server_map;

void CreateServer(std::string client_tun_ip, uint16_t conn_port) {

}
#endif