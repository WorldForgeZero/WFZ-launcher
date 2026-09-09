#include "cursor.h"

#include <raylib.h>

static bool g_pointer_requested = false;
static bool g_pointer_active = false;

void WFZBeginCursorFrame()
{
    g_pointer_requested = false;
}

void WFZRequestPointerCursor()
{
    g_pointer_requested = true;
}

void WFZEndCursorFrame()
{
    if (g_pointer_requested == g_pointer_active)
        return;

    SetMouseCursor(        g_pointer_requested            ? MOUSE_CURSOR_POINTING_HAND            : MOUSE_CURSOR_DEFAULT);

    g_pointer_active = g_pointer_requested;
}
