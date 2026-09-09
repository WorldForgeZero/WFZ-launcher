#include "checkbox.h"

#include "../cursor.h"
#include "../font.h"
#include "../theme.h"

float WFZCheckboxRow(const char *title, const char *description, Rectangle bounds, bool &checked)
{
    constexpr float horizontal_padding = 16.0f;
    constexpr float top_padding = 14.0f;
    constexpr float bottom_padding = 14.0f;

    constexpr float title_size = 22.0f;
    constexpr float description_size = 17.0f;
    constexpr float description_line_gap = 5.0f;

    constexpr float title_description_gap = 12.0f;

    constexpr float box_size = 24.0f;
    constexpr float box_margin = 14.0f;

    const float description_width = bounds.width - horizontal_padding * 2.0f - box_size - box_margin;

    const float description_height = WFZMeasureTextWrappedHeight(description, description_width, description_size, description_line_gap);

    const float required_height = top_padding + title_size + title_description_gap + description_height + bottom_padding;

    bounds.height = required_height;

    const Vector2 mouse = GetMousePosition();

    const bool hovered = CheckCollisionPointRec(mouse, bounds);

    const bool pressed = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    const Rectangle box{
        bounds.x + bounds.width - box_size - box_margin,
        bounds.y + top_padding, box_size, box_size};

    if (hovered)
    {
        WFZRequestPointerCursor();
        DrawRectangleRounded(bounds, 0.04f, 6, wfz_color_control_hover);
    }

    Color box_color = wfz_color_control;

    if (pressed)
    {
        box_color = wfz_color_control_pressed;
    }

    DrawRectangleRounded(box, 0.15f, 4, box_color);
    DrawRectangleRoundedLinesEx(
        box,
        0.15f,
        4,
        1.0f,
        hovered
            ? wfz_color_accent
            : wfz_color_control_border);

    if (checked)
    {
        constexpr float inset = 5.0f;

        DrawRectangleRounded(
            {box.x + inset, box.y + inset, box.width - inset * 2.0f, box.height - inset * 2.0f},
            0.2f,
            4,
            wfz_color_accent);
    }

    WFZDrawText(
        title,
        bounds.x + horizontal_padding,
        bounds.y + top_padding,
        title_size,
        wfz_color_text);

    WFZDrawTextWrapped(
        description,
        {bounds.x + horizontal_padding, bounds.y + top_padding + title_size + title_description_gap, description_width, description_height},
        description_size,
        description_line_gap,
        wfz_color_text_secondary);

    if (hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        checked = !checked;
    }

    return required_height;
}
