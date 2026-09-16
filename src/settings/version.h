#pragma once

#include <string_view>

inline constexpr const char *launcher_version = "0.0.0";

// `< 0`: left is older
// `= 0`: versions are equal
// `> 0`: left is newer
int CompareVersions(std::string_view left, std::string_view right);
