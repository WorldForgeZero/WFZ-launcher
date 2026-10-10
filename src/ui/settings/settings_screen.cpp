#include "settings_screen.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include "app/restart.h"
#include "app/thread_manager.h"
#include "etc/paths.h"
#include "etc/source_storage.h"
#include "settings/settings.h"

namespace fs = std::filesystem;

namespace
{
    enum class SettingType
    {
        Toggle,
        Text,
        Button,
    };

    enum class SettingTextId
    {
        None,
        SourceDir,
    };

    enum class SettingAction
    {
        None,
        ChangeSourceDir,
        ResetSourceDir,
        DeleteSourceData,
        ResetSettings,
        DeleteEverything,
    };

    struct SettingEntryDefinition
    {
        SettingType type;
        const char *element_id;
        const char *title;
        const char *description;

        WFZSettingId setting_id = WFZSettingId::LoadLauncherNews;

        SettingAction action = SettingAction::None;
        const char *action_label = nullptr;
        bool dangerous = false;
        SettingTextId text_id = SettingTextId::None;
    };

    struct SettingSectionDefinition
    {
        const char *element_id;
        const char *title;
        const SettingEntryDefinition *entries;
        std::size_t entry_count;
    };

    constexpr SettingEntryDefinition g_install_entries[] = {
        {
            SettingType::Text,
            "settings-source-dir",
            "Директория WFZSource",
            "Расположение данных лаунчера и игры. Изменение требует перезапуска.",
            WFZSettingId::LoadLauncherNews,
            SettingAction::ChangeSourceDir,
            "ИЗМЕНИТЬ",
            false,
            SettingTextId::SourceDir,
        },
        {
            SettingType::Button,
            "settings-source-default",
            "Стандартная директория",
            "Вернуть WFZSource рядом с исполняемым файлом",
            WFZSettingId::LoadLauncherNews,
            SettingAction::ResetSourceDir,
            "ПО УМОЛЧАНИЮ",
        },
    };

    constexpr SettingEntryDefinition g_news_entries[] = {
        {
            SettingType::Toggle,
            "setting-load-launcher-news",
            "Новости лаунчера",
            "Получать новости и обновления лаунчера",
            WFZSettingId::LoadLauncherNews,
        },
        {
            SettingType::Toggle,
            "setting-load-game-news",
            "Новости игры",
            "Получать новости World Forge Zero",
            WFZSettingId::LoadGameNews,
        },
    };

    constexpr SettingEntryDefinition g_behavior_entries[] = {
        {
            SettingType::Toggle,
            "setting-launch-after-update",
            "Запуск после обновления",
            "Сразу запускать игру после завершения обновления",
            WFZSettingId::LaunchAfterUpdate,
        },
        {
            SettingType::Toggle,
            "setting-check-launcher-updates",
            "Обновления лаунчера",
            "Проверять наличие обновлений лаунчера при запуске",
            WFZSettingId::CheckLauncherUpdates,
        },
        {
            SettingType::Toggle,
            "setting-launcher-auto-update",
            "Автообновление лаунчера",
            "Автоматически пытаться обновить лаунчер при доступной новой версии",
            WFZSettingId::LauncherAutoUpdate,
        },
    };

    constexpr SettingEntryDefinition g_advanced_entries[] = {
        {
            SettingType::Toggle,
            "setting-dev-mode",
            "Dev режим",
            "Включает функции для разработки. Если не знаете, зачем он нужен - он вам не нужен.",
            WFZSettingId::DevMode,
        },
        {
            SettingType::Toggle,
            "setting-admin-bypass",
            "Admin режим",
            "Отключает некоторые ограничения интерфейса. Реальных прав не предоставляет.",
            WFZSettingId::AdminBypass,
        },
    };

