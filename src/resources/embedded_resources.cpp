#include "embedded_resources.h"

#ifdef WFZ_EMBED_ASSETS

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace wfz::resources
{
    namespace
    {
        struct Entry
        {
            std::string_view path;
            std::size_t offset;
            std::size_t size;
        };

#include "generated/embedded_resources.inc"
    }

    ResourceView Find(
        const std::string_view path) noexcept
    {
        const Entry *begin =
            EMBEDDED_ENTRIES;

        const Entry *end =
            EMBEDDED_ENTRIES +
            EMBEDDED_ENTRY_COUNT;

        const Entry *entry =
            std::lower_bound(
                begin,
                end,
                path,
                [](const Entry &entry,
                   const std::string_view value)
                {
                    return entry.path < value;
                });

        if (entry == end ||
            entry->path != path)
        {
            return {};
        }

        return ResourceView{
            EMBEDDED_DATA + entry->offset,
            entry->size,
        };
    }
}

#else

namespace wfz::resources
{
    ResourceView Find(
        std::string_view) noexcept
    {
        return {};
    }
}

#endif
