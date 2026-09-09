#pragma once

#include <raylib.h>

#include "../font.h"
#include "../theme.h"

#include "../widgets/button.h"

static float WFZClamp01(float value)
{
    if (value < 0.0f)
        return 0.0f;

    if (value > 1.0f)
        return 1.0f;

    return value;
}

static float WFZLerp(float from, float to, float t)
{
    return from + (to - from) * t;
}

static float WFZEaseOutCubic(float t)
{
    const float inverse = 1.0f - WFZClamp01(t);
    return 1.0f - inverse * inverse * inverse;
}

namespace
{
    struct WFZLoadingState
    {
        float progress = 0.0f;
        float displayed_progress = 0.0f;

        float ready_time = 0.0f;

        bool ready = false;

        const char *status = "Подготовка...";
    };

    WFZLoadingState g_loading_state{};

    void WFZUpdateDummyLoading()
    {
        const float dt = GetFrameTime();

        if (!g_loading_state.ready)
        {
            g_loading_state.progress += dt * 0.15f;

            if (g_loading_state.progress < 0.25f)
            {
                g_loading_state.status = "Проверка файлов...";
            }
            else if (g_loading_state.progress < 0.55f)
            {
                g_loading_state.status = "Проверка обновлений...";
            }
            else if (g_loading_state.progress < 0.85f)
            {
                g_loading_state.status = "Подготовка игры...";
            }
            else
            {
                g_loading_state.status = "Завершение...";
            }

            if (g_loading_state.progress >= 1.0f)
            {
                g_loading_state.progress = 1.0f;
                g_loading_state.ready = true;
                g_loading_state.status = "Готово";
            }
        }

        g_loading_state.displayed_progress = WFZLerp(
            g_loading_state.displayed_progress,
            g_loading_state.progress,
            WFZClamp01(dt * 8.0f));

        if (g_loading_state.ready)
        {
            g_loading_state.ready_time += dt;
        }
    }
}
