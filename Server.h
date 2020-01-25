#ifdef __linux__
#pragma once

#include "connection/IConnection.h"
#include "connection/UDPConnection.h"
#include "connection/TCPConnection.h"
#include "state/Context.h"
#include <boost/enable_shared_from_this.hpp>

class Server : public boost::enable_shared_from_this<Server> {

    const std::string vpn_default_if_name = "dsvpn";

public:
    Server(std::string client_tun_ip, uint16_t conn_port, std::string conn_key);

    Server(context_detail detail);

    ~Server();

    void Run();

    void Stop();

private:
    uint8_t io_index = 0;
    uint32_t client_tun_ip_integer;
    sockaddr_in6 client_tun_ip6_integer;
    boost::shared_ptr<Context> context;
    boost::shared_ptr<IConnection> connection;
};

#include <unordered_map>
#include <mutex>
static std::mutex server_mutex;
static std::unordered_map<uint16_t, boost::shared_ptr<Server>> server_map;

bool CreateServer(std::string client_tun_ip, uint16_t conn_port, std::string conn_key);

bool DestroyServer(uint16_t conn_port);

#endif