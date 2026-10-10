#pragma once

#include <functional>
#include <string_view>

namespace Rml
{
    class ElementDocument;
}

namespace wfz::ui::news_screen
{
    enum class Feed
    {
        Launcher,
        Game,
    };

    using ReadCallback = std::function<void(Feed)>;

    bool Init(Rml::ElementDocument *document);
    void Shutdown();

    bool SetContent(Feed feed, std::string_view content);

    void SetLoading(Feed feed);
    void SetError(Feed feed, std::string_view message = {});
    void SetUnavailable(Feed feed);
    void SetUnread(Feed feed, bool unread);

    void SetReadCallback(ReadCallback callback);

    void OnShow();
    void OnHide();
}
