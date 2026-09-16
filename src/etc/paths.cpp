#include "paths.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

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

        std::vector<wchar_t> buffer(256);

        while (true)
        {
            const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

            if (length == 0)
            {
                throw std::runtime_error("Failed to get executable path");
            }

            if (length < buffer.size())
            {
                return fs::path(buffer.data(), buffer.data() + length);
            }

            buffer.resize(buffer.size() * 2);
        }

#else

        std::vector<char> buffer(256);

        while (true)
        {
            const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size());

            if (length < 0)
            {
                throw std::runtime_error("Failed to get executable path");
            }

            if (static_cast<std::size_t>(length) < buffer.size())
            {
                return fs::path(std::string(buffer.data(), static_cast<std::size_t>(length)));
            }

            buffer.resize(buffer.size() * 2);
        }

#endif
    }

    struct Paths
    {
        fs::path executable;
        fs::path executable_dir;

        fs::path source;

        fs::path temp;
        fs::path config;
        fs::path game;
        fs::path launcher;
        fs::path logs;

        Paths()
            : executable(GetExecutablePathPlatform()),
              executable_dir(executable.parent_path()),
              source(executable_dir / "WFZSource"),
              temp(source / "tmp"),
              config(source / "config"),
              game(source / "game"),
              launcher(source / "launcher"),
              logs(source / "logs")
        {
        }
    };

    const Paths &GetPaths()
    {
        static const Paths paths;
        return paths;
    }
}

namespace wfz::paths
{
    const fs::path &ExecutablePath() { return GetPaths().executable; }
    const fs::path &ExecutableDir() { return GetPaths().executable_dir; }

    const fs::path &SourceDir() { return GetPaths().source; }

    const fs::path &TempDir() { return GetPaths().temp; }
    const fs::path &ConfigDir() { return GetPaths().config; }
    const fs::path &GameDir() { return GetPaths().game; }
    const fs::path &LauncherDir() { return GetPaths().launcher; }
    const fs::path &LogsDir() { return GetPaths().logs; }
}
