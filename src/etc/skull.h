#pragma once

#include <atomic>

namespace wfz::app
{
    inline std::atomic_bool exit_requested{false};

    inline void RequestExit() noexcept
    {
        exit_requested.store(            true,            std::memory_order_relaxed);
    }

    inline bool ExitRequested() noexcept
    {
        return exit_requested.load(            std::memory_order_relaxed);
    }
}
