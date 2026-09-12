#include "info_menu.h"

#include <cstddef>
#include <string>

#include <raylib.h>

#include "../font.h"
#include "../theme.h"

#include "../widgets/button.h"
#include "../widgets/scroll_area.h"

#include "settings/version.h"

namespace
{
    struct WFZThirdPartyEntry
    {
        const char *name;
        const char *version;
        const char *license;
    };

    static constexpr WFZThirdPartyEntry g_third_party[]{
        {"raylib", "6.0", "zlib/libpng"},
        {"OpenSSL", "3.5.8", "Apache-2.0"},
        {"cpp-httplib", "", "MIT"},
        {"nlohmann/json", "", "MIT"},
        {"Monocraft", "4.2.1", "OFL-1.1"}};

    void WFZDrawInfoSectionTitle(const char *text, const float x, const float y)
    {
        WFZDrawText(text, x, y, 20.0f, wfz_color_accent);
    }
}

void WFZDrawInfo(const float screen_width, const float screen_height, WFZScreen &current_screen)
{
    static float scroll_offset = 0.0f;

    ClearBackground(wfz_color_background);

    constexpr float left = 32.0f;
    constexpr float right = 32.0f;

    constexpr float header_height = 92.0f;
    constexpr float footer_height = 74.0f;

    constexpr float scrollbar_gap = 14.0f;
    constexpr float scrollbar_width = 4.0f;

    // Header.
    WFZDrawText(
        "ИНФОРМАЦИЯ",
        left,
        28.0f,
        32.0f,
        wfz_color_text);

    DrawRectangle(
        static_cast<int>(left),
        72,
        static_cast<int>(screen_width - left - right),
        2,
        wfz_color_accent);

    // Content geometry.
    const float content_top = header_height;
    const float content_bottom = screen_height - footer_height;
    const float content_width = screen_width - left - right - scrollbar_gap - scrollbar_width;
    const float content_view_height = content_bottom - content_top;

    constexpr float top_padding = 18.0f;
    constexpr float bottom_padding = 24.0f;

    constexpr float launcher_title_height = 38.0f;
    constexpr float version_height = 40.0f;
    constexpr float description_height = 54.0f;

    constexpr float section_title_height = 36.0f;
    constexpr float row_height = 40.0f;

    const float content_height =
        top_padding +
        launcher_title_height +
        version_height +
        description_height +
        section_title_height +
        row_height *
            static_cast<float>(std::size(g_third_party)) +
        bottom_padding;

    WFZScrollArea scroll{
        {left,
         content_top,
         content_width,
         content_view_height},
        content_height,
        scroll_offset};

    WFZUpdateScrollArea(scroll);

    scroll_offset = scroll.scroll_offset;

    WFZBeginScrollArea(scroll);

    float y = content_top + top_padding - scroll.scroll_offset;

    // Launcher information.
    WFZDrawText(
        "WORLD FORGE ZERO LAUNCHER",
        left,
        y,
        26.0f,
        wfz_color_text);

    y += launcher_title_height;
    const std::string ver_label = std::string("Версия лаунчера v") + launcher_version;
    WFZDrawText(ver_label.c_str(), left, y, 18.0f, wfz_color_text_secondary);

    y += version_height;
    WFZDrawText(
        "Лаунчер для установки, обновления и запуска World Forge Zero.",
        left,
        y,
        18.0f,
        wfz_color_text_secondary);

    y += description_height;

    // Third-party software.
    WFZDrawInfoSectionTitle("СТОРОННЕЕ ПО", left, y);

    y += section_title_height;

    constexpr float version_x_offset = 220.0f;
    constexpr float license_x_offset = 340.0f;

    for (const WFZThirdPartyEntry &entry : g_third_party)
    {
        WFZDrawText(entry.name, left, y, 20.0f, wfz_color_text);

        if (entry.version[0] != '\0')
        {
            WFZDrawText(entry.version, left + version_x_offset, y, 18.0f, wfz_color_text_muted);
        }

        WFZDrawText(entry.license, left + license_x_offset, y, 18.0f, wfz_color_text_secondary);

        y += row_height;
    }

    WFZEndScrollArea();

    // Scrollbar.
    WFZDrawScrollBar(scroll, screen_width - right - scrollbar_width, scrollbar_width);

    // Footer.
    const float footer_y = screen_height - footer_height;

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
        46.0f};

    if (WFZButton("НАЗАД", back_area, 18.0f, wfz_button_secondary))
    {
        current_screen = WFZScreen::MainMenu;
        return;
    }
}
