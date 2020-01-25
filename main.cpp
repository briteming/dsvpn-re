#include "utils/IOWorker.h"
#include "Client.h"
#include "Server.h"
#include <sodium.h>
#include <csignal>
#include <spdlog/spdlog.h>

boost::shared_ptr<Client> client = nullptr;

void signal_handler(int signal)
{
    if (client)
    {
        client->Stop();
        client.reset();
    }

    IOWorker::GetInstance()->Stop();
    SPDLOG_INFO("Ctrl C Signal, Stopping");
}

int main() {
    spdlog::set_pattern("[%Y-%m-%d %T] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::trace);
    std::signal(SIGINT, signal_handler);

    auto res = sodium_init();
    if (res != 0) {
        SPDLOG_ERROR("sodium_init failed");
        return -1;
    }

    SPDLOG_INFO("DSVPN-REIMPL is an OpenSource project [https://code.dllexport.com/mario/dsvpn-reimpl]");
    SPDLOG_INFO("DSVPN is the original project [https://github.com/jedisct1/dsvpn]");

//    CreateServer(DEFAULT_CLIENT_IP, 1800, "12345678");
//    getchar();
//    DestroyServer(1800);
//    getchar();

    client = boost::make_shared<Client>();
    client->Run();
    IOWorker::GetInstance()->Run();
}