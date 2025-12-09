#include "Engine/Engine.hpp"

#include "Engine/Renderer.hpp"
#include "Engine/Log.hpp"
#include "Engine/Window.hpp"
#include "Engine/Input.hpp"
#include "Engine/Scene.hpp"

#include <chrono>
#include <thread>

namespace Eden
{

std::unique_ptr<Renderer> createVulkanRenderer(Window& window);

namespace
{
    std::unique_ptr<Renderer> createFallbackRenderer()
    {
        class NullRenderer final : public Renderer
        {
        public:
            void beginFrame() override {}
            void endFrame() override {}
            void resize(unsigned int, unsigned int) override {}
            void setViewport(const Viewport&) override {}
            void setCamera(const Camera&) override {}
            void clear(const Color&) override {}
            void submitDrawCommand(PrimitiveShape, const DrawParams&) override {}
        };

        EDEN_CORE_WARN("Using NullRenderer (no graphics backend available)");
        return std::make_unique<NullRenderer>();
    }
} // namespace

struct Engine::Impl
{
    explicit Impl(const EngineConfig& config)
        : config(config)
        , input()
        , window(createWindow(WindowConfig{
                  config.width,
                  config.height,
                  config.title,
                  true},
              input))
        , lastWidth(0)
        , lastHeight(0)
    {
        EDEN_ASSERT(window != nullptr, "Failed to create engine window");

        lastWidth = window->width();
        lastHeight = window->height();

#ifdef EDEN_ENABLE_VULKAN
        renderer = createVulkanRenderer(*window);
#else
        renderer = nullptr;
#endif

        if (!renderer)
        {
            renderer = createFallbackRenderer();
        }
    }

    Input input;
    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Scene> scene;
    bool running{false};
    EngineConfig config;
    float accumulator{0.0f};
    unsigned int lastWidth;
    unsigned int lastHeight;
    std::function<void(unsigned int, unsigned int)> windowResizeCallback;
};

Engine::Engine(const EngineConfig& config)
    : impl_(std::make_unique<Impl>(config))
{
}

Engine::~Engine() = default;

void Engine::setScene(std::unique_ptr<Scene> scene)
{
    if (impl_->scene)
    {
        impl_->scene->onDetach();
    }

    impl_->scene = std::move(scene);

    if (impl_->scene)
    {
        impl_->scene->onAttach();

        // Ensure the scene is aware of the current window size.
        impl_->scene->onResize(impl_->lastWidth, impl_->lastHeight);
    }
}

Scene* Engine::getScene() noexcept
{
    return impl_->scene.get();
}

const Scene* Engine::getScene() const noexcept
{
    return impl_->scene.get();
}

void Engine::run()
{
    impl_->running = true;

    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();

    const float targetDelta =
        (impl_->config.targetFrameRate > 0.0f)
            ? (1.0f / impl_->config.targetFrameRate)
            : 0.0f;

    EDEN_CORE_INFO(
        "Engine main loop started (fixed timestep: {}, target FPS: {})",
        impl_->config.fixedTimestep,
        impl_->config.targetFrameRate);

    while (impl_->running)
    {
        impl_->input.beginFrame();
        impl_->window->pollEvents();

        if (impl_->window->shouldClose())
        {
            stop();
        }

        const auto currentWidth = impl_->window->width();
        const auto currentHeight = impl_->window->height();
        if (currentWidth != impl_->lastWidth || currentHeight != impl_->lastHeight)
        {
            impl_->lastWidth = currentWidth;
            impl_->lastHeight = currentHeight;

            impl_->renderer->resize(currentWidth, currentHeight);

            if (impl_->scene)
            {
                impl_->scene->onResize(currentWidth, currentHeight);
            }

            if (impl_->windowResizeCallback)
            {
                impl_->windowResizeCallback(currentWidth, currentHeight);
            }
        }

        auto now = clock::now();
        std::chrono::duration<float> delta = now - lastTime;
        lastTime = now;

        const float frameDelta = delta.count();

        if (impl_->scene)
        {
            impl_->renderer->beginFrame();

            if (impl_->config.fixedTimestep && targetDelta > 0.0f)
            {
                impl_->accumulator += frameDelta;

                while (impl_->accumulator >= targetDelta)
                {
                    impl_->scene->onUpdate(targetDelta);
                    impl_->accumulator -= targetDelta;
                }

                impl_->scene->onRender(*impl_->renderer);
            }
            else
            {
                impl_->scene->onUpdate(frameDelta);
                impl_->scene->onRender(*impl_->renderer);
            }

            impl_->renderer->endFrame();
        }

        // Simple frame limiting for variable timestep mode.
        if (!impl_->config.fixedTimestep && targetDelta > 0.0f)
        {
            const auto targetDuration =
                std::chrono::duration<float>(targetDelta);
            const auto frameDuration = clock::now() - now;

            if (frameDuration < targetDuration)
            {
                std::this_thread::sleep_for(targetDuration - frameDuration);
            }
        }
    }

    impl_->running = false;

    EDEN_CORE_INFO("Engine main loop exited");
}

void Engine::stop() noexcept
{
    impl_->running = false;
}

Renderer& Engine::getRenderer() noexcept
{
    return *impl_->renderer;
}

const Renderer& Engine::getRenderer() const noexcept
{
    return *impl_->renderer;
}

Input& Engine::getInput() noexcept
{
    return impl_->input;
}

const Input& Engine::getInput() const noexcept
{
    return impl_->input;
}

void Engine::setWindowResizeCallback(
    std::function<void(unsigned int, unsigned int)> callback)
{
    impl_->windowResizeCallback = std::move(callback);
}

} // namespace Eden
