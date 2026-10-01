#include "settings_screen.h"

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include "settings/settings.h"

namespace
{
    struct SettingEntryDefinition
    {
        WFZSettingId id;

        const char *element_id;
        const char *title;
        const char *description;
    };

    struct SettingSectionDefinition
    {
        const char *element_id;
        const char *title;

        const SettingEntryDefinition *entries;
        std::size_t entry_count;
    };

    constexpr SettingEntryDefinition g_news_entries[] = {
        {
            WFZSettingId::LoadLauncherNews,
            "setting-load-launcher-news",
            "Новости лаунчера",
            "Получать новости и обновления лаунчера",
        },
        {
            WFZSettingId::LoadGameNews,
            "setting-load-game-news",
            "Новости игры",
            "Получать новости World Forge Zero",
        },
    };

    constexpr SettingEntryDefinition g_behavior_entries[] = {
        {
            WFZSettingId::LaunchAfterUpdate,
            "setting-launch-after-update",
            "Запуск после обновления",
            "Сразу запускать игру после завершения обновления",
        },
        {
            WFZSettingId::CheckLauncherUpdates,
            "setting-check-launcher-updates",
            "Обновления лаунчера",
            "Проверять наличие обновлений лаунчера при запуске",
        },
        {
            WFZSettingId::LauncherAutoUpdate,
            "setting-launcher-auto-update",
            "Авто обновление лаунчера",
            "Автоматически пытаться обновить лаунчер при доступной новой версии",
        },
    };

    constexpr SettingEntryDefinition g_advanced_entries[] = {
        {
            WFZSettingId::DevMode,
            "setting-dev-mode",
            "Dev режим",
            "Включает функции для разработки. Если вы не знаете, зачем он нужен - он вам не нужен.",
        },
        {
            WFZSettingId::AdminBypass,
            "setting-admin-bypass",
            "Admin режим",
            "Отключает некоторые ограничения интерфейса лаунчера. Реальных прав не предоставляет.",
        },
    };

    constexpr SettingSectionDefinition g_sections[] = {
        {
            "settings-news",
            "НОВОСТИ",
            g_news_entries,
            std::size(g_news_entries),
        },
        {
            "settings-behavior",
            "ПОВЕДЕНИЕ",
            g_behavior_entries,
            std::size(g_behavior_entries),
        },
        {
            "settings-advanced",
            "РАСШИРЕННЫЕ",
            g_advanced_entries,
            std::size(g_advanced_entries),
        },
    };

    struct RuntimeEntry
    {
        const SettingEntryDefinition *definition = nullptr;
        Rml::Element *row = nullptr;
    };

    struct RuntimeSection
    {
        const SettingSectionDefinition *definition = nullptr;
        Rml::Element *panel = nullptr;

        std::vector<RuntimeEntry> entries;
    };

    struct EventBinding
    {
        Rml::Element *element = nullptr;
        Rml::String event;
        std::unique_ptr<Rml::EventListener> listener;
    };

    Rml::ElementDocument *g_document = nullptr;

    Rml::ElementFormControlInput *g_search = nullptr;
    Rml::Element *g_empty = nullptr;

    std::vector<RuntimeSection> g_runtime_sections;
    std::vector<EventBinding> g_event_bindings;

    Rml::String EscapeRml(std::string_view text)
    {
        Rml::String result;
        result.reserve(text.size());

        for (const char c : text)
        {
            switch (c)
            {
            case '&':
                result += "&amp;";
                break;

            case '<':
                result += "&lt;";
                break;

            case '>':
                result += "&gt;";
                break;

            case '"':
                result += "&quot;";
                break;

            default:
                result += c;
                break;
            }
        }

        return result;
    }

