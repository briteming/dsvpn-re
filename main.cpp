#include "state/Context.h"
#include "state/Constant.h"
#include <boost/asio/spawn.hpp>
#include "utils/IOWorker.h"
#include "state/ContextHelper.h"
int main() {

    IOWorker::GetInstance()->AsyncRun();

    auto detail = context_detail {
        .is_server = true,
        .ext_if_name = "auto",
        .gateway_ip = "auto",
        .tun_if_name = DEFAULT_TUN_IFNAME,
        .local_tun_ip = DEFAULT_SERVER_IP,
        .remote_tun_ip = DEFAULT_CLIENT_IP,
        .local_tun_ip6 = "64:ff9b::" + std::string(DEFAULT_SERVER_IP),
        .remote_tun_ip6 = "64:ff9b::" + std::string(DEFAULT_CLIENT_IP),
        .server_ip_or_name = "auto",
        .server_ip_resolved = "auto",
        .server_port = 443,
    };

    auto con = ContextHelper::CreateContext(detail);
    con->Init();
    Router::AddClient(con.get());
    getchar();
    Router::DeleteClient(con.get());
    con.reset();
    getchar();

//    auto context = boost::make_shared<Context>(IOWorker::GetInstance()->GetRandomContext());
//    auto res = context->InitByFile();
//    if (!res) {
//        return -1;
//    }
//
//    context->GetTunDevice()->Spawn([](TunDevice* tun, boost::asio::yield_context yield){
//        char read_buffer[1500];
//        while(true) {
//            boost::system::error_code ec;
//            auto bytes_read = tun->Read(boost::asio::buffer(read_buffer, 1500), yield[ec]);
//            if (ec) {
//                printf("read err --> %s\n", ec.message().c_str());
//                return;
//            }
//            printf("read %zu bytes\n", bytes_read);
//        }
//    });
//
//    Router::SetClientDefaultRoute(context.get());
//
//    getchar();
//    Router::UnsetClientDefaultRoute(context.get());
//    context.reset();
//    getchar();
}