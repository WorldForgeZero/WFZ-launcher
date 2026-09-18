#include "news_data.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "crypto/sha256.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace wfz::news
{
    namespace
    {
        char LowerAscii(const char value)
        {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
        }

        bool CaseInsensitiveLess(const std::string &lhs, const std::string &rhs)
        {
            return std::lexicographical_compare(
                lhs.begin(),
                lhs.end(),
                rhs.begin(),
                rhs.end(),
                [](const char left, const char right)
                {
                    return LowerAscii(left) < LowerAscii(right);
                });
        }

        const char *GetKindHashName(
            const Kind kind)
        {
            switch (kind)
            {
            case Kind::Add:
                return "add";

            case Kind::Remove:
                return "rm";

            case Kind::Fix:
                return "fix";

            case Kind::Tweak:
                return "tweak";
            }

            return "";
        }

        void AppendHashField(std::string &data, const std::string &value)
        {
            data += std::to_string(value.size());
            data.push_back(':');
            data += value;
            data.push_back(';');
        }

        std::string CalculateDayHash(const Day &day)
        {
            std::string data;

            AppendHashField(data, day.date);

            for (const Author &author : day.authors)
            {
                AppendHashField(data, author.name);

                for (const Item &item : author.items)
                {
                    AppendHashField(data, GetKindHashName(item.kind));
                    AppendHashField(data, item.text);
                }
            }

            return wfz::crypto::sha256(data);
        }

        Author &GetOrCreateAuthor(std::vector<Author> &authors, const std::string &name)
        {
            const auto it = std::find_if(
                authors.begin(),
                authors.end(),
                [&](const Author &author)
                {
                    return author.name == name;
                });

            if (it != authors.end())
                return *it;

            authors.push_back({
                .name = name,
                .items = {},
            });

            return authors.back();
        }

        bool ContainsItem(const Author &author, const Kind kind, const std::string &text)
        {
            return std::any_of(
                author.items.begin(),
                author.items.end(),
                [&](const Item &item)
                {
                    return item.kind == kind && item.text == text;
                });
        }

        void ParseCategory(const json &day, const char *key, const Kind kind, std::vector<Author> &authors)
        {
            if (!day.contains(key))
                return;

            const json &groups = day.at(key);

            if (!groups.is_array())
                return;

            for (const json &group : groups)
            {
                if (!group.is_array() || group.size() < 2 || !group[0].is_string())
                    continue;

                const std::string author_name = group[0].get<std::string>();

                if (author_name.empty())
                    continue;

                Author &author = GetOrCreateAuthor(authors, author_name);

                for (std::size_t i = 1; i < group.size(); ++i)
                {
                    if (!group[i].is_string())
                        continue;

                    std::string text = group[i].get<std::string>();

                    if (text.empty())
                        continue;

                    if (ContainsItem(author, kind, text))
                        continue;

                    author.items.push_back({
                        .kind = kind,
                        .text = std::move(text),
                    });
                }
            }
        }

        std::vector<Day> Parse(const json &data)
        {
            std::vector<Day> result;

            if (!data.is_array())
                return result;

            result.reserve(data.size());

            for (const json &day : data)
            {
                if (!day.is_object() || !day.contains("date") || !day["date"].is_string())
                {
                    continue;
                }

                Day parsed{
                    .date = day["date"].get<std::string>(),
                    .authors = {},
                    .hash = {},
                };

                ParseCategory(day, "add", Kind::Add, parsed.authors);
                ParseCategory(day, "rm", Kind::Remove, parsed.authors);
                ParseCategory(day, "fix", Kind::Fix, parsed.authors);
                ParseCategory(day, "tweak", Kind::Tweak, parsed.authors);

                parsed.authors.erase(
                    std::remove_if(
                        parsed.authors.begin(),
                        parsed.authors.end(),
                        [](const Author &author)
                        {
                            return author.items.empty();
                        }),
                    parsed.authors.end());

                std::sort(
                    parsed.authors.begin(),
                    parsed.authors.end(),
                    [](const Author &lhs, const Author &rhs)
                    {
                        return CaseInsensitiveLess(lhs.name, rhs.name);
                    });

                parsed.hash = CalculateDayHash(parsed);

                result.push_back(std::move(parsed));
            }

            std::sort(
                result.begin(),
                result.end(),
                [](const Day &lhs, const Day &rhs)
                {
                    return lhs.date > rhs.date;
                });

            return result;
        }
    }

    std::optional<std::vector<Day>> Load(const fs::path &path)
    {
        std::ifstream file(path);

        if (!file.is_open())
            return std::nullopt;

        try
        {
            json data;
            file >> data;

            return Parse(data);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
}
