#pragma once

#include "connection/IConnection.h"
#include "connection/UDPConnection.h"
#include "connection/TCPConnection.h"
#include "state/Context.h"
#include "utils/IOWorker.h"
#include "utils/NetworkCheck.h"
#include "route/Router.h"
#include "misc/ip.h"

#ifdef __APPLE__
#define TUN_PACKET_HL 4
#elif __linux__
#define TUN_PACKET_HL 0
#endif

class Client : public boost::enable_shared_from_this<Client> {
public:

    ~Client();

    void Run();

    void Reconnect();

    void Stop();

private:
    boost::shared_ptr<Context> context;
    boost::shared_ptr<IConnection> connection;
    std::mutex connect_mutex;
    std::atomic_bool stopped = false;
    void makeConnection() {
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
    }
};