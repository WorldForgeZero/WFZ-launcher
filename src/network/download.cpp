#include "network/download.h"
#include "network/url.h"

#include <httplib/httplib.h>

#include <fstream>
#include <stdexcept>
#include <string>

namespace wfz::network
{
    void download(const std::string &url, const std::filesystem::path &destination)
    {
        const auto parsed = detail::parse_url(url);

        if (!destination.parent_path().empty())
            std::filesystem::create_directories(destination.parent_path());

        std::filesystem::path temporary = destination;
        temporary += ".part";

        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output)
            throw std::runtime_error("Failed to open destination file");

        httplib::Client client(parsed.origin);
        client.set_follow_location(true);
        client.enable_system_ca(true);

        const auto response = client.Get(
            parsed.target,
            [&](const char *data, std::size_t size)
            {
                output.write(data, static_cast<std::streamsize>(size));
                return static_cast<bool>(output);
            });

        output.close();

        if (!response)
        {
            std::filesystem::remove(temporary);
            throw std::runtime_error("HTTP request failed");
        }

        if (response->status < 200 || response->status >= 300)
        {
            std::filesystem::remove(temporary);
            throw std::runtime_error("HTTP request returned status " + std::to_string(response->status));
        }

        if (!output)
        {
            std::filesystem::remove(temporary);
            throw std::runtime_error("Failed to write downloaded file");
        }

        std::filesystem::rename(temporary, destination);
    }
}