    std::string FoldSearchText(std::string_view text)
    {
        std::string result;
        result.reserve(text.size());

        for (std::size_t i = 0; i < text.size();)
        {
            const auto c = static_cast<unsigned char>(text[i]);

            // ASCII A-Z -> a-z.
            if (c >= 'A' && c <= 'Z')
            {
                result.push_back(
                    static_cast<char>(c + ('a' - 'A')));

                ++i;
                continue;
            }

            // UTF-8 Cyrillic.
            if (c == 0xD0 && i + 1 < text.size())
            {
                const auto next =
                    static_cast<unsigned char>(text[i + 1]);

                // Ё -> ё.
                if (next == 0x81)
                {
                    result.push_back(static_cast<char>(0xD1));
                    result.push_back(static_cast<char>(0x91));

                    i += 2;
                    continue;
                }

                // А-П -> а-п.
                if (next >= 0x90 && next <= 0x9F)
                {
                    result.push_back(static_cast<char>(0xD0));
                    result.push_back(
                        static_cast<char>(next + 0x20));

                    i += 2;
                    continue;
                }

                // Р-Я -> р-я.
                if (next >= 0xA0 && next <= 0xAF)
                {
                    result.push_back(static_cast<char>(0xD1));
                    result.push_back(
                        static_cast<char>(next - 0x20));

                    i += 2;
                    continue;
                }
            }

            result.push_back(static_cast<char>(c));

            ++i;
        }

        return result;
    }

    bool ContainsSearch(
        std::string_view text,
        const std::string &search)
    {
        if (search.empty())
            return true;

        const std::string folded = FoldSearchText(text);

        return folded.find(search) != std::string::npos;
    }

    void SetVisible(Rml::Element *element, bool visible)
    {
        if (!element)
            return;

        if (visible)
            element->RemoveProperty("display");
        else
            element->SetProperty("display", "none");
    }

    Rml::String MakeRowId(const char *element_id)
    {
        Rml::String result = "row-";
        result += element_id;

        return result;
    }

    Rml::String BuildSettingsRml()
    {
        Rml::String rml;

        for (const SettingSectionDefinition &section : g_sections)
        {
            rml += "<div class=\"settings-panel\" id=\"";
            rml += section.element_id;
            rml += "\">";

            rml += "<div class=\"settings-panel-title\">";
            rml += EscapeRml(section.title);
            rml += "</div>";

            rml += "<div class=\"settings-panel-line\"></div>";

            for (
                std::size_t i = 0;
                i < section.entry_count;
                ++i)
            {
                const SettingEntryDefinition &entry =
                    section.entries[i];

                rml += "<label class=\"setting-row\" id=\"";
                rml += MakeRowId(entry.element_id);
                rml += "\">";

                rml += "<div class=\"setting-text\">";

                rml += "<div class=\"setting-title\">";
                rml += EscapeRml(entry.title);
                rml += "</div>";

                rml += "<div class=\"setting-description\">";
                rml += EscapeRml(entry.description);
                rml += "</div>";

                rml += "</div>";

                rml += "<input id=\"";
                rml += entry.element_id;
                rml += "\" class=\"setting-toggle\" type=\"checkbox\"";

                if (wfz::settings::GetBool(entry.id))
                {
                    rml += " checked=\"checked\"";
                }

                rml += " />";

                rml += "</label>";
            }

            rml += "</div>";
        }

        rml +=
            "<div id=\"settings-empty\">"
            "Ничего не найдено"
            "</div>";

        return rml;
    }

    void ApplySearch()
    {
        if (!g_search || !g_empty)
            return;

        const std::string search =
            FoldSearchText(g_search->GetValue());

        bool any_visible = false;

        for (RuntimeSection &runtime_section : g_runtime_sections)
        {
            const SettingSectionDefinition &section =
                *runtime_section.definition;

            const bool section_matches =
                ContainsSearch(section.title, search);

            bool section_visible = false;

            for (RuntimeEntry &runtime_entry : runtime_section.entries)
            {
                const SettingEntryDefinition &entry =
                    *runtime_entry.definition;

                const bool entry_visible =
                    section_matches ||
                    ContainsSearch(entry.title, search) ||
                    ContainsSearch(entry.description, search);

                SetVisible(runtime_entry.row, entry_visible);

                if (entry_visible)
                    section_visible = true;
            }

            SetVisible(
                runtime_section.panel,
                section_visible);

            if (section_visible)
                any_visible = true;
        }

        SetVisible(g_empty, !any_visible);
    }

