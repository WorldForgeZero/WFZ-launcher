#pragma once

#include <raylib.h>

#include "../font.h"
#include "../theme.h"

#include "../widgets/button.h"

#include "../loading/loading_state.h"

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

inline void WFZUpdateLoading()
{
    const float dt = GetFrameTime();

    wfz::loading_state::SetDisplayProgress(
        WFZLerp(
            wfz::loading_state::GetDisplayProgress(),
            wfz::loading_state::GetProgress(),
            WFZClamp01(dt * 8.0f)));

    if (wfz::loading_state::GetReady())
        wfz::loading_state::SetReadyTime(wfz::loading_state::GetReadyTime() + dt);
}
