#pragma once

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
};

namespace wfz::settings
{
    void Load();
    bool Save();

    bool GetBool(WFZSettingId id);
    void SetBool(WFZSettingId id, bool value);

    WFZLauncherSettings GetSnapshot();

    void Reset();
}
