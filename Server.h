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

    Server(const boost::shared_ptr<Context>& context);

    ~Server();

    void Run();

    void Stop();

    boost::shared_ptr<class Context> Context();
    boost::shared_ptr<IConnection> Connection();

private:
    uint8_t io_index = 0;
    uint32_t client_tun_ip_integer;
    sockaddr_in6 client_tun_ip6_integer;
    boost::shared_ptr<class Context> context;
    boost::shared_ptr<IConnection> connection;
};

#include <unordered_map>
#include <mutex>
static std::mutex server_mutex;
static std::unordered_map<uint16_t, boost::shared_ptr<Server>> server_map;
static std::unordered_map<uint32_t, boost::shared_ptr<Server>> server_map_tun_ip;

bool CreateServer(std::string client_tun_ip, uint16_t conn_port, std::string conn_key);

bool DestroyServer(uint16_t conn_port);

#else
class Context;
class Server {
public:
    Server(const boost::shared_ptr<Context>& context){}
    void Run(){}
    void Stop(){}
};

#endif