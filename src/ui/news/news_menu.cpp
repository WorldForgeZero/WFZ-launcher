#include "news_menu.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <raylib.h>

#include "etc/paths.h"

#include "settings/settings.h"

#include "news_data.h"

#include "../font.h"
#include "../theme.h"

#include "../widgets/button.h"
#include "../widgets/scroll_area.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace
{
    using WFZNewsKind = wfz::news::Kind;
    using WFZNewsItem = wfz::news::Item;
    using WFZNewsAuthor = wfz::news::Author;
    using WFZNewsDay = wfz::news::Day;

    enum class WFZNewsSource
    {
        Launcher,
        Game,
    };

    struct WFZNewsReadMarker
    {
        std::string date;
        std::string hash;
    };

    struct WFZNewsState
    {
        std::vector<WFZNewsDay> days;
        std::vector<float> day_heights;

        fs::file_time_type last_write_time{};

        WFZNewsReadMarker read_marker;

        double next_file_check = 0.0;

        float scroll_offset = 0.0f;

        float layout_width = -1.0f;
        float layout_height = 0.0f;

        bool layout_dirty = true;

        bool loaded = false;
        bool file_exists = false;
    };

    constexpr double NEWS_FILE_CHECK_INTERVAL = 1.0;

    constexpr float NEWS_TOP_PADDING = 18.0f;
    constexpr float NEWS_BOTTOM_PADDING = 24.0f;

    WFZNewsSource g_selected_source = WFZNewsSource::Launcher;

    bool g_read_state_loaded = false;

    const fs::path &GetLauncherNewsPath()
    {
        static const fs::path path = wfz::paths::LauncherDir() / "news" / "launcher" / "news_content.json";
        return path;
    }

    const fs::path &GetGameNewsPath()
    {
        static const fs::path path = wfz::paths::LauncherDir() / "news" / "game" / "news_content.json";
        return path;
    }

    const fs::path &GetReadStatePath()
    {
        static const fs::path path = wfz::paths::LauncherDir() / "news" / "read_state.json";
        return path;
    }
    // endregion

    // region state
    WFZNewsState &GetNewsState(const WFZNewsSource source)
    {
        static WFZNewsState launcher;
        static WFZNewsState game;

        switch (source)
        {
        case WFZNewsSource::Launcher:
            return launcher;

        case WFZNewsSource::Game:
            return game;
        }

        return launcher;
    }

    const fs::path &GetNewsPath(const WFZNewsSource source)
    {
        switch (source)
        {
        case WFZNewsSource::Launcher:
            return GetLauncherNewsPath();

        case WFZNewsSource::Game:
            return GetGameNewsPath();
        }

        return GetLauncherNewsPath();
    }

    const char *GetSourceName(const WFZNewsSource source)
    {
        switch (source)
        {
        case WFZNewsSource::Launcher:
            return "ЛАУНЧЕР";

        case WFZNewsSource::Game:
            return "ИГРА";
        }

        return "";
    }

    const char *GetSourceStateKey(const WFZNewsSource source)
    {
        switch (source)
        {
        case WFZNewsSource::Launcher:
            return "launcher";

        case WFZNewsSource::Game:
            return "game";
        }

        return "";
    }
    // endregion

    // region utils
    std::string FormatDate(const std::string &date)
    {
        if (date.size() != 10 || date[4] != '-' || date[7] != '-')
            return date;

        return date.substr(8, 2) + "." + date.substr(5, 2) + "." + date.substr(0, 4);
    }

    const char *GetKindLabel(const WFZNewsKind kind)
    {
        switch (kind)
        {
        case WFZNewsKind::Add:
            return "ADD";

        case WFZNewsKind::Remove:
            return "REM";

        case WFZNewsKind::Fix:
            return "FIX";

        case WFZNewsKind::Tweak:
            return "TW";
        }

        return "?";
    }
    // endregion

    // region read state
    void LoadReadMarker(const json &root, const WFZNewsSource source)
    {
        const char *key = GetSourceStateKey(source);

        if (!root.contains(key) || !root[key].is_object())
            return;

        const json &entry = root[key];

        WFZNewsReadMarker &marker = GetNewsState(source).read_marker;

        if (entry.contains("date") && entry["date"].is_string())
        {
            marker.date = entry["date"].get<std::string>();
        }

        if (entry.contains("hash") && entry["hash"].is_string())
        {
            marker.hash = entry["hash"].get<std::string>();
        }
    }

    void LoadReadState()
    {
        if (g_read_state_loaded)
            return;

        g_read_state_loaded = true;

        std::ifstream file(GetReadStatePath());

        if (!file.is_open())
            return;

        try
        {
            json root;
            file >> root;

            if (!root.is_object())
                return;

            LoadReadMarker(root, WFZNewsSource::Launcher);

            LoadReadMarker(root, WFZNewsSource::Game);
        }
        catch (...)
        {
            // Broken read state is non-critical.
        }
    }

    json MakeReadMarkerJson(const WFZNewsReadMarker &marker)
    {
        return {
            {"date", marker.date},
            {"hash", marker.hash},
        };
    }

    void SaveReadState()
    {
        const fs::path &path = GetReadStatePath();

        std::error_code ec;

        fs::create_directories(path.parent_path(), ec);

        if (ec)
            return;

        const json root{
            {
                "launcher",
                MakeReadMarkerJson(GetNewsState(WFZNewsSource::Launcher).read_marker),
            },
            {
                "game",
                MakeReadMarkerJson(GetNewsState(WFZNewsSource::Game).read_marker),
            },
        };

        const fs::path tmp_path = path.string() + ".tmp";

        try
        {
            {
                std::ofstream file(tmp_path);

                if (!file.is_open())
                    return;

                file << root.dump(4) << '\n';

                if (!file.good())
                    return;
            }

            fs::rename(tmp_path, path, ec);

            if (!ec)
                return;

            ec.clear();

            fs::remove(path, ec);

            ec.clear();

            fs::rename(tmp_path, path, ec);
        }
        catch (...)
        {
            // Read state is non-critical.
        }
    }

    bool IsDayUnread(const WFZNewsState &state, const WFZNewsDay &day)
    {
        const WFZNewsReadMarker &marker = state.read_marker;

        if (marker.date.empty())
            return true;

        if (day.date > marker.date)
            return true;

        if (day.date < marker.date)
            return false;

        return day.hash != marker.hash;
    }

    bool HasUnreadNews(const WFZNewsState &state)
    {
        for (const WFZNewsDay &day : state.days)
            if (IsDayUnread(state, day))
                return true;

        return false;
    }

    void MarkDayRead(const WFZNewsSource source, const WFZNewsDay &day)
    {
        WFZNewsState &state = GetNewsState(source);

        WFZNewsReadMarker &marker = state.read_marker;

        if (!marker.date.empty() && day.date < marker.date)
            return;

        if (marker.date == day.date && marker.hash == day.hash)
            return;

        marker.date = day.date;
        marker.hash = day.hash;

        SaveReadState();
    }
    // endregion

    // region loading
    bool TryLoadNews(const WFZNewsSource source)
    {
        auto parsed = wfz::news::Load(GetNewsPath(source));

        if (!parsed)
            return false;

        WFZNewsState &state = GetNewsState(source);

        state.days = std::move(*parsed);

        state.loaded = true;
        state.layout_dirty = true;

        return true;
    }

    void UpdateNewsFile(const WFZNewsSource source)
    {
        WFZNewsState &state = GetNewsState(source);

        const double now = GetTime();

        if (now < state.next_file_check)
            return;

        state.next_file_check = now + NEWS_FILE_CHECK_INTERVAL;

        const fs::path &path = GetNewsPath(source);

        std::error_code ec;

        const bool exists = fs::exists(path, ec);

        if (ec || !exists)
        {
            state.file_exists = false;
            return;
        }

        state.file_exists = true;

        const fs::file_time_type write_time = fs::last_write_time(path, ec);

        if (ec)
            return;

        if (state.loaded && write_time == state.last_write_time)
            return;

        if (!TryLoadNews(source))
            return;

        state.last_write_time = write_time;
    }
    // endregion

    // region tabs
    bool DrawNewsTab(const Rectangle area, const char *text, const bool active, const bool unread)
    {
        const Vector2 mouse = GetMousePosition();

        const bool hovered = CheckCollisionPointRec(mouse, area);

        if (active || hovered)
        {
            DrawRectangleRounded(area, 0.08f, 6, wfz_color_panel);
        }

        const Vector2 text_size = WFZMeasureText(text, 18.0f);

        const float text_x = area.x + (area.width - text_size.x) * 0.5f;

        const float text_y = area.y + (area.height - text_size.y) * 0.5f;

        WFZDrawText(
            text,
            text_x,
            text_y,
            18.0f,
            active
                ? wfz_color_text
                : wfz_color_text_secondary);

        if (unread)
        {
            DrawCircleV(
                {
                    area.x + area.width - 12.0f,
                    area.y + 11.0f,
                },
                3.5f,
                wfz_color_accent);
        }

        if (active)
        {
            DrawRectangle(
                static_cast<int>(area.x),
                static_cast<int>(area.y + area.height - 2.0f),
                static_cast<int>(area.width),
                2,
                wfz_color_accent);
        }

        return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }
    // endregion

    // region measuring
    float MeasureNewsItem(const WFZNewsItem &item, const float width)
    {
        constexpr float badge_width = 52.0f;
        constexpr float badge_gap = 12.0f;
        constexpr float font_size = 18.0f;
        constexpr float line_gap = 5.0f;
        constexpr float min_height = 24.0f;
        constexpr float bottom_gap = 8.0f;
        const float text_width = width - badge_width - badge_gap;

        const float text_height = WFZMeasureTextWrappedHeight(item.text.c_str(), text_width, font_size, line_gap);

        return std::max(min_height, text_height) + bottom_gap;
    }

    float MeasureAuthor(const WFZNewsAuthor &author, const float width)
    {
        constexpr float author_height = 30.0f;
        constexpr float bottom_gap = 12.0f;
        float height = author_height;

        for (const WFZNewsItem &item : author.items)
        {
            height += MeasureNewsItem(item, width);
        }

        return height + bottom_gap;
    }

    float MeasureDay(const WFZNewsDay &day, const float width)
    {
        constexpr float date_height = 34.0f;
        constexpr float separator_gap = 14.0f;
        constexpr float bottom_gap = 26.0f;
        float height = date_height + separator_gap;

        for (const WFZNewsAuthor &author : day.authors)
        {
            height += MeasureAuthor(author, width);
        }

        return height + bottom_gap;
    }

    void UpdateNewsLayout(WFZNewsState &state, const float width)
    {
        if (!state.layout_dirty && state.layout_width == width)
        {
            return;
        }

        state.day_heights.clear();

        state.day_heights.reserve(state.days.size());

        state.layout_height = NEWS_TOP_PADDING + NEWS_BOTTOM_PADDING;

        for (const WFZNewsDay &day : state.days)
        {
            const float height = MeasureDay(day, width);

            state.day_heights.push_back(height);

            state.layout_height += height;
        }

        state.layout_width = width;

        state.layout_dirty = false;
    }
    // endregion

    // region drawing
    void DrawKindBadge(const WFZNewsKind kind, const float x, const float y)
    {
        constexpr float width = 52.0f;
        constexpr float height = 24.0f;
        const Rectangle area{
            x,
            y,
            width,
            height,
        };

        DrawRectangleRounded(
            area,
            0.12f,
            4,
            wfz_color_panel);

        DrawRectangleRoundedLinesEx(
            area,
            0.12f,
            4,
            1.0f,
            wfz_color_panel_border);

        const char *label = GetKindLabel(kind);

        const Vector2 text_size = WFZMeasureText(label, 14.0f);

        WFZDrawText(
            label,
            x + (width - text_size.x) * 0.5f,
            y + (height - text_size.y) * 0.5f,
            14.0f,
            wfz_color_accent);
    }

    float DrawNewsItem(const WFZNewsItem &item, const float x, const float y, const float width)
    {
        constexpr float badge_width = 52.0f;
        constexpr float badge_gap = 12.0f;
        constexpr float font_size = 18.0f;
        constexpr float line_gap = 5.0f;
        constexpr float min_height = 24.0f;
        constexpr float bottom_gap = 8.0f;

        DrawKindBadge(
            item.kind,
            x,
            y);

        const float text_x = x + badge_width + badge_gap;
        const float text_width = width - badge_width - badge_gap;

        const float text_height =
            WFZDrawTextWrapped(
                item.text.c_str(),
                {
                    text_x,
                    y + 1.0f,
                    text_width,
                    0.0f,
                },
                font_size,
                line_gap,
                wfz_color_text_secondary);

        return std::max(min_height, text_height) + bottom_gap;
    }

    float DrawAuthor(const WFZNewsAuthor &author, const float x, float y, const float width)
    {
        WFZDrawText(
            author.name.c_str(),
            x,
            y,
            19.0f,
            wfz_color_text);

        y += 30.0f;

        for (const WFZNewsItem &item : author.items)
        {
            y += DrawNewsItem(item, x, y, width);
        }

        return y + 12.0f;
    }

    void DrawDay(const WFZNewsDay &day, const bool unread, const float x, const float y, const float width)
    {
        const std::string date = FormatDate(day.date);

        WFZDrawText(
            date.c_str(),
            x,
            y,
            21.0f,
            wfz_color_text);

        if (unread)
        {
            const Vector2 date_size = WFZMeasureText(date.c_str(), 21.0f);

            DrawCircleV(
                {
                    x + date_size.x + 13.0f,
                    y + 10.0f,
                },
                4.0f,
                wfz_color_accent);
        }

        const float line_y = y + 32.0f;

        DrawRectangle(
            static_cast<int>(x),
            static_cast<int>(line_y),
            static_cast<int>(width),
            1,
            unread
                ? wfz_color_accent
                : wfz_color_panel_border);

        float author_y = line_y + 14.0f;

        for (const WFZNewsAuthor &author : day.authors)
        {
            author_y = DrawAuthor(author, x, author_y, width);
        }
    }
    // endregion
}

void WFZDrawNews(const float screen_width, const float screen_height, WFZScreen &current_screen)
{
    LoadReadState();

    const WFZLauncherSettings settings = wfz::settings::GetSnapshot();

    const bool launcher_enabled = settings.load_launcher_news;
    const bool game_enabled = settings.load_game_news;

    if (launcher_enabled)
    {
        UpdateNewsFile(WFZNewsSource::Launcher);
    }

    if (game_enabled)
    {
        UpdateNewsFile(WFZNewsSource::Game);
    }

    WFZNewsState &launcher_state = GetNewsState(WFZNewsSource::Launcher);
    WFZNewsState &game_state = GetNewsState(WFZNewsSource::Game);

    ClearBackground(wfz_color_background);

    constexpr float left = 28.0f;
    constexpr float right = 32.0f;
    constexpr float title_y = 22.0f;
    constexpr float tabs_y = 66.0f;
    constexpr float tab_width = 160.0f;
    constexpr float tab_height = 42.0f;
    constexpr float tab_gap = 8.0f;
    constexpr float header_line_y = 120.0f;
    constexpr float content_top = 136.0f;
    constexpr float footer_height = 74.0f;
    constexpr float scrollbar_gap = 14.0f;
    constexpr float scrollbar_width = 4.0f;
    constexpr float content_inner_padding = 4.0f;

    WFZDrawText("НОВОСТИ", left, title_y, 30.0f, wfz_color_text);

    const Rectangle launcher_tab{
        left,
        tabs_y,
        tab_width,
        tab_height,
    };

    const Rectangle game_tab{
        left + tab_width + tab_gap,
        tabs_y,
        tab_width,
        tab_height,
    };

    if (DrawNewsTab(
            launcher_tab,
            GetSourceName(WFZNewsSource::Launcher),
            g_selected_source == WFZNewsSource::Launcher,
            launcher_enabled && HasUnreadNews(launcher_state)))
    {
        g_selected_source = WFZNewsSource::Launcher;
    }

    if (DrawNewsTab(
            game_tab,
            GetSourceName(WFZNewsSource::Game),
            g_selected_source == WFZNewsSource::Game,
            game_enabled && HasUnreadNews(game_state)))
    {
        g_selected_source = WFZNewsSource::Game;
    }

    WFZNewsState &current_state = GetNewsState(g_selected_source);

    const bool current_enabled =
        g_selected_source == WFZNewsSource::Launcher
            ? launcher_enabled
            : game_enabled;

    DrawRectangle(
        static_cast<int>(left),
        static_cast<int>(header_line_y),
        static_cast<int>(screen_width - left - right),
        1,
        wfz_color_panel_border);

    const float footer_y = screen_height - footer_height;
    const float content_bottom = footer_y;

    const float scroll_width = screen_width - left - right - scrollbar_gap - scrollbar_width;
    const float content_height = content_bottom - content_top;
    const float news_x = left + content_inner_padding;
    const float news_width = scroll_width - content_inner_padding * 2.0f;

    if (!current_enabled)
    {
        const char *message = "НОВОСТИ ДЛЯ ЭТОЙ КАТЕГОРИИ ОТКЛЮЧЕНЫ В НАСТРОЙКАХ";

        const Vector2 text_size = WFZMeasureText(message, 20.0f);

        WFZDrawText(
            message,
            left + (scroll_width - text_size.x) * 0.5f,
            content_top + (content_height - text_size.y) * 0.5f,
            20.0f,
            wfz_color_text_muted);
    }
    else if (!current_state.loaded)
    {
        const char *message =
            current_state.file_exists
                ? "ЧТЕНИЕ НОВОСТЕЙ..."
                : "ОЖИДАНИЕ НОВОСТЕЙ...";

        const Vector2 text_size = WFZMeasureText(message, 20.0f);

        WFZDrawText(
            message,
            left + (scroll_width - text_size.x) * 0.5f,
            content_top + (content_height - text_size.y) * 0.5f,
            20.0f,
            wfz_color_text_muted);
    }
    else if (current_state.days.empty())
    {
        const char *message = "НОВОСТЕЙ ПОКА НЕТ";
        const Vector2 text_size = WFZMeasureText(message, 20.0f);

        WFZDrawText(
            message,
            left + (scroll_width - text_size.x) * 0.5f,
            content_top + (content_height - text_size.y) * 0.5f,
            20.0f,
            wfz_color_text_muted);
    }
    else
    {
        UpdateNewsLayout(current_state, news_width);

        WFZScrollArea scroll{
            {
                left,
                content_top,
                scroll_width,
                content_height,
            },
            current_state.layout_height,
            current_state.scroll_offset,
        };

        WFZUpdateScrollArea(scroll);

        current_state.scroll_offset = scroll.scroll_offset;

        WFZBeginScrollArea(scroll);

        float y = content_top + NEWS_TOP_PADDING - scroll.scroll_offset;

        for (std::size_t i = 0; i < current_state.days.size(); ++i)
        {
            const WFZNewsDay &day = current_state.days[i];
            const float day_height = current_state.day_heights[i];
            const float day_top = y;
            const float day_bottom = y + day_height;

            const bool visible =
                day_bottom >= content_top &&
                day_top <= content_bottom;

            if (visible)
            {
                const bool unread = IsDayUnread(current_state, day);

                DrawDay(
                    day,
                    unread,
                    news_x,
                    y,
                    news_width);

                if (unread &&
                    day_bottom >= content_top &&
                    day_bottom <= content_bottom)
                {
                    MarkDayRead(g_selected_source, day);
                }
            }

            y += day_height;
        }

        WFZEndScrollArea();

        WFZDrawScrollBar(
            scroll,
            screen_width - right - scrollbar_width,
            scrollbar_width);
    }

    // Footer.
    DrawRectangle(
        0,
        static_cast<int>(footer_y),
        static_cast<int>(screen_width),
        static_cast<int>(footer_height),
        wfz_color_background);

    DrawRectangle(
        static_cast<int>(left),
        static_cast<int>(footer_y),
        static_cast<int>(screen_width - left - right),
        1,
        wfz_color_panel_border);

    const Rectangle back_area{
        left,
        footer_y + 14.0f,
        140.0f,
        46.0f,
    };

    if (WFZButton("НАЗАД", back_area, 18.0f, wfz_button_secondary))
    {
        current_screen = WFZScreen::MainMenu;
        return;
    }
}