    class SettingChangeListener final
        : public Rml::EventListener
    {
    public:
        explicit SettingChangeListener(WFZSettingId id)
            : id_(id)
        {
        }

        void ProcessEvent(Rml::Event &event) override
        {
            Rml::Element *element =
                event.GetTargetElement();

            if (!element)
                return;

            const bool checked =
                element->HasAttribute("checked");

            wfz::settings::SetBool(id_, checked);
            wfz::settings::Save();
        }

    private:
        WFZSettingId id_;
    };

    class SearchListener final
        : public Rml::EventListener
    {
    public:
        void ProcessEvent(Rml::Event &) override
        {
            ApplySearch();
        }
    };

    void BindEvent(
        Rml::Element *element,
        const Rml::String &event,
        std::unique_ptr<Rml::EventListener> listener)
    {
        if (!element || !listener)
            return;

        element->AddEventListener(
            event,
            listener.get());

        g_event_bindings.push_back(
            {
                element,
                event,
                std::move(listener),
            });
    }

    void ClearEventBindings()
    {
        for (EventBinding &binding : g_event_bindings)
        {
            if (binding.element && binding.listener)
            {
                binding.element->RemoveEventListener(
                    binding.event,
                    binding.listener.get());
            }
        }

        g_event_bindings.clear();
    }

    bool BuildRuntimeData()
    {
        g_runtime_sections.clear();
        g_runtime_sections.reserve(std::size(g_sections));

        for (const SettingSectionDefinition &section : g_sections)
        {
            Rml::Element *panel =
                g_document->GetElementById(section.element_id);

            if (!panel)
                return false;

            RuntimeSection runtime_section;

            runtime_section.definition = &section;
            runtime_section.panel = panel;
            runtime_section.entries.reserve(section.entry_count);

            for (
                std::size_t i = 0;
                i < section.entry_count;
                ++i)
            {
                const SettingEntryDefinition &entry =
                    section.entries[i];

                const Rml::String row_id =
                    MakeRowId(entry.element_id);

                Rml::Element *row =
                    g_document->GetElementById(row_id);

                Rml::Element *input =
                    g_document->GetElementById(
                        entry.element_id);

                if (!row || !input)
                    return false;

                runtime_section.entries.push_back(
                    {
                        &entry,
                        row,
                    });

                BindEvent(
                    input,
                    "change",
                    std::make_unique<SettingChangeListener>(
                        entry.id));
            }

            g_runtime_sections.push_back(
                std::move(runtime_section));
        }

        return true;
    }
}

namespace wfz::ui::settings_screen
{
    bool Init(Rml::ElementDocument *document)
    {
        Shutdown();

        if (!document)
            return false;

        g_document = document;

        Rml::Element *content =
            document->GetElementById("settings-content");

        Rml::Element *search =
            document->GetElementById("settings-search");

        if (!content || !search)
        {
            Shutdown();
            return false;
        }

        g_search =
            dynamic_cast<Rml::ElementFormControlInput *>(search);

        if (!g_search)
        {
            Shutdown();
            return false;
        }

        content->SetInnerRML(BuildSettingsRml());

        g_empty =
            document->GetElementById("settings-empty");

        if (!g_empty)
        {
            Shutdown();
            return false;
        }

        if (!BuildRuntimeData())
        {
            Shutdown();
            return false;
        }

        BindEvent(
            search,
            "keyup",
            std::make_unique<SearchListener>());

        BindEvent(
            search,
            "change",
            std::make_unique<SearchListener>());

        ApplySearch();

        return true;
    }

    void Shutdown()
    {
        ClearEventBindings();

        g_runtime_sections.clear();

        g_search = nullptr;
        g_empty = nullptr;
        g_document = nullptr;
    }
}
