#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace wfz::resources
{
    struct ResourceView
    {
        const std::uint8_t *data = nullptr;
        std::size_t size = 0;

        explicit operator bool() const noexcept
        {
            return data != nullptr;
        }
    };

    ResourceView Find(std::string_view path) noexcept;
}
