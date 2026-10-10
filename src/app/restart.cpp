#include "restart.h"

#include <filesystem>
#include <string>
#include <vector>

#include "app/exit_req.h"
#include "etc/paths.h"

#ifdef _WIN32
#include <windows.h>

#include <shellapi.h>
#else
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace wfz::app
{
    namespace
    {
        bool g_was_restarted = false;
    }

    bool WasRestarted() noexcept { return g_was_restarted; }

    bool WaitForPreviousInstance(int argc, char **argv)
    {
#ifdef _WIN32
        (void)argc;
        (void)argv;
        int count = 0;
        LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &count);
        if (!args)
            return false;

        const bool requested = count > 1 && std::wstring(args[1]) == L"--wait-launcher-exit";
        if (!requested)
        {
            LocalFree(args);
            return true;
        }

        if (count != 3)
        {
            LocalFree(args);
            return false;
        }

        wchar_t *end = nullptr;
        const unsigned long value = wcstoul(args[2], &end, 10);
        const bool valid = *args[2] && *end == 0 && value != 0;
        LocalFree(args);
        if (!valid)
            return false;

        HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(value));
        if (process)
        {
            const DWORD wait = WaitForSingleObject(process, INFINITE);
            CloseHandle(process);
            g_was_restarted = (wait == WAIT_OBJECT_0);
            return g_was_restarted;
        }

        g_was_restarted = GetLastError() == ERROR_INVALID_PARAMETER;
        return g_was_restarted;
#else
        if (argc <= 1 || std::string(argv[1]) != "--wait-launcher-exit")
            return true;

        if (argc != 3)
            return false;

        char *end = nullptr;
        const long pid_value = std::strtol(argv[2], &end, 10);
        if (!*argv[2] || *end != 0 || pid_value <= 0)
            return false;

        const auto pid = static_cast<pid_t>(pid_value);
        while (kill(pid, 0) == 0 || errno == EPERM)
            usleep(100000);

        g_was_restarted = errno == ESRCH;
        return g_was_restarted;
#endif
    }

    bool Restart()
    {
        const fs::path exe = wfz::paths::ExecutablePath();
#ifdef _WIN32
        const std::wstring command = L"\"" + exe.wstring() + L"\" --wait-launcher-exit " + std::to_wstring(GetCurrentProcessId());
        std::vector<wchar_t> buffer(command.begin(), command.end());
        buffer.push_back(L'\0');
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        const auto cwd = wfz::paths::ExecutableDir().wstring();
        if (!CreateProcessW(exe.c_str(), buffer.data(), nullptr, nullptr, FALSE, 0, nullptr, cwd.c_str(), &startup, &process))
            return false;

        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
#else
        const pid_t parent_pid = getpid();
        const pid_t child = fork();
        if (child < 0)
            return false;

        if (child == 0)
        {
            const std::string pid = std::to_string(static_cast<long>(parent_pid));
            const std::string executable = exe.string();
            if (chdir(wfz::paths::ExecutableDir().c_str()) != 0)
                _exit(127);

            execl(executable.c_str(), executable.c_str(), "--wait-launcher-exit", pid.c_str(), (char *)nullptr);
            _exit(127);
        }
#endif
        RequestExit();
        return true;
    }
}
