#include "bootstrap.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "app/alerts.h"
#include "app/exit_req.h"
#include "app/loading_state.h"
#include "app/thread_manager.h"
#include "daemon/retry.h"
#include "etc/logger.h"
#include "etc/paths.h"
#include "etc/self_update.h"
#include "network/http.h"
#include "settings/settings.h"
#include "settings/version.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace
{
    std::atomic_bool g_bootstrap_scheduled{false};
    std::atomic_bool g_update_scheduled{false};

    // Keep mirror selection in one place. This can become a config later.
    const std::vector<std::string> MASTER_URLS = {
        "https://cdn.jsdelivr.net/gh/WorldForgeZero/WFZ-launcher@master/jsons/master_manifest.json",
        "https://raw.githubusercontent.com/WorldForgeZero/WFZ-launcher/refs/heads/master/jsons/master_manifest.json"};

    fs::path MasterPath() { return wfz::paths::LauncherDir() / "master_manifest.json"; }

    json ValidateMaster(const std::string &body)
    {
        auto data = json::parse(body);
        if (!data.is_object() || !data.contains("launcher") || !data["launcher"].is_object())
            throw std::runtime_error("Master manifest lacks launcher object");
        const auto &launcher = data.at("launcher");
        for (const char *field : {"latest_ver", "min_support", "base_download_url", "asset_win", "asset_linux"})
        {
            if (!launcher.contains(field) || !launcher.at(field).is_string() || launcher.at(field).get<std::string>().empty())
                throw std::runtime_error(std::string("Invalid launcher manifest field: ") + field);
        }
        // Check versions using the existing comparator.
        CompareVersions(launcher.at("latest_ver").get<std::string>(), launcher.at("min_support").get<std::string>());
        return data;
    }

    json ReadCachedMaster()
    {
        std::ifstream file(MasterPath(), std::ios::binary);
        if (!file)
            throw std::runtime_error("No cached master manifest");
        return ValidateMaster(std::string(std::istreambuf_iterator<char>(file),
                                          std::istreambuf_iterator<char>()));
    }

    json LoadMaster()
    {
        try
        {
            return wfz::daemon::WithRetry("Мастер-манифест", MASTER_URLS, [](const std::string &url)
                                          {
                const std::string body = wfz::network::GetText(url, "Мастер-манифест", 1024 * 1024);
                auto manifest = ValidateMaster(body);
                wfz::network::WriteAtomic(MasterPath(), body);
                return manifest; });
        }
        catch (const wfz::daemon::Cancelled &)
        {
            throw;
        }
        catch (const std::exception &e)
        {
            wfz::logger::Warning("Master network fetch failed: %s; checking cache", e.what());
            try
            {
                auto cached = ReadCachedMaster();
                wfz::loading_state::SetStatus("Сеть недоступна. Используется сохранённый манифест.");
                return cached;
            }
            catch (const std::exception &cache_error)
            {
                wfz::logger::Warning("Cached master manifest unavailable: %s", cache_error.what());
                return nullptr;
            }
        }
    }

    int Revision(const json &j)
    {
        if (!j.is_array() || j.size() != 1 || !j.at(0).is_number_integer())
            throw std::runtime_error("News revision must be [integer]");
        int value = j.at(0).get<int>();
        if (value < 0)
            throw std::runtime_error("Negative news revision");
        return value;
    }

    bool CachedNewsValid(const fs::path &content)
    {
        try
        {
            std::ifstream file(content, std::ios::binary);
            if (!file)
                return false;
            json j;
            file >> j;
            return j.is_array();
        }
        catch (...)
        {
            return false;
        }
    }

    int CachedRevision(const fs::path &path)
    {
        try
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
                return -1;
            json j;
            file >> j;
            return Revision(j);
        }
        catch (...)
        {
            return -1;
        }
    }

    void UpdateNews(const std::string &name, const std::string &base, const fs::path &dir)
    {
        if (base.empty())
            return;
        const std::string prefix = base.back() == '/' ? base : base + '/';
        const fs::path revision_path = dir / "news.json";
        const fs::path content_path = dir / "news_content.json";
        try
        {
            wfz::daemon::WithRetry(name + ": новости", {prefix + "news.json"}, [&](const std::string &url)
                                   {
                const std::string revision_body = wfz::network::GetText(url, name + ": ревизия", 1024);
                const int remote = Revision(json::parse(revision_body));
                if (CachedRevision(revision_path) == remote && CachedNewsValid(content_path))
                {
                    wfz::logger::Info("%s news unchanged: revision %d", name.c_str(), remote);
                    return true;
                }
                wfz::logger::Info("[%s: новости] downloading: %s", name.c_str(), (prefix + "news_content.json").c_str());
                const std::string content = wfz::network::GetText(prefix + "news_content.json", name + ": новости", 32 * 1024 * 1024);
                if (!json::parse(content).is_array())
                    throw std::runtime_error("News content must be a JSON array");
                    
                // Commit the content first. A crash before revision commit causes a retry next run.
                wfz::network::WriteAtomic(content_path, content);
                wfz::network::WriteAtomic(revision_path, revision_body);
                wfz::logger::Info("%s news updated to revision %d", name.c_str(), remote);
                return true; }, name == "Game");
        }
        catch (const wfz::daemon::Cancelled &)
        {
            throw;
        }
        catch (const std::exception &e)
        {
            wfz::logger::Warning("%s news: %s; retained local cache", name.c_str(), e.what());
        }
    }

    std::string LauncherUrl(const json &j)
    {
        const auto &launcher = j.at("launcher");
#ifdef _WIN32
        const std::string asset = launcher.at("asset_win").get<std::string>();
#else
        const std::string asset = launcher.at("asset_linux").get<std::string>();
#endif
        std::string base = launcher.at("base_download_url").get<std::string>();
        if (!base.empty() && base.back() != '/')
            base += '/';
        return base + asset;
    }

    std::string LauncherHash(const json &j)
    {
        const auto &launcher = j.at("launcher");
#ifdef _WIN32
        const char *key = "sha256_win";
#else
        const char *key = "sha256_linux";
#endif
        if (!launcher.contains(key))
            return {};
        const std::string hash = launcher.at(key).get<std::string>();
        if (hash.size() != 64 || hash.find_first_not_of("0123456789abcdef") != std::string::npos)
            throw std::runtime_error("Invalid launcher SHA-256 digest in master manifest");
        return hash;
    }

    fs::path UpdateBinaryPath()
    {
#ifdef _WIN32
        return wfz::paths::TempDir() / "wfz_launcher_update.exe";
#else
        return wfz::paths::TempDir() / "wfz_launcher_update";
#endif
    }

    void DoUpdate(const json &manifest)
    {
        const auto url = LauncherUrl(manifest);
        const auto hash = LauncherHash(manifest);
        // This build permits a missing digest for compatibility with the existing manifest.
        if (hash.empty())
            wfz::logger::Warning("Launcher update has no SHA-256 in manifest; relying on TLS");

        const fs::path target = UpdateBinaryPath();
        wfz::daemon::WithRetry("Обновление лаунчера", {url}, [&](const std::string &address)
                               {
            wfz::network::DownloadFile(address, target, "Обновление лаунчера", hash);
            return true; });
        wfz::loading_state::SetStatus("Установка обновления лаунчера...");
        if (!wfz::self_update::Begin(target))
        {
            wfz::alerts::Post({"ОШИБКА УСТАНОВКИ",
                               "Обновление скачано, но не удалось запустить замену исполняемого файла.",
                               "self_update::Begin() failed", "ПОВТОРИТЬ",
                               wfz::alerts::Action::RetryAll});
            throw std::runtime_error("Failed to start launcher updater");
        }
        wfz::app::RequestExit();
    }

    bool ProcessLauncher(const json &master)
    {
        if (master.is_null())
            return true; // Offline with no manifest: keep UI available.
        const auto &launcher = master.at("launcher");
        const auto latest = launcher.at("latest_ver").get<std::string>();
        const auto minimum = launcher.at("min_support").get<std::string>();
        const bool mandatory = CompareVersions(launcher_version, minimum) < 0;
        if (!mandatory && CompareVersions(launcher_version, latest) >= 0)
            return true;

        const auto options = wfz::settings::GetSnapshot();
        if (!mandatory && !options.check_launcher_updates)
            return true;
        if (!mandatory && !options.launcher_auto_update)
        {
            wfz::alerts::Post({"ДОСТУПНО ОБНОВЛЕНИЕ",
                               std::string("Доступна версия ") + latest + " (сейчас " + launcher_version + ").",
                               "Можно установить сейчас либо отложить.", "УСТАНОВИТЬ",
                               wfz::alerts::Action::InstallUpdate});
            return true;
        }
        try
        {
            DoUpdate(master);
            return false; // updater launch requested; current process exits.
        }
        catch (const wfz::daemon::Cancelled &)
        {
            throw;
        }
        catch (const std::exception &e)
        {
            wfz::logger::Error("Launcher update failed: %s", e.what());
            if (mandatory)
            {
                // Network failures already generated a retry popup after 3 attempts.
                return false; // Do not report an unsupported build as ready.
            }
            return true;
        }
    }

    void RunBootstrap()
    {
        wfz::loading_state::SetReady(false);
        wfz::loading_state::SetProgress(0);
        try
        {
            for (const fs::path &dir : {wfz::paths::TempDir(), wfz::paths::GameDir(), wfz::paths::LauncherDir()})
                fs::create_directories(dir);
            const json master = LoadMaster();
            if (wfz::app::ExitRequested())
                return;
            const bool usable = ProcessLauncher(master);
            if (wfz::app::ExitRequested())
                return;
            if (!usable)
            {
                wfz::loading_state::SetStatus("Обновление необходимо для продолжения.");
                return;
            }
            if (!master.is_null() && master.contains("news") && master.at("news").is_object())
            {
                const auto &news = master.at("news");
                const auto options = wfz::settings::GetSnapshot();
                if (options.load_launcher_news && news.contains("launcher_url") && news.at("launcher_url").is_string())
                    UpdateNews("Launcher", news.at("launcher_url").get<std::string>(), wfz::paths::LauncherDir());
                if (options.load_game_news && news.contains("game_url") && news.at("game_url").is_string())
                    UpdateNews("Game", news.at("game_url").get<std::string>(), wfz::paths::GameDir());
            }
            if (wfz::app::ExitRequested())
                return;
            wfz::loading_state::SetStatus("Проверки завершены. Клиент игры пока не установлен.");
            wfz::loading_state::SetProgress(1);
            wfz::loading_state::SetReady(true);
        }
        catch (const wfz::daemon::Cancelled &)
        {
        }
        catch (const std::exception &e)
        {
            wfz::logger::Error("Daemon bootstrap failed: %s", e.what());
            wfz::alerts::Post({"ОШИБКА ИНИЦИАЛИЗАЦИИ", "Не удалось завершить инициализацию лаунчера.",
                               e.what(), "ПОВТОРИТЬ", wfz::alerts::Action::RetryAll});
            wfz::loading_state::SetStatus("Ошибка инициализации.");
        }
        catch (...)
        {
            wfz::logger::Error("Daemon bootstrap failed: unknown exception");
            wfz::loading_state::SetStatus("Неизвестная ошибка инициализации.");
        }
    }

    void InstallUpdateJob()
    {
        try
        {
            DoUpdate(ReadCachedMaster());
        }
        catch (const wfz::daemon::Cancelled &)
        {
        }
        catch (const std::exception &e)
        {
            wfz::logger::Error("Manual launcher update failed: %s", e.what());
            // Network failures already generated a popup after three attempts.
            if (std::string(e.what()).find("Обновление лаунчера:") == std::string::npos)
                wfz::alerts::Post({"ОБНОВЛЕНИЕ НЕ УДАЛОСЬ", "Не удалось установить обновление.",
                                   e.what(), "ПОВТОРИТЬ", wfz::alerts::Action::RetryAll});
        }
    }
}

namespace wfz::daemon
{
    void Start()
    {
        if (g_bootstrap_scheduled.exchange(true))
            return;
        wfz::app::ThreadManager::instance().submit([]
                                                   {
            RunBootstrap();
            g_bootstrap_scheduled.store(false); });
    }

    void RetryAll() { Start(); }

    void InstallUpdate()
    {
        if (g_update_scheduled.exchange(true))
            return;
        wfz::app::ThreadManager::instance().submit([]
                                                   {
            InstallUpdateJob();
            g_update_scheduled.store(false); });
    }
}