    constexpr SettingEntryDefinition g_maintenance_entries[] = {
        {
            SettingType::Button,
            "settings-source-delete",
            "Удалить данные",
            "Очистить всю директорию WFZSource, не затрагивая настройки",
            WFZSettingId::LoadLauncherNews,
            SettingAction::DeleteSourceData,
            "УДАЛИТЬ",
            true,
        },
        {
            SettingType::Button,
            "settings-settings-reset",
            "Сбросить настройки",
            "Вернуть настройки лаунчера к стандартным значениям",
            WFZSettingId::LoadLauncherNews,
            SettingAction::ResetSettings,
            "СБРОСИТЬ",
        },
        {
            SettingType::Button,
            "settings-all-delete",
            "Полная очистка",
            "Удалить WFZSource и сбросить все настройки",
            WFZSettingId::LoadLauncherNews,
            SettingAction::DeleteEverything,
            "УДАЛИТЬ ВСЁ",
            true,
        },
    };

    constexpr SettingSectionDefinition g_sections[] = {
        {"settings-install", "УСТАНОВКА", g_install_entries, std::size(g_install_entries)},
        {"settings-news", "НОВОСТИ", g_news_entries, std::size(g_news_entries)},
        {"settings-behavior", "ПОВЕДЕНИЕ", g_behavior_entries, std::size(g_behavior_entries)},
        {"settings-advanced", "РАСШИРЕННЫЕ", g_advanced_entries, std::size(g_advanced_entries)},
        {"settings-maintenance", "ОЧИСТКА", g_maintenance_entries, std::size(g_maintenance_entries)},
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
    Rml::ElementFormControlInput *g_source_input = nullptr;
    Rml::Element *g_source_status = nullptr;
    Rml::Element *g_path_dialog = nullptr;
    Rml::Element *g_path_dialog_info = nullptr;
    Rml::Element *g_path_dialog_status = nullptr;
    Rml::Element *g_delete_dialog = nullptr;
    Rml::Element *g_delete_dialog_info = nullptr;
    fs::path g_requested_dir;

    enum class DeleteChoice
    {
        Data,
        Settings,
        Both
    };
    DeleteChoice g_delete_choice = DeleteChoice::Data;

    struct TransferTask
    {
        std::mutex mutex;
        bool done = false;
        bool success = false;
        std::string error;
        fs::path destination;
        bool delete_old = false;
    };
    std::shared_ptr<TransferTask> g_transfer;
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

    std::string GetTextValue(SettingTextId id)
    {
        switch (id)
        {
        case SettingTextId::SourceDir:
            return wfz::settings::GetSourceDir().u8string();
        case SettingTextId::None:
            return {};
        }
        return {};
    }

    Rml::String MakeStatusId(const char *element_id)
    {
        return Rml::String(element_id) + "-status";
    }

    Rml::String MakeActionId(const char *element_id)
    {
        return Rml::String(element_id) + "-action";
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
            rml += "</div><div class=\"settings-panel-line\"></div>";

            for (std::size_t i = 0; i < section.entry_count; ++i)
            {
                const SettingEntryDefinition &entry = section.entries[i];

                // The same text structure is used for all setting types.
                if (entry.type == SettingType::Toggle)
                    rml += "<label class=\"setting-row setting-row-toggle\" id=\"";
                else if (entry.type == SettingType::Text)
                    rml += "<div class=\"setting-row setting-row-text\" id=\"";
                else
                    rml += "<div class=\"setting-row setting-row-button\" id=\"";

                rml += MakeRowId(entry.element_id);
                rml += "\">";
                rml += "<div class=\"setting-text\"><div class=\"setting-title\">";
                rml += EscapeRml(entry.title);
                rml += "</div><div class=\"setting-description\">";
                rml += EscapeRml(entry.description);
                rml += "</div></div>";

                switch (entry.type)
                {
                case SettingType::Toggle:
                    rml += "<input type=\"checkbox\" class=\"setting-toggle\" id=\"";
                    rml += entry.element_id;
                    rml += "\"";
                    if (wfz::settings::GetBool(entry.setting_id))
                        rml += " checked=\"checked\"";
                    rml += " />";
                    break;

                case SettingType::Text:
                    rml += "<div class=\"setting-control-line\">";
                    rml += "<input type=\"text\" class=\"setting-input\" id=\"";
                    rml += entry.element_id;
                    rml += "\" value=\"";
                    rml += EscapeRml(GetTextValue(entry.text_id));
                    rml += "\" />";
                    rml += "<button class=\"wfz-button setting-action-button\" id=\"";
                    rml += MakeActionId(entry.element_id);
                    rml += "\">";
                    rml += "<span class=\"button-label\">";
                    rml += EscapeRml(entry.action_label);
                    rml += "</span></button></div>";
                    rml += "<div class=\"setting-status\" id=\"";
                    rml += MakeStatusId(entry.element_id);
                    rml += "\"></div>";
                    break;

                case SettingType::Button:
                    rml += "<button class=\"wfz-button setting-action-button";
                    if (entry.dangerous)
                        rml += " setting-action-danger";
                    rml += "\" id=\"";
                    rml += entry.element_id;
                    rml += "\">";
                    rml += "<span class=\"button-label\">";
                    rml += EscapeRml(entry.action_label);
                    rml += "</span></button>";
                    break;
                }

                rml += entry.type == SettingType::Toggle ? "</label>" : "</div>";
            }
            rml += "</div>";
        }
        rml += "<div id=\"settings-empty\">Ничего не найдено</div>";
        return rml;
    }

