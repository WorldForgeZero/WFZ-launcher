#include "settings.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "app/restart.h"
#include "etc/paths.h"
#include "etc/source_storage.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace
{
    std::mutex g_settings_mutex;
    WFZLauncherSettings g_settings{};

    fs::path GetSettingsPath()
    {
        return wfz::paths::ConfigDir() / "settings.json";
    }

    bool GetBoolValue(const WFZLauncherSettings &settings, const WFZSettingId id)
    {
        switch (id)
        {
        case WFZSettingId::LoadLauncherNews:
            return settings.load_launcher_news;

        case WFZSettingId::LoadGameNews:
            return settings.load_game_news;

        case WFZSettingId::LaunchAfterUpdate:
            return settings.launch_after_update;

        case WFZSettingId::LauncherAutoUpdate:
            return settings.launcher_auto_update;

        case WFZSettingId::CheckLauncherUpdates:
            return settings.check_launcher_updates;

        case WFZSettingId::AdminBypass:
            return settings.admin_bypass;

        case WFZSettingId::DevMode:
            return settings.dev_mode;
        }

        return false;
    }

    void SetBoolValue(WFZLauncherSettings &settings, const WFZSettingId id, const bool value)
    {
        switch (id)
        {
        case WFZSettingId::LoadLauncherNews:
            settings.load_launcher_news = value;
            break;

        case WFZSettingId::LoadGameNews:
            settings.load_game_news = value;
            break;

        case WFZSettingId::LaunchAfterUpdate:
            settings.launch_after_update = value;
            break;

        case WFZSettingId::LauncherAutoUpdate:
            settings.launcher_auto_update = value;
            break;

        case WFZSettingId::CheckLauncherUpdates:
            settings.check_launcher_updates = value;
            break;

        case WFZSettingId::AdminBypass:
            settings.admin_bypass = value;
            break;

        case WFZSettingId::DevMode:
            settings.dev_mode = value;
            break;
        }
    }

    json ToJson(const WFZLauncherSettings &settings)
    {
        return {
            {"load_launcher_news", settings.load_launcher_news},
            {"load_game_news", settings.load_game_news},

            {"launch_after_update", settings.launch_after_update},
            {"check_launcher_updates", settings.check_launcher_updates},
            {"launcher_auto_update", settings.launcher_auto_update},

            {"admin_bypass", settings.admin_bypass},
            {"dev_mode", settings.dev_mode},

            {"source_dir", settings.source_dir},
            {"pending_cleanup_dir", settings.pending_cleanup_dir},
            {"pending_reset_settings", settings.pending_reset_settings},
        };
    }

    void FromJson(const json &data, WFZLauncherSettings &settings)
    {
        if (data.contains("load_launcher_news"))
        {
            settings.load_launcher_news = data.at("load_launcher_news").get<bool>();
        }

        if (data.contains("load_game_news"))
        {
            settings.load_game_news = data.at("load_game_news").get<bool>();
        }

        if (data.contains("launch_after_update"))
        {
            settings.launch_after_update = data.at("launch_after_update").get<bool>();
        }

        if (data.contains("launcher_auto_update"))
        {
            settings.launcher_auto_update = data.at("launcher_auto_update").get<bool>();
        }

        if (data.contains("check_launcher_updates"))
        {
            settings.check_launcher_updates = data.at("check_launcher_updates").get<bool>();
        }

        if (data.contains("admin_bypass"))
        {
            settings.admin_bypass = data.at("admin_bypass").get<bool>();
        }

        if (data.contains("dev_mode"))
        {
            settings.dev_mode = data.at("dev_mode").get<bool>();
        }

        if (data.contains("pending_reset_settings") && data.at("pending_reset_settings").is_boolean())
            settings.pending_reset_settings = data.at("pending_reset_settings").get<bool>();

        if (data.contains("pending_cleanup_dir") && data.at("pending_cleanup_dir").is_string())
        {
            const std::string value = data.at("pending_cleanup_dir").get<std::string>();
            const fs::path path = fs::u8path(value);
            if (path.is_absolute())
                settings.pending_cleanup_dir = path.lexically_normal().u8string();
        }

        if (data.contains("source_dir") && data.at("source_dir").is_string())
        {
            const std::string source_dir = data.at("source_dir").get<std::string>();

            if (!source_dir.empty())
            {
                const fs::path path = fs::u8path(source_dir).lexically_normal();

                if (path.is_absolute())
                    settings.source_dir = path.u8string();
            }
        }
    }
}

