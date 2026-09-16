#include "version.h"

#include <array>
#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    struct Version
    {
        int major = 0;
        int minor = 0;
        int patch = 0;

        std::string prerelease;
        int prerelease_number = 0;
    };

    int ParseNumber(std::string_view value)
    {
        if (value.empty())
            throw std::invalid_argument("Empty version number");

        int result = 0;

        const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);

        if (ec != std::errc{} || ptr != value.data() + value.size() || result < 0)
            throw std::invalid_argument("Invalid version number");

        return result;
    }

    std::string ToLower(std::string_view value)
    {
        std::string result(value);

        for (char &character : result)
            if (character >= 'A' && character <= 'Z')
                character = static_cast<char>(character - 'A' + 'a');

        return result;
    }

    int GetPrereleaseRank(std::string_view tag)
    {
        if (tag == "dev")
            return 0;

        if (tag == "a" || tag == "alfa" || tag == "alpha")
            return 10;

        if (tag == "b" || tag == "beta")
            return 20;

        if (tag == "rc")
            return 30;

        throw std::invalid_argument("Unknown prerelease tag: " + std::string(tag));
    }

    Version ParseVersion(std::string_view value)
    {
        Version version;

        if (const auto plus = value.find('+'); plus != std::string_view::npos)
            value = value.substr(0, plus);

        const auto dash = value.find('-');

        const std::string_view numeric = value.substr(0, dash);

        const auto first_dot = numeric.find('.');

        if (first_dot == std::string_view::npos)
            throw std::invalid_argument("Invalid version");

        const auto second_dot = numeric.find('.', first_dot + 1);

        if (second_dot == std::string_view::npos)
            throw std::invalid_argument("Invalid version");

        if (numeric.find('.', second_dot + 1) != std::string_view::npos)
            throw std::invalid_argument("Invalid version");

        version.major = ParseNumber(numeric.substr(0, first_dot));

        version.minor = ParseNumber(numeric.substr(first_dot + 1, second_dot - first_dot - 1));

        version.patch = ParseNumber(numeric.substr(second_dot + 1));

        if (dash == std::string_view::npos)
            return version;

        const std::string_view prerelease = value.substr(dash + 1);

        if (prerelease.empty())
            throw std::invalid_argument("Invalid prerelease");

        const auto dot = prerelease.find('.');

        if (dot == std::string_view::npos)
        {
            version.prerelease = ToLower(prerelease);

            return version;
        }

        if (prerelease.find('.', dot + 1) != std::string_view::npos)
            throw std::invalid_argument("Invalid prerelease");

        version.prerelease = ToLower(prerelease.substr(0, dot));
        version.prerelease_number = ParseNumber(prerelease.substr(dot + 1));

        return version;
    }
}
int CompareVersions(std::string_view left, std::string_view right)
{
    const Version lhs = ParseVersion(left);
    const Version rhs = ParseVersion(right);

    const std::array<int, 3> lhs_numeric{
        lhs.major,
        lhs.minor,
        lhs.patch};

    const std::array<int, 3> rhs_numeric{
        rhs.major,
        rhs.minor,
        rhs.patch};

    if (lhs_numeric < rhs_numeric)
        return -1;

    if (lhs_numeric > rhs_numeric)
        return 1;

    const bool lhs_stable = lhs.prerelease.empty();
    const bool rhs_stable = rhs.prerelease.empty();

    if (lhs_stable && rhs_stable)
        return 0;

    if (lhs_stable)
        return 1;

    if (rhs_stable)
        return -1;

    const int lhs_rank = GetPrereleaseRank(lhs.prerelease);
    const int rhs_rank = GetPrereleaseRank(rhs.prerelease);

    if (lhs_rank < rhs_rank)
        return -1;

    if (lhs_rank > rhs_rank)
        return 1;

    if (lhs.prerelease_number < rhs.prerelease_number)
        return -1;

    if (lhs.prerelease_number > rhs.prerelease_number)
        return 1;

    return 0;
}