    void ApplySearch()
    {
        if (!g_search || !g_empty)
            return;

        const std::string search = FoldSearchText(g_search->GetValue());
        bool any_visible = false;

        for (RuntimeSection &runtime_section : g_runtime_sections)
        {
            const SettingSectionDefinition &section = *runtime_section.definition;
            const bool section_matches = ContainsSearch(section.title, search);
            bool section_visible = false;

            for (RuntimeEntry &runtime_entry : runtime_section.entries)
            {
                const SettingEntryDefinition &entry = *runtime_entry.definition;
                const bool entry_visible = section_matches ||
                                           ContainsSearch(entry.title, search) ||
                                           ContainsSearch(entry.description, search) ||
                                           (entry.action_label && ContainsSearch(entry.action_label, search));

                SetVisible(runtime_entry.row, entry_visible);
                section_visible |= entry_visible;
            }
            SetVisible(runtime_section.panel, section_visible);
            any_visible |= section_visible;
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
        }

    private:
        WFZSettingId id_;
    };

    void ShowPathStatus(const char *message, bool error)
    {
        if (!g_source_status)
            return;

        g_source_status->SetInnerRML(message);
        g_source_status->SetProperty("color", error ? "#ff7575" : "#9fcb9f");
    }

    // Clipboard text is UTF-8 in RmlUi. Convert UTF-8 to native wide paths
    // through std::filesystem::u8path, not the Windows ANSI codepage.
    fs::path ParseUserPath(std::string value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return {};
        const auto last = value.find_last_not_of(" \t\r\n");
        value = value.substr(first, last - first + 1);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
#ifdef _WIN32
        // Both separators are accepted by Windows, but use one canonical UI form.
        std::replace(value.begin(), value.end(), '\\', '/');
#endif
        return fs::u8path(value).lexically_normal();
    }

    void ShowModal(Rml::Element *modal, bool show)
    {
        if (!modal)
            return;
        if (show)
            modal->SetProperty("display", "flex");
        else
            modal->SetProperty("display", "none");
    }

    void ShowTransferDialog(const fs::path &new_dir)
    {
        g_requested_dir = new_dir;
        const std::string current = wfz::paths::SourceDir().u8string();
        const std::string requested = new_dir.u8string();
        g_path_dialog_info->SetInnerRML(
            "Сейчас: " + EscapeRml(current) +
            "<p>Новая папка: " + EscapeRml(requested) +
            "</p><p>Выбери, что сделать с уже скачанными файлами. "
            "Копируются только game/ и launcher/. "
            "Текущие данные не удалятся до проверки копии и перезапуска.</p>");
        g_path_dialog_status->SetInnerRML("");
        ShowModal(g_path_dialog, true);
    }

