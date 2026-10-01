#pragma once

namespace Rml
{
    class Context;
}

namespace wfz::ui
{
    bool Init(Rml::Context *context);

    void Update(float delta_time);

    void Shutdown();
}
