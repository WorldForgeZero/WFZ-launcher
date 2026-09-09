#pragma once

#include <cstddef>

enum class WFZSettingType
{
    Checkbox
};

struct WFZLauncherSettings
{
    bool load_launcher_news = true;
    bool load_game_news = true;

    bool launch_after_update = false;
    bool check_launcher_updates = true;
};

struct WFZSettingEntry
{
    const char *title;
    const char *description;

    WFZSettingType type;

    bool *bool_value;
};

struct WFZSettingsSection
{
    const char *title;

    WFZSettingEntry *entries;
    std::size_t entry_count;
};
