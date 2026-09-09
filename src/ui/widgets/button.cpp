#include "button.h"

#include "../cursor.h"
#include "../font.h"

bool WFZButton(const char *text, Rectangle bounds, float font_size, const WFZButtonStyle &style)
{
    static Rectangle active_bounds{};
    static bool has_active_button = false;

    const Vector2 mouse = GetMousePosition();

    const bool hovered = CheckCollisionPointRec(mouse, bounds);

    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        active_bounds = bounds;
        has_active_button = true;
    }

    const bool active =
        has_active_button &&
        active_bounds.x == bounds.x &&
        active_bounds.y == bounds.y &&
        active_bounds.width == bounds.width &&
        active_bounds.height == bounds.height;

    Color background = style.background;

    Color border = style.border;

    if (active && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        background = style.background_pressed;
    }
    else if (hovered)
    {
        background = style.background_hover;
        border = style.border_hover;
    }

    if (hovered)
    {
        WFZRequestPointerCursor();
    }

    DrawRectangleRounded(
        bounds,
        0.12f,
        6,
        background);

    DrawRectangleRoundedLinesEx(
        bounds,
        0.12f,
        6,
        1.0f,
        border);

    WFZDrawTextCentered(text, bounds, font_size, style.text);

    bool clicked = false;

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        clicked = active && hovered;

        if (active)
            has_active_button = false;
    }

    return clicked;
}
