#define CPPHTTPLIB_MBEDTLS_SUPPORT

#include <httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <raylib.h>

#include <mbedtls/md.h>

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

static std::string sha256(const std::string &data)
{
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    if (md_info == nullptr)
    {
        throw std::runtime_error("SHA-256 is not available");
    }

    std::array<unsigned char, 32> hash{};

    const int result = mbedtls_md(
        md_info,
        reinterpret_cast<const unsigned char *>(data.data()),
        data.size(),
        hash.data());

    if (result != 0)
    {
        throw std::runtime_error("Failed to calculate SHA-256");
    }

    std::ostringstream stream;

    stream << std::hex << std::setfill('0');

    for (unsigned char byte : hash)
    {
        stream << std::setw(2) << static_cast<int>(byte);
    }

    return stream.str();
}

int main()
{
    try
    {
        SetTraceLogLevel(LOG_WARNING);

        // nlohmann/json smoke test
        const auto json = nlohmann::json::parse(
            R"({"name":"WFZ Launcher","version":1})");

        if (json.at("name") != "WFZ Launcher")
        {
            throw std::runtime_error("JSON test failed");
        }

        // Mbed TLS SHA-256 smoke test
        const std::string hash = sha256("abc");

        if (
            hash !=
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad")
        {
            throw std::runtime_error("SHA-256 test failed");
        }

        // cpp-httplib + Mbed TLS compile/link smoke test.
        // No network request yet.
        httplib::Client client("https://example.com");
        client.set_connection_timeout(5);

        std::cout << "JSON: OK\n";
        std::cout << "Mbed TLS SHA-256: OK\n";
        std::cout << "cpp-httplib HTTPS client: OK\n";

        InitWindow(800, 450, "WFZ Launcher");
        SetTargetFPS(60);

        while (!WindowShouldClose())
        {
            BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText(
                "All dependency smoke tests passed.",
                180,
                210,
                20,
                DARKGREEN);

            EndDrawing();
        }

        CloseWindow();

        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Dependency smoke test failed: "
                  << error.what()
                  << '\n';

        return 1;
    }
}
