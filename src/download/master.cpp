#include "master.h"

#include "network/download.h"
#include "network/fetch.h"

#include "ui/loading/loading_state.h"

#include "etc/paths.h"

namespace wfzp = wfz::paths;
namespace wfzl = wfz::loading_state;

const static char *manifest_url = "https://raw.githubusercontent.com/WorldForgeZero/WFZ-launcher/refs/heads/master/jsons/master_manifest.json";

void SetUpDirs()
{
    // Ересь
    std::filesystem::create_directories(wfzp::TempDir());
    std::filesystem::create_directories(wfzp::ConfigDir());
    std::filesystem::create_directories(wfzp::GameDir());
    std::filesystem::create_directories(wfzp::LauncherDir());
    std::filesystem::create_directories(wfzp::LogsDir());
}

void DoMagic()
{
    // Функция занимается всей бекграунд задачей вокруг обновления, закачки, и прочего порнографического.
    // TODO: Назвать функцию адекватно. Может быть. Как и модуль а не просто master.

    wfzl::SetStatus("Настройка деррикторий лаунчера...");
    SetUpDirs();

    wfzl::SetStatus("Загрузка манифеста лаунчера...");
    wfz::network::download(manifest_url, (wfzp::TempDir() / "manifest.json"));

    wfzl::SetStatus("Занимаюсь магией...");
    // do magic stuff

    return;
}
