#include "news_screen.h"

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <RmlUi/Core.h>
#include <nlohmann/json.hpp>

#include "crypto/sha256.h"
#include "etc/logger.h"
#include "etc/paths.h"
#include "settings/settings.h"

namespace
{
    using Feed = wfz::ui::news_screen::Feed;
    using json = nlohmann::json;

    enum class LoadState
    {
        Waiting,
        Loading,
        Ready,
        Error,
        Unavailable,
    };

    struct NewsItem
    {
        std::string kind;
        std::string text;
    };

    struct NewsAuthor
    {
        std::string name;
        std::vector<NewsItem> items;
    };

    struct NewsDay
    {
        std::string date;
        std::vector<NewsAuthor> authors;
    };

    struct FeedData
    {
        LoadState state = LoadState::Waiting;
        std::string markup;
        std::string error;
        bool has_entries = false;
        bool unread = false;
        std::optional<std::string> cached_sha256;
        std::size_t version = 0;
    };

    struct EventBinding
    {
        Rml::Element *element = nullptr;
        std::unique_ptr<Rml::EventListener> listener;
    };

    constexpr std::array<std::pair<const char *, const char *>, 4> CATEGORIES = {{
        {"add", "ADD"},
        {"fix", "FIX"},
        {"rm", "REM"},
        {"tweak", "TWEAK"},
    }};

    Rml::Element *g_list = nullptr;
    Rml::Element *g_scroll_container = nullptr;
    Rml::Element *g_status = nullptr;
    Rml::Element *g_tab_launcher = nullptr;
    Rml::Element *g_tab_game = nullptr;
    Rml::Element *g_dot_launcher = nullptr;
    Rml::Element *g_dot_game = nullptr;

    std::array<FeedData, 2> g_feeds;
    std::vector<EventBinding> g_bindings;
    wfz::ui::news_screen::ReadCallback g_on_read;

    Feed g_selected = Feed::Launcher;
    bool g_visible = false;
    std::optional<Feed> g_rendered_feed;
    std::size_t g_rendered_version = 0;
    std::string g_rendered_status;

    std::size_t Index(const Feed feed)
    {
        return feed == Feed::Launcher ? 0u : 1u;
    }

    FeedData &Data(const Feed feed)
    {
        return g_feeds[Index(feed)];
    }

    bool IsEnabled(const Feed feed)
    {
        const WFZSettingId setting = feed == Feed::Launcher
                                         ? WFZSettingId::LoadLauncherNews
                                         : WFZSettingId::LoadGameNews;

        return wfz::settings::GetBool(setting);
    }

    Rml::Element *RequireElement(Rml::ElementDocument *document, const char *id)
    {
        Rml::Element *element = document->GetElementById(id);

        if (!element)
            wfz::logger::Error("Failed to find RmlUi element: %s", id);

        return element;
    }

