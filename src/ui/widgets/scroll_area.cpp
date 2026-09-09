#include "scroll_area.h"

#include <algorithm>

#include "../theme.h"

void WFZUpdateScrollArea(WFZScrollArea &scroll, const float wheel_speed)
{
    const float max_scroll = std::max(0.0f, scroll.content_height - scroll.view.height);
    scroll.scroll_offset = std::clamp(scroll.scroll_offset, 0.0f, max_scroll);
    const Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, scroll.view))
    {
        scroll.scroll_offset -= GetMouseWheelMove() * wheel_speed;
        scroll.scroll_offset = std::clamp(scroll.scroll_offset, 0.0f, max_scroll);
    }
}

void WFZBeginScrollArea(const WFZScrollArea &scroll)
{
    BeginScissorMode(
        static_cast<int>(scroll.view.x),
        static_cast<int>(scroll.view.y),
        static_cast<int>(scroll.view.width),
        static_cast<int>(scroll.view.height));
}

void WFZEndScrollArea()
{
    EndScissorMode();
}

void WFZDrawScrollBar(const WFZScrollArea &scroll, const float x, const float width)
{
    const float max_scroll = std::max(0.0f, scroll.content_height - scroll.view.height);

    if (max_scroll <= 0.0f)
        return;

    const float track_y = scroll.view.y + 8.0f;
    const float track_height = scroll.view.height - 16.0f;
    const float visible_fraction = std::min(1.0f, scroll.view.height / scroll.content_height);
    const float thumb_height = std::max(32.0f, track_height * visible_fraction);
    const float scroll_fraction = scroll.scroll_offset / max_scroll;
    const float thumb_y = track_y + (track_height - thumb_height) * scroll_fraction;

    DrawRectangle(
        static_cast<int>(x),
        static_cast<int>(track_y),
        static_cast<int>(width),
        static_cast<int>(track_height),
        wfz_color_control);

    DrawRectangle(
        static_cast<int>(x),
        static_cast<int>(thumb_y),
        static_cast<int>(width),
        static_cast<int>(thumb_height),
        wfz_color_accent);
}
