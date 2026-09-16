#include "self_update.h"

#include "paths.h"

#include <chrono>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32

// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on

#else

#include <cerrno>
#include <csignal>
#include <sys/types.h>
#include <unistd.h>

#endif

namespace fs = std::filesystem;

namespace
{
    constexpr int REPLACE_RETRY_COUNT = 100;
    constexpr auto REPLACE_RETRY_DELAY = std::chrono::milliseconds(100);

#ifdef _WIN32

    using ProcessId = DWORD;
    using NativeString = std::wstring;

    const NativeString APPLY_UPDATE_ARG = L"--apply-self-update";

    const NativeString CLEANUP_UPDATE_ARG = L"--cleanup-self-update";

#else

    using ProcessId = pid_t;
    using NativeString = std::string;

    const NativeString APPLY_UPDATE_ARG = "--apply-self-update";

    const NativeString CLEANUP_UPDATE_ARG = "--cleanup-self-update";

#endif

    enum class StartupMode
    {
        Normal,
        ApplyUpdate,
        CleanupUpdate,
        Invalid
    };

    struct StartupRequest
    {
        StartupMode mode = StartupMode::Normal;

        fs::path path;
        ProcessId parent_pid = 0;
    };

    NativeString PathToNativeString(const fs::path &path)
    {
#ifdef _WIN32
        return path.wstring();
#else
        return path.string();
#endif
    }

    NativeString ProcessIdToNativeString(ProcessId process_id)
    {
#ifdef _WIN32
        return std::to_wstring(process_id);
#else
        return std::to_string(static_cast<long long>(process_id));
#endif
    }

    ProcessId CurrentProcessId()
    {
#ifdef _WIN32
        return GetCurrentProcessId();
#else
        return getpid();
#endif
    }

