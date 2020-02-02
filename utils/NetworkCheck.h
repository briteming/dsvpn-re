#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_context.hpp>

class NetworkCheck {
public:
    static bool NetworkAvaliable() {
        char buf[1];
        boost::asio::io_context io_context;
        boost::asio::ip::udp::socket test_socket(io_context);
        test_socket.open(boost::asio::ip::udp::v4());
        auto ep = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("8.8.8.8"), 53);
        boost::system::error_code ec;
        test_socket.send_to(boost::asio::buffer(buf, 1), ep, 0, ec);
        if (ec) {
            return false;
        }
        return true;
    }
};