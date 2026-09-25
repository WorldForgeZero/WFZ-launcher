#include "manifest.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "app/exit_req.h"
#include "app/loading_state.h"

#include "etc/logger.h"
#include "etc/paths.h"

#include "network/download.h"

namespace fs = std::filesystem;

namespace app = wfz::app;
namespace loading = wfz::loading_state;
namespace network = wfz::network;
namespace paths = wfz::paths;
namespace logger = wfz::logger;

namespace
{
    constexpr const char *MANIFEST_URL = "https://raw.githubusercontent.com/WorldForgeZero/WFZ-launcher/refs/heads/master/jsons/master_manifest.json";

    constexpr int RETRY_DELAY_SECONDS = 10;

    bool ValidateManifest(const fs::path &path)
    {
        try
        {
            std::ifstream file(path);

            if (!file.is_open())
                return false;

            nlohmann::json manifest;
            file >> manifest;

            return manifest.is_object();
        }
        catch (...)
        {
            return false;
        }
    }

    bool WaitBeforeRetry()
    {
        for (int seconds = RETRY_DELAY_SECONDS; seconds > 0; --seconds)
        {
            loading::SetStatus("Сеть недоступна. Повторная попытка через " + std::to_string(seconds) + " сек.");

            for (int i = 0; i < 10; ++i)
            {
                if (app::ExitRequested())
                    return false;

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        return true;
    }
}

bool DownloadManifest()
{
    const fs::path target = paths::LauncherDir() / "master_manifest.json";
    const fs::path temporary = paths::TempDir() / "master_manifest.json.tmp";

    while (!app::ExitRequested())
    {
        try
        {
            loading::SetStatus("Загрузка манифеста...");

            std::error_code ec;
            fs::remove(temporary, ec);

            network::download(MANIFEST_URL, temporary);
            if (!ValidateManifest(temporary))
            {
                throw std::runtime_error("Downloaded master manifest is invalid");
            }

            fs::copy_file(temporary, target, fs::copy_options::overwrite_existing);
            fs::remove(temporary, ec);
            logger::Info("Master manifest downloaded successfully");
            return true;
        }
        catch (const std::exception &e)
        {
            logger::Warning("Failed to download master manifest: %s", e.what());
        }
        catch (...)
        {
            logger::Warning("Failed to download master manifest: unknown error");
        }

        if (!WaitBeforeRetry())
            return false;
    }

    return false;
}
