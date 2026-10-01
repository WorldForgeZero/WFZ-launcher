#pragma once

namespace Rml
{
    class ElementDocument;
}

namespace wfz::ui::settings_screen
{
    bool Init(Rml::ElementDocument *document);
    void Shutdown();
}
