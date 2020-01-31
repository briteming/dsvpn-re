#include "utils/IOWorker.h"
#include "Client.h"
#include "Server.h"
#include <sodium.h>
#include <csignal>
#include <spdlog/spdlog.h>
#include "state/Constant.h"
#include "utils/SingleApp.h"

boost::shared_ptr<Client> client = nullptr;

void signal_handler(int signal)
{
    if (client)
    {
        client->Stop();
        client.reset();
    }

//    DestroyServer(1800);
//    DestroyServer(1900);

    IOWorker::GetInstance()->Stop();
    SPDLOG_INFO("Stopping");
}

int main() {
    spdlog::set_pattern("[%Y-%m-%d %T] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
    SPDLOG_INFO("DSVPN-REIMPL[{}] is an OpenSource project [https://code.dllexport.com/mario/dsvpn-reimpl]", DSVPN_VERSION);
    SPDLOG_INFO("DSVPN is the original project [https://github.com/jedisct1/dsvpn]");

    if (getuid()) {
        SPDLOG_INFO("root is required, retry with sudo");
        return -1;
    }

    SingleApp::Check();

    std::signal(SIGINT, signal_handler);
//    std::signal(SIGSTOP, signal_handler);

    auto res = sodium_init();
    if (res != 0) {
        SPDLOG_ERROR("sodium_init failed");
        return -1;
    }

//    CreateServer(DEFAULT_CLIENT_IP, 1800, "12345678");
//    CreateServer("192.168.192.120", 1900, "12345678");

    client = boost::make_shared<Client>();
    client->Run();
    IOWorker::GetInstance()->Run();
}