#include "paths.h"

#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32

#include <windows.h>

#else

#include <pwd.h>
#include <sys/types.h>
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

#ifdef _WIN32

    fs::path GetLocalAppDataPath()
    {
        const DWORD required_size = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);

        if (required_size == 0)
        {
            throw std::runtime_error("Failed to get LOCALAPPDATA");
        }

        std::vector<wchar_t> buffer(required_size);

        const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer.data(), static_cast<DWORD>(buffer.size()));

        if (length == 0 || length >= buffer.size())
        {
            throw std::runtime_error("Failed to read LOCALAPPDATA");
        }

        return fs::path(buffer.data());
    }

#else

    fs::path GetHomePath()
    {
        if (const char *home = std::getenv("HOME"))
        {
            if (*home != '\0')
                return fs::u8path(home);
        }

        const passwd *entry = getpwuid(getuid());

        if (entry && entry->pw_dir && entry->pw_dir[0] != '\0')
        {
            return fs::u8path(entry->pw_dir);
        }

        throw std::runtime_error("Failed to get user home directory");
    }

#endif

    fs::path GetConfigDirPlatform()
    {
#ifdef _WIN32

        return GetLocalAppDataPath() / L"WFZ";

#else

        if (const char *xdg_config_home = std::getenv("XDG_CONFIG_HOME"))
            if (*xdg_config_home != '\0')
                return fs::u8path(xdg_config_home) / "wfz";

        return GetHomePath() / ".config" / "wfz";

#endif
    }

    struct Paths
    {
        fs::path executable;
        fs::path executable_dir;

        fs::path config;

        fs::path default_source;
        fs::path source;

        std::mutex mutex;

        Paths()
            : executable(GetExecutablePathPlatform()),
              executable_dir(executable.parent_path()),
              config(GetConfigDirPlatform()),
              default_source(executable_dir / "WFZSource"),
              source(default_source)
        {
        }
    };

    Paths &GetPaths()
    {
        static Paths paths;
        return paths;
    }
}

namespace wfz::paths
{
    fs::path ExecutablePath()
    {
        return GetPaths().executable;
    }

    fs::path ExecutableDir()
    {
        return GetPaths().executable_dir;
    }

    fs::path ConfigDir()
    {
        return GetPaths().config;
    }

    fs::path DefaultSourceDir()
    {
        return GetPaths().default_source;
    }

    fs::path SourceDir()
    {
        Paths &paths = GetPaths();

        std::lock_guard<std::mutex> lock(paths.mutex);

        return paths.source;
    }

    void SetSourceDir(fs::path path)
    {
        if (path.empty())
        {
            throw std::invalid_argument("Source directory cannot be empty");
        }

        if (!path.is_absolute())
        {
            throw std::invalid_argument("Source directory must be absolute");
        }

        path = path.lexically_normal();

        Paths &paths = GetPaths();

        std::lock_guard<std::mutex> lock(paths.mutex);

        paths.source = std::move(path);
    }

    fs::path TempDir()
    {
        return SourceDir() / "tmp";
    }

    fs::path GameDir()
    {
        return SourceDir() / "game";
    }

    fs::path LauncherDir()
    {
        return SourceDir() / "launcher";
    }

    fs::path LogsDir()
    {
        return SourceDir() / "logs";
    }
}
