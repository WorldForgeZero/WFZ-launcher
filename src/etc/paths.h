#pragma once

#include <filesystem>

namespace wfz::paths
{
    std::filesystem::path ExecutablePath();
    std::filesystem::path ExecutableDir();

    /*
     * Platform-specific launcher configuration directory.
     *
     * Windows:
     *   %LOCALAPPDATA%\WFZ
     *
     * Linux:
     *   $XDG_CONFIG_HOME/wfz
     *
     * Fallback:
     *   ~/.config/wfz
     */
    std::filesystem::path ConfigDir();

    /*
     * WFZ data directory.
     *
     * This path can be changed by launcher settings.
     */
    std::filesystem::path SourceDir();
    void SetSourceDir(std::filesystem::path path);

    /*
     * Default value used when no source directory
     * has been configured yet.
     */
    std::filesystem::path DefaultSourceDir();

    /*
     * Paths derived from SourceDir().
     */
    std::filesystem::path TempDir();
    std::filesystem::path GameDir();
    std::filesystem::path LauncherDir();
    std::filesystem::path LogsDir();
}
