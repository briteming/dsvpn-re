#include "Client.h"
#include <spdlog/spdlog.h>
Client::Client(const boost::shared_ptr<Context>& context) {
    this->context = context;
    SPDLOG_INFO("Platform: {}", BOOST_PLATFORM);
    SPDLOG_INFO("Connection Mode [{}]", ConnProtocolTypeToString(this->context->ConnProtocol()));
    SPDLOG_INFO("Local TunIP: [{}] Remote TunIP: [{}]", context->LocalTunIP(), context->RemoteTunIP());
    if (this->context->IPv6()) {
        SPDLOG_INFO("IPv6 enabled");
        SPDLOG_INFO("Local TunIPv6: [{}] Remote TunIPv6: [{}]", context->LocalTunIP6(), context->RemoteTunIP6());
    }

    SPDLOG_INFO("Due to dns poisoning, you may need to setup pure dns(IPv4/v6) manually");

}

Client::~Client() {
    SPDLOG_DEBUG("Client die");
}

void Client::Run() {
    this->client_tun_ip_integer = inet_addr(this->context->LocalTunIP().c_str());
    Reconnect();
}

// only reconnect when connection->Send return error
void Client::Reconnect() {
    auto self(this->shared_from_this());
    std::unique_lock<std::mutex> lg(this->connect_mutex, std::try_to_lock);
    if(!lg.owns_lock()){
        return;
    }
    if (!this->context) {
        SPDLOG_ERROR("Client context is nullptr");
        return;
    }

    Router::UnsetClientDefaultRoute(context.get());

    // if we have conn exist, close it
    if (this->connection)
    {
        this->connection->Close();
        this->connection.reset();
    }

    // if network is not available which means the cable or wifi is disconnect
    // recheck every 2 sec
    while(!NetworkCheck::NetworkAvaliable()) {
        if (this->stopped) return;
        SPDLOG_INFO("Network is not available");
        sleep(2);
    }

    SPDLOG_INFO("Connecting to {}:{}",context->ServerIPResolved(), context->ServerPort());
    bool res = false;
    // make a new conn
    makeConnection();
    while (!res) {
        res = this->connection->Connect(context->ServerIPResolved(), context->ServerPort());
        if (!res) {
            SPDLOG_INFO("Connect failed, retrying");
            if (this->stopped) return;
            sleep(3);
        }else {
            SPDLOG_INFO("Connected");
        }
    }

    // recv from tun and send to remote
    context->GetTunDevice()->Spawn([self, connection = this->connection, this](TunDevice* tun, boost::asio::yield_context& yield){
        while(true) {
            boost::system::error_code ec;
            auto bytes_read = tun->Read(boost::asio::buffer(connection->GetTunBuffer() - TUN_PACKET_HL, this->context->MTU() + TUN_PACKET_HL), yield[ec]);
            if (ec) {
                SPDLOG_DEBUG("tun read err --> {}", ec.message());
                return;
            }

            auto ip_hdr = (iphdr*)connection->GetTunBuffer();

            // don't tunnel none ip packet
            if (ip_hdr->ip_v != 4 && ip_hdr->ip_v != 6)
                continue;

            if (ip_hdr->ip_v == 4) {
                if (ip_hdr->ip_src.s_addr != this->client_tun_ip_integer) {
                    continue;
                }
            }

            // might get err if we recv icmp dst unrechable
            auto bytes_send = connection->Send(boost::asio::buffer(connection->GetTunBuffer(), bytes_read - TUN_PACKET_HL), yield[ec]);
            if (ec) {
                SPDLOG_DEBUG("connection send err --> {}", ec.message());
                Reconnect();
                return;
            }
        }
    });

    //recv from remote and send to tun
    this->connection->Spawn([self, connection = this->connection, context = this->context, this](boost::asio::yield_context yield){
        while(true) {
            boost::system::error_code ec;
            auto bytes_read = connection->Receive(boost::asio::buffer(connection->GetConnBuffer(), this->context->MTU() + ProtocolHeader::Size()), yield[ec]);
            if (ec) {
                SPDLOG_DEBUG("connection recv err --> {}", ec.message());
//                if (this->context) {
//                    Reconnect();
//                }
                return;
            }

            if (bytes_read == 0) {
                // this should never happen
                // the the conn_key is wrong, the server won't send anything back
                SPDLOG_DEBUG("decrypt error");
                continue;
            }
            if (bytes_read == -1) {
                SPDLOG_DEBUG("recv packet from none original server");
                continue;
            }
#ifdef __APPLE__
            auto ip_header = (iphdr*)(connection->GetConnBuffer() + ProtocolHeader::Size());
            if (ip_header->ip_v == 4)
                *(uint32_t*)((char*)ip_header - TUN_PACKET_HL) = 33554432;
            else if (ip_header->ip_v == 6)
                *(uint32_t*)((char*)ip_header - TUN_PACKET_HL) = 503316480;
#endif
            auto bytes_send = context->GetTunDevice()->Write(boost::asio::buffer((void*)(connection->GetConnBuffer() + ProtocolHeader::Size() - TUN_PACKET_HL), bytes_read + TUN_PACKET_HL), yield[ec]);
            if (ec) {
                SPDLOG_DEBUG("tun write err --> {}", ec.message());
                return;
            }
        }
    });

    SPDLOG_INFO("Connection is Ready");
    sleep(1);
    // finally we change the route
    Router::SetClientDefaultRoute(context.get());

}

void Client::Stop() {
    Router::UnsetClientDefaultRoute(context.get());
    this->stopped = true;
    if (this->connection) {
        this->connection->Close();
        this->context->Stop();
    }
    this->connection.reset();
}