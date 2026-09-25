#include "logger.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
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

    const char *LogLevelName(wfz::logger::Level level)
    {
        switch (level)
        {
        case wfz::logger::Level::Trace:
            return "TRACE";

        case wfz::logger::Level::Debug:
            return "DEBUG";

        case wfz::logger::Level::Info:
            return "INFO";

        case wfz::logger::Level::Warning:
            return "WARNING";

        case wfz::logger::Level::Error:
            return "ERROR";

        case wfz::logger::Level::Fatal:
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

        std::vector<char> buffer(
            static_cast<std::size_t>(length) + 1);

        va_copy(copy, args);

        std::vsnprintf(
            buffer.data(),
            buffer.size(),
            format,
            copy);

        va_end(copy);

        return std::string(
            buffer.data(),
            static_cast<std::size_t>(length));
    }

    std::tm GetLocalTime(std::time_t time)
    {
        std::tm result{};

#ifdef _WIN32
        localtime_s(&result, &time);
#else
        localtime_r(&time, &result);
#endif

        return result;
    }

    std::string BuildPrefix(wfz::logger::Level level)
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);

        const std::tm local_time = GetLocalTime(time);

        std::ostringstream stream;

        stream
            << '['
            << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S")
            << "] ["
            << LogLevelName(level)
            << "] ";

        return stream.str();
    }

    void Write(
        wfz::logger::Level level,
        const std::string &message)
    {
        const std::string line =
            BuildPrefix(level) + message;

        std::lock_guard<std::mutex> lock(g_log_mutex);

        if (level == wfz::logger::Level::Warning ||
            level == wfz::logger::Level::Error ||
            level == wfz::logger::Level::Fatal)
        {
            std::cerr << line << '\n';
        }
        else
        {
            std::cout << line << '\n';
        }

        if (g_log_file.is_open())
        {
            g_log_file << line << '\n';
            g_log_file.flush();
        }
    }

    void LogV(
        wfz::logger::Level level,
        const char *format,
        va_list args)
    {
        if (!format)
            return;

        Write(
            level,
            FormatMessage(format, args));
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
        std::lock_guard<std::mutex> lock(g_log_mutex);

        if (g_log_file.is_open())
            return true;

        std::error_code ec;

        fs::create_directories(
            wfz::paths::LogsDir(),
            ec);

        if (ec)
        {
            std::cerr
                << "Failed to create log directory: "
                << ec.message()
                << '\n';

            return false;
        }

        const fs::path log_path =
            wfz::paths::LogsDir() / "launcher.log";

        g_log_file.open(
            log_path,
            std::ios::out | std::ios::trunc);

        if (!g_log_file.is_open())
        {
            std::cerr
                << "Failed to open log file: "
                << log_path
                << '\n';

            return false;
        }

        return true;
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(g_log_mutex);

        if (!g_log_file.is_open())
            return;

        g_log_file.flush();
        g_log_file.close();
    }

    void Log(Level level, const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        LogV(level, format, args);

        va_end(args);
    }

    void Trace(const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        LogV(Level::Trace, format, args);

        va_end(args);
    }

    void Debug(const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        LogV(Level::Debug, format, args);

        va_end(args);
    }

    void Info(const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        LogV(Level::Info, format, args);

        va_end(args);
    }

    void Warning(const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        LogV(Level::Warning, format, args);

        va_end(args);
    }

    void Error(const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        LogV(Level::Error, format, args);

        va_end(args);
    }

    void Fatal(const char *format, ...)
    {
        va_list args;
        va_start(args, format);

        LogV(Level::Fatal, format, args);

        va_end(args);
    }

    void DumpStartupInfo()
    {
        Info("============================================================");

        Info("WFZ Launcher startup information");
        Info("Launcher version: %s", launcher_version);
        Info("Platform: %s", GetPlatformName());
        Info("Architecture: %s", GetArchitecture());
        Info("Build type: %s", GetBuildType());
        Info("Compiler: %s", GetCompilerName());

#ifdef __VERSION__
        Info("Compiler version: %s", __VERSION__);
#endif

        Info(
            "Pointer size: %zu-bit",
            sizeof(void *) * 8);

        Info(
            "Hardware threads: %u",
            std::thread::hardware_concurrency());

        Info(
            "Executable: %s",
            wfz::paths::ExecutablePath()
                .string()
                .c_str());

        Info(
            "Executable directory: %s",
            wfz::paths::ExecutableDir()
                .string()
                .c_str());

        Info(
            "Source directory: %s",
            wfz::paths::SourceDir()
                .string()
                .c_str());

        std::error_code ec;

        const fs::path working_directory =
            fs::current_path(ec);

        if (!ec)
        {
            Info(
                "Working directory: %s",
                working_directory.string().c_str());
        }
        else
        {
            Warning(
                "Failed to get working directory: %s",
                ec.message().c_str());
        }

        Info("============================================================");
    }
}
