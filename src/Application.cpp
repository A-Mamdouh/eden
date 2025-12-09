#include "Engine/Application.hpp"

namespace Eden
{

Application::Application(const EngineConfig& config)
    : engine_(config)
{
    Log::init();

    engine_.setWindowResizeCallback(
        [this](unsigned int width, unsigned int height)
        {
            onWindowResized(width, height);
        });
}

int Application::run()
{
    EDEN_CORE_INFO("Starting application main loop");

    onInit();
    engine_.setScene(createInitialScene());
    engine_.run();
    onShutdown();

    EDEN_CORE_INFO("Application shutdown complete");

    return 0;
}

} // namespace Eden
