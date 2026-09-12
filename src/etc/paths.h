#pragma once

#include <filesystem>

namespace wfz::paths
{
    const std::filesystem::path &ExecutableDir();
    const std::filesystem::path &SourceDir();

    inline std::filesystem::path TempDir()
    {
        return SourceDir() / "tmp";
    }

    inline std::filesystem::path ConfigDir()
    {
        return SourceDir() / "config";
    }

    inline std::filesystem::path GameDir()
    {
        return SourceDir() / "game";
    }

    inline std::filesystem::path LauncherDir()
    {
        return SourceDir() / "launcher";
    }

    inline std::filesystem::path LogsDir()
    {
        return SourceDir() / "logs";
    }
}
