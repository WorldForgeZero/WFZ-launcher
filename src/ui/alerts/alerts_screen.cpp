#include "alerts_screen.h"

#include <RmlUi/Core.h>

#include <string>

#include "app/alerts.h"
#include "daemon/bootstrap.h"
#include "etc/logger.h"

namespace
{
    Rml::ElementDocument *g_document = nullptr;
    Rml::Element *g_title = nullptr;
    Rml::Element *g_message = nullptr;
    Rml::Element *g_details = nullptr;
    Rml::Element *g_action_button = nullptr;
    wfz::alerts::Action g_action = wfz::alerts::Action::None;
    bool g_open = false;

    std::string EscapeRml(const std::string &source)
    {
        std::string escaped;
        for (char c : source)
        {
            switch (c)
            {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            default:
                escaped += c;
                break;
            }
        }
        return escaped;
    }

    void Dismiss()
    {
        g_open = false;
        g_action = wfz::alerts::Action::None;
        if (g_document)
            g_document->Hide();
    }

    class Listener final : public Rml::EventListener
    {
    public:
        explicit Listener(bool primary) : primary_(primary) {}

        void ProcessEvent(Rml::Event &) override
        {
            const auto action = g_action;
            Dismiss();
            if (!primary_)
                return;
            switch (action)
            {
            case wfz::alerts::Action::RetryAll:
                wfz::daemon::RetryAll();
                break;

            case wfz::alerts::Action::InstallUpdate:
                wfz::daemon::InstallUpdate();
                break;

            case wfz::alerts::Action::None:
                break;
            }
        }

    private:
        bool primary_;
    };

    Listener g_accept{true};
    Listener g_dismiss{false};
}

namespace wfz::ui::alerts_screen
{
    bool Init(Rml::ElementDocument *document)
    {
        g_document = document;
        if (!document)
            return false;

        g_title = document->GetElementById("alert-title");
        g_message = document->GetElementById("alert-message");
        g_details = document->GetElementById("alert-details");
        g_action_button = document->GetElementById("alert-action");
        auto *close = document->GetElementById("alert-dismiss");

        if (!g_title || !g_message || !g_details || !g_action_button || !close)
        {
            wfz::logger::Error("Alert popup document is incomplete");
            return false;
        }

        g_action_button->AddEventListener("click", &g_accept);
        close->AddEventListener("click", &g_dismiss);
        document->Hide();
        return true;
    }

    void Update()
    {
        if (!g_document || g_open)
            return;

        wfz::alerts::Alert next;
        if (!wfz::alerts::TryPop(next))
            return;

        g_action = next.action;
        g_title->SetInnerRML(EscapeRml(next.title));
        g_message->SetInnerRML(EscapeRml(next.message));
        g_details->SetInnerRML(EscapeRml(next.details));
        if (next.action == wfz::alerts::Action::None)
            g_action_button->SetProperty("display", "none");

        else
        {
            g_action_button->SetProperty("display", "flex");
            g_action_button->SetInnerRML("<span class=\"button-label\">" + EscapeRml(next.action_label) + "</span>");
        }
        g_open = true;
        g_document->Show();
    }

    void Shutdown()
    {
        Dismiss();
        g_title = nullptr;
        g_message = nullptr;
        g_details = nullptr;
        g_action_button = nullptr;
        g_document = nullptr;
        wfz::alerts::Clear();
    }
}
