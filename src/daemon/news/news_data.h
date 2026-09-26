#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wfz::news
{
    enum class Kind
    {
        Add,
        Remove,
        Fix,
        Tweak,
    };

    struct Item
    {
        Kind kind;
        std::string text;
    };

    struct Author
    {
        std::string name;
        std::vector<Item> items;
    };

    struct Day
    {
        std::string date;
        std::vector<Author> authors;
        std::string hash;
    };

    std::optional<std::vector<Day>> Load(const std::filesystem::path &path);
}
