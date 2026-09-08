#include "network/url.h"

#include <stdexcept>
#include <string>

namespace wfz::network::detail
{
    ParsedUrl parse_url(const std::string &url)
    {
        const std::size_t scheme_end = url.find("://");

        if (scheme_end == std::string::npos)
            throw std::runtime_error("Invalid URL: missing scheme");

        const std::size_t path_start = url.find('/', scheme_end + 3);

        if (path_start == std::string::npos)
            return {url, "/"};

        return {url.substr(0, path_start), url.substr(path_start)};
    }

    httplib::Result get_resp(const std::string &url)
    {
        const auto parsed = parse_url(url);

        httplib::Client client(parsed.origin);
        client.set_follow_location(true);
        client.enable_system_ca(true);

        auto response = client.Get(parsed.target);

        if (!response)
            throw std::runtime_error("HTTP request failed: " + httplib::to_string(response.error()));

        return response;
    }
}
