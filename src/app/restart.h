#pragma once

namespace wfz::app
{
    bool WaitForPreviousInstance(int argc, char **argv);

    bool WasRestarted() noexcept;

    bool Restart();
}
