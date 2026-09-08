#pragma once

#include <filesystem>
#include <string>

namespace wfz::network
{
    void download(const std::string &url, const std::filesystem::path &destination);
}
