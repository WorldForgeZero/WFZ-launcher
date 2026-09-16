#pragma once

#include <string>

namespace wfz::loading_state
{
    void SetProgress(float progress);
    float GetProgress();
    void DeltaProgress(float delta);

    void SetStatus(std::string status);
    std::string GetStatus();

    void SetReady(bool is_ready);
    bool GetReady();

    void SetDisplayProgress(float progress);
    float GetDisplayProgress();
    void DeltaDisplayProgress(float delta);

    void SetReadyTime(float time);
    float GetReadyTime();
}
