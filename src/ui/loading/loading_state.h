#pragma once

namespace wfz::loading_state
{
    void SetProgress(float progress);
    float GetProgress();
    void DeltaProgress(float progress);

    void SetStatus(const char *status);
    const char *GetStatus();

    void SetReady(bool isReady);
    bool GetReady();

    void SetDisplayProgress(float progress);
    float GetDisplayProgress();
    void DeltaDisplayProgress(float progress);

    void SetReadyTime(float time);
    float GetReadyTime();
}
