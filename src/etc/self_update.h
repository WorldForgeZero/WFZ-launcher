#pragma once

#include <filesystem>
#include <optional>

namespace wfz::self_update
{
    // Handles internal self-update command-line modes.
    //
    // Returns:
    //   std::nullopt - continue normal launcher startup.
    //   int          - terminate process with this exit code.
    std::optional<int> HandleStartupArguments(int argc, char **argv);

    // Starts the staged launcher in self-update mode.
    //
    // The current process is NOT terminated by this function.
    // After a successful call, the application should shut down normally.
    bool Begin(const std::filesystem::path &new_executable);
}
