#include "news_update.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>

#include "etc/logger.h"
#include "etc/paths.h"

#include "network/download.h"
#include "network/fetch.h"

#include "settings/settings.h"

#include "app/loading_state.h"

namespace fs = std::filesystem;
using json = nlohmann::json;
namespace logger = wfz::logger;

namespace
{
    const fs::path &GetMasterManifestPath()
    {
        static const fs::path path = wfz::paths::LauncherDir() / "master_manifest.json";

        return path;
    }

    json LoadMasterManifest()
    {
        std::ifstream file(GetMasterManifestPath());

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open master manifest");
        }

        json manifest;
        file >> manifest;

        if (!manifest.is_object())
        {
            throw std::runtime_error("Invalid master manifest");
        }

        return manifest;
    }

    std::string NormalizeBaseUrl(std::string url)
    {
        if (!url.empty() && url.back() != '/')
        {
            url.push_back('/');
        }

        return url;
    }

    int GetRevision(const json &manifest)
    {
        if (!manifest.is_array() || manifest.size() != 1 || !manifest[0].is_number_integer())
        {
            throw std::runtime_error("Invalid news manifest");
        }

        return manifest[0].get<int>();
    }

    std::optional<int> LoadLocalRevision(const fs::path &path)
    {
        try
        {
            std::ifstream file(path);

            if (!file.is_open())
                return std::nullopt;

            json manifest;
            file >> manifest;

            return GetRevision(manifest);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    void SaveLocalManifest(const fs::path &path, const json &manifest)
    {
        std::ofstream file(path, std::ios::out | std::ios::trunc);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open local news manifest");
        }

        file << manifest.dump() << '\n';

        if (!file.good())
        {
            throw std::runtime_error("Failed to write local news manifest");
        }
    }

    bool ValidateNewsContent(const fs::path &path)
    {
        try
        {
            std::ifstream file(path);

            if (!file.is_open())
                return false;

            json content;
            file >> content;

            return content.is_array();
        }
        catch (...)
        {
            return false;
        }
    }

    void CommitDownloadedFile(const fs::path &temporary, const fs::path &destination)
    {
        std::error_code ec;

        fs::rename(temporary, destination, ec);

        if (!ec)
            return;

        ec.clear();

        fs::remove(destination, ec);

        ec.clear();

        fs::rename(temporary, destination, ec);

        if (ec)
        {
            throw std::runtime_error("Failed to install downloaded news: " + ec.message());
        }
    }

    void UpdateNewsSource(const char *name, const std::string &base_url, const fs::path &directory)
    {
        const std::string normalized_url = NormalizeBaseUrl(base_url);

        if (normalized_url.empty())
        {
            throw std::runtime_error("News URL is empty");
        }

        const std::string manifest_url = normalized_url + "news.json";
        const std::string content_url = normalized_url + "news_content.json";

        const fs::path manifest_path = directory / "news.json";
        const fs::path content_path = directory / "news_content.json";

        const fs::path temporary_path = directory / "news_content.json.next";

        const json remote_manifest = wfz::network::fetch_json(manifest_url);

        const int remote_revision = GetRevision(remote_manifest);

        const std::optional<int> local_revision = LoadLocalRevision(manifest_path);

        if (local_revision && *local_revision == remote_revision && fs::exists(content_path))
        {
            logger::Info("%s news are up to date: revision %d", name, remote_revision);

            return;
        }

        fs::create_directories(directory);

        std::error_code ec;

        fs::remove(temporary_path, ec);

        try
        {
            logger::Info("Downloading %s news: revision %d", name, remote_revision);
            wfz::network::download(content_url, temporary_path);

            if (!ValidateNewsContent(temporary_path))
            {
                throw std::runtime_error("Downloaded news content is invalid");
            }

            CommitDownloadedFile(temporary_path, content_path);
            SaveLocalManifest(manifest_path, remote_manifest);

            logger::Info("%s news updated successfully: revision %d", name, remote_revision);
        }
        catch (...)
        {
            ec.clear();

            fs::remove(temporary_path, ec);

            throw;
        }
    }

    void TryUpdateNewsSource(const char *name, const json &news_manifest, const char *url_key, const fs::path &directory)
    {
        if (!news_manifest.contains(url_key) || !news_manifest[url_key].is_string())
        {
            logger::Info("%s news source is not configured", name);

            return;
        }

        const std::string base_url = news_manifest[url_key].get<std::string>();

        if (base_url.empty())
        {
            logger::Info("%s news source is not configured", name);

            return;
        }

        try
        {
            UpdateNewsSource(name, base_url, directory);
        }
        catch (const std::exception &e)
        {
            logger::Info("%s news are unavailable: %s", name, e.what());
        }
        catch (...)
        {
            logger::Info("%s news are unavailable", name);
        }
    }
}

void ProcessNewsUpdates()
{
    const WFZLauncherSettings settings = wfz::settings::GetSnapshot();

    if (!settings.load_launcher_news && !settings.load_game_news)
    {
        logger::Info("News updates are disabled in settings");

        return;
    }

    json master_manifest;

    try
    {
        master_manifest = LoadMasterManifest();
    }
    catch (const std::exception &e)
    {
        logger::Warning("Failed to load master manifest for news: %s", e.what());

        return;
    }
    catch (...)
    {
        logger::Warning("Failed to load master manifest for news");

        return;
    }

    if (!master_manifest.contains("news") || !master_manifest["news"].is_object())
    {
        logger::Info("News sources are not configured");

        return;
    }

    const json &news = master_manifest["news"];

    if (settings.load_launcher_news)
    {
        wfz::loading_state::SetStatus("Обновление новостей лаунчера...");

        TryUpdateNewsSource(
            "Launcher",
            news,
            "launcher_url",
            wfz::paths::LauncherDir() / "news" / "launcher");
    }
    else
    {
        logger::Info("Launcher news updates are disabled in settings");
    }

    if (settings.load_game_news)
    {
        wfz::loading_state::SetStatus("Обновление новостей игры...");

        TryUpdateNewsSource(
            "Game",
            news,
            "game_url",
            wfz::paths::LauncherDir() / "news" / "game");
    }
    else
    {
        logger::Info("Game news updates are disabled in settings");
    }
}
