#include "font.h"

#include <string>

static Font g_font{};

static constexpr float wfz_font_spacing = 0.0f;

#if defined(EMBEDED_FONT)
extern "C"
{
    extern const unsigned char _binary_assets_fonts_Monocraft_ttf_start[];
    extern const unsigned char _binary_assets_fonts_Monocraft_ttf_end[];
}
#endif

void WFZLoadFont()
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

#if defined(EMBEDED_FONT)
    const int font_data_size = static_cast<int>(_binary_assets_fonts_Monocraft_ttf_end - _binary_assets_fonts_Monocraft_ttf_start);

    g_font = LoadFontFromMemory(
        ".ttf",
        _binary_assets_fonts_Monocraft_ttf_start,
        font_data_size,
        256,
        codepoints,
        codepoint_count);

#else
    g_font = LoadFontEx("assets/fonts/Monocraft.ttf", 256, codepoints, codepoint_count);
#endif

    UnloadCodepoints(codepoints);

    SetTextureFilter(
        g_font.texture,
        TEXTURE_FILTER_POINT);
}

void WFZUnloadFont()
{
    UnloadFont(g_font);
}

void WFZDrawText(const char *text, float x, float y, float size, Color color)
{
    DrawTextEx(
        g_font,
        text,
        {x, y},
        size,
        wfz_font_spacing,
        color);
}

Vector2 WFZMeasureText(const char *text, float size)
{
    return MeasureTextEx(
        g_font,
        text,
        size,
        wfz_font_spacing);
}

void WFZDrawTextCentered(const char *text, Rectangle bounds, float size, Color color)
{
    const Vector2 text_size = WFZMeasureText(text, size);

    WFZDrawText(text,
                bounds.x + (bounds.width - text_size.x) / 2.0f,
                bounds.y + (bounds.height - text_size.y) / 2.0f,
                size,
                color);
}

static float WFZProcessWrappedText(const char *text, Rectangle bounds, float font_size, float line_gap, Color color, bool draw)
{
    if (text == nullptr || *text == '\0')
        return 0.0f;

    const float line_height =
        font_size + line_gap;

    float y = bounds.y;

    std::string line;
    std::string word;

    const std::string source{text};

    auto finish_line =
        [&](const std::string &value)
    {
        if (value.empty())
            return;

        if (draw)
        {
            WFZDrawText(
                value.c_str(),
                bounds.x,
                y,
                font_size,
                color);
        }

        y += line_height;
    };

    auto push_word =
        [&](const std::string &value)
    {
        if (value.empty())
            return;

        if (line.empty())
        {
            line = value;
            return;
        }

        const std::string candidate =
            line + " " + value;

        if (WFZMeasureText(
                candidate.c_str(),
                font_size)
                .x <= bounds.width)
        {
            line = candidate;
        }
        else
        {
            finish_line(line);
            line = value;
        }
    };

    for (std::size_t i = 0; i <= source.size(); ++i)
    {
        const char character =
            i < source.size()
                ? source[i]
                : '\0';

        if (character == ' ' ||
            character == '\n' ||
            character == '\0')
        {
            push_word(word);
            word.clear();

            if (character == '\n')
            {
                finish_line(line);
                line.clear();
            }

            continue;
        }

        word += character;
    }

    finish_line(line);

    return y - bounds.y;
}

float WFZDrawTextWrapped(const char *text, Rectangle bounds, float font_size, float line_gap, Color color)
{
    return WFZProcessWrappedText(
        text,
        bounds,
        font_size,
        line_gap,
        color,
        true);
}

float WFZMeasureTextWrappedHeight(const char *text, float width, float font_size, float line_gap)
{
    return WFZProcessWrappedText(
        text,
        {0.0f,
         0.0f,
         width,
         0.0f},
        font_size,
        line_gap,
        BLANK,
        false);
}
