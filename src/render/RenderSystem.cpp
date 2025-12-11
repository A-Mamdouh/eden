#include "Eden/render/RenderSystem.hpp"
#include "core/Log.hpp"
#include "Eden/platform/Platform.hpp"
#include "Eden/scene/SceneManager.hpp"
#include "Eden/script/ScriptSystem.hpp"

namespace Eden
{

std::unique_ptr<RenderSystem> RenderSystem::instance_ = nullptr;

std::unique_ptr<Renderer> createVulkanRenderer(Window &window);

RenderSystem& RenderSystem::getInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = std::unique_ptr<RenderSystem>(new RenderSystem());
    }
    return *instance_;
}

RenderSystem::RenderSystem() = default;

void RenderSystem::init(const ConfigurationService&)
{
    if (!renderer_)
    {
        auto window = Platform::getInstance().getWindow();
        if (window)
        {
            renderer_ = createVulkanRenderer(*window);
        }
    }
}

void RenderSystem::update(float)
{
    pollEvents();

    if (!renderer_)
    {
        return;
    }

    auto scene = activeScene_.lock();
    if (!scene)
    {
        return;
    }

    renderer_->beginFrame();
    ScriptSystem::getInstance().renderScripts(scene->getRegistry(), *renderer_);
    renderer_->renderScene(*scene);
    renderer_->endFrame();
}

void RenderSystem::pollEvents()
{
    auto window = Platform::getInstance().getWindow();
    if (window)
    {
        window->pollEvents();
    }
}

bool RenderSystem::shouldClose() const noexcept
{
    auto window = Platform::getInstance().getWindow();
    return window ? window->shouldClose() : true;
}

void RenderSystem::setActiveScene(const std::weak_ptr<Scene>& scene)
{
    activeScene_ = scene;
}

Renderer& RenderSystem::renderer()
{
    EDEN_ASSERT(renderer_ != nullptr, "Renderer not initialized");
    return *renderer_;
}

} // namespace Eden