    Rml::String EscapeRml(std::string_view text)
    {
        Rml::String escaped;
        escaped.reserve(text.size());

        for (const char c : text)
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
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped += c;
                break;
            }
        }

        return escaped;
    }

    NewsAuthor &FindAuthor(NewsDay &day, const std::string &name)
    {
        const auto it = std::find_if(
            day.authors.begin(),
            day.authors.end(),
            [&name](const NewsAuthor &author)
            { return author.name == name; });

        if (it != day.authors.end())
            return *it;

        day.authors.push_back({name, {}});
        return day.authors.back();
    }

    bool ParseContent(std::string_view content, std::vector<NewsDay> &days)
    {
        const json root = json::parse(content.begin(), content.end());

        if (!root.is_array())
            return false;

        for (const auto &day_json : root)
        {
            if (!day_json.is_object() ||
                !day_json.contains("date") ||
                !day_json["date"].is_string())
            {
                continue;
            }

            NewsDay day;
            day.date = day_json["date"].get<std::string>();

            for (const auto &[field, kind] : CATEGORIES)
            {
                const auto category = day_json.find(field);
                if (category == day_json.end() || !category->is_array())
                    continue;

                for (const auto &entry : *category)
                {
                    if (!entry.is_array() || entry.size() < 2 || !entry[0].is_string())
                        continue;

                    const std::string author = entry[0].get<std::string>();
                    if (author.empty())
                        continue;

                    for (std::size_t i = 1; i < entry.size(); ++i)
                    {
                        if (!entry[i].is_string())
                            continue;

                        const std::string change = entry[i].get<std::string>();
                        if (change.empty())
                            continue;

                        FindAuthor(day, author).items.push_back({kind, change});
                    }
                }
            }

            if (!day.authors.empty())
            {
                std::sort(day.authors.begin(), day.authors.end(),
                          [](const NewsAuthor &a, const NewsAuthor &b)
                          {
                              return a.name < b.name;
                          });
                days.push_back(std::move(day));
            }
        }

        std::stable_sort(days.begin(), days.end(),
                         [](const NewsDay &a, const NewsDay &b)
                         {
                             return a.date > b.date;
                         });

        return true;
    }

    Rml::String BuildMarkup(const std::vector<NewsDay> &days)
    {
        Rml::String markup;
        markup.reserve(4096);

        for (const NewsDay &day : days)
        {
            markup += "<div class=\"news-day\">";
            markup += "<div class=\"news-date\">";
            markup += EscapeRml(day.date);
            markup += "</div>";

            for (const NewsAuthor &author : day.authors)
            {
                markup += "<div class=\"news-author\">";
                markup += "<div class=\"news-author-name\">";
                markup += EscapeRml(author.name);
                markup += "</div>";

                for (const NewsItem &item : author.items)
                {
                    markup += "<div class=\"news-item\">";
                    markup += "<div class=\"news-kind\">";
                    markup += EscapeRml(item.kind);
                    markup += "</div>";
                    markup += "<div class=\"news-item-text\">";
                    markup += EscapeRml(item.text);
                    markup += "</div></div>";
                }

                markup += "</div>";
            }

            markup += "</div>";
        }

        return markup;
    }

    void RefreshDots()
    {
        if (g_dot_launcher)
        {
            const bool show = Data(Feed::Launcher).unread && Data(Feed::Launcher).has_entries && IsEnabled(Feed::Launcher);
            g_dot_launcher->SetProperty("display", show ? "block" : "none");
        }

        if (g_dot_game)
        {
            const bool show = Data(Feed::Game).unread && Data(Feed::Game).has_entries && IsEnabled(Feed::Game);
            g_dot_game->SetProperty("display", show ? "block" : "none");
        }
    }

    void MarkSelectedRead()
    {
        FeedData &data = Data(g_selected);

        if (!g_visible || !IsEnabled(g_selected) || !data.unread || !data.has_entries || data.state != LoadState::Ready)
            return;

        data.unread = false;
        RefreshDots();

        if (g_on_read)
            g_on_read(g_selected);
    }

    void RefreshView()
    {
        if (!g_status || !g_list || !g_tab_launcher || !g_tab_game)
            return;

        g_tab_launcher->SetClass("active", g_selected == Feed::Launcher);
        g_tab_game->SetClass("active", g_selected == Feed::Game);

        RefreshDots();

        const FeedData &data = Data(g_selected);
        const bool enabled = IsEnabled(g_selected);
        const bool show_list = enabled && data.has_entries;

        g_list->SetProperty("display", show_list ? "block" : "none");
        g_status->SetProperty("display", show_list ? "none" : "block");

        if (show_list)
        {
            const bool same_feed = g_rendered_feed && *g_rendered_feed == g_selected;
            if (!same_feed || g_rendered_version != data.version)
            {
                const float scroll_top = same_feed && g_scroll_container
                                             ? g_scroll_container->GetScrollTop()
                                             : 0.0f;

                g_list->SetInnerRML(data.markup);
                g_rendered_feed = g_selected;
                g_rendered_version = data.version;

                if (g_scroll_container)
                    g_scroll_container->SetScrollTop(scroll_top);
            }
            return;
        }

        std::string message;

        if (!enabled)
            message = "ЗАГРУЗКА НОВОСТЕЙ ОТКЛЮЧЕНА";

        else if (data.state == LoadState::Error)
            message = data.error.empty() ? "НЕ УДАЛОСЬ ЗАГРУЗИТЬ НОВОСТИ" : data.error;

        else if (data.state == LoadState::Ready)
            message = "НОВОСТЕЙ ПОКА НЕТ";

        else if (data.state == LoadState::Unavailable)
            message = "НОВОСТИ ПОКА НЕДОСТУПНЫ";

        else if (data.state == LoadState::Loading)
            message = "ЗАГРУЗКА НОВОСТЕЙ...";

        else
            message = "ОЖИДАНИЕ НОВОСТЕЙ...";

        if (g_rendered_status != message)
        {
            g_status->SetInnerRML(EscapeRml(message));
            g_rendered_status = std::move(message);
        }
    }

    void SelectFeed(const Feed feed)
    {
        g_selected = feed;
        RefreshView();
        MarkSelectedRead();
    }

    void RefreshCachedNews(const Feed feed, const std::filesystem::path &path)
    {
        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);

        if (error)
        {
            wfz::logger::Warning("Failed to check news cache: %s", path.string().c_str());
            return;
        }

        if (!exists)
            return;

        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            wfz::logger::Warning("Failed to open news cache: %s", path.string().c_str());
            wfz::ui::news_screen::SetError(feed, "НЕ УДАЛОСЬ ОТКРЫТЬ ЛОКАЛЬНЫЕ НОВОСТИ");
            return;
        }

        const std::string content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        if (file.bad())
        {
            wfz::logger::Warning("Failed to read news cache: %s", path.string().c_str());
            wfz::ui::news_screen::SetError(feed, "НЕ УДАЛОСЬ ПРОЧИТАТЬ ЛОКАЛЬНЫЕ НОВОСТИ");
            return;
        }

        try
        {
            const std::string digest = wfz::crypto::sha256(content);
            FeedData &data = Data(feed);

            if (data.cached_sha256 && *data.cached_sha256 == digest)
                return;

            data.cached_sha256 = digest;
            if (!wfz::ui::news_screen::SetContent(feed, content))
                wfz::logger::Warning("Invalid cached news JSON: %s", path.string().c_str());
        }
        catch (const std::exception &e)
        {
            wfz::logger::Warning("Failed to hash news cache '%s': %s", path.string().c_str(), e.what());
            wfz::ui::news_screen::SetError(feed, "НЕ УДАЛОСЬ ПРОВЕРИТЬ ЛОКАЛЬНЫЕ НОВОСТИ");
        }
    }

    class TabListener final : public Rml::EventListener
    {
    public:
        explicit TabListener(Feed feed) : feed_(feed) {}

        void ProcessEvent(Rml::Event &) override
        {
            SelectFeed(feed_);
        }

    private:
        Feed feed_;
    };

    void BindTab(Rml::Element *element, Feed feed)
    {
        auto listener = std::make_unique<TabListener>(feed);
        element->AddEventListener("click", listener.get());
        g_bindings.push_back({element, std::move(listener)});
    }
}

