#pragma once

namespace Rml
{
    class ElementDocument;
}

namespace wfz::ui::alerts_screen
{
    bool Init(Rml::ElementDocument *document);

    // Call only on the UI thread.
    void Update();
    void Shutdown();
}
