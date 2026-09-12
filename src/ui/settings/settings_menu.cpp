#include "settings_menu.h"

#include <cstddef>
#include <string>

#include <raylib.h>

#include "../../settings/settings.h"

#include "../font.h"
#include "../theme.h"

#include "../widgets/button.h"
#include "../widgets/checkbox.h"
#include "../widgets/scroll_area.h"
#include "../widgets/search_box.h"

enum class WFZSettingType
{
    Checkbox
};

struct WFZSettingEntry
{
    const char *title;
    const char *description;

    WFZSettingType type;
    WFZSettingId id;
};

struct WFZSettingsSection
{
    const char *title;

    WFZSettingEntry *entries;
    std::size_t entry_count;
};

static WFZSettingEntry g_news_entries[]{
    {
        "Новости лаунчера",
        "Получать новости и обновления лаунчера",
        WFZSettingType::Checkbox,
        WFZSettingId::LoadLauncherNews,
    },
    {
        "Новости игры",
        "Получать новости World Forge Zero",
        WFZSettingType::Checkbox,
        WFZSettingId::LoadGameNews,
    },
};

static WFZSettingEntry g_behavior_entries[]{
    {
        "Запуск после обновления",
        "Сразу запускать игру после завершения обновления",
        WFZSettingType::Checkbox,
        WFZSettingId::LaunchAfterUpdate,
    },
    {
        "Обновления лаунчера",
        "Проверять наличие обновлений лаунчера при запуске",
        WFZSettingType::Checkbox,
        WFZSettingId::CheckLauncherUpdates,
    },
};

static WFZSettingEntry g_advanced_entries[]{
    {
        "Dev режим",
        "Включает функции для разработки. Если вы не знаете, зачем он нужен - он вам не нужен.",
        WFZSettingType::Checkbox,
        WFZSettingId::DevMode,
    },
    {
        "Admin режим",
        "Отключает некоторые ограничения интерфейса лаунчера. Реальных прав не предоставляет.",
        WFZSettingType::Checkbox,
        WFZSettingId::AdminBypass,
    },
};

static WFZSettingsSection g_sections[]{
    {
        "НОВОСТИ",
        g_news_entries,
        std::size(g_news_entries),
    },
    {
        "ПОВЕДЕНИЕ",
        g_behavior_entries,
        std::size(g_behavior_entries),
    },
    {
        "РАСШИРЕННЫЕ",
        g_advanced_entries,
        std::size(g_advanced_entries),
    },
};

static int WFZToLowerCodepoint(int codepoint)
{
    if (codepoint >= 'A' && codepoint <= 'Z')
        return codepoint + ('a' - 'A');

    if (codepoint >= 0x0410 && codepoint <= 0x042F)
        return codepoint + 0x20;

    if (codepoint == 0x0401)
        return 0x0451;

    return codepoint;
}

static std::string WFZLowerString(const char *text)
{
    std::string result;

    if (text == nullptr)
        return result;

    int codepoint_count = 0;

    int *codepoints = LoadCodepoints(text, &codepoint_count);

    for (int i = 0; i < codepoint_count; ++i)
    {
        const int codepoint = WFZToLowerCodepoint(codepoints[i]);

        int utf8_size = 0;

        const char *utf8 = CodepointToUTF8(codepoint, &utf8_size);
        result.append(utf8, static_cast<std::size_t>(utf8_size));
    }

    UnloadCodepoints(codepoints);

    return result;
}

static bool WFZSettingMatchesSearch(const WFZSettingEntry &entry, const std::string &query)
{
    if (query.empty())
        return true;

    const std::string lowered_query = WFZLowerString(query.c_str());
    const std::string title = WFZLowerString(entry.title);
    const std::string description = WFZLowerString(entry.description);

    return title.find(lowered_query) != std::string::npos ||
           description.find(lowered_query) != std::string::npos;
}

