#include "Eden/core/Application.hpp"

#include "Eden/core/Engine.hpp"
#include "core/Log.hpp"
#include "Eden/platform/Platform.hpp"
#include "Eden/render/RenderSystem.hpp"
#include "Eden/scene/SceneManager.hpp"
#include "Eden/script/ScriptSystem.hpp"

namespace Eden
{

std::unique_ptr<Application> Application::instance_ = nullptr;

Application& Application::getInstance()
{
    EDEN_ASSERT(instance_ != nullptr, "Application has not been created yet");
    return *instance_;
}

void Application::setInstance(std::unique_ptr<Application> instance) {
    instance_ = std::move(instance);
}

Application::Application(const AppConfig& config)
    : config_(config)
{
    Log::init();
}

Application::Application(const EngineConfig& config)
: Application(AppConfig{config})
{}

Application::~Application() = default;

void Application::exit() {
    Engine::getInstance().stop();
}

int Application::run()
{
    EDEN_CORE_INFO("Starting application main loop");

    auto initialScene = createInitialScene();
    if (initialScene)
    {
        auto scenePtr = std::shared_ptr<Scene>(std::move(initialScene));
        auto& sceneManager = Engine::getInstance().sceneManager();
        sceneManager.setScene(scenePtr);
        Engine::getInstance().renderSystem().setActiveScene(scenePtr);
        ScriptContext ctx{};
        ctx.input = Engine::getInstance().platform().getInput().get();
        ctx.scene = scenePtr.get();
        ctx.application = &Application::getInstance();
        Engine::getInstance().scriptSystem().setContext(ctx);
    }

    onInit();
    Engine::getInstance().run();
    onShutdown();

    EDEN_CORE_INFO("Application shutdown complete");

    return 0;
}

} // namespace Eden
