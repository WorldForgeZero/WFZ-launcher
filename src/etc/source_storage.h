#pragma once

#include <filesystem>
#include <string>

namespace wfz::source_storage
{
    enum class Operation
    {
        KeepOld,
        Copy,
        Move,
        DiscardOld
    };

    bool Prepare(const std::filesystem::path &old_dir, const std::filesystem::path &new_dir, Operation operation, std::string &error);

    bool Cleanup(const std::filesystem::path &dir, std::string &error);

    bool ValidateRoots(const std::filesystem::path &old_dir, const std::filesystem::path &new_dir, std::string &error);
}
