#pragma once

#include "../utils/Singleton.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <vector>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

class IOWorker : public Singleton<IOWorker> {
public:
    IOWorker() : io_contexts(boost::thread::hardware_concurrency()) {
        for(int i = 0; i < boost::thread::hardware_concurrency(); i++) {
            workers.push_back(boost::asio::make_work_guard(io_contexts[i]));
        }
    }

    uint8_t GetRandomIndex() {
        return randomNum(0, boost::thread::hardware_concurrency() - 1);
    }

    boost::asio::io_context& GetRandomContext() {
        return io_contexts[randomNum(0, boost::thread::hardware_concurrency() - 1)];
    }

    boost::asio::io_context& GetContextBy(size_t index) {
        if (index < 0 || index > boost::thread::hardware_concurrency() - 1) {
            return GetRandomContext();
        }
        return io_contexts[index];
    }

    void Run() {
        AsyncRun();
        this->thread_group.join_all();
    }

    void AsyncRun() {
        boost::lock_guard(this->mutex);
        if (thread_group.size() != 0) return;
        for(int i = 0; i < boost::thread::hardware_concurrency(); i++) {
            thread_group.create_thread([io = &io_contexts[i]](){
                io->run();
            });
        }
    }

    void Stop() {
        for(int i = 0; i < boost::thread::hardware_concurrency(); i++) {
            workers[i].reset();
            io_contexts[i].stop();
        }
    }

private:
    std::vector<boost::asio::io_context> io_contexts;
    std::vector<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> workers;
    boost::thread_group thread_group;
    boost::mutex mutex;
    inline uint8_t randomNum(int a, int b)
    {
        if (a > b) return 0;
        return rand() % (b - a + 1) + a;
    }

};