    bool CommitAndRestart(const fs::path &new_dir, bool cleanup_old)
    {
        const fs::path old_dir = wfz::settings::GetSourceDir();
        if (!wfz::settings::SetSourceDir(new_dir, cleanup_old))
        {
            ShowPathStatus("Не удалось сохранить новое расположение.", true);
            return false;
        }
        if (!wfz::app::Restart())
        {
            // A failed restart must not leave deletion armed for next launch.
            if (!wfz::settings::SetSourceDir(old_dir, false))
                ShowPathStatus("Ошибка отката. Проверь настройки перед следующим запуском.", true);
            else
                ShowPathStatus("Не удалось запустить новый процесс.", true);
            return false;
        }
        return true;
    }

    class SourcePathListener final : public Rml::EventListener
    {
    public:
        explicit SourcePathListener(bool use_default) : use_default_(use_default) {}
        void ProcessEvent(Rml::Event &) override
        {
            if (!g_source_input || g_transfer)
                return;
            try
            {
                const fs::path path = use_default_
                                          ? wfz::paths::DefaultSourceDir()
                                          : ParseUserPath(g_source_input->GetValue());
                if (path.empty() || !path.is_absolute())
                {
                    ShowPathStatus("Укажи абсолютный путь к папке.", true);
                    return;
                }
                const fs::path old = wfz::paths::SourceDir();
                if (fs::weakly_canonical(old) == fs::weakly_canonical(path))
                {
                    ShowPathStatus("Это уже текущая папка установки.", false);
                    return;
                }
                std::string error;
                if (!wfz::source_storage::ValidateRoots(old, path, error))
                {
                    ShowPathStatus("Небезопасное расположение: вложенные или системные папки.", true);
                    return;
                }
                ShowTransferDialog(path);
            }
            catch (...)
            {
                ShowPathStatus("Недопустимый путь к папке.", true);
            }
        }

    private:
        bool use_default_;
    };

    class TransferChoiceListener final : public Rml::EventListener
    {
    public:
        explicit TransferChoiceListener(wfz::source_storage::Operation choice) : choice_(choice) {}
        void ProcessEvent(Rml::Event &) override
        {
            if (g_transfer)
                return;
            if (choice_ == wfz::source_storage::Operation::KeepOld ||
                choice_ == wfz::source_storage::Operation::DiscardOld)
            {
                ShowModal(g_path_dialog, false);
                CommitAndRestart(g_requested_dir,
                                 choice_ == wfz::source_storage::Operation::DiscardOld);
                return;
            }

            auto task = std::make_shared<TransferTask>();
            task->destination = g_requested_dir;
            task->delete_old = choice_ == wfz::source_storage::Operation::Move;
            g_transfer = task;
            g_path_dialog_status->SetInnerRML("Копирование и проверка файлов... Подожди.");
            const fs::path old_dir = wfz::paths::SourceDir();
            wfz::app::ThreadManager::instance().submit([task, old_dir, choice = choice_]()
                                                       {
                std::string error;
                const bool ok = wfz::source_storage::Prepare(
                    old_dir, task->destination, choice, error);
                std::lock_guard<std::mutex> lock(task->mutex);
                task->success = ok;
                task->error = std::move(error);
                task->done = true; });
        }

    private:
        wfz::source_storage::Operation choice_;
    };

    class TransferCancelListener final : public Rml::EventListener
    {
    public:
        void ProcessEvent(Rml::Event &) override
        {
            if (!g_transfer)
                ShowModal(g_path_dialog, false);
        }
    };

