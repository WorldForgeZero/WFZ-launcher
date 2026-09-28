#pragma once

namespace Rml
{
    class ElementDocument;
}

namespace wfz::ui::info_screen
{
    bool Init(Rml::ElementDocument *document);
    void Shutdown();
}
