#pragma once

#include <filesystem>
#include <string>

enum class WFZSettingId
{
    LoadLauncherNews,
    LoadGameNews,

    LaunchAfterUpdate,
    LauncherAutoUpdate,
    CheckLauncherUpdates,

    AdminBypass,
    DevMode,
};

struct WFZLauncherSettings
{
    bool load_launcher_news = true;
    bool load_game_news = true;

    bool launch_after_update = false;
    bool launcher_auto_update = true;
    bool check_launcher_updates = true;

    bool admin_bypass = false;
    bool dev_mode = false;

    std::string source_dir;

    std::string pending_cleanup_dir;
    bool pending_reset_settings = false;
};

namespace wfz::settings
{
    void Load();
    bool Save();

    bool GetBool(WFZSettingId id);
    void SetBool(WFZSettingId id, bool value);

    WFZLauncherSettings GetSnapshot();

    std::filesystem::path GetSourceDir();
    bool SetSourceDir(const std::filesystem::path &path, bool delete_old_after_restart = false);

    bool ScheduleMaintenance(const std::filesystem::path &cleanup_dir, bool reset_settings);
    bool CancelMaintenance();

    void Reset();
}