static bool WFZSectionHasMatches(const WFZSettingsSection &section, const std::string &query)
{
    for (std::size_t i = 0; i < section.entry_count; ++i)
        if (WFZSettingMatchesSearch(section.entries[i], query))
            return true;

    return false;
}

static float WFZMeasureSettingEntryHeight(const WFZSettingEntry &entry, const float width)
{
    switch (entry.type)
    {
    case WFZSettingType::Checkbox:
    {
        constexpr float horizontal_padding = 16.0f;

        constexpr float title_size = 22.0f;

        constexpr float description_size = 17.0f;
        constexpr float description_line_gap = 5.0f;

        constexpr float title_description_gap = 12.0f;

        constexpr float top_padding = 14.0f;
        constexpr float bottom_padding = 14.0f;

        constexpr float box_size = 24.0f;
        constexpr float box_margin = 14.0f;

        const float description_width =
            width -
            horizontal_padding * 2.0f -
            box_size -
            box_margin;

        const float description_height =
            WFZMeasureTextWrappedHeight(
                entry.description,
                description_width,
                description_size,
                description_line_gap);

        return top_padding +
               title_size +
               title_description_gap +
               description_height +
               bottom_padding;
    }
    }

    return 0.0f;
}

static float WFZMeasureSectionHeight(const WFZSettingsSection &section, const std::string &query, const float width)
{
    constexpr float header_height = 62.0f;
    constexpr float row_gap = 10.0f;
    constexpr float bottom_padding = 12.0f;

    float height = header_height;
    bool first = true;

    for (std::size_t i = 0; i < section.entry_count; ++i)
    {
        const WFZSettingEntry &entry = section.entries[i];

        if (!WFZSettingMatchesSearch(entry, query))
            continue;

        if (!first)
            height += row_gap;

        height += WFZMeasureSettingEntryHeight(entry, width - 20.0f);

        first = false;
    }

    return height + bottom_padding;
}

static void WFZDrawSettingsPanel(const Rectangle bounds, const char *title)
{
    DrawRectangleRounded(
        bounds,
        0.02f,
        8,
        wfz_color_panel);

    DrawRectangleRoundedLinesEx(
        bounds,
        0.02f,
        8,
        1.0f,
        wfz_color_control_border);

    WFZDrawText(
        title,
        bounds.x + 20.0f,
        bounds.y + 16.0f,
        22.0f,
        wfz_color_text);

    DrawRectangle(
        static_cast<int>(bounds.x + 20.0f),
        static_cast<int>(bounds.y + 52.0f),
        static_cast<int>(bounds.width - 40.0f),
        1,
        wfz_color_control_border);
}

static void WFZDrawVerticalFade(const Rectangle bounds, const Color color, const bool fade_in)
{
    const int height = static_cast<int>(bounds.height);

    if (height <= 0)
        return;

    for (int i = 0; i < height; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(height);

        const float alpha =
            fade_in
                ? t
                : 1.0f - t;

        Color line_color = color;

        line_color.a = static_cast<unsigned char>(static_cast<float>(color.a) * alpha);

        DrawRectangle(
            static_cast<int>(bounds.x),
            static_cast<int>(bounds.y) + i,
            static_cast<int>(bounds.width),
            1,
            line_color);
    }
}

