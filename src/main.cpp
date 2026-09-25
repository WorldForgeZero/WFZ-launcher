#include <iostream>

#include <GLFW/glfw3.h>
#include <RmlUi/Core.h>

#include "RmlUi_Backend.h"

#include "app/exit_req.h"
#include "app/thread_manager.h"

#include "daemon/bootstrap.h"

#include "settings/settings.h"

#include "etc/logger.h"
#include "etc/self_update.h"

namespace wfza = wfz::app;

int main(int argc, char **argv)
{
    if (const auto result = wfz::self_update::HandleStartupArguments(argc, argv))
    {
        return *result;
    }

    constexpr int window_width = 800;
    constexpr int window_height = 450;

    wfz::logger::Init();
    wfz::logger::DumpStartupInfo();

    wfz::settings::Load();

    if (!Backend::Initialize(
            "World Forge Zero",
            window_width,
            window_height,
            true))
    {
        std::cerr << "Failed to initialize RmlUi backend\n";

        wfz::logger::Shutdown();
        return 1;
    }

    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());

    if (!Rml::Initialise())
    {
        std::cerr << "Failed to initialize RmlUi\n";

        Backend::Shutdown();
        wfz::logger::Shutdown();
        return 1;
    }

    GLFWwindow *window = glfwGetCurrentContext();

    if (!window)
    {
        std::cerr << "Failed to get GLFW window\n";

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();
        return 1;
    }

    glfwSetWindowSizeLimits(
        window,
        window_width,
        window_height,
        GLFW_DONT_CARE,
        GLFW_DONT_CARE);

    Rml::Context *context = Rml::CreateContext(
        "main",
        Rml::Vector2i(window_width, window_height));

    if (!context)
    {
        std::cerr << "Failed to create RmlUi context\n";

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();
        return 1;
    }

    if (!Rml::LoadFontFace("assets/fonts/Monocraft.ttf"))
    {
        std::cerr << "Failed to load main font\n";

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();
        return 1;
    }

    Rml::ElementDocument *document =
        context->LoadDocument("assets/ui/main.rml");

    if (!document)
    {
        std::cerr << "Failed to load main document\n";

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();
        return 1;
    }

    document->Show();

    wfza::ThreadManager::instance().submit(RunDaemonBootstrap);

    while (!wfza::ExitRequested() &&
           Backend::ProcessEvents(context))
    {
        context->Update();

        Backend::BeginFrame();
        context->Render();
        Backend::PresentFrame();
    }

    wfza::RequestExit();

    wfza::ThreadManager::instance().shutdown();

    document->Close();

    Rml::Shutdown();
    Backend::Shutdown();

    wfz::logger::Shutdown();

    return 0;
}
