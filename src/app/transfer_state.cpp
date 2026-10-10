#include "transfer_state.h"

#include <algorithm>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <utility>

#include "app/loading_state.h"

namespace
{
    std::mutex g_mutex;
    wfz::transfer_state::Snapshot g_snapshot;

    std::string Status(const wfz::transfer_state::Snapshot &s)
    {
        std::ostringstream out;
        out << s.task;
        if (s.attempt > 0)
            out << " (" << s.attempt << '/' << s.max_attempts << ')';

        if (s.phase == wfz::transfer_state::Phase::Downloading && s.received)
        {
            out << ": " << std::fixed << std::setprecision(1)
                << (static_cast<double>(s.received) / 1048576.0) << " MiB";

            if (s.total)
                out << " / " << (static_cast<double>(s.total) / 1048576.0) << " MiB";

            if (s.bytes_per_second > 0)
                out << ", " << (s.bytes_per_second / 1048576.0) << " MiB/s";

            if (s.eta_seconds >= 0)
                out << ", ~" << static_cast<unsigned>(s.eta_seconds + 0.5) << " сек";
        }
        return out.str();
    }

    void Publish(const wfz::transfer_state::Snapshot &s)
    {
        wfz::loading_state::SetStatus(Status(s));
        if (s.phase == wfz::transfer_state::Phase::Downloading && s.total)
            wfz::loading_state::SetProgress(static_cast<float>(std::min(1.0, static_cast<double>(s.received) / s.total)));
    }
}

namespace wfz::transfer_state
{
    Snapshot Get()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_snapshot;
    }

    void Start(std::string task, std::string url, int attempt, int max_attempts)
    {
        Snapshot copy;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_snapshot = {};
            g_snapshot.phase = Phase::Connecting;
            g_snapshot.task = std::move(task);
            g_snapshot.url = std::move(url);
            g_snapshot.attempt = attempt;
            g_snapshot.max_attempts = max_attempts;
            copy = g_snapshot;
        }
        wfz::loading_state::SetProgress(0);
        Publish(copy);
    }

    void SetTask(std::string task, std::string url)
    {
        Snapshot copy;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_snapshot.task = std::move(task);
            g_snapshot.url = std::move(url);
            g_snapshot.phase = Phase::Connecting;
            g_snapshot.received = 0;
            g_snapshot.total = 0;
            g_snapshot.bytes_per_second = 0;
            g_snapshot.eta_seconds = -1;
            copy = g_snapshot;
        }
        Publish(copy);
    }

    void Tick(std::uint64_t received, std::uint64_t total, double speed)
    {
        Snapshot copy;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_snapshot.phase = Phase::Downloading;
            g_snapshot.received = received;
            g_snapshot.total = total;
            g_snapshot.bytes_per_second = speed;
            g_snapshot.eta_seconds = (total > received && speed > 0)
                                         ? static_cast<double>(total - received) / speed
                                         : -1;
            copy = g_snapshot;
        }
        Publish(copy);
    }

    void SetPhase(Phase phase)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_snapshot.phase = phase;
    }

    void Fail(std::string error)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_snapshot.phase = Phase::Failed;
        g_snapshot.error = std::move(error);
    }

    void Reset()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_snapshot = {};
    }
}
