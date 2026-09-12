#include "main_menu.h"

#include <raylib.h>

#include <string>

#include "../font.h"
#include "../theme.h"

#include "../widgets/button.h"

#include "settings/version.h"

#include "network/thread_manager.h"

#include "main_menu_loading.h"

void WFZDrawMainMenu(const float screen_width, const float screen_height, WFZScreen &current_screen)
{
    ClearBackground(wfz_color_background);

    WFZUpdateLoading();

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

    const std::string launcher_label = std::string("Лаунчер v") + launcher_version;

    WFZDrawText(
        launcher_label.c_str(),
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
    constexpr float info_width = 42.0f;

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

    const Rectangle info_area{
        news_area.x - info_width - gap,
        top_y,
        info_width,
        top_height};

    if (WFZButton("Новости", news_area, 18.0f, wfz_button_secondary))
    {
        current_screen = WFZScreen::News;
        return;
    }

    if (WFZButton("Настройки", settings_area, 18.0f, wfz_button_secondary))
    {
        current_screen = WFZScreen::Settings;
        return;
    }

    if (WFZButton("I", info_area, 22.0f, wfz_button_secondary))
    {
        current_screen = WFZScreen::Info;
        return;
    }

    // Bottom launcher area.
    constexpr float left = 32.0f;
    constexpr float bottom_margin = 32.0f;

    constexpr float control_width = 320.0f;

    constexpr float progress_height = 7.0f;
    constexpr float button_height = 54.0f;

    constexpr float status_size = 22.0f;
    constexpr float status_gap = 10.0f;

    constexpr float morph_duration = 0.32f;
    constexpr float status_fade_duration = 0.28f;

    const float bottom =
        screen_height - bottom_margin;

    // Status text alpha.
    float status_alpha = 1.0f;

    if (wfz::loading_state::GetReady())
    {
        const float fade_t = WFZClamp01(wfz::loading_state::GetReadyTime() / status_fade_duration);
        status_alpha = 1.0f - fade_t;
    }

    // Current loading phase.
    if (status_alpha > 0.0f)
    {
        Color status_color = wfz_color_text;
        status_color.a = static_cast<unsigned char>(static_cast<float>(status_color.a) * status_alpha);
        WFZDrawText(wfz::loading_state::GetStatus(), left, bottom - progress_height - status_gap - status_size, status_size, status_color);
    }

    if (!wfz::loading_state::GetReady())
    {
        const Rectangle progress_background{
            left,
            bottom - progress_height,
            control_width,
            progress_height};

        DrawRectangleRounded(
            progress_background,
            0.5f,
            6,
            wfz_color_progress_background);

        Rectangle progress_value = progress_background;

        progress_value.width = control_width * wfz::loading_state::GetDisplayProgress();

        if (progress_value.width > 0.0f)
        {
            DrawRectangleRounded(
                progress_value,
                0.5f,
                6,
                wfz_color_accent);
        }
    }
    else
    {
        const float morph_t = WFZEaseOutCubic(wfz::loading_state::GetReadyTime() / morph_duration);

        const float current_height = WFZLerp(progress_height, button_height, morph_t);

        const Rectangle control_area{
            left,
            bottom - current_height,
            control_width,
            current_height};

        if (morph_t < 1.0f)
        {
            DrawRectangleRounded(control_area, 0.12f, 6, wfz_color_accent);

            float text_t = (morph_t - 0.35f) / 0.65f;

            text_t = WFZClamp01(text_t);

            Color text_color = wfz_color_text_dark;

            text_color.a = static_cast<unsigned char>(static_cast<float>(text_color.a) * text_t);

            WFZDrawTextCentered("ИГРАТЬ", control_area, 26.0f, text_color);
        }
        else
        {
            if (WFZButton("ИГРАТЬ", control_area, 26.0f, wfz_button_primary))
            {
            }
        }
    }
}
