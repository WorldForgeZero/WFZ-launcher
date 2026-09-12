#include "loading_state.h"

#include <atomic>

static float WFZClamp01(float value) // Это дубликат потом бы сделать надо бы рефактор
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
    std::atomic<const char *> g_status{"Ожидание сети..."};

    std::atomic<bool> g_ready = false;

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
