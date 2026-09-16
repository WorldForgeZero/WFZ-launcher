#include "logger.h"

#include <raylib.h>

#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "paths.h"

#include "settings/version.h"

namespace fs = std::filesystem;

namespace
{
    std::ofstream g_log_file;
    std::mutex g_log_mutex;

    const char *LogLevelName(int level)
    {
        switch (level)
        {
        case LOG_TRACE:
            return "TRACE";

        case LOG_DEBUG:
            return "DEBUG";

        case LOG_INFO:
            return "INFO";

        case LOG_WARNING:
            return "WARNING";

        case LOG_ERROR:
            return "ERROR";

        case LOG_FATAL:
            return "FATAL";

        default:
            return "UNKNOWN";
        }
    }

    std::string FormatMessage(const char *format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

        const int length = std::vsnprintf(nullptr, 0, format, copy);

        va_end(copy);

        if (length <= 0)
            return {};

        std::vector<char> buffer(static_cast<std::size_t>(length) + 1);

        va_copy(copy, args);
        std::vsnprintf(buffer.data(), buffer.size(), format, copy);
        va_end(copy);

        return std::string(buffer.data(), static_cast<std::size_t>(length));
    }

    void RaylibLogCallback(int log_level, const char *text, va_list args)
    {
        const std::string message = FormatMessage(text, args);

        std::lock_guard<std::mutex> lock(g_log_mutex);

        if (g_log_file.is_open())
        {
            g_log_file << '[' << LogLevelName(log_level) << "] " << message << '\n';
            g_log_file.flush();
        }
    }

    const char *GetPlatformName()
    {
#ifdef _WIN32
        return "Windows";
#elif defined(__linux__)
        return "Linux";
#else
        return "Unknown";
#endif
    }

    const char *GetBuildType()
    {
#ifdef NDEBUG
        return "Release";
#else
        return "Debug";
#endif
    }

    const char *GetCompilerName()
    {
#ifdef __clang__
        return "Clang";
#elif defined(__GNUC__)
        return "GCC";
#elif defined(_MSC_VER)
        return "MSVC";
#else
        return "Unknown";
#endif
    }

    const char *GetArchitecture()
    {
#if defined(__x86_64__) || defined(_M_X64)
        return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
        return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
        return "ARM64";
#elif defined(__arm__) || defined(_M_ARM)
        return "ARM";
#else
        return "Unknown";
#endif
    }
}

namespace wfz::logger
{
    bool Init()
    {
        std::error_code ec;

        fs::create_directories(wfz::paths::LogsDir(), ec);

        if (ec)
            return false;

        const fs::path log_path = wfz::paths::LogsDir() / "launcher.log";

        g_log_file.open(log_path, std::ios::out | std::ios::trunc);

        if (!g_log_file.is_open())
            return false;

        SetTraceLogCallback(RaylibLogCallback);
        SetTraceLogLevel(LOG_TRACE);

        return true;
    }

    void DumpStartupInfo()
    {
        TraceLog(LOG_INFO, "============================================================");

        TraceLog(LOG_INFO, "WFZ Launcher startup information");
        TraceLog(LOG_INFO, "Launcher version: %s", launcher_version);
        TraceLog(LOG_INFO, "Platform: %s", GetPlatformName());
        TraceLog(LOG_INFO, "Architecture: %s", GetArchitecture());
        TraceLog(LOG_INFO, "Build type: %s", GetBuildType());
        TraceLog(LOG_INFO, "Compiler: %s", GetCompilerName());
#ifdef __VERSION__
        TraceLog(LOG_INFO, "Compiler version: %s", __VERSION__);
#endif
        TraceLog(LOG_INFO, "Pointer size: %zu-bit", sizeof(void *) * 8);
        TraceLog(LOG_INFO, "Hardware threads: %u", std::thread::hardware_concurrency());
        TraceLog(LOG_INFO, "Executable: %s", wfz::paths::ExecutablePath().string().c_str());
        TraceLog(LOG_INFO, "Executable directory: %s", wfz::paths::ExecutableDir().string().c_str());
        TraceLog(LOG_INFO, "Source directory: %s", wfz::paths::SourceDir().string().c_str());

        std::error_code ec;

        const fs::path working_directory = fs::current_path(ec);

        if (!ec)
        {
            TraceLog(LOG_INFO, "Working directory: %s", working_directory.string().c_str());
        }
        else
        {
            TraceLog(LOG_WARNING, "Failed to get working directory: %s", ec.message().c_str());
        }

        TraceLog(LOG_INFO, "============================================================");
    }

    void Shutdown()
    {
        SetTraceLogCallback(nullptr);

        std::lock_guard<std::mutex> lock(g_log_mutex);

        if (!g_log_file.is_open())
            return;

        g_log_file.flush();
        g_log_file.close();
    }
}
