#pragma once

#include <filesystem>
#include <optional>

namespace wfz::self_update
{
    std::optional<int> HandleStartupArguments(int argc, char **argv);

    bool Begin(const std::filesystem::path &new_executable);
}
