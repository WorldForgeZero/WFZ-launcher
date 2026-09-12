#include <raylib.h>

#include "ui/cursor.h"
#include "ui/font.h"
#include "ui/screen.h"

#include "ui/info/info_menu.h"
#include "ui/main/main_menu.h"
#include "ui/settings/settings_menu.h"

#include "network/thread_manager.h"

#include "download/master.h"

int main()
{
    constexpr int window_width = 800;
    constexpr int window_height = 450;

    SetConfigFlags(
        FLAG_VSYNC_HINT |
        FLAG_WINDOW_RESIZABLE);

    InitWindow(window_width, window_height, "World Forge Zero");
    SetWindowMinSize(window_width, window_height);

    WFZLoadFont();

    ThreadManager::instance();
    ThreadManager::instance().submit(DoMagic);
    WFZScreen current_screen = WFZScreen::MainMenu;
    while (!WindowShouldClose())
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

        default:
            // Сейфгард на случай если кто-то (я) идиот и не добавил обработчик
            current_screen = WFZScreen::MainMenu;
            break;
        }

        EndDrawing();
        WFZEndCursorFrame();
    }

    WFZUnloadFont();
    CloseWindow();
    ThreadManager::instance().shutdown();

    return 0;
}