    std::optional<ProcessId> ParseProcessId(const NativeString &value)
    {
        try
        {
            std::size_t consumed = 0;

            const unsigned long long parsed = std::stoull(value, &consumed);

            if (consumed != value.size())
                return std::nullopt;

            if (parsed == 0)
                return std::nullopt;

            if (parsed > static_cast<unsigned long long>(std::numeric_limits<ProcessId>::max()))
            {
                return std::nullopt;
            }

            return static_cast<ProcessId>(parsed);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    bool CopyExecutable(const fs::path &source, const fs::path &target)
    {
        std::error_code ec;

        fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec);

        if (ec)
            return false;

#ifndef _WIN32

        const fs::perms permissions = fs::status(source, ec).permissions();

        if (ec)
            return false;

        fs::permissions(target, permissions, fs::perm_options::replace, ec);

        if (ec)
            return false;

#endif

        return true;
    }

    bool ReplaceExecutable(const fs::path &source, const fs::path &target)
    {
        for (int attempt = 0; attempt < REPLACE_RETRY_COUNT; ++attempt)
        {
            if (CopyExecutable(source, target))
            {
                return true;
            }

            std::this_thread::sleep_for(REPLACE_RETRY_DELAY);
        }

        return false;
    }

    void RemoveFileBestEffort(const fs::path &path)
    {
        for (int attempt = 0; attempt < REPLACE_RETRY_COUNT; ++attempt)
        {
            std::error_code ec;

            fs::remove(path, ec);

            if (!ec)
                return;

            std::this_thread::sleep_for(REPLACE_RETRY_DELAY);
        }
    }

    bool PrepareExecutable(const fs::path &path)
    {
#ifdef _WIN32

        (void)path;
        return true;

#else

        std::error_code ec;

        fs::permissions(
            path,
            fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
            fs::perm_options::add,
            ec);

        return !ec;

#endif
    }

#ifdef _WIN32

    std::wstring QuoteWindowsArgument(const std::wstring &argument)
    {
        if (argument.empty())
            return L"\"\"";

        if (argument.find_first_of(L" \t\"") == std::wstring::npos)
        {
            return argument;
        }

        std::wstring result;
        result.push_back(L'"');

        std::size_t backslashes = 0;

        for (const wchar_t character : argument)
        {
            if (character == L'\\')
            {
                ++backslashes;
                continue;
            }

            if (character == L'"')
            {
                result.append(backslashes * 2 + 1, L'\\');

                result.push_back(L'"');

                backslashes = 0;
                continue;
            }

            result.append(backslashes, L'\\');

            backslashes = 0;

            result.push_back(character);
        }

        result.append(backslashes * 2, L'\\');

        result.push_back(L'"');

        return result;
    }

    bool StartProcess(const fs::path &executable, const std::vector<NativeString> &arguments)
    {
        std::wstring command_line = QuoteWindowsArgument(executable.wstring());

        for (const auto &argument : arguments)
        {
            command_line.push_back(L' ');

            command_line += QuoteWindowsArgument(argument);
        }

        std::vector<wchar_t> command_buffer(
            command_line.begin(),
            command_line.end());

        command_buffer.push_back(L'\0');

        STARTUPINFOW startup_info{};
        startup_info.cb = sizeof(startup_info);

        PROCESS_INFORMATION process_info{};

        const BOOL created =
            CreateProcessW(
                executable.c_str(),
                command_buffer.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &startup_info,
                &process_info);

        if (!created)
            return false;

        CloseHandle(process_info.hThread);

        CloseHandle(process_info.hProcess);

        return true;
    }

    void WaitForProcess(ProcessId process_id)
    {
        HANDLE process =
            OpenProcess(
                SYNCHRONIZE,
                FALSE,
                process_id);

        if (process == nullptr)
            return;

        WaitForSingleObject(process, INFINITE);
        CloseHandle(process);
    }

    std::vector<NativeString>
    GetStartupArguments(int argc, char **argv)
    {
        (void)argc;
        (void)argv;

        int argument_count = 0;

        LPWSTR *arguments =
            CommandLineToArgvW(
                GetCommandLineW(),
                &argument_count);

        if (arguments == nullptr)
            return {};

        std::vector<NativeString> result;
        result.reserve(static_cast<std::size_t>(argument_count));

        for (int i = 0; i < argument_count; ++i)
        {
            result.emplace_back(arguments[i]);
        }

        LocalFree(arguments);

        return result;
    }

#else

    bool StartProcess(const fs::path &executable, const std::vector<NativeString> &arguments)
    {
        const pid_t child = fork();

        if (child < 0)
            return false;

        if (child != 0)
            return true;

        std::vector<std::string> storage;

        storage.reserve(arguments.size() + 1);

        storage.emplace_back(executable.string());

        for (const auto &argument : arguments)
            storage.push_back(argument);

        std::vector<char *> argv;

        argv.reserve(storage.size() + 1);

        for (std::string &argument : storage)
            argv.push_back(argument.data());

        argv.push_back(nullptr);

        execv(executable.c_str(), argv.data());

        _exit(127);
    }

    void WaitForProcess(ProcessId process_id)
    {
        while (true)
        {
            if (kill(process_id, 0) == 0)
            {
                std::this_thread::sleep_for(REPLACE_RETRY_DELAY);

                continue;
            }

            if (errno == EPERM)
            {
                std::this_thread::sleep_for(REPLACE_RETRY_DELAY);

                continue;
            }

            if (errno == ESRCH)
                return;

            return;
        }
    }

    std::vector<NativeString>
    GetStartupArguments(int argc, char **argv)
    {
        std::vector<NativeString> result;

        result.reserve(static_cast<std::size_t>(argc));

        for (int i = 0; i < argc; ++i)
        {
            result.emplace_back(
                argv[i] != nullptr
                    ? argv[i]
                    : "");
        }

        return result;
    }

#endif

    StartupRequest ParseStartupRequest(int argc, char **argv)
    {
        const auto arguments = GetStartupArguments(argc, argv);

        if (arguments.size() < 2)
            return {};

        if (arguments[1] == APPLY_UPDATE_ARG)
        {
            if (arguments.size() != 4)
            {
                return {StartupMode::Invalid, {}, 0};
            }

            const auto parent_pid = ParseProcessId(arguments[3]);

            if (!parent_pid)
            {
                return {StartupMode::Invalid, {}, 0};
            }

            return {StartupMode::ApplyUpdate, fs::path(arguments[2]), *parent_pid};
        }

        if (arguments[1] == CLEANUP_UPDATE_ARG)
        {
            if (arguments.size() != 3)
            {
                return {StartupMode::Invalid, {}, 0};
            }

            return {StartupMode::CleanupUpdate, fs::path(arguments[2]), 0};
        }

        return {};
    }

    int ApplyUpdate(const fs::path &target, ProcessId parent_pid)
    {
        WaitForProcess(parent_pid);

        const fs::path updater = wfz::paths::ExecutablePath();

        if (!ReplaceExecutable(updater, target))
        {
            return 1;
        }

        if (!StartProcess(
                target,
                {
                    CLEANUP_UPDATE_ARG,
                    PathToNativeString(updater),
                }))
        {
            return 1;
        }

        return 0;
    }
}

namespace wfz::self_update
{
    std::optional<int> HandleStartupArguments(int argc, char **argv)
    {
        const StartupRequest request = ParseStartupRequest(argc, argv);

        switch (request.mode)
        {
        case StartupMode::Normal:
            return std::nullopt;

        case StartupMode::Invalid:
            return 1;

        case StartupMode::ApplyUpdate:
            return ApplyUpdate(request.path, request.parent_pid);

        case StartupMode::CleanupUpdate:
            RemoveFileBestEffort(request.path);

            return std::nullopt;
        }

        return 1;
    }

    bool Begin(const fs::path &new_executable)
    {
        std::error_code ec;

        if (!fs::exists(new_executable, ec) || ec)
        {
            return false;
        }

        if (!fs::is_regular_file(new_executable, ec) || ec)
        {
            return false;
        }

        if (!PrepareExecutable(new_executable))
        {
            return false;
        }

        return StartProcess(
            new_executable,
            {
                APPLY_UPDATE_ARG,
                PathToNativeString(wfz::paths::ExecutablePath()),
                ProcessIdToNativeString(CurrentProcessId()),
            });
    }
}
