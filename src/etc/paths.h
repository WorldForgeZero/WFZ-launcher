#pragma once

#include <filesystem>

namespace wfz::paths
{
    const std::filesystem::path &ExecutablePath();
    const std::filesystem::path &ExecutableDir();

    const std::filesystem::path &SourceDir();

    const std::filesystem::path &TempDir();
    const std::filesystem::path &ConfigDir();
    const std::filesystem::path &GameDir();
    const std::filesystem::path &LauncherDir();
    const std::filesystem::path &LogsDir();
}
