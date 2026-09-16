#include <raylib.h>

#include "app/exit_req.h"
#include "app/thread_manager.h"

#include "ui/cursor.h"
#include "ui/font.h"
#include "ui/screen.h"

#include "ui/info/info_menu.h"
#include "ui/main/main_menu.h"
#include "ui/settings/settings_menu.h"

#include "daemon/bootstrap.h"

#include "etc/logger.h"
#include "etc/self_update.h"

namespace wfza = wfz::app;

int main(int argc, char **argv)
{
    if (const auto result = wfz::self_update::HandleStartupArguments(argc, argv))
    {
        return *result;
    }

    constexpr int window_width = 800;
    constexpr int window_height = 450;

    wfz::logger::Init();
    wfz::logger::DumpStartupInfo();

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "World Forge Zero");
    SetWindowMinSize(window_width, window_height);

    WFZLoadFont();

    wfza::ThreadManager::instance().submit(RunDaemonBootstrap);
    WFZScreen current_screen = WFZScreen::MainMenu;
    while (!WindowShouldClose() && !wfza::ExitRequested())
    {
        WFZBeginCursorFrame();
        BeginDrawing();

        const float screen_width = static_cast<float>(GetScreenWidth());
        const float screen_height = static_cast<float>(GetScreenHeight());

        switch (current_screen)
        {
        case WFZScreen::MainMenu:
            WFZDrawMainMenu(screen_width, screen_height, current_screen);
            break;

        case WFZScreen::Settings:
            WFZDrawSettings(screen_width, screen_height, current_screen);
            break;

        case WFZScreen::Info:
            WFZDrawInfo(screen_width, screen_height, current_screen);
            break;

            // TODO: Сделать news скрин

        default:
            // Сейфгард на случай если кто-то (я) идиот и не добавил обработчик
            current_screen = WFZScreen::MainMenu;
            break;
        }

        EndDrawing();
        WFZEndCursorFrame();
    }

    wfza::RequestExit();

    WFZUnloadFont();
    CloseWindow();

    wfza::ThreadManager::instance().shutdown();
    wfz::logger::Shutdown();

    return 0;
}
