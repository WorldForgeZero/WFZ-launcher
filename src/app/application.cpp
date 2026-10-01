#include "application.h"

#include <GLFW/glfw3.h>
#include <RmlUi/Core.h>

#include "RmlUi_Backend.h"

#include "app/exit_req.h"
#include "app/thread_manager.h"
#include "app/window_icon.h"

#include "etc/logger.h"
#include "etc/self_update.h"

#include "settings/settings.h"

#ifdef WFZ_EMBED_ASSETS
#include "ui/embedded_file_interface.h"
#endif

#include "ui/ui.h"

namespace wfz::app
{
    namespace
    {
        constexpr int WINDOW_WIDTH = 800;
        constexpr int WINDOW_HEIGHT = 450;

        class Application final
        {
        public:
            ~Application()
            {
                Shutdown();
            }

            int Run(int argc, char **argv)
            {
                if (const auto result = wfz::self_update::HandleStartupArguments(argc, argv))
                {
                    return *result;
                }

                if (!Initialize())
                    return 1;

                StartDaemon();
                MainLoop();

                return 0;
            }

        private:
            bool Initialize()
            {
                wfz::logger::Init();
                logger_initialized_ = true;

                wfz::logger::DumpStartupInfo();

                wfz::settings::Load();

                if (!Backend::Initialize("World Forge Zero", WINDOW_WIDTH, WINDOW_HEIGHT, true))
                {
                    wfz::logger::Error("Failed to initialize RmlUi backend");

                    return false;
                }

                backend_initialized_ = true;

                Rml::SetSystemInterface(Backend::GetSystemInterface());
                Rml::SetRenderInterface(Backend::GetRenderInterface());
#ifdef WFZ_EMBED_ASSETS
                Rml::SetFileInterface(&file_interface_);
#endif

                if (!Rml::Initialise())
                {
                    wfz::logger::Error("Failed to initialize RmlUi");

                    return false;
                }

                rml_initialized_ = true;

                GLFWwindow *window = glfwGetCurrentContext();

                if (!window)
                {
                    wfz::logger::Error("Failed to get GLFW window");

                    return false;
                }

                if (!SetWindowIcon(window))
                {
                    wfz::logger::Warning("Failed to set window icon");
                }

                glfwSetWindowSizeLimits(
                    window,
                    WINDOW_WIDTH,
                    WINDOW_HEIGHT,
                    GLFW_DONT_CARE,
                    GLFW_DONT_CARE);

                context_ = Rml::CreateContext("main", Rml::Vector2i(WINDOW_WIDTH, WINDOW_HEIGHT));

                if (!context_)
                {
                    wfz::logger::Error("Failed to create RmlUi context");

                    return false;
                }

                if (!wfz::ui::Init(context_))
                {
                    wfz::logger::Error("Failed to initialize UI");

                    return false;
                }

                ui_initialized_ = true;

                return true;
            }

            void StartDaemon()
            {
                // ThreadManager::instance().submit(TODO:);

                daemon_started_ = true;
            }

            void MainLoop()
            {
                double previous_time = glfwGetTime();

                while (!ExitRequested() && Backend::ProcessEvents(context_))
                {
                    const double current_time = glfwGetTime();

                    const float delta_time = static_cast<float>(current_time - previous_time);

                    previous_time = current_time;
                    wfz::ui::Update(delta_time);

                    context_->Update();

                    Backend::BeginFrame();
                    context_->Render();
                    Backend::PresentFrame();
                }
            }

            void Shutdown()
            {
                RequestExit();

                if (daemon_started_)
                {
                    ThreadManager::instance().shutdown();
                    daemon_started_ = false;
                }

                if (ui_initialized_)
                {
                    wfz::ui::Shutdown();
                    ui_initialized_ = false;
                }

                if (rml_initialized_)
                {
                    Rml::Shutdown();

                    context_ = nullptr;
                    rml_initialized_ = false;
                }

                if (backend_initialized_)
                {
                    Backend::Shutdown();
                    backend_initialized_ = false;
                }

                if (logger_initialized_)
                {
                    wfz::logger::Shutdown();
                    logger_initialized_ = false;
                }
            }

            Rml::Context *context_ = nullptr;

#ifdef WFZ_EMBED_ASSETS
            wfz::ui::EmbeddedFileInterface file_interface_;
#endif

            bool logger_initialized_ = false;
            bool backend_initialized_ = false;
            bool rml_initialized_ = false;
            bool ui_initialized_ = false;
            bool daemon_started_ = false;
        };
    }

    int Run(int argc, char **argv)
    {
        Application application;

        return application.Run(argc, argv);
    }
}
