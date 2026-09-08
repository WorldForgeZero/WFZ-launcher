#include <raylib.h>

static Font g_font{};
static bool g_news_open = false;

static constexpr float FONT_SPACING = 0.0f;

// WFZ colors.
static constexpr Color wfz_color_background{14, 14, 14, 255};
static constexpr Color wfz_color_banner_placeholder{24, 24, 24, 255};
static constexpr Color wfz_color_text{232, 232, 232, 255};
static constexpr Color wfz_color_text_muted{154, 154, 154, 255};
static constexpr Color wfz_color_accent{255, 152, 0, 255};
static constexpr Color wfz_color_accent_hover{255, 183, 77, 255};
static constexpr Color wfz_color_accent_pressed{230, 130, 0, 255};
static constexpr Color wfz_color_dark_text{14, 14, 14, 255};
static constexpr Color wfz_color_button{28, 28, 28, 240};
static constexpr Color wfz_color_button_hover{42, 42, 42, 255};
static constexpr Color wfz_color_button_pressed{52, 52, 52, 255};
static constexpr Color wfz_color_button_border{65, 65, 65, 255};
static constexpr Color wfz_color_button_border_hover{255, 152, 0, 255};
static constexpr Color wfz_color_progress_background{50, 50, 50, 255};
static constexpr Color wfz_color_modal_background{18, 18, 18, 255};
static constexpr Color wfz_color_modal_overlay{0, 0, 0, 180};

static void WFZDrawText(const char *text, float x, float y, float size, Color color)
{
    DrawTextEx(g_font, text, {x, y}, size, FONT_SPACING, color);
}

static Vector2 WFZMeasureText(const char *text, float size)
{
    return MeasureTextEx(g_font, text, size, FONT_SPACING);
}

static bool WFZButton(
    const char *text,
    Rectangle bounds,
    float font_size,
    Color normal_color,
    Color hover_color,
    Color pressed_color,
    Color text_color,
    Color border_color,
    Color border_hover_color)
{
    const Vector2 mouse = GetMousePosition();

    const bool hovered =
        CheckCollisionPointRec(mouse, bounds);

    const bool pressed =
        hovered &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    Color button_color = normal_color;

    if (pressed)
    {
        button_color = pressed_color;
    }
    else if (hovered)
    {
        button_color = hover_color;
    }

    DrawRectangleRounded(bounds, 0.12f, 6, button_color);

    DrawRectangleRoundedLinesEx(
        bounds,
        0.12f,
        6,
        1.0f,
        hovered
            ? border_hover_color
            : border_color);

    const Vector2 text_size = WFZMeasureText(text, font_size);

    WFZDrawText(
        text,
        bounds.x +
            (bounds.width - text_size.x) / 2.0f,
        bounds.y +
            (bounds.height - text_size.y) / 2.0f,
        font_size,
        text_color);

    return hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void WFZLoadFont()
{
    int codepoint_count = 0;

    int *codepoints = LoadCodepoints(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
        "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
        "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
        "—–…«»„“”№©",
        &codepoint_count);

    g_font = LoadFontEx(
        "assets/fonts/Monocraft.ttf",
        256,
        codepoints,
        codepoint_count);

    UnloadCodepoints(codepoints);

    SetTextureFilter(g_font.texture, TEXTURE_FILTER_POINT);
}

static void WFZDrawNewsWindow()
{
    const float screen_width = static_cast<float>(GetScreenWidth());

    const float screen_height = static_cast<float>(GetScreenHeight());

    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
        wfz_color_modal_overlay);

    Rectangle panel{
        (screen_width - 760.0f) / 2.0f,
        (screen_height - 420.0f) / 2.0f,
        760.0f,
        420.0f};

    DrawRectangleRounded(
        panel,
        0.025f,
        8,
        wfz_color_modal_background);

    DrawRectangleRoundedLinesEx(
        panel,
        0.025f,
        8,
        1.0f,
        wfz_color_button_border);

    WFZDrawText(
        "НОВОСТИ",
        panel.x + 28.0f,
        panel.y + 24.0f,
        28.0f,
        wfz_color_text);

    DrawRectangle(
        static_cast<int>(panel.x + 28.0f),
        static_cast<int>(panel.y + 68.0f),
        static_cast<int>(panel.width - 56.0f),
        2,
        wfz_color_accent);

    WFZDrawText(
        "08.09.2026",
        panel.x + 28.0f,
        panel.y + 96.0f,
        16.0f,
        wfz_color_text_muted);

    WFZDrawText(
        "Лаунчер World Forge Zero ожил",
        panel.x + 28.0f,
        panel.y + 124.0f,
        22.0f,
        wfz_color_text);

    WFZDrawText(
        "Добавлен новый интерфейс лаунчера.",
        panel.x + 28.0f,
        panel.y + 160.0f,
        18.0f,
        wfz_color_text_muted);

    WFZDrawText(
        "Поддержка русского языка тоже каким-то чудом работает.",
        panel.x + 28.0f,
        panel.y + 186.0f,
        18.0f,
        wfz_color_text_muted);

    WFZDrawText(
        "07.09.2026",
        panel.x + 28.0f,
        panel.y + 236.0f,
        16.0f,
        wfz_color_text_muted);

    WFZDrawText(
        "World Forge Zero",
        panel.x + 28.0f,
        panel.y + 264.0f,
        22.0f,
        wfz_color_text);

    WFZDrawText(
        "Новостей пока нет. Это вообще dummy окно.",
        panel.x + 28.0f,
        panel.y + 300.0f,
        18.0f,
        wfz_color_text_muted);

    Rectangle close_button{
        panel.x + panel.width - 150.0f,
        panel.y + panel.height - 58.0f,
        122.0f,
        36.0f};

    if (WFZButton(
            "ЗАКРЫТЬ",
            close_button,
            17.0f,
            wfz_color_button,
            wfz_color_button_hover,
            wfz_color_button_pressed,
            wfz_color_text,
            wfz_color_button_border,
            wfz_color_button_border_hover))
    {
        g_news_open = false;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        g_news_open = false;
    }
}

