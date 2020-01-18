#include "state/Context.h"
#include <boost/asio/spawn.hpp>
#include "utils/IOWorker.h"

int main() {
    auto context = std::make_unique<Context>(IOWorker::GetInstance()->GetRandomContext());
    auto res = context->Init();
    if (!res) {
        return -1;
    }
    auto& tun = context->TunDevice();
    tun.Spawn([&tun](boost::asio::yield_context yield){
        char read_buffer[1500];
        while(true) {
            boost::system::error_code ec;
            auto bytes_read = tun.Read(boost::asio::buffer(read_buffer, 1500), yield[ec]);
            printf("read %zu bytes\n", bytes_read);
        }
    });

    Router::SetClientDefaultRoute(context.get());

    IOWorker::GetInstance()->AsyncRun();

    getchar();
    Router::UnsetClientDefaultRoute(context.get());
    IOWorker::GetInstance()->Stop();
    getchar();
}