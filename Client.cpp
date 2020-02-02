#include "Client.h"
#include <spdlog/spdlog.h>

Client::~Client() {
    SPDLOG_DEBUG("Client die");
}

void Client::Run() {
    SPDLOG_INFO("Platform: {}", BOOST_PLATFORM);
    this->context = boost::make_shared<Context>(IOWorker::GetInstance()->GetContextBy(0));
    auto res = context->InitByFile();
    if (!res) {
        return;
    }
    SPDLOG_INFO("Connection Mode {}", ConnProtocolTypeToString(this->context->ConnProtocol()));
    if (this->context->IPv6())
        SPDLOG_INFO("IPv6 enabled");
    SPDLOG_INFO("Due to dns poisoning, you may need to setup pure dns(IPv4/v6) manually");
    Reconnect();
}


void Client::Reconnect() {
    auto self(this->shared_from_this());
    std::lock_guard<std::mutex> lg(this->connect_mutex);
    if (!this->context) {
        SPDLOG_ERROR("Client context is nullptr");
        return;
    }
    sleep(2);
    Router::UnsetClientDefaultRoute(context.get());

    if (this->connection)
    {
        this->connection->Close();
        this->connection.reset();
    }

    while(!NetworkCheck::NetworkAvaliable()) {
        if (this->stopped) return;
        SPDLOG_INFO("Network is not available");
        sleep(2);
    }

    SPDLOG_INFO("Connecting to {}:{}",context->ServerIPResolved().c_str(), context->ServerPort());
    bool res = false;
    Router::SetClientDefaultRoute(context.get());
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
            auto bytes_read = tun->Read(boost::asio::buffer(connection->GetTunBuffer() - TUN_PACKET_HL, DEFAULT_TUN_MTU + TUN_PACKET_HL), yield[ec]);
            if (ec) {
                SPDLOG_DEBUG("tun read err --> {}", ec.message());
                return;
            }

            // don't tunnel none ip packet
            if (((iphdr*)connection->GetTunBuffer())->ip_v != 4 && ((iphdr*)connection->GetTunBuffer())->ip_v != 6)
                continue;

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
            auto bytes_read = connection->Receive(boost::asio::buffer(connection->GetConnBuffer(), DEFAULT_TUN_MTU + ProtocolHeader::Size()), yield[ec]);
            if (ec) {
                SPDLOG_DEBUG("connection recv err --> {}", ec.message());
//                    if (this->context && this->context->ConnProtocol() == ConnProtocolType::TCP) {
//                        Reconnect();
//                    }
                return;
            }

            if (bytes_read == 0) {
                // this should never happen
                // the the conn_key is wrong, the server won't send anything back
                SPDLOG_DEBUG("decrypt error");
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