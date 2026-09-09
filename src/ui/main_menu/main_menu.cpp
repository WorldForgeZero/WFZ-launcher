#include "main_menu.h"

#include <raylib.h>

#include "../font.h"
#include "../theme.h"

#include "../widgets/button.h"

void WFZDrawMainMenu(
    const float screen_width,
    const float screen_height,
    WFZScreen &current_screen)
{
    ClearBackground(wfz_color_background);

    // Temporary banner placeholder.
    DrawRectangle(
        0,
        0,
        static_cast<int>(screen_width),
        static_cast<int>(screen_height),
        wfz_color_banner_placeholder);

    // Banner darkening.
    DrawRectangleGradientV(
        0,
        0,
        static_cast<int>(screen_width),
        static_cast<int>(screen_height),
        wfz_color_overlay_top,
        wfz_color_overlay_bottom);

    // Logo/title area.
    WFZDrawText(
        "WORLD FORGE ZERO",
        32.0f,
        28.0f,
        40.0f,
        wfz_color_text);

    WFZDrawText(
        "Лаунчер",
        34.0f,
        78.0f,
        20.0f,
        wfz_color_text_muted);

    // Top navigation.
    constexpr float top_y = 26.0f;
    constexpr float top_height = 42.0f;
    constexpr float gap = 12.0f;

    constexpr float settings_width = 152.0f;
    constexpr float news_width = 132.0f;

    const Rectangle settings_area{
        screen_width - settings_width - 32.0f,
        top_y,
        settings_width,
        top_height};

    const Rectangle news_area{
        settings_area.x - news_width - gap,
        top_y,
        news_width,
        top_height};

    DrawRectangleRounded(
        news_area,
        0.12f,
        6,
        wfz_color_panel);

    DrawRectangleRoundedLinesEx(
        news_area,
        0.12f,
        6,
        1.0f,
        wfz_color_panel_border);

    WFZDrawTextCentered(
        "НОВОСТИ",
        news_area,
        18.0f,
        wfz_color_text);

    if (WFZButton("Настройки", settings_area, 18.0f, wfz_button_secondary))
    {
        current_screen = WFZScreen::Settings;
        return;
    }

    // Bottom launcher area.
    constexpr float left = 32.0f;
    constexpr float bottom_margin = 32.0f;

    const float bottom =
        screen_height - bottom_margin;

    WFZDrawText(
        "Готово",
        left,
        bottom - 104.0f,
        22.0f,
        wfz_color_text);

    // Progress bar.
    DrawRectangle(
        static_cast<int>(left),
        static_cast<int>(bottom - 68.0f),
        440,
        4,
        wfz_color_progress_background);

    DrawRectangle(
        static_cast<int>(left),
        static_cast<int>(bottom - 68.0f),
        440,
        4,
        wfz_color_accent);

    // Play area placeholder.
    const Rectangle play_area{
        left,
        bottom - 54.0f,
        320.0f,
        54.0f};

    DrawRectangleRounded(
        play_area,
        0.12f,
        6,
        wfz_color_accent);

    WFZDrawTextCentered(
        "ИГРАТЬ",
        play_area,
        26.0f,
        wfz_color_text_dark);
}
