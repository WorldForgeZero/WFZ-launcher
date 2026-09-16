#include "bootstrap.h"

#include <filesystem>

#include "ui/loading/loading_state.h"

#include "etc/paths.h"

namespace loading = wfz::loading_state;
namespace paths = wfz::paths;

namespace fs = std::filesystem;

namespace
{
    void SetUpDirs()
    {
        // Ересь
        fs::create_directories(paths::TempDir());
        fs::create_directories(paths::ConfigDir());
        fs::create_directories(paths::GameDir());
        fs::create_directories(paths::LauncherDir());
        fs::create_directories(paths::LogsDir());
    }
}

void RunDaemonBootstrap()
{
    loading::SetStatus("Инициализация...");

    SetUpDirs();
}
