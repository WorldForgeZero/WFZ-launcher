#pragma once

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace wfz::network
{
    class Error : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class HttpStatusError final : public Error
    {
    public:
        HttpStatusError(int status, const std::string &message, int retry_after_seconds = 0)
            : Error(message), status_(status), retry_after_seconds_(retry_after_seconds) {}

        int status() const noexcept { return status_; }
        int retry_after_seconds() const noexcept { return retry_after_seconds_; }

    private:
        int status_;
        int retry_after_seconds_;
    };

    // A single attempt. Retries belong to daemon/retry.h.
    // Only HTTPS is accepted, TLS certificates are verified.
    std::string GetText(const std::string &url, const std::string &task, std::size_t max_bytes = 4 * 1024 * 1024);

    // Downloads into destination.part then commits the completed file.
    // expected_sha256 may be empty when the manifest has no digest.
    void DownloadFile(const std::string &url, const std::filesystem::path &destination, const std::string &task, const std::string &expected_sha256 = {});

    // Writes via temporary file. The old file is retained when writing fails.
    void WriteAtomic(const std::filesystem::path &destination, const std::string &body);
}
