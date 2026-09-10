#pragma once

namespace wfz::loading_state
{
    void SetProgress(float progress);
    float GetProgress();

    void SetStatus(const char *status);
    const char *GetStatus();

    void SetReady(bool isReady);
    bool GetReady();

    void SetDisplayProgress(float progress);
    float GetDisplayProgress();

    void SetReadyTime(float time);
    float GetReadyTime();
}
