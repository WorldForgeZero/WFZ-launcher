#include "bootstrap.h"

#include <filesystem>

#include "app/loading_state.h"

#include "etc/paths.h"

#include "launcher/launcher_update.h"
#include "manifest/manifest.h"
#include "news/news_update.h"

namespace loading = wfz::loading_state;
namespace paths = wfz::paths;

namespace fs = std::filesystem;

namespace
{
    void SetUpDirs()
    {
        loading::SetStatus("Инициализация...");

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
    SetUpDirs();

    if (!DownloadManifest())
        return;

    if (!ProcessLauncherUpdate())
        return;

    ProcessNewsUpdates();

    loading::SetStatus("Увы это dev билд лаунчера. Он может только сам обновиться и всё.");
}
