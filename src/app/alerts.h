#pragma once

#include <string>

namespace wfz::alerts
{
    enum class Action
    {
        None,
        RetryAll,
        InstallUpdate
    };

    struct Alert
    {
        std::string title;
        std::string message;
        std::string details;
        std::string action_label;
        Action action = Action::None;
    };

    // Thread safe: worker produces alerts, UI consumes them.
    void Post(Alert alert);
    bool TryPop(Alert &out);
    void Clear();
}
