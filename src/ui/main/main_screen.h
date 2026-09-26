#pragma once

namespace Rml
{
    class ElementDocument;
}

namespace wfz::ui::main_screen
{
    bool Init(Rml::ElementDocument *document);

    void Update(float delta_time);

    void Shutdown();
}