namespace wfz::settings
{
    void Load()
    {
        fs::create_directories(wfz::paths::ConfigDir());

        const fs::path path = GetSettingsPath();

        if (!fs::exists(path))
        {
            Save();
            return;
        }

        try
        {
            std::ifstream file(path);

            if (!file)
                return;

            json data;
            file >> data;
            file.close(); // Windows cannot remove an open settings.json.

            WFZLauncherSettings loaded{};

            FromJson(data, loaded);

            const fs::path source_dir = loaded.source_dir.empty()
                                            ? wfz::paths::DefaultSourceDir()
                                            : fs::u8path(loaded.source_dir);

            wfz::paths::SetSourceDir(source_dir);

            {
                std::lock_guard<std::mutex> lock(g_settings_mutex);
                g_settings = loaded;
            }

            if (wfz::app::WasRestarted() && (!loaded.pending_cleanup_dir.empty() || loaded.pending_reset_settings))
            {
                std::string error;
                const bool cleaned = loaded.pending_cleanup_dir.empty() || wfz::source_storage::Cleanup(fs::u8path(loaded.pending_cleanup_dir), error);
                if (cleaned)
                {
                    if (loaded.pending_reset_settings)
                    {
                        {
                            std::lock_guard<std::mutex> lock(g_settings_mutex);
                            g_settings = WFZLauncherSettings{};
                        }
                        wfz::paths::SetSourceDir(wfz::paths::DefaultSourceDir());
                        std::error_code ec;
                        fs::remove(GetSettingsPath(), ec);
                        if (ec)
                            std::cerr << "Failed to reset settings: " << ec.message() << '\n';
                    }
                    else
                    {
                        {
                            std::lock_guard<std::mutex> lock(g_settings_mutex);
                            g_settings.pending_cleanup_dir.clear();
                            g_settings.pending_reset_settings = false;
                        }
                        Save();
                    }
                }
                else
                {
                    std::cerr << "Source cleanup failed: " << error << '\n';
                }
            }
        }
        catch (...)
        {
            // Keep default/current settings if the config is malformed.
        }
    }

    bool Save()
    {
        try
        {
            fs::create_directories(wfz::paths::ConfigDir());

            WFZLauncherSettings snapshot;

            {
                std::lock_guard<std::mutex> lock(g_settings_mutex);
                snapshot = g_settings;
            }

            std::ofstream file(GetSettingsPath());

            if (!file)
                return false;

            file << ToJson(snapshot).dump(4) << '\n';

            return file.good();
        }
        catch (...)
        {
            return false;
        }
    }

    bool GetBool(const WFZSettingId id)
    {
        std::lock_guard<std::mutex> lock(g_settings_mutex);
        return GetBoolValue(g_settings, id);
    }

    void SetBool(const WFZSettingId id, const bool value)
    {
        bool changed = false;

        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);

            if (GetBoolValue(g_settings, id) != value)
            {
                SetBoolValue(g_settings, id, value);
                changed = true;
            }
        }

        if (changed)
            Save();
    }

    WFZLauncherSettings GetSnapshot()
    {
        std::lock_guard<std::mutex> lock(g_settings_mutex);
        return g_settings;
    }

    fs::path GetSourceDir()
    {
        std::lock_guard<std::mutex> lock(g_settings_mutex);

        return g_settings.source_dir.empty()
                   ? wfz::paths::DefaultSourceDir()
                   : fs::u8path(g_settings.source_dir);
    }

    bool SetSourceDir(const fs::path &requested_path, bool delete_old_after_restart)
    {
        fs::path path;
        std::string serialized;

        try
        {
            if (!requested_path.empty())
            {
                path = requested_path.lexically_normal();

                if (!path.is_absolute())
                    return false;

                std::error_code ec;
                fs::create_directories(path, ec);

                if (ec || !fs::is_directory(path, ec) || ec)
                    return false;

                if (path != wfz::paths::DefaultSourceDir())
                    serialized = path.u8string();
            }
        }
        catch (...)
        {
            return false;
        }

        std::string old_value;
        std::string old_cleanup;
        bool old_reset = false;

        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);
            old_value = g_settings.source_dir;
            old_cleanup = g_settings.pending_cleanup_dir;
            old_reset = g_settings.pending_reset_settings;

            if (old_value == serialized && !delete_old_after_restart &&
                g_settings.pending_cleanup_dir.empty() && !g_settings.pending_reset_settings)
                return true;

            g_settings.source_dir = serialized;
            g_settings.pending_cleanup_dir = delete_old_after_restart
                                                 ? wfz::paths::SourceDir().u8string()
                                                 : std::string{};

            g_settings.pending_reset_settings = false;
        }

        if (Save())
            return true;

        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);
            if (g_settings.source_dir == serialized)
            {
                g_settings.source_dir = old_value;
                g_settings.pending_cleanup_dir = old_cleanup;
                g_settings.pending_reset_settings = old_reset;
            }
        }

        return false;
    }

    bool ScheduleMaintenance(const fs::path &dir, bool reset_settings)
    {
        if (!dir.empty() && !dir.is_absolute())
            return false;

        WFZLauncherSettings previous;
        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);
            previous = g_settings;
            g_settings.pending_cleanup_dir = dir.empty() ? "" : dir.lexically_normal().u8string();
            g_settings.pending_reset_settings = reset_settings;
        }
        if (Save())
            return true;

        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);
            g_settings = previous;
        }
        return false;
    }

    bool CancelMaintenance()
    {
        WFZLauncherSettings previous;
        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);
            previous = g_settings;
            g_settings.pending_cleanup_dir.clear();
            g_settings.pending_reset_settings = false;
        }

        if (Save())
            return true;

        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);
            g_settings = previous;
        }
        return false;
    }

    void Reset()
    {
        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);
            g_settings = WFZLauncherSettings{};
        }

        Save();
    }
}
