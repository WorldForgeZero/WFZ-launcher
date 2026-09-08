#include "network/download.h"

#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        wfz::network::download(
            "https://raw.githubusercontent.com/WorldForgeZero/"
            "WFZ-launcher/refs/heads/master/master_manifest.json",
            "build/smoke/master_manifest.json");

        std::cout << "download: OK\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "Smoke test failed: "
            << error.what()
            << '\n';

        return 1;
    }
}
