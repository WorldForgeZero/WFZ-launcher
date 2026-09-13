#include "paths.h"

#include <filesystem>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace
{
    fs::path GetExecutablePathPlatform()
    {
#ifdef _WIN32
        wchar_t buffer[MAX_PATH];

        const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

        if (length == 0 || length >= MAX_PATH)
        {
            throw std::runtime_error("Failed to get executable path");
        }

        return fs::path(buffer);

#else
        char buffer[4096];

        const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer));

        if (length <= 0)
        {
            throw std::runtime_error("Failed to get executable path");
        }

        return fs::path(std::string(buffer, static_cast<std::size_t>(length)));
#endif
    }
}

namespace wfz::paths
{
    const fs::path &ExecutablePath()
    {
        static const fs::path path = GetExecutablePathPlatform();

        return path;
    }

    const fs::path &ExecutableDir()
    {
        static const fs::path path = ExecutablePath().parent_path();

        return path;
    }

    const fs::path &SourceDir()
    {
        static const fs::path path = ExecutableDir() / "WFZSource";

        return path;
    }
}
