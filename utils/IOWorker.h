#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <vector>

class IOWorker {
public:

private:
    std::vector<boost::asio::io_context> io_contexts;
    class boost::asio::executor_work_guard<boost::asio::io_context::executor_type> worker;
};


