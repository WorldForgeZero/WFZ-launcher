#include "self_update.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "paths.h"

#ifdef _WIN32
#include <windows.h>

#include <shellapi.h>
#else
#include <cerrno>
#include <csignal>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace
{
    constexpr const char *APPLY_UPDATE_ARG = "--apply-self-update";

    constexpr const char *CLEANUP_UPDATE_ARG = "--cleanup-self-update";

    constexpr int REPLACE_RETRY_COUNT = 100;

    constexpr auto REPLACE_RETRY_DELAY = std::chrono::milliseconds(100);

    bool ReplaceExecutable(const fs::path &source, const fs::path &target)
    {
        for (int attempt = 0; attempt < REPLACE_RETRY_COUNT; ++attempt)
        {
            std::error_code ec;

            fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec);

            if (!ec)
            {
#ifndef _WIN32
                const fs::perms permissions = fs::status(source, ec).permissions();

                if (!ec)
                {
                    fs::permissions(target, permissions, fs::perm_options::replace, ec);
                }
#endif

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

            if (!fs::exists(path, ec))
                return;

            fs::remove(path, ec);

            if (!ec)
                return;

            std::this_thread::sleep_for(REPLACE_RETRY_DELAY);
        }
    }

#ifdef _WIN32

    std::wstring QuoteWindowsArgument(const std::wstring &argument)
    {
        if (argument.empty())
            return L"\"\"";

        const bool needs_quotes = argument.find_first_of(L" \t\"") != std::wstring::npos;

        if (!needs_quotes)
            return argument;

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

    bool StartProcess(const fs::path &executable, const std::vector<std::wstring> &arguments)
    {
        std::wstring command_line = QuoteWindowsArgument(executable.wstring());

        for (const std::wstring &argument : arguments)
        {
            command_line.push_back(L' ');
            command_line += QuoteWindowsArgument(argument);
        }

        std::vector<wchar_t> command_buffer(command_line.begin(), command_line.end());

        command_buffer.push_back(L'\0');

        STARTUPINFOW startup_info{};
        startup_info.cb = sizeof(startup_info);

        PROCESS_INFORMATION process_info{};

        const BOOL result =
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

        if (!result)
            return false;

        CloseHandle(process_info.hThread);

        CloseHandle(process_info.hProcess);

        return true;
    }

    void WaitForProcess(const DWORD process_id)
    {
        HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, process_id);

        if (process == nullptr)
            return;

        WaitForSingleObject(process, INFINITE);

        CloseHandle(process);
    }

    std::vector<std::wstring>
    GetWindowsArguments()
    {
        int argc = 0;

        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        std::vector<std::wstring> result;

        if (argv == nullptr)
            return result;

        result.reserve(static_cast<std::size_t>(argc));

        for (int i = 0; i < argc; ++i)
            result.emplace_back(argv[i]);

        LocalFree(argv);

        return result;
    }

    int ApplyUpdateWindows(const fs::path &target, const DWORD parent_pid)
    {
        WaitForProcess(parent_pid);

        const fs::path updater = wfz::paths::ExecutablePath();

        if (!ReplaceExecutable(updater, target))
            return 1;

        if (!StartProcess(
                target,
                {
                    std::wstring(
                        CLEANUP_UPDATE_ARG,
                        CLEANUP_UPDATE_ARG +
                            std::char_traits<char>::length(
                                CLEANUP_UPDATE_ARG)),
                    updater.wstring(),
                }))
        {
            return 1;
        }

        return 0;
    }

#else

    bool StartProcess(const fs::path &executable, const std::vector<std::string> &arguments)
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

    void WaitForProcess(const pid_t process_id)
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

            std::this_thread::sleep_for(REPLACE_RETRY_DELAY);
        }
    }

    int ApplyUpdateLinux(const fs::path &target, const pid_t parent_pid)
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
                    updater.string(),
                }))
        {
            return 1;
        }

        return 0;
    }

#endif
}

namespace wfz::self_update
{
    std::optional<int> HandleStartupArguments(int argc, char **argv)
    {
#ifdef _WIN32

        (void)argc;
        (void)argv;

        const auto arguments = GetWindowsArguments();

        if (arguments.size() >= 2 &&
            arguments[1] ==
                L"--apply-self-update")
        {
            if (arguments.size() != 4)
                return 1;

            const fs::path target = arguments[2];

            DWORD parent_pid = 0;

            try
            {
                parent_pid = static_cast<DWORD>(std::stoul(arguments[3]));
            }
            catch (...)
            {
                return 1;
            }

            return ApplyUpdateWindows(target, parent_pid);
        }

        if (arguments.size() >= 2 &&
            arguments[1] ==
                L"--cleanup-self-update")
        {
            if (arguments.size() >= 3)
            {
                RemoveFileBestEffort(fs::path(arguments[2]));
            }

            return std::nullopt;
        }

#else

        if (argc >= 2 && std::string_view(argv[1]) == APPLY_UPDATE_ARG)
        {
            if (argc != 4)
                return 1;

            const fs::path target = argv[2];

            pid_t parent_pid = 0;

            try
            {
                parent_pid = static_cast<pid_t>(std::stol(argv[3]));
            }
            catch (...)
            {
                return 1;
            }

            return ApplyUpdateLinux(target, parent_pid);
        }

        if (argc >= 2 && std::string_view(argv[1]) == CLEANUP_UPDATE_ARG)
        {
            if (argc >= 3)
            {
                RemoveFileBestEffort(fs::path(argv[2]));
            }

            return std::nullopt;
        }

#endif

        return std::nullopt;
    }

    bool Begin(const fs::path &new_executable)
    {
        std::error_code ec;

        if (!fs::exists(new_executable, ec))
        {
            return false;
        }

#ifndef _WIN32

        fs::permissions(new_executable,
                        fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add,
                        ec);

        if (ec)
            return false;

        const std::string parent_pid = std::to_string(static_cast<long long>(getpid()));

        return StartProcess(new_executable,
                            {
                                APPLY_UPDATE_ARG,
                                wfz::paths::ExecutablePath().string(),
                                parent_pid,
                            });

#else

        const std::wstring parent_pid = std::to_wstring(GetCurrentProcessId());

        return StartProcess(
            new_executable,
            {
                L"--apply-self-update",
                wfz::paths::ExecutablePath().wstring(),
                parent_pid,
            });

#endif
    }
}
