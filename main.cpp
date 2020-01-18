#include "state/Context.h"
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>

int main() {

    auto context = std::make_unique<Context>();
    auto res = context->Init();
    auto& tun = context->TunDevice();
    tun.Spawn([&tun](boost::asio::yield_context yield){
        char read_buffer[1500];
        while(true) {
            boost::system::error_code ec;
            auto bytes_read = tun.Read(boost::asio::buffer(read_buffer, 1500), yield[ec]);
            printf("read %zu bytes\n", bytes_read);
        }
    });
    boost::thread t([&context](){
        context->Run();
    });
    t.detach();

    Router::SetClientDefaultRoute(context.get());
    getchar();
    Router::UnsetClientDefaultRoute(context.get());
    getchar();
}