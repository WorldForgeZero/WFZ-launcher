#include "master.h"

#include "network/download.h"
#include "network/fetch.h"

#include "ui/loading/loading_state.h"

#include "etc/paths.h"
#include "etc/self_update.h"
#include "etc/skull.h"

#include "settings/settings.h"
#include "settings/version.h"

#include <array>
#include <charconv>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace wfzp = wfz::paths;
namespace wfzl = wfz::loading_state;
namespace wfzn = wfz::network;

namespace
{
    constexpr const char *manifest_url = "https://raw.githubusercontent.com/WorldForgeZero/WFZ-launcher/refs/heads/master/jsons/master_manifest.json";

    struct ParsedVersion
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
        {
            throw std::invalid_argument("Empty numeric version component");
        }

        int result = 0;

        const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);

        if (ec != std::errc{} || ptr != value.data() + value.size() || result < 0)
        {
            throw std::invalid_argument("Invalid numeric version component");
        }

        return result;
    }

    std::string ToLower(std::string value)
    {
        for (char &character : value)
        {
            if (character >= 'A' && character <= 'Z')
            {
                character = static_cast<char>(character - 'A' + 'a');
            }
        }

        return value;
    }

    ParsedVersion ParseVersion(std::string_view version)
    {
        ParsedVersion result;

        // Ignore build metadata:
        // 1.2.3-rc.1+abcdef
        const std::size_t plus_position = version.find('+');

        if (plus_position != std::string_view::npos)
        {
            version = version.substr(0, plus_position);
        }

        const std::size_t dash_position = version.find('-');
        const std::string_view core = version.substr(0, dash_position);
        const std::size_t first_dot = core.find('.');

        if (first_dot == std::string_view::npos)
        {
            throw std::invalid_argument("Invalid version");
        }

        const std::size_t second_dot = core.find('.', first_dot + 1);

        if (second_dot == std::string_view::npos)
        {
            throw std::invalid_argument("Invalid version");
        }

        if (core.find('.', second_dot + 1) != std::string_view::npos)
        {
            throw std::invalid_argument("Invalid version");
        }

        result.major = ParseNumber(core.substr(0, first_dot));

        result.minor = ParseNumber(core.substr(first_dot + 1, second_dot - first_dot - 1));

        result.patch = ParseNumber(core.substr(second_dot + 1));

        if (dash_position == std::string_view::npos)
        {
            return result;
        }

        const std::string_view prerelease = version.substr(dash_position + 1);

        if (prerelease.empty())
        {
            throw std::invalid_argument("Empty prerelease");
        }

        const std::size_t prerelease_dot = prerelease.find('.');

        if (prerelease_dot == std::string_view::npos)
        {
            result.prerelease = ToLower(std::string(prerelease));

            return result;
        }

        if (prerelease.find('.', prerelease_dot + 1) != std::string_view::npos)
        {
            throw std::invalid_argument("Invalid prerelease");
        }

        result.prerelease = ToLower(std::string(prerelease.substr(0, prerelease_dot)));

        result.prerelease_number = ParseNumber(prerelease.substr(prerelease_dot + 1));

        return result;
    }

    int GetPrereleaseRank(const std::string &tag, const nlohmann::json &order)
    {
        const auto iterator = order.find(tag);

        if (iterator == order.end())
        {
            throw std::invalid_argument("Unknown prerelease tag: " + tag);
        }

        if (!iterator->is_number_integer())
        {
            throw std::invalid_argument("Prerelease rank is not an integer");
        }

        return iterator->get<int>();
    }

    int CompareVersions(const ParsedVersion &left, const ParsedVersion &right, const nlohmann::json &prerelease_order)
    {
        const std::array<int, 3> left_numbers{
            left.major,
            left.minor,
            left.patch};

        const std::array<int, 3> right_numbers{
            right.major,
            right.minor,
            right.patch};

        if (left_numbers < right_numbers)
            return -1;

        if (left_numbers > right_numbers)
            return 1;

        const bool left_stable = left.prerelease.empty();

        const bool right_stable = right.prerelease.empty();

        if (left_stable && right_stable)
        {
            return 0;
        }

        // Stable release is always newer than
        // any prerelease with the same numeric version.
        if (left_stable)
            return 1;

        if (right_stable)
            return -1;

        const int left_rank = GetPrereleaseRank(left.prerelease, prerelease_order);

        const int right_rank = GetPrereleaseRank(right.prerelease, prerelease_order);

        if (left_rank < right_rank)
            return -1;

        if (left_rank > right_rank)
            return 1;

        if (left.prerelease_number < right.prerelease_number)
            return -1;

        if (left.prerelease_number > right.prerelease_number)
            return 1;

        return 0;
    }

    int CompareVersions(std::string_view left, std::string_view right, const nlohmann::json &prerelease_order)
    {
        return CompareVersions(ParseVersion(left), ParseVersion(right), prerelease_order);
    }

    void SetUpDirs()
    {
        // Ересь
        fs::create_directories(wfzp::TempDir());
        fs::create_directories(wfzp::ConfigDir());
        fs::create_directories(wfzp::GameDir());
        fs::create_directories(wfzp::LauncherDir());
        fs::create_directories(wfzp::LogsDir());
    }

    fs::path GetUpdatePath()
    {
#ifdef _WIN32
        return wfzp::TempDir() / "wfz_launcher_update.exe";
#else
        return wfzp::TempDir() / "wfz_launcher_update";
#endif
    }

    std::string GetLauncherDownloadUrl(const nlohmann::json &launcher_block)
    {
#ifdef _WIN32
        return launcher_block.at("base_download_url").get<std::string>() +
               launcher_block.at("asset_win").get<std::string>();
#else
        return launcher_block.at("base_download_url").get<std::string>() +
               launcher_block.at("asset_linux").get<std::string>();
#endif
    }

    bool InstallLauncherUpdate(const nlohmann::json &launcher_block)
    {
        const std::string download_url = GetLauncherDownloadUrl(launcher_block);

        const fs::path update_path = GetUpdatePath();

        std::error_code ec;
        fs::remove(update_path, ec);

        wfzl::SetStatus("Загрузка обновления лаунчера...");

        wfzn::download(download_url, update_path);

        wfzl::SetStatus("Подготовка обновления лаунчера...");

        if (!wfz::self_update::Begin(update_path))
        {
            throw std::runtime_error("Failed to start launcher self-update");
        }

        return true;
    }

    bool ProcessLauncherBlock(const nlohmann::json &launcher_block, const nlohmann::json &versioning_block)
    {
        const std::string latest_version = launcher_block.at("latest_ver").get<std::string>();
        const std::string min_support = launcher_block.at("min_support").get<std::string>();
        const auto &prerelease_order = versioning_block.at("prerelease_order");

        /*
         * Manifest sanity check.
         *
         * min_support must never be newer than latest_ver.
         */
        if (CompareVersions(min_support, latest_version, prerelease_order) > 0)
        {
            throw std::runtime_error("Manifest min_support is newer than latest_ver");
        }

        const int current_vs_latest = CompareVersions(launcher_version, latest_version, prerelease_order);

        const int current_vs_minimum = CompareVersions(launcher_version, min_support, prerelease_order);

        /*
         * Local build is equal or newer than the remote latest version.
         * Never downgrade.
         */
        if (current_vs_latest >= 0)
        {
            return false;
        }

        const bool mandatory_update = current_vs_minimum < 0;

        const auto settings = wfz::settings::GetSnapshot();

        /*
         * min_support is a compatibility floor.
         * It ignores optional update settings.
         */
        if (mandatory_update)
        {
            wfzl::SetStatus("Требуется обновление лаунчера...");

            return InstallLauncherUpdate(launcher_block);
        }

        /*
         * User disabled ordinary launcher update checks.
         */
        if (!settings.check_launcher_updates)
        {
            return false;
        }

        if (settings.launcher_auto_update)
        {
            wfzl::SetStatus("Найдена новая версия лаунчера...");

            return InstallLauncherUpdate(launcher_block);
        }

        /*
         * Optional update exists, but automatic installation
         * is disabled.
         *
         * TODO: Send update information to the UI popup.
         */
        wfzl::SetStatus("Доступно обновление лаунчера.");

        return false;
    }
}

void DoMagic()
{
    wfzl::SetStatus("Настройка директорий лаунчера...");

    SetUpDirs();

    wfzl::SetStatus("Загрузка манифеста лаунчера...");

    const nlohmann::json manifest = wfzn::fetch_json(manifest_url);

    const bool update_started = ProcessLauncherBlock(manifest.at("launcher"), manifest.at("versioning"));

    if (update_started)
    {
        /*
         * IMPORTANT:
         * This runs in the worker thread.
         *
         * Do not call exit() or CloseWindow() here.
         * Request normal shutdown of the main loop.
         */
        wfz::app::RequestExit();
        return;
    }

    // Continue bootstrap magic here.
}
