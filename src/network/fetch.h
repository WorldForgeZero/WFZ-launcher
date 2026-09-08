#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace wfz::network
{
    nlohmann::json fetch_json(const std::string &url);
}
