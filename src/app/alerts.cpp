#include "alerts.h"

#include <mutex>
#include <queue>
#include <utility>

namespace
{
    std::mutex g_mutex;
    std::queue<wfz::alerts::Alert> g_queue;
}

namespace wfz::alerts
{
    void Post(Alert alert)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_queue.push(std::move(alert));
    }

    bool TryPop(Alert &out)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_queue.empty())
            return false;

        out = std::move(g_queue.front());
        g_queue.pop();
        return true;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        while (!g_queue.empty())
            g_queue.pop();
    }
}
