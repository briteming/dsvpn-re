#ifdef __linux__

#include "Server.h"
#include "utils/IOWorker.h"
#include "route/Router.h"
#include "state/ContextHelper.h"
#include "state/Constant.h"
#include "misc/ip.h"
#include <linux/ipv6.h>
#include <spdlog/spdlog.h>
#include "utils/Shell.h"
#include <arpa/inet.h>

static inline int ipv6_addr_compare(const struct in6_addr *a1, const struct in6_addr *a2)
{
    return memcmp(a1, a2, sizeof(struct in6_addr));
}

Server::Server(std::string client_tun_ip, uint16_t conn_port, std::string conn_key) {

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
            .server_port = conn_port,
            .conn_protocol = ConnProtocolType::UDP
    };
    this->io_index = IOWorker::GetInstance()->GetRandomIndex();
    this->context = ContextHelper::CreateContextAtIOIndex(detail, this->io_index);
}

Server::Server(context_detail detail) {
    this->context = ContextHelper::CreateContext(detail);
}

Server::~Server() {
    SPDLOG_DEBUG("server die");
}

void Server::Run() {

    auto res = context->Init();
    if (!res) {
        return;
    }
    Shell shell;
    shell.Run("sysctl net.ipv4.ip_forward=1");
    shell.Run("sysctl net.ipv6.conf.all.forwarding=1");
    this->client_tun_ip_integer = inet_addr(this->context->RemoteTunIP().c_str());
    inet_pton(AF_INET6, this->context->RemoteTunIP6().c_str(), &this->client_tun_ip6_integer.sin6_addr);

    switch (this->context->ConnProtocol()) {
        case ConnProtocolType::UDP: {
            this->connection = boost::make_shared<UDPConnection>(IOWorker::GetInstance()->GetContextBy(this->io_index), this->context->ConnKey());
            break;
        }
        case ConnProtocolType::TCP: {
            this->connection = boost::make_shared<TCPConnection>(IOWorker::GetInstance()->GetContextBy(this->io_index), this->context->ConnKey());
            break;
        }
        default: {
            SPDLOG_ERROR("unknow ConnProtocolType");
            return;
        }
    }
    res = connection->Bind("0.0.0.0", context->ServerPort());
    if (!res) {
        return;
    }

    this->connection->Accept();
    //recv from client and send to tun
    auto self(this->shared_from_this());
    this->connection->Spawn([self, this, connection = this->connection, context = this->context](boost::asio::yield_context yield){
        while(true) {
            boost::system::error_code ec;
            auto bytes_read = connection->ReceiveFrom(boost::asio::buffer(connection->GetConnBuffer(), DEFAULT_TUN_MTU + ProtocolHeader::Size()), yield[ec]);
            if (ec) {
                //printf("recv err --> {}\n", ec.message().c_str());
                return;
            }
            if (bytes_read == 0) {
                SPDLOG_INFO("decrypt error");
                continue;
            }

            auto ip_hdr = (iphdr*)(connection->GetConnBuffer() + ProtocolHeader::Size());
            if (ip_hdr->ip_v != 4 && ip_hdr->ip_v != 6) {
                SPDLOG_DEBUG("[{}] recv none ip packet", this->context->ServerPort());
                continue;
            }

            if (ip_hdr->ip_v == 4) {
                if (ip_hdr->ip_src.s_addr != this->client_tun_ip_integer) {
                    SPDLOG_DEBUG("[{}] Local Tun IPv4 mismatch, config {}, get {}", this->context->ServerPort(), this->context->RemoteTunIP(), std::string(inet_ntoa(ip_hdr->ip_src)));
                    continue;
                }
            }

            if (ip_hdr->ip_v == 6) {
                auto ipv6_hdr = (ipv6hdr*)ip_hdr;
                if (ipv6_addr_compare(&this->client_tun_ip6_integer.sin6_addr, &ipv6_hdr->daddr) != 0) {
                    SPDLOG_DEBUG("[{}] Local Tun IPv6 mismatch");
                    continue;
                }
            }

            // if subnet
            if (0)
            {
                std::lock_guard<std::mutex> lg(server_mutex);
                auto it = server_map_tun_ip.find(ip_hdr->ip_src.s_addr);
                if (it == server_map_tun_ip.end()) continue;
                const auto& other_host = it->second->Connection();

            }
            //
            auto bytes_send = context->GetTunDevice()->Write(boost::asio::buffer(connection->GetConnBuffer() + ProtocolHeader::Size(), bytes_read), yield[ec]);
            if (ec) {
                SPDLOG_DEBUG("send err --> {}", ec.message().c_str());
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
                //printf("read err --> {}\n", ec.message().c_str());
                return;
            }
            auto ip_hdr = (iphdr*)connection->GetTunBuffer();
            if (ip_hdr->ip_v != 4 && ip_hdr->ip_v != 6) {
                continue;
            }

            if (ip_hdr->ip_v == 4) {
                if (ip_hdr->ip_dst.s_addr != this->client_tun_ip_integer) {
                    continue;
                }
            }

            if (ip_hdr->ip_v == 6) {
                auto ipv6_hdr = (ipv6hdr*)ip_hdr;
                if (ipv6_addr_compare(&this->client_tun_ip6_integer.sin6_addr, &ipv6_hdr->daddr) != 0) {
                    continue;
                }
            }

            auto bytes_send = connection->SendTo(boost::asio::buffer(connection->GetTunBuffer(), bytes_read), yield[ec]);
            if (ec.value() != boost::system::errc::bad_file_descriptor) {
                continue;
            }else {
                //printf("send conn err --> {}", ec.message().c_str());
                return;
            }
        }
    });

    Router::AddClient(context.get());
    SPDLOG_INFO("DSVPN add client, TUN_IP: {}, ListenPort: {}, Protocol: {}",this->context->RemoteTunIP(), this->context->ServerPort(), ConnProtocolTypeToString(this->context->ConnProtocol()));

}

void Server::Stop() {
    Router::DeleteClient(context.get());
    this->connection->Close();
    this->context->Stop();
    this->connection.reset();
    this->context.reset();
}

boost::shared_ptr<Context> Server::Context() {
    return this->context;
}

boost::shared_ptr<IConnection> Server::Connection() {
    return this->connection;
}

bool CreateServer(std::string client_tun_ip, uint16_t conn_port, std::string conn_key) {
    std::lock_guard<std::mutex> lg(server_mutex);
    if (server_map.find(conn_port) != server_map.end()) {
        SPDLOG_INFO("server already exist");
        return false;
    }
    auto server = boost::make_shared<Server>(client_tun_ip.c_str(), conn_port, conn_key);
    server_map.insert({conn_port, server});
    server_map_tun_ip.insert({inet_addr(client_tun_ip.c_str()), server});
    server->Run();
    return true;
}

bool DestroyServer(uint16_t conn_port) {
    std::lock_guard<std::mutex> lg(server_mutex);
    auto it = server_map.find(conn_port);
    if ( it == server_map.end()) {
        SPDLOG_INFO("server not found in server_map");
        return false;
    }
    it->second->Stop();

    auto it2 = server_map_tun_ip.find(inet_addr(it->second->Context()->RemoteTunIP().c_str()));
    if ( it == server_map.end()) {
        SPDLOG_INFO("server not found in server_map_tun_ip");
        return false;
    }
    server_map_tun_ip.erase(it2);
    server_map.erase(it);
    return true;
}

#endif