    class DeleteDialogListener final : public Rml::EventListener
    {
    public:
        explicit DeleteDialogListener(DeleteChoice choice) : choice_(choice) {}
        void ProcessEvent(Rml::Event &) override
        {
            if (g_transfer)
                return;
            g_delete_choice = choice_;
            std::string message;
            if (choice_ == DeleteChoice::Settings)
                message = "Сбросить все настройки лаунчера до стандартных? Данные игры останутся.";
            else if (choice_ == DeleteChoice::Both)
                message = "СБРОСИТЬ настройки И УДАЛИТЬ всю папку WFZSource?";
            else
                message = "Удалить всю текущую папку WFZSource вместе с содержимым?";
            const std::string explanation = choice_ == DeleteChoice::Settings
                                                ? "Файлы WFZSource останутся. Лаунчер перезапустится."
                                                : "Будут удалены ВСЕ файлы в WFZSource. Восстановить их нельзя. Лаунчер перезапустится.";
            g_delete_dialog_info->SetInnerRML(EscapeRml(message) +
                                              "<p>Папка: " + EscapeRml(wfz::paths::SourceDir().u8string()) +
                                              "</p><p>" + EscapeRml(explanation) + "</p>");
            ShowModal(g_delete_dialog, true);
        }

    private:
        DeleteChoice choice_;
    };

    class DeleteConfirmListener final : public Rml::EventListener
    {
    public:
        explicit DeleteConfirmListener(bool confirm) : confirm_(confirm) {}
        void ProcessEvent(Rml::Event &) override
        {
            ShowModal(g_delete_dialog, false);
            if (!confirm_)
                return;
            const fs::path active = wfz::paths::SourceDir();
            const bool reset_settings = g_delete_choice != DeleteChoice::Data;
            const bool delete_data = g_delete_choice != DeleteChoice::Settings;
            if (!wfz::settings::ScheduleMaintenance(
                    delete_data ? active : fs::path{}, reset_settings))
            {
                ShowPathStatus("Не удалось сохранить запрос на удаление.", true);
                return;
            }
            if (!wfz::app::Restart())
            {
                wfz::settings::CancelMaintenance();
                ShowPathStatus("Не удалось перезапустить лаунчер. Ничего не удалено.", true);
            }
        }

    private:
        bool confirm_;
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

    std::unique_ptr<Rml::EventListener> MakeActionListener(SettingAction action)
    {
        switch (action)
        {
        case SettingAction::ChangeSourceDir:
            return std::make_unique<SourcePathListener>(false);
        case SettingAction::ResetSourceDir:
            return std::make_unique<SourcePathListener>(true);
        case SettingAction::DeleteSourceData:
            return std::make_unique<DeleteDialogListener>(DeleteChoice::Data);
        case SettingAction::ResetSettings:
            return std::make_unique<DeleteDialogListener>(DeleteChoice::Settings);
        case SettingAction::DeleteEverything:
            return std::make_unique<DeleteDialogListener>(DeleteChoice::Both);
        case SettingAction::None:
            return nullptr;
        }
        return nullptr;
    }

