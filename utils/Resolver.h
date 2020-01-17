#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>

bool resolve_ip(std::string ip_or_name, std::string& ip_out)
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::asio::ip::tcp::resolver::query query(ip_or_name, "80");
    boost::system::error_code ec;
    boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query, ec);
    if (ec) {
        return false;
    }
    auto ep = iter->endpoint();
    if (ep.address().is_v4() || ep.address().is_v6()) {
        ip_out = ep.address().to_string();
        return true;
    }
    return false;
}