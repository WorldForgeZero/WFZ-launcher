#pragma once

namespace wfz::logger
{
    enum class Level
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
    };

    bool Init();
    void Shutdown();

    void Log(Level level, const char *format, ...);

    void Trace(const char *format, ...);
    void Debug(const char *format, ...);
    void Info(const char *format, ...);
    void Warning(const char *format, ...);
    void Error(const char *format, ...);
    void Fatal(const char *format, ...);

    void DumpStartupInfo();
}