void WFZDrawSettings(const float screen_width, const float screen_height, WFZScreen &current_screen)
{
    static std::string search_query;
    static float scroll_offset = 0.0f;

    constexpr float left = 32.0f;
    constexpr float right = 32.0f;

    constexpr float header_height = 140.0f;
    constexpr float footer_height = 74.0f;
    constexpr float footer_fade_height = 36.0f;

    constexpr float scrollbar_gap = 14.0f;
    constexpr float scrollbar_width = 4.0f;

    constexpr float panel_gap = 18.0f;

    ClearBackground(wfz_color_background);

    // Header.
    WFZDrawText(
        "НАСТРОЙКИ",
        left,
        28.0f,
        32.0f,
        wfz_color_text);

    const Rectangle search_bounds{
        left,
        72.0f,
        screen_width - left - right,
        44.0f,
    };

    WFZSearchBox(search_bounds, search_query);

    DrawRectangle(
        static_cast<int>(left),
        128,
        static_cast<int>(screen_width - left - right),
        2,
        wfz_color_accent);

    // Content geometry.
    const float content_width =
        screen_width -
        left -
        right -
        scrollbar_gap -
        scrollbar_width;

    const float content_top = header_height;

    const float content_bottom = screen_height - footer_height;

    const float content_view_height = content_bottom - content_top;

    float content_height = 16.0f;

    bool any_matches = false;

    for (const WFZSettingsSection &section : g_sections)
    {
        if (!WFZSectionHasMatches(section, search_query))
            continue;

        any_matches = true;

        content_height += WFZMeasureSectionHeight(section, search_query, content_width);
        content_height += panel_gap;
    }

    if (any_matches)
    {
        content_height -= panel_gap;
    }
    else
    {
        content_height += 80.0f;
    }

    content_height += 16.0f;

    WFZScrollArea scroll{
        {
            left,
            content_top,
            content_width,
            content_view_height,
        },
        content_height,
        scroll_offset,
    };

    WFZUpdateScrollArea(scroll);

    scroll_offset = scroll.scroll_offset;

    // Settings.
    WFZBeginScrollArea(scroll);

    float y = content_top + 16.0f - scroll.scroll_offset;

    if (!any_matches)
    {
        WFZDrawText(
            "Ничего не найдено",
            left + 8.0f,
            y + 20.0f,
            22.0f,
            wfz_color_text_secondary);
    }
    else
    {
        for (WFZSettingsSection &section : g_sections)
        {
            if (!WFZSectionHasMatches(section, search_query))
                continue;

            const float panel_height =
                WFZMeasureSectionHeight(
                    section,
                    search_query,
                    content_width);

            const Rectangle panel{
                left,
                y,
                content_width,
                panel_height,
            };

            WFZDrawSettingsPanel(
                panel,
                section.title);

            float row_y =
                panel.y +
                62.0f;

            bool first = true;

            for (std::size_t i = 0; i < section.entry_count; ++i)
            {
                WFZSettingEntry &entry = section.entries[i];

                if (!WFZSettingMatchesSearch(entry, search_query))
                    continue;

                if (!first)
                    row_y += 10.0f;

                switch (entry.type)
                {
                case WFZSettingType::Checkbox:
                {
                    bool value = wfz::settings::GetBool(entry.id);

                    const bool previous_value = value;

                    row_y +=
                        WFZCheckboxRow(
                            entry.title,
                            entry.description,
                            {
                                panel.x + 10.0f,
                                row_y,
                                panel.width - 20.0f,
                                0.0f,
                            },
                            value);

                    if (value != previous_value)
                    {
                        wfz::settings::SetBool(entry.id, value);
                    }

                    break;
                }
                }

                first = false;
            }

            y += panel.height + panel_gap;
        }
    }

    WFZEndScrollArea();

    // Scrollbar.
    WFZDrawScrollBar(
        scroll,
        screen_width - right - scrollbar_width,
        scrollbar_width);

    // Footer.
    const float footer_y =
        screen_height -
        footer_height;

    WFZDrawVerticalFade(
        {
            0.0f,
            footer_y - footer_fade_height,
            screen_width,
            footer_fade_height,
        },
        wfz_color_background,
        true);

    DrawRectangle(
        0,
        static_cast<int>(footer_y),
        static_cast<int>(screen_width),
        static_cast<int>(footer_height),
        wfz_color_background);

    constexpr float back_width = 142.0f;
    constexpr float back_height = 44.0f;

    const Rectangle back_area{
        screen_width - right - back_width,
        screen_height - 59.0f,
        back_width,
        back_height,
    };

    if (WFZButton("НАЗАД", back_area, 20.0f, wfz_button_secondary))
    {
        current_screen = WFZScreen::MainMenu;
    }
}
