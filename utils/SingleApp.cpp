//
// Created by System Administrator on 2020-01-25.
//

#include "SingleApp.h"

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_context.hpp>
#include <spdlog/spdlog.h>

void SingleApp::Check() {
        static boost::asio::io_context io_context;
        static boost::asio::ip::udp::socket singleapp_socket(io_context);
        singleapp_socket.open(boost::asio::ip::udp::v4());
        auto ep = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("0.0.0.0"), 65500);
        boost::system::error_code ec;
        singleapp_socket.bind(ep, ec);
        if (ec) {
            SPDLOG_INFO("Another DSVPN is running, please check UDP Port: 65500");
            exit(-1);
        }
        return;
}