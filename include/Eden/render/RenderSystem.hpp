#pragma once

#ifndef EDEN_ENGINE_RENDER_SYSTEM_HPP
#define EDEN_ENGINE_RENDER_SYSTEM_HPP

#include <Eden/render/Renderer.hpp>
#include <Eden/core/System.hpp>
#include <memory>

namespace Eden
{

class Scene;

/**
 * Singleton that owns the window and renderer and provides helper
 * rendering utilities for ECS-backed scenes.
 */
class RenderSystem : public ISystem
{
public:
    static RenderSystem& getInstance();

    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;
    RenderSystem(RenderSystem&&) noexcept = delete;
    RenderSystem& operator=(RenderSystem&&) noexcept = delete;

    void init(const ConfigurationService& config) override;
    void update(float deltaTime) override;

    void pollEvents();
    bool shouldClose() const noexcept;

    void setActiveScene(const std::weak_ptr<class Scene>& scene);

    Renderer& renderer();
    
    private:
    RenderSystem();

    std::weak_ptr<class Scene> activeScene_{};
    static std::unique_ptr<RenderSystem> instance_;
    std::unique_ptr<Renderer> renderer_{};
};

} // namespace Eden

#endif // EDEN_ENGINE_RENDER_SYSTEM_HPP
