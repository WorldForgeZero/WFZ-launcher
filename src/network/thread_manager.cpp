#include "thread_manager.h"

ThreadManager &ThreadManager::instance()
{
    static ThreadManager instance;
    return instance;
}

ThreadManager::ThreadManager()
    : worker_(&ThreadManager::worker_loop, this)
{
}

ThreadManager::~ThreadManager()
{
    shutdown();
}

void ThreadManager::submit(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (stopping_)
            return;

        tasks_.push(std::move(task));
    }

    condition_.notify_one();
}

void ThreadManager::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!stopping_)
        {
            stopping_ = true;

            while (!tasks_.empty())
                tasks_.pop();
        }
    }

    condition_.notify_one();

    if (worker_.joinable())
        worker_.join();
}

void ThreadManager::worker_loop()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            condition_.wait(lock, [this]
                            { return stopping_ || !tasks_.empty(); });

            if (stopping_)
                return;

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        try
        {
            task();
        }
        catch (...)
        {
            // надо бы логировать. Но как-то похуй =)
        }
    }
}
