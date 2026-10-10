#pragma once

#include <cstdint>
#include <string>

namespace wfz::transfer_state
{
    enum class Phase
    {
        Idle,
        Connecting,
        Downloading,
        Validating,
        Completed,
        Failed,
        Cancelled
    };

    struct Snapshot
    {
        Phase phase = Phase::Idle;
        std::string task;
        std::string url;
        std::string error;
        int attempt = 0;
        int max_attempts = 3;
        std::uint64_t received = 0;
        std::uint64_t total = 0;
        double bytes_per_second = 0;
        double eta_seconds = -1;
    };

    // Thread safe. UI reads a snapshot, worker writes it.
    Snapshot Get();
    void Start(std::string task, std::string url, int attempt, int max_attempts);
    void SetTask(std::string task, std::string url);
    void Tick(std::uint64_t received, std::uint64_t total, double bytes_per_second);
    void SetPhase(Phase phase);
    void Fail(std::string error);
    void Reset();
}
