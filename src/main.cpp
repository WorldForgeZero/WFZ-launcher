#include <GLFW/glfw3.h>
#include <RmlUi/Core.h>

#include "RmlUi_Backend.h"

#include "app/exit_req.h"
#include "app/thread_manager.h"

#include "daemon/bootstrap.h"

#include "settings/settings.h"

#include "etc/logger.h"
#include "etc/self_update.h"

#include "ui/main/main_screen.h"

namespace wfza = wfz::app;

namespace
{
    class NavigationListener final : public Rml::EventListener
    {
    public:
        NavigationListener(
            Rml::ElementDocument *source,
            Rml::ElementDocument *target)
            : source_(source),
              target_(target)
        {
        }

        void ProcessEvent(Rml::Event &) override
        {
            if (source_)
                source_->Hide();

            if (target_)
                target_->Show();
        }

    private:
        Rml::ElementDocument *source_;
        Rml::ElementDocument *target_;
    };

    bool BindClick(
        Rml::ElementDocument *document,
        const char *element_id,
        Rml::EventListener *listener)
    {
        if (!document)
            return false;

        Rml::Element *element =
            document->GetElementById(element_id);

        if (!element)
        {
            wfz::logger::Error(
                "Failed to find RmlUi element: %s",
                element_id);

            return false;
        }

        element->AddEventListener(
            "click",
            listener);

        return true;
    }
}

int main(int argc, char **argv)
{
    if (const auto result =
            wfz::self_update::HandleStartupArguments(argc, argv))
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
        wfz::logger::Error(
            "Failed to initialize RmlUi backend");

        wfz::logger::Shutdown();
        return 1;
    }

    Rml::SetSystemInterface(
        Backend::GetSystemInterface());

    Rml::SetRenderInterface(
        Backend::GetRenderInterface());

    if (!Rml::Initialise())
    {
        wfz::logger::Error(
            "Failed to initialize RmlUi");

        Backend::Shutdown();
        wfz::logger::Shutdown();

        return 1;
    }

    GLFWwindow *window =
        glfwGetCurrentContext();

    if (!window)
    {
        wfz::logger::Error(
            "Failed to get GLFW window");

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

    Rml::Context *context =
        Rml::CreateContext(
            "main",
            Rml::Vector2i(
                window_width,
                window_height));

    if (!context)
    {
        wfz::logger::Error(
            "Failed to create RmlUi context");

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();

        return 1;
    }

    if (!Rml::LoadFontFace(
            "assets/fonts/Monocraft.ttf"))
    {
        wfz::logger::Error(
            "Failed to load main font");

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();

        return 1;
    }

    Rml::ElementDocument *main_document =
        context->LoadDocument(
            "assets/ui/main/main.rml");

    Rml::ElementDocument *settings_document =
        context->LoadDocument(
            "assets/ui/settings/settings.rml");

    Rml::ElementDocument *news_document =
        context->LoadDocument(
            "assets/ui/news/news.rml");

    Rml::ElementDocument *info_document =
        context->LoadDocument(
            "assets/ui/info/info.rml");

    if (!main_document ||
        !settings_document ||
        !news_document ||
        !info_document)
    {
        wfz::logger::Error(
            "Failed to load one or more RmlUi documents");

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();

        return 1;
    }

    /*
     * Navigation listeners must stay alive for as long
     * as the documents using them stay alive.
     */

    NavigationListener main_to_settings{
        main_document,
        settings_document};

    NavigationListener main_to_news{
        main_document,
        news_document};

    NavigationListener main_to_info{
        main_document,
        info_document};

    NavigationListener settings_to_main{
        settings_document,
        main_document};

    NavigationListener news_to_main{
        news_document,
        main_document};

    NavigationListener info_to_main{
        info_document,
        main_document};

    bool navigation_ok = true;

    navigation_ok &=
        BindClick(
            main_document,
            "open-settings",
            &main_to_settings);

    navigation_ok &=
        BindClick(
            main_document,
            "open-news",
            &main_to_news);

    navigation_ok &=
        BindClick(
            main_document,
            "open-info",
            &main_to_info);

    navigation_ok &=
        BindClick(
            settings_document,
            "settings-back",
            &settings_to_main);

    navigation_ok &=
        BindClick(
            news_document,
            "news-back",
            &news_to_main);

    navigation_ok &=
        BindClick(
            info_document,
            "info-back",
            &info_to_main);

    if (!navigation_ok)
    {
        wfz::logger::Error(
            "Failed to initialize UI navigation");

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();

        return 1;
    }

    if (!wfz::ui::main_screen::Init(main_document))
    {
        wfz::logger::Error(
            "Failed to initialize main UI screen");

        Rml::Shutdown();
        Backend::Shutdown();
        wfz::logger::Shutdown();

        return 1;
    }

    settings_document->Hide();
    news_document->Hide();
    info_document->Hide();

    main_document->Show();

    double previous_time = glfwGetTime();
    wfza::ThreadManager::instance().submit(RunDaemonBootstrap);

    while (!wfza::ExitRequested() && Backend::ProcessEvents(context))
    {
        const double current_time = glfwGetTime();

        const float delta_time = static_cast<float>(current_time - previous_time);

        previous_time = current_time;

        wfz::ui::main_screen::Update(delta_time);

        context->Update();

        Backend::BeginFrame();

        context->Render();

        Backend::PresentFrame();
    }

    wfza::RequestExit();

    wfza::ThreadManager::instance().shutdown();

    wfz::ui::main_screen::Shutdown();

    main_document->Close();
    settings_document->Close();
    news_document->Close();
    info_document->Close();

    Rml::Shutdown();
    Backend::Shutdown();

    wfz::logger::Shutdown();

    return 0;
}
