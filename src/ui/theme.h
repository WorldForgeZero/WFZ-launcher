#pragma once

#include <raylib.h>

struct WFZButtonStyle
{
    Color background;
    Color background_hover;
    Color background_pressed;

    Color border;
    Color border_hover;

    Color text;
};

inline constexpr Color wfz_color_background{14, 14, 14, 255};

inline constexpr Color wfz_color_banner_placeholder{24, 24, 24, 255};

inline constexpr Color wfz_color_text{232, 232, 232, 255};
inline constexpr Color wfz_color_text_secondary{185, 185, 185, 255};
inline constexpr Color wfz_color_text_muted{154, 154, 154, 255};
inline constexpr Color wfz_color_text_dark{14, 14, 14, 255};

inline constexpr Color wfz_color_overlay_top{0, 0, 0, 40};
inline constexpr Color wfz_color_overlay_bottom{0, 0, 0, 210};

inline constexpr Color wfz_color_control{28, 28, 28, 240};
inline constexpr Color wfz_color_control_hover{42, 42, 42, 255};
inline constexpr Color wfz_color_control_pressed{52, 52, 52, 255};
inline constexpr Color wfz_color_control_border{65, 65, 65, 255};

inline constexpr Color wfz_color_accent{255, 152, 0, 255};
inline constexpr Color wfz_color_accent_hover{255, 183, 77, 255};
inline constexpr Color wfz_color_accent_pressed{230, 130, 0, 255};

inline constexpr Color wfz_color_panel{28, 28, 28, 240};
inline constexpr Color wfz_color_panel_border{65, 65, 65, 255};
inline constexpr Color wfz_color_progress_background{50, 50, 50, 255};

inline constexpr WFZButtonStyle wfz_button_secondary{
    wfz_color_panel,
    wfz_color_control_hover,
    wfz_color_control_pressed,
    wfz_color_panel_border,
    wfz_color_accent,
    wfz_color_text};

inline constexpr WFZButtonStyle wfz_button_primary{
    wfz_color_accent,
    wfz_color_accent_hover,
    wfz_color_accent_pressed,
    wfz_color_accent,
    wfz_color_accent_hover,
    wfz_color_text_dark};
