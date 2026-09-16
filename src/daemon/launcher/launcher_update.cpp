#include "launcher_update.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>
#include <raylib.h>

#include "app/exit_req.h"

#include "etc/paths.h"
#include "etc/self_update.h"

#include "network/download.h"

#include "settings/settings.h"
#include "settings/version.h"

#include "ui/loading/loading_state.h"

namespace fs = std::filesystem;

namespace app = wfz::app;
namespace loading = wfz::loading_state;
namespace network = wfz::network;
namespace paths = wfz::paths;

namespace
{
    nlohmann::json LoadMasterManifest()
    {
        const fs::path path = paths::LauncherDir() / "master_manifest.json";

        std::ifstream file(path);

        if (!file.is_open())
            throw std::runtime_error("Failed to open master manifest");

        nlohmann::json manifest;
        file >> manifest;

        return manifest;
    }

    std::string GetLauncherDownloadUrl(const nlohmann::json &launcher)
    {
        const std::string base_url = launcher.at("base_download_url").get<std::string>();

#ifdef _WIN32
        return base_url + launcher.at("asset_win").get<std::string>();
#else
        return base_url + launcher.at("asset_linux").get<std::string>();
#endif
    }

    fs::path GetLauncherUpdatePath()
    {
#ifdef _WIN32
        return paths::TempDir() / "wfz_launcher_update.exe";
#else
        return paths::TempDir() / "wfz_launcher_update";
#endif
    }

    void StartLauncherUpdate(
        const nlohmann::json &launcher)
    {
        const std::string download_url = GetLauncherDownloadUrl(launcher);

        const fs::path update_path = GetLauncherUpdatePath();

        std::error_code ec;
        fs::remove(update_path, ec);

        loading::SetStatus("Загрузка обновления лаунчера...");

        network::download(download_url, update_path);

        loading::SetStatus("Установка обновления лаунчера...");

        if (!wfz::self_update::Begin(update_path))
        {
            throw std::runtime_error("Failed to start launcher self-update");
        }

        TraceLog(LOG_INFO, "Launcher self-update started");

        app::RequestExit();
    }
}

bool ProcessLauncherUpdate()
{
    loading::SetStatus("Проверка обновлений лаунчера...");

    const nlohmann::json manifest = LoadMasterManifest();

    const auto &launcher = manifest.at("launcher");
    const std::string latest_version = launcher.at("latest_ver").get<std::string>();
    const std::string minimum_version = launcher.at("min_support").get<std::string>();

    if (CompareVersions(launcher_version, latest_version) >= 0)
    {
        TraceLog(LOG_INFO, "Launcher is up to date: %s", launcher_version);

        return true;
    }

    const bool mandatory_update = CompareVersions(launcher_version, minimum_version) < 0;
    const auto settings = wfz::settings::GetSnapshot();
    if (mandatory_update)
    {
        TraceLog(LOG_WARNING, "Launcher version %s is below minimum supported version %s", launcher_version, minimum_version.c_str());

        StartLauncherUpdate(launcher);

        return false;
    }

    if (!settings.check_launcher_updates)
    {
        TraceLog(LOG_INFO, "Launcher update available, but update checks are disabled");

        return true;
    }

    if (!settings.launcher_auto_update)
    {
        TraceLog(LOG_INFO, "Launcher update available: %s -> %s", launcher_version, latest_version.c_str());

        // TODO:
        // Send update information to UI popup.

        return true;
    }

    TraceLog(LOG_INFO, "Updating launcher: %s -> %s", launcher_version, latest_version.c_str());

    StartLauncherUpdate(launcher);

    return false;
}