int main()
{
    constexpr int window_width = 1100;
    constexpr int window_height = 620;

    InitWindow(
        window_width,
        window_height,
        "World Forge Zero");

    SetTargetFPS(30);

    WFZLoadFont();

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(
            wfz_color_background);

        // Temporary banner placeholder.
        DrawRectangle(
            0,
            0,
            GetScreenWidth(),
            GetScreenHeight(),
            wfz_color_banner_placeholder);

        // Darken the lower part of the banner.
        DrawRectangleGradientV(
            0,
            0,
            GetScreenWidth(),
            GetScreenHeight(),
            Color{0, 0, 0, 40},
            Color{0, 0, 0, 210});

        // Title.
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

        constexpr float left = 32.0f;

        const float bottom =
            static_cast<float>(GetScreenHeight()) -
            32.0f;

        // Status.
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

        // Play button.
        Rectangle play_button{
            left,
            bottom - 54.0f,
            320.0f,
            54.0f};

        if (!g_news_open &&
            WFZButton(
                "ИГРАТЬ",
                play_button,
                26.0f,
                wfz_color_accent,
                wfz_color_accent_hover,
                wfz_color_accent_pressed,
                wfz_color_dark_text,
                wfz_color_accent,
                wfz_color_accent_hover))
        {
            TraceLog(
                LOG_INFO,
                "PLAY pressed");
        }

        // Top buttons.
        constexpr float top_button_height = 42.0f;
        constexpr float top_button_gap = 12.0f;

        Rectangle settings_button{
            static_cast<float>(GetScreenWidth()) - 184.0f,
            26.0f,
            152.0f,
            top_button_height};

        Rectangle news_button{
            settings_button.x - 132.0f - top_button_gap,
            26.0f,
            132.0f,
            top_button_height};

        if (!g_news_open &&
            WFZButton(
                "НОВОСТИ",
                news_button,
                18.0f,
                wfz_color_button,
                wfz_color_button_hover,
                wfz_color_button_pressed,
                wfz_color_text,
                wfz_color_button_border,
                wfz_color_button_border_hover))
        {
            g_news_open = true;
        }

        if (!g_news_open &&
            WFZButton(
                "НАСТРОЙКИ",
                settings_button,
                18.0f,
                wfz_color_button,
                wfz_color_button_hover,
                wfz_color_button_pressed,
                wfz_color_text,
                wfz_color_button_border,
                wfz_color_button_border_hover))
        {
            TraceLog(
                LOG_INFO,
                "SETTINGS pressed");
        }

        if (g_news_open)
        {
            WFZDrawNewsWindow();
        }

        // Mouse cursor.
        const Vector2 mouse =
            GetMousePosition();

        bool over_button = false;

        if (!g_news_open)
        {
            over_button =
                CheckCollisionPointRec(
                    mouse,
                    play_button) ||
                CheckCollisionPointRec(
                    mouse,
                    news_button) ||
                CheckCollisionPointRec(
                    mouse,
                    settings_button);
        }

        SetMouseCursor(
            over_button
                ? MOUSE_CURSOR_POINTING_HAND
                : MOUSE_CURSOR_DEFAULT);

        EndDrawing();
    }

    UnloadFont(g_font);
    CloseWindow();

    return 0;
}
