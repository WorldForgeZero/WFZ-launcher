#pragma once

#include <filesystem>
#include <string>

namespace wfz::crypto
{
    std::string sha256(const std::string &data);
    std::string sha256_file(const std::filesystem::path &path);
}
