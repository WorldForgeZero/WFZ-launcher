#include "main_screen.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <RmlUi/Core.h>

#include "app/loading_state.h"

#include "etc/logger.h"

#include "settings/version.h"

namespace loading = wfz::loading_state;

namespace
{
    Rml::Element *g_version = nullptr;

    Rml::Element *g_status = nullptr;

    Rml::Element *g_progress = nullptr;
    Rml::Element *g_progress_value = nullptr;

    Rml::Element *g_morph_label = nullptr;
    Rml::Element *g_play_button = nullptr;

    std::string g_last_status;

    float Clamp01(const float value)
    {
        return std::clamp(value, 0.0f, 1.0f);
    }

    float Lerp(const float from, const float to, const float t)
    {
        return from + (to - from) * t;
    }

    float EaseOutCubic(const float value)
    {
        const float t = Clamp01(value);
        const float inverse = 1.0f - t;

        return 1.0f - inverse * inverse * inverse;
    }

    std::string ToPixels(const float value)
    {
        return std::to_string(static_cast<int>(std::round(value))) + "px";
    }

    std::string ToPercent(const float value)
    {
        return std::to_string(value) + "%";
    }

    std::string ToNumber(const float value)
    {
        return std::to_string(value);
    }

    Rml::Element *RequireElement(Rml::ElementDocument *document, const char *id)
    {
        Rml::Element *element = document->GetElementById(id);

        if (!element)
            wfz::logger::Error("Failed to find RmlUi element: %s", id);

        return element;
    }

    void ResetLoadingVisuals()
    {
        g_status->SetProperty("opacity", "1");
        g_progress->SetProperty("display", "block");
        g_progress->SetProperty("height", "7px");
        g_progress->SetProperty("background-color", "#323232");
        g_progress_value->SetProperty("display", "block");
        g_morph_label->SetProperty("opacity", "0");
        g_play_button->SetProperty("display", "none");
        g_play_button->SetProperty("opacity", "0");
    }
}

namespace wfz::ui::main_screen
{
    bool Init(Rml::ElementDocument *document)
    {
        if (!document)
            return false;

        g_version = RequireElement(document, "launcher-version");
        g_status = RequireElement(document, "loading-status");
        g_progress = RequireElement(document, "loading-progress");
        g_progress_value = RequireElement(document, "loading-progress-value");
        g_morph_label = RequireElement(document, "play-morph-label");
        g_play_button = RequireElement(document, "play-button");

        if (!g_version ||
            !g_status ||
            !g_progress ||
            !g_progress_value ||
            !g_morph_label ||
            !g_play_button)
        {
            return false;
        }

        g_version->SetInnerRML(std::string("Лаунчер v") + launcher_version);
        g_last_status.clear();

        loading::SetDisplayProgress(0.0f);
        loading::SetReadyTime(0.0f);

        ResetLoadingVisuals();

        return true;
    }

    void Update(const float delta_time)
    {
        if (!g_status)
            return;

        const std::string status = loading::GetStatus();
        if (status != g_last_status)
        {
            g_status->SetInnerRML(status);
            g_last_status = status;
        }

        const float current_progress = loading::GetDisplayProgress();
        const float target_progress = loading::GetProgress();
        const float smoothing = Clamp01(delta_time * 8.0f);
        const float displayed_progress = Lerp(current_progress, target_progress, smoothing);

        loading::SetDisplayProgress(displayed_progress);
        g_progress_value->SetProperty("width", ToPercent(displayed_progress * 100.0f));

        if (!loading::GetReady())
        {
            loading::SetReadyTime(0.0f);

            ResetLoadingVisuals();

            return;
        }

        const float ready_time = loading::GetReadyTime() + delta_time;

        loading::SetReadyTime(ready_time);

        constexpr float morph_duration = 0.32f;
        constexpr float status_fade_duration = 0.28f;
        const float status_alpha = 1.0f - Clamp01(ready_time / status_fade_duration);

        g_status->SetProperty("opacity", ToNumber(status_alpha));

        const float morph = EaseOutCubic(ready_time / morph_duration);
        const float current_height = Lerp(7.0f, 54.0f, morph);

        g_progress->SetProperty("height", ToPixels(current_height));
        g_progress->SetProperty("background-color", "#ff9800");
        g_progress_value->SetProperty("display", "none");

        const float morph_text_alpha = Clamp01((morph - 0.35f) / 0.45f);
        constexpr float button_fade_start = 0.75f;
        const float button_alpha = Clamp01((morph - button_fade_start) / (1.0f - button_fade_start));
        const float morph_label_alpha = morph_text_alpha * (1.0f - button_alpha);

        g_morph_label->SetProperty("opacity", ToNumber(morph_label_alpha));

        if (button_alpha > 0.0f)
        {
            g_play_button->SetProperty("display", "flex");
            g_play_button->SetProperty("opacity", ToNumber(button_alpha));
        }

        if (morph >= 1.0f)
        {
            g_progress->SetProperty("display", "none");
            g_morph_label->SetProperty("opacity", "0");
            g_play_button->SetProperty("display", "flex");
            g_play_button->SetProperty("opacity", "1");
        }
    }

    void Shutdown()
    {
        g_version = nullptr;

        g_status = nullptr;

        g_progress = nullptr;
        g_progress_value = nullptr;

        g_morph_label = nullptr;
        g_play_button = nullptr;

        g_last_status.clear();
    }
}
