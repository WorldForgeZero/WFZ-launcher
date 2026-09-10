#include "loading_state.h"

#include <atomic>

namespace
{
    std::atomic<float> g_progress{0.0f};
    std::atomic<const char *> g_status{"Ожидание сети..."};

    std::atomic<bool> g_ready = false;

    float g_displayed_progress = 0.0f;
    float g_ready_time = 0.0f;
}

namespace wfz::loading_state
{
    void SetProgress(float progress)
    {
        if (progress < 0.0f)
        {
            progress = 0.0f;
        }
        else if (progress > 1.0f)
        {
            progress = 1.0f;
        }

        g_progress.store(progress, std::memory_order_relaxed);
    }

    float GetProgress()
    {
        return g_progress.load(std::memory_order_relaxed);
    }

    void SetStatus(const char *status)
    {
        g_status.store(status, std::memory_order_relaxed);
    }

    const char *GetStatus()
    {
        return g_status.load(std::memory_order_relaxed);
    }

    void SetReady(bool isReady)
    {
        g_ready.store(isReady, std::memory_order_relaxed);
    }

    bool GetReady()
    {
        return g_ready.load(std::memory_order_relaxed);
    }

    void SetDisplayProgress(float progress)
    {
        if (progress < 0.0f)
        {
            progress = 0.0f;
        }
        else if (progress > 1.0f)
        {
            progress = 1.0f;
        }

        g_displayed_progress = progress;
    }

    float GetDisplayProgress()
    {
        return g_displayed_progress;
    }

    void SetReadyTime(float time)
    {
        g_ready_time = time;
    }

    float GetReadyTime()
    {
        return g_ready_time;
    }
}
