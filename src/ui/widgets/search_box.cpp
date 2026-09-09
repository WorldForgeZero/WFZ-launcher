#include "search_box.h"

#include "../cursor.h"
#include "../font.h"
#include "../theme.h"

bool WFZSearchBox(Rectangle bounds, std::string &text)
{
    static bool focused = false;

    const Vector2 mouse = GetMousePosition();

    const bool hovered = CheckCollisionPointRec(mouse, bounds);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        focused = hovered;
    }

    bool changed = false;

    if (focused)
    {
        int codepoint = GetCharPressed();

        while (codepoint > 0)
        {
            if (codepoint >= 32)
            {
                int utf8_size = 0;
                const char *utf8 = CodepointToUTF8(codepoint, &utf8_size);
                text.append(utf8, static_cast<std::size_t>(utf8_size));
                changed = true;
            }

            codepoint = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty())
        {
            const char *start = text.c_str();

            const char *end = start + text.size();

            const char *cursor = end - 1;

            while (cursor > start && ((*cursor & 0xC0) == 0x80))
            {
                --cursor;
            }

            text.erase(static_cast<std::size_t>(cursor - start));

            changed = true;
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            focused = false;
        }
    }

    Color background = wfz_color_control;

    if (focused)
    {
        background = wfz_color_control_hover;
    }

    DrawRectangleRounded(
        bounds,
        0.08f,
        6,
        background);

    DrawRectangleRoundedLinesEx(
        bounds,
        0.08f,
        6,
        1.0f,
        focused || hovered
            ? wfz_color_accent
            : wfz_color_control_border);

    if (text.empty())
    {
        WFZDrawText("Поиск...", bounds.x + 14.0f, bounds.y + 12.0f, 18.0f, wfz_color_text_muted);
    }
    else
    {
        WFZDrawText(text.c_str(), bounds.x + 14.0f, bounds.y + 12.0f, 18.0f, wfz_color_text);
    }

    if (hovered && !focused)
    {
        WFZRequestPointerCursor();
    }

    return changed;
}
