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
    Rml::Element *g_scroll = nullptr;
    Rml::Element *g_message = nullptr;
    Rml::Element *g_details = nullptr;
    Rml::Element *g_details_toggle = nullptr;
    Rml::Element *g_action_button = nullptr;
    wfz::alerts::Action g_action = wfz::alerts::Action::None;
    bool g_open = false;
    bool g_has_details = false;
    bool g_show_details = false;

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

    void SetDetailsVisible(const bool visible)
    {
        g_show_details = visible && g_has_details;
        g_details->SetProperty("display", g_show_details ? "block" : "none");
        g_details_toggle->SetInnerRML(g_show_details
                                          ? "<span class=\"button-label\">СКРЫТЬ ДЕТАЛИ</span>"
                                          : "<span class=\"button-label\">ПОДРОБНОСТИ</span>");
    }

    void Dismiss()
    {
        g_open = false;
        g_action = wfz::alerts::Action::None;
        g_has_details = false;
        g_show_details = false;
        if (g_document)
            g_document->Hide();
    }

    class Listener final : public Rml::EventListener
    {
    public:
        enum class Kind
        {
            Accept,
            Dismiss,
            ToggleDetails
        };
        explicit Listener(Kind kind) : kind_(kind) {}

        void ProcessEvent(Rml::Event &) override
        {
            if (kind_ == Kind::ToggleDetails)
            {
                if (g_has_details)
                    SetDetailsVisible(!g_show_details);

                return;
            }

            const auto action = g_action;
            Dismiss();
            if (kind_ != Kind::Accept)
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
        Kind kind_;
    };

    Listener g_accept{Listener::Kind::Accept};
    Listener g_dismiss{Listener::Kind::Dismiss};
    Listener g_toggle{Listener::Kind::ToggleDetails};
}

namespace wfz::ui::alerts_screen
{
    bool Init(Rml::ElementDocument *document)
    {
        g_document = document;
        if (!document)
            return false;
        g_title = document->GetElementById("alert-title");
        g_scroll = document->GetElementById("alert-scroll");
        g_message = document->GetElementById("alert-message");
        g_details = document->GetElementById("alert-details");
        g_details_toggle = document->GetElementById("alert-details-toggle");
        g_action_button = document->GetElementById("alert-action");
        auto *close = document->GetElementById("alert-dismiss");
        if (!g_title || !g_scroll || !g_message || !g_details || !g_details_toggle || !g_action_button || !close)
        {
            wfz::logger::Error("Alert popup document is incomplete");
            return false;
        }
        g_action_button->AddEventListener("click", &g_accept);
        g_details_toggle->AddEventListener("click", &g_toggle);
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

        g_has_details = !next.details.empty();
        g_details_toggle->SetProperty("display", g_has_details ? "flex" : "none");
        SetDetailsVisible(false);
        g_scroll->SetScrollTop(0.0f);

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
        g_scroll = nullptr;
        g_message = nullptr;
        g_details = nullptr;
        g_details_toggle = nullptr;
        g_action_button = nullptr;
        g_document = nullptr;
        wfz::alerts::Clear();
    }
}