    bool BuildRuntimeData()
    {
        g_runtime_sections.clear();
        g_runtime_sections.reserve(std::size(g_sections));

        for (const SettingSectionDefinition &section : g_sections)
        {
            Rml::Element *panel = g_document->GetElementById(section.element_id);
            if (!panel)
                return false;

            RuntimeSection runtime_section;
            runtime_section.definition = &section;
            runtime_section.panel = panel;
            runtime_section.entries.reserve(section.entry_count);

            for (std::size_t i = 0; i < section.entry_count; ++i)
            {
                const SettingEntryDefinition &entry = section.entries[i];
                Rml::Element *row = g_document->GetElementById(MakeRowId(entry.element_id));
                if (!row)
                    return false;

                runtime_section.entries.push_back({&entry, row});

                Rml::Element *control = nullptr;
                Rml::String event;
                std::unique_ptr<Rml::EventListener> listener;

                switch (entry.type)
                {
                case SettingType::Toggle:
                    control = g_document->GetElementById(entry.element_id);
                    event = "change";
                    listener = std::make_unique<SettingChangeListener>(entry.setting_id);
                    break;

                case SettingType::Text:
                    control = g_document->GetElementById(MakeActionId(entry.element_id));
                    event = "click";
                    listener = MakeActionListener(entry.action);
                    if (entry.action == SettingAction::ChangeSourceDir)
                    {
                        g_source_input = dynamic_cast<Rml::ElementFormControlInput *>(
                            g_document->GetElementById(entry.element_id));
                        if (!g_source_input)
                            return false;
                    }
                    break;

                case SettingType::Button:
                    control = g_document->GetElementById(entry.element_id);
                    event = "click";
                    listener = MakeActionListener(entry.action);
                    break;
                }

                if (!control || !listener)
                    return false;
                BindEvent(control, event, std::move(listener));
            }

            g_runtime_sections.push_back(std::move(runtime_section));
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

        g_source_status = document->GetElementById("settings-source-dir-status");

        g_path_dialog = document->GetElementById("settings-path-dialog");
        g_path_dialog_info = document->GetElementById("settings-path-dialog-info");
        g_path_dialog_status = document->GetElementById("settings-path-dialog-status");
        g_delete_dialog = document->GetElementById("settings-delete-dialog");
        g_delete_dialog_info = document->GetElementById("settings-delete-dialog-info");

        if (!g_source_status || !g_path_dialog ||
            !g_path_dialog_info || !g_path_dialog_status ||
            !g_delete_dialog || !g_delete_dialog_info)
        {
            Shutdown();
            return false;
        }

        if (!BuildRuntimeData())
        {
            Shutdown();
            return false;
        }

        using Op = wfz::source_storage::Operation;
        BindEvent(document->GetElementById("settings-path-copy"), "click", std::make_unique<TransferChoiceListener>(Op::Copy));
        BindEvent(document->GetElementById("settings-path-move"), "click", std::make_unique<TransferChoiceListener>(Op::Move));
        BindEvent(document->GetElementById("settings-path-keep"), "click", std::make_unique<TransferChoiceListener>(Op::KeepOld));
        BindEvent(document->GetElementById("settings-path-discard"), "click", std::make_unique<TransferChoiceListener>(Op::DiscardOld));
        BindEvent(document->GetElementById("settings-path-cancel"), "click", std::make_unique<TransferCancelListener>());
        BindEvent(document->GetElementById("settings-delete-confirm"), "click", std::make_unique<DeleteConfirmListener>(true));
        BindEvent(document->GetElementById("settings-delete-cancel"), "click", std::make_unique<DeleteConfirmListener>(false));

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

    void Update(float)
    {
        if (!g_transfer)
            return;
        bool done = false;
        bool success = false;
        std::string error;
        {
            std::lock_guard<std::mutex> lock(g_transfer->mutex);
            done = g_transfer->done;
            if (done)
            {
                success = g_transfer->success;
                error = g_transfer->error;
            }
        }
        if (!done)
            return;
        const auto task = g_transfer;
        g_transfer.reset();
        if (!success)
        {
            g_path_dialog_status->SetInnerRML("Ошибка переноса: " + EscapeRml(error));
            return;
        }
        ShowModal(g_path_dialog, false);
        CommitAndRestart(task->destination, task->delete_old);
    }

    void Shutdown()
    {
        ClearEventBindings();

        g_runtime_sections.clear();

        g_search = nullptr;
        g_source_input = nullptr;
        g_source_status = nullptr;
        g_path_dialog = nullptr;
        g_path_dialog_info = nullptr;
        g_path_dialog_status = nullptr;
        g_delete_dialog = nullptr;
        g_delete_dialog_info = nullptr;
        g_transfer.reset();
        g_empty = nullptr;
        g_document = nullptr;
    }
}
