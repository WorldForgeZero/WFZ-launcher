#include "settings.h"

#include <filesystem>
#include <fstream>
#include <mutex>

#include <nlohmann/json.hpp>

#include "etc/paths.h"

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

            {"admin_bypass", settings.admin_bypass},
            {"dev_mode", settings.dev_mode},
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

            WFZLauncherSettings loaded{};

            FromJson(data, loaded);

            {
                std::lock_guard<std::mutex> lock(g_settings_mutex);
                g_settings = loaded;
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

    void Reset()
    {
        {
            std::lock_guard<std::mutex> lock(g_settings_mutex);
            g_settings = WFZLauncherSettings{};
        }

        Save();
    }
}
