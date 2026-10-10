#include "info_screen.h"

#include <RmlUi/Core.h>

#include "etc/logger.h"
#include "settings/version.h"

namespace
{
    Rml::Element *g_version = nullptr;
}

namespace wfz::ui::info_screen
{
    bool Init(Rml::ElementDocument *document)
    {
        if (!document)
            return false;

        g_version = document->GetElementById("info-launcher-version");

        if (!g_version)
        {
            wfz::logger::Error("Failed to find RmlUi element: info-launcher-version");
            return false;
        }

        g_version->SetInnerRML(launcher_version);

        return true;
    }

    void Shutdown()
    {
        g_version = nullptr;
    }
}
