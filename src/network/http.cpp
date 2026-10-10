#include "http.h"

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#error "cpp-httplib must be compiled with CPPHTTPLIB_OPENSSL_SUPPORT for HTTPS"
#endif

#include <httplib/httplib.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>

#include "app/exit_req.h"
#include "app/transfer_state.h"
#include "crypto/sha256.h"

namespace fs = std::filesystem;

namespace
{
    struct Parsed
    {
        std::string origin;
        std::string path;
    };

    Parsed ParseHttps(const std::string &url)
    {
        constexpr const char *prefix = "https://";
        if (url.compare(0, 8, prefix) != 0)
            throw wfz::network::Error("Only HTTPS URLs are supported");

        const std::size_t path = url.find_first_of("/?#", 8);
        const std::string origin = url.substr(0, path);
        if (origin.size() <= 8 || origin.find('@') != std::string::npos)
            throw wfz::network::Error("Invalid HTTPS host");

        const std::string resource = path == std::string::npos ? "/" : (url[path] == '/' ? url.substr(path) : "/" + url.substr(path));
        const auto fragment = resource.find('#');
        return {origin, resource.substr(0, fragment)};
    }

    std::unique_ptr<httplib::Client> MakeClient(const Parsed &url)
    {
        auto client = std::make_unique<httplib::Client>(url.origin);
        client->set_follow_location(true);
        client->enable_system_ca(true);
        client->set_connection_timeout(6, 0);
        client->set_read_timeout(15, 0);
        client->set_write_timeout(15, 0);
        return client;
    }

    void CheckResponse(const httplib::Result &result)
    {
        if (wfz::app::ExitRequested())
            throw wfz::network::Error("Request cancelled: launcher is exiting");

        if (!result)
            throw wfz::network::Error("Connection/TLS/timeout error: " + httplib::to_string(result.error()));

        if (result->status < 200 || result->status >= 300)
        {
            std::ostringstream message;
            message << "HTTP " << result->status;
            if (result->status == 403 || result->status == 451)

                message << " (access denied or restricted)";
            if (result->status == 404)
                message << " (file not found)";

            throw wfz::network::Error(message.str());
        }
    }

    struct Meter
    {
        std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point last = started;
        std::uint64_t received = 0;
        std::uint64_t total = 0;

        void Update(std::uint64_t bytes, std::uint64_t length, bool force = false)
        {
            received = bytes;
            total = length;
            const auto now = std::chrono::steady_clock::now();
            if (!force && now - last < std::chrono::milliseconds(250))
                return;

            last = now;
            const double seconds = std::chrono::duration<double>(now - started).count();
            wfz::transfer_state::Tick(received, total, seconds > 0 ? received / seconds : 0.0);
        }
    };

    void Commit(const fs::path &part, const fs::path &target)
    {
        fs::path backup = target;
        backup += ".previous";
        std::error_code ec;
        if (!fs::exists(target, ec) && fs::exists(backup, ec))
        {
            fs::rename(backup, target, ec);
            if (ec)
                throw wfz::network::Error("Cannot recover previous file: " + ec.message());
        }

        ec.clear();
        fs::remove(backup, ec);
        ec.clear();
        const bool had_previous = fs::exists(target, ec) && !ec;
        if (had_previous)
        {
            fs::rename(target, backup, ec);
            if (ec)
                throw wfz::network::Error("Cannot back up previous file: " + ec.message());
        }

        fs::rename(part, target, ec);
        if (ec)
        {
            if (had_previous)
            {
                std::error_code recovery;
                fs::rename(backup, target, recovery);
                if (recovery)
                    throw wfz::network::Error("Cannot install or restore file: " + ec.message() + "; rollback: " + recovery.message());
            }
            throw wfz::network::Error("Cannot install downloaded file: " + ec.message());
        }

        fs::remove(backup, ec);
    }

    struct RemovePartOnFail
    {
        fs::path path;
        ~RemovePartOnFail()
        {
            std::error_code ec;
            fs::remove(path, ec);
        }
    };
}

namespace wfz::network
{
    std::string GetText(const std::string &url, const std::string &task, std::size_t max_bytes)
    {
        wfz::transfer_state::SetTask(task, url);
        const Parsed parsed = ParseHttps(url);
        auto client = MakeClient(parsed);
        std::string body;
        Meter meter;
        bool too_large = false;
        const auto response = client->Get(parsed.path, [&](const char *data, std::size_t size)
                                          {
                if (wfz::app::ExitRequested()) return false;
                if (size > max_bytes || body.size() > max_bytes - size)
                {
                    too_large = true;
                    return false;
                }
                body.append(data, size);
                meter.Update(body.size(), meter.total);
                return true; }, [&](std::uint64_t current, std::uint64_t total)
                                          {
                meter.Update(current, total);
                return !wfz::app::ExitRequested(); });

        if (too_large)
            throw Error(task + ": response exceeds " + std::to_string(max_bytes) + " bytes");

        CheckResponse(response);
        meter.Update(body.size(), meter.total, true);
        return body;
    }

    void WriteAtomic(const fs::path &destination, const std::string &body)
    {
        fs::create_directories(destination.parent_path());
        fs::path part = destination;
        part += ".part";
        RemovePartOnFail cleanup{part};
        {
            std::ofstream file(part, std::ios::binary | std::ios::trunc);
            if (!file || !file.write(body.data(), static_cast<std::streamsize>(body.size())))
                throw Error("Cannot write temporary file: " + part.string());

            file.close();
            if (!file)
                throw Error("Failed to finish writing: " + part.string());
        }
        Commit(part, destination);
    }

    void DownloadFile(const std::string &url, const fs::path &destination, const std::string &task, const std::string &expected_sha256)
    {
        wfz::transfer_state::SetTask(task, url);
        const Parsed parsed = ParseHttps(url);
        auto client = MakeClient(parsed);
        fs::create_directories(destination.parent_path());
        fs::path part = destination;
        part += ".part";
        RemovePartOnFail cleanup{part};
        std::ofstream output(part, std::ios::binary | std::ios::trunc);

        if (!output)
            throw Error("Cannot open temporary destination file");

        bool failed_write = false;
        Meter meter;
        std::uint64_t received = 0;
        const auto response = client->Get(parsed.path, [&](const char *data, std::size_t size)
                                          {
                if (wfz::app::ExitRequested()) return false;
                output.write(data, static_cast<std::streamsize>(size));
                if (!output)
                {
                    failed_write = true;
                    return false;
                }
                received += size;
                meter.Update(received, meter.total);
                return true; }, [&](std::uint64_t current, std::uint64_t total)
                                          {
                meter.Update(current, total);
                return !wfz::app::ExitRequested(); });

        output.close();
        if (failed_write || !output)
            throw Error("Failed writing downloaded file to disk");

        CheckResponse(response);
        meter.Update(received, meter.total, true);
        wfz::transfer_state::SetPhase(wfz::transfer_state::Phase::Validating);
        if (!expected_sha256.empty())
        {
            if (wfz::crypto::sha256_file(part) != expected_sha256)
                throw Error("SHA-256 mismatch for downloaded file");
        }
        Commit(part, destination);
    }
}
