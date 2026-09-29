#include "ui.h"

#include <RmlUi/Core.h>

#include "app/loading_state.h"

#include "etc/logger.h"

#include "ui/info/info_screen.h"
#include "ui/main/main_screen.h"

namespace wfz::ui
{
    namespace
    {
        enum class Screen
        {
            Main,
            Settings,
            News,
            Info,
        };

        Rml::ElementDocument *g_main_document = nullptr;
        Rml::ElementDocument *g_settings_document = nullptr;
        Rml::ElementDocument *g_news_document = nullptr;
        Rml::ElementDocument *g_info_document = nullptr;

        void HideAllDocuments()
        {
            if (g_main_document)
                g_main_document->Hide();

            if (g_settings_document)
                g_settings_document->Hide();

            if (g_news_document)
                g_news_document->Hide();

            if (g_info_document)
                g_info_document->Hide();
        }

        void ShowScreen(const Screen screen)
        {
            HideAllDocuments();

            switch (screen)
            {
            case Screen::Main:
                if (g_main_document)
                    g_main_document->Show();

                break;

            case Screen::Settings:
                if (g_settings_document)
                    g_settings_document->Show();

                break;

            case Screen::News:
                if (g_news_document)
                    g_news_document->Show();

                break;

            case Screen::Info:
                if (g_info_document)
                    g_info_document->Show();

                break;
            }
        }

        class NavigationListener final : public Rml::EventListener
        {
        public:
            void ProcessEvent(
                Rml::Event &event) override
            {
                Rml::Element *element = event.GetCurrentElement();

                if (!element)
                    return;

                const Rml::String &id = element->GetId();

                if (id == "open-settings")
                {
                    ShowScreen(Screen::Settings);
                    return;
                }

                if (id == "open-news")
                {
                    ShowScreen(Screen::News);
                    return;
                }

                if (id == "open-info")
                {
                    ShowScreen(Screen::Info);
                    return;
                }

                if (id == "settings-back" || id == "news-back" || id == "info-back")
                    ShowScreen(Screen::Main);
            }
        };

        NavigationListener g_navigation_listener;

        Rml::ElementDocument *LoadDocument(Rml::Context *context, const char *path)
        {
            Rml::ElementDocument *document = context->LoadDocument(path);

            if (!document)
            {
                wfz::logger::Error("Failed to load RmlUi document: %s", path);
            }

            return document;
        }

        bool BindNavigation(Rml::ElementDocument *document, const char *element_id)
        {
            if (!document)
                return false;

            Rml::Element *element = document->GetElementById(element_id);

            if (!element)
            {
                wfz::logger::Error("Failed to find RmlUi element: %s", element_id);

                return false;
            }

            element->AddEventListener(
                "click",
                &g_navigation_listener);

            return true;
        }

        void CloseDocument(
            Rml::ElementDocument *&document)
        {
            if (!document)
                return;

            document->Close();
            document = nullptr;
        }
    }

    bool Init(Rml::Context *context)
    {
        if (!context)
            return false;

        if (!Rml::LoadFontFace("assets/fonts/Monocraft.ttf"))
        {
            wfz::logger::Error("Failed to load main font");
            return false;
        }

        g_main_document = LoadDocument(context, "assets/ui/main/main.rml");
        g_settings_document = LoadDocument(context, "assets/ui/settings/settings.rml");
        g_news_document = LoadDocument(context, "assets/ui/news/news.rml");

        g_info_document = LoadDocument(context, "assets/ui/info/info.rml");

        if (!g_main_document ||
            !g_settings_document ||
            !g_news_document ||
            !g_info_document)
        {
            Shutdown();
            return false;
        }

        if (!main_screen::Init(g_main_document))
        {
            wfz::logger::Error("Failed to initialize main UI screen");

            Shutdown();
            return false;
        }

        if (!info_screen::Init(g_info_document))
        {
            wfz::logger::Error("Failed to initialize info UI screen");

            Shutdown();
            return false;
        }

        const bool navigation_ok =
            BindNavigation(g_main_document, "open-settings") &&
            BindNavigation(g_main_document, "open-news") &&
            BindNavigation(g_main_document, "open-info") &&
            BindNavigation(g_settings_document, "settings-back") &&
            BindNavigation(g_news_document, "news-back") &&
            BindNavigation(g_info_document, "info-back");

        if (!navigation_ok)
        {
            wfz::logger::Error("Failed to initialize UI navigation");
            Shutdown();
            return false;
        }

        ShowScreen(Screen::Main);

        // TODO: REMOVE BEFORE MERGE
        wfz::loading_state::SetStatus("Лаунчер готов");
        wfz::loading_state::SetProgress(1.0f);
        wfz::loading_state::SetReady(true);

        return true;
    }

    void Update(const float delta_time)
    {
        main_screen::Update(delta_time);
    }

    void Shutdown()
    {
        main_screen::Shutdown();
        info_screen::Shutdown();

        CloseDocument(g_main_document);
        CloseDocument(g_settings_document);
        CloseDocument(g_news_document);
        CloseDocument(g_info_document);
    }
}
