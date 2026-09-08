#include "network/fetch.h"
#include "network/url.h"

#include <httplib/httplib.h>

#include <stdexcept>
#include <string>

namespace wfz::network
{
    nlohmann::json fetch_json(const std::string &url)
    {
        const auto response = detail::get_resp(url);

        if (response->status < 200 || response->status >= 300)
        {
            throw std::runtime_error("HTTP request returned status " + std::to_string(response->status));
        }

        return nlohmann::json::parse(response->body);
    }
}
