#pragma once

#include <httplib/httplib.h>

#include <string>

namespace wfz::network::detail
{
    struct ParsedUrl
    {
        std::string origin;
        std::string target;
    };

    ParsedUrl parse_url(const std::string &url);

    httplib::Result get_resp(const std::string &url);
}