namespace wfz::ui::news_screen
{
    bool Init(Rml::ElementDocument *document)
    {
        Shutdown();

        if (!document)
            return false;

        g_list = RequireElement(document, "news-list");
        g_scroll_container = RequireElement(document, "news-content");
        g_status = RequireElement(document, "news-status");
        g_tab_launcher = RequireElement(document, "news-tab-launcher");
        g_tab_game = RequireElement(document, "news-tab-game");
        g_dot_launcher = RequireElement(document, "news-unread-launcher");
        g_dot_game = RequireElement(document, "news-unread-game");

        if (!g_list || !g_scroll_container || !g_status || !g_tab_launcher || !g_tab_game || !g_dot_launcher || !g_dot_game)
        {
            Shutdown();
            return false;
        }

        BindTab(g_tab_launcher, Feed::Launcher);
        BindTab(g_tab_game, Feed::Game);
        RefreshView();
        return true;
    }

    bool SetContent(Feed feed, std::string_view content)
    {
        std::vector<NewsDay> days;

        try
        {
            if (!ParseContent(content, days))
            {
                SetError(feed, "НЕВЕРНЫЙ ФОРМАТ НОВОСТЕЙ");
                return false;
            }
        }
        catch (const std::exception &e)
        {
            wfz::logger::Warning("Failed to parse news JSON: %s", e.what());
            SetError(feed, "НЕВЕРНЫЙ ФОРМАТ НОВОСТЕЙ");
            return false;
        }

        FeedData &data = Data(feed);
        data.markup = BuildMarkup(days);
        ++data.version;
        data.has_entries = !days.empty();
        data.state = LoadState::Ready;
        data.error.clear();

        if (feed == g_selected && g_visible)
        {
            RefreshView();
            MarkSelectedRead();
        }
        else
        {
            RefreshDots();
        }

        return true;
    }

    void SetLoading(Feed feed)
    {
        Data(feed).state = LoadState::Loading;

        if (feed == g_selected)
            RefreshView();
    }

    void SetError(Feed feed, std::string_view message)
    {
        FeedData &data = Data(feed);
        data.state = LoadState::Error;
        data.error = std::string(message);

        if (feed == g_selected)
            RefreshView();
    }

    void SetUnavailable(Feed feed)
    {
        Data(feed).state = LoadState::Unavailable;

        if (feed == g_selected)
            RefreshView();
    }

    void SetUnread(Feed feed, bool unread)
    {
        Data(feed).unread = unread;
        RefreshDots();

        if (feed == g_selected)
            MarkSelectedRead();
    }

    void SetReadCallback(ReadCallback callback)
    {
        g_on_read = std::move(callback);
    }

    void OnShow()
    {
        g_visible = true;

        RefreshCachedNews(Feed::Launcher, wfz::paths::LauncherDir() / "news_content.json");
        RefreshCachedNews(Feed::Game, wfz::paths::GameDir() / "news_content.json");

        RefreshView();
        MarkSelectedRead();
    }

    void OnHide()
    {
        g_visible = false;
    }

    void Shutdown()
    {
        for (EventBinding &binding : g_bindings)
        {
            if (binding.element && binding.listener)
                binding.element->RemoveEventListener("click", binding.listener.get());
        }

        g_bindings.clear();
        g_on_read = nullptr;
        g_feeds = {};
        g_selected = Feed::Launcher;
        g_visible = false;
        g_rendered_feed.reset();
        g_rendered_version = 0;
        g_rendered_status.clear();

        g_list = nullptr;
        g_scroll_container = nullptr;
        g_status = nullptr;
        g_tab_launcher = nullptr;
        g_tab_game = nullptr;
        g_dot_launcher = nullptr;
        g_dot_game = nullptr;
    }
}
