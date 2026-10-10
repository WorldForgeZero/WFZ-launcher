#pragma once

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace wfz::network
{
    class Error final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    std::string GetText(const std::string &url, const std::string &task, std::size_t max_bytes = 4 * 1024 * 1024);

    void DownloadFile(const std::string &url, const std::filesystem::path &destination, const std::string &task, const std::string &expected_sha256 = {});

    void WriteAtomic(const std::filesystem::path &destination, const std::string &body);
}
