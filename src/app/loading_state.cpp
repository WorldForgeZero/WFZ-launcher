#include "loading_state.h"

#include <atomic>
#include <mutex>
#include <string>
#include <utility>

static float WFZClamp01(float value) // TODO: Move to common utils.
{
    if (value < 0.0f)
        return 0.0f;

    if (value > 1.0f)
        return 1.0f;

    return value;
}

namespace
{
    std::atomic<float> g_progress{0.0f};

    std::mutex g_status_mutex;
    std::string g_status{":skull:"};

    std::atomic<bool> g_ready{false};

    // UI thread only.
    float g_displayed_progress = 0.0f;
    float g_ready_time = 0.0f;
}

namespace wfz::loading_state
{
    void SetProgress(float progress)
    {
        g_progress.store(WFZClamp01(progress), std::memory_order_relaxed);
    }

    float GetProgress()
    {
        return g_progress.load(std::memory_order_relaxed);
    }

    void DeltaProgress(float delta)
    {
        const float current = g_progress.load(std::memory_order_relaxed);
        g_progress.store(WFZClamp01(current + delta), std::memory_order_relaxed);
    }

    void SetStatus(std::string status)
    {
        std::lock_guard<std::mutex> lock(g_status_mutex);
        g_status = std::move(status);
    }

    std::string GetStatus()
    {
        std::lock_guard<std::mutex> lock(g_status_mutex);
        return g_status;
    }

    void SetReady(bool is_ready)
    {
        g_ready.store(is_ready, std::memory_order_relaxed);
    }

    bool GetReady()
    {
        return g_ready.load(std::memory_order_relaxed);
    }

    void SetDisplayProgress(float progress)
    {
        g_displayed_progress = WFZClamp01(progress);
    }

    float GetDisplayProgress()
    {
        return g_displayed_progress;
    }

    void DeltaDisplayProgress(float delta)
    {
        g_displayed_progress = WFZClamp01(g_displayed_progress + delta);
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
