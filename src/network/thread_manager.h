#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadManager
{
public:
    static ThreadManager &instance();

    ThreadManager(const ThreadManager &) = delete;
    ThreadManager &operator=(const ThreadManager &) = delete;

    void submit(std::function<void()> task);
    void shutdown();

private:
    ThreadManager();
    ~ThreadManager();

    void worker_loop();

    std::thread worker_;

    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::function<void()>> tasks_;

    bool stopping_ = false;
};
