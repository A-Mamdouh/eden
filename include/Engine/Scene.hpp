//#pragma once

#ifndef EDEN_ENGINE_SCENE_HPP
#define EDEN_ENGINE_SCENE_HPP

#include <functional>

#include "Engine/Camera.hpp"
#include "Engine/Renderer.hpp"

namespace Eden
{

    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    
class Renderer;

/**
 * High-level scene abstraction.
 *
 * Users subclass Scene to implement their game logic.
 * The engine owns the active Scene instance and calls its lifecycle methods.
 */
class Scene
{
public:
    Scene() = default;
    virtual ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) noexcept = delete;
    Scene& operator=(Scene&&) noexcept = delete;

    /**
     * Called when the scene becomes active.
     */
    virtual void onAttach() {}

    /**
     * Called when the scene is no longer active.
     */
    virtual void onDetach() {}

    /**
     * Called once per frame to update game logic.
     *
     * @param deltaTime Time in seconds since the previous frame.
     */
    virtual void onUpdate(float deltaTime) {}

    /**
     * Called once per frame to issue rendering commands.
     */
    virtual void onRender(Renderer& renderer) {}

    /**
     * Called when the window backing this scene is resized.
     *
     * @param width  New width in pixels.
     * @param height New height in pixels.
     */
    virtual void onResize(unsigned int width, unsigned int height) {}

    /**
     * Project a world-space position to screen-space pixels.
     *
     * Returns {0,0} if no projection handler is set.
     */
    Vec2 projectToScreen(const Vec3& world) const
    {
        if (projectToScreenHandler_)
        {
            return projectToScreenHandler_(world);
        }
        return Vec2{0.0f, 0.0f};
    }

    /**
     * Update the projection context used by projectToScreen.
     */
    void setProjectToScreenContext(const Camera& camera, const Viewport& viewport)
    {
        projectToScreenHandler_ = [camera, viewport](const Vec3& world) -> Vec2 {
            const Mat4 vp = camera.projection * camera.view;
            const Vec4 clip = vp * Vec4{world, 1.0f};
            if (clip.w == 0.0f)
            {
                return Vec2{0.0f, 0.0f};
            }
            const Vec3 ndc = Vec3{clip} / clip.w;
            const float px = (ndc.x * 0.5f + 0.5f) * viewport.width + viewport.x;
            const float py = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewport.height + viewport.y;
            return Vec2{px, py};
        };
    }

protected:
    /**
     * Request that the application quits at the end of the current frame.
     */
    void requestQuit()
    {
        if (quitHandler_)
        {
            quitHandler_();
        }
    }

private:
    friend class Engine;

    void setQuitHandler(std::function<void()> handler)
    {
        quitHandler_ = std::move(handler);
    }

    void setProjectToScreenHandler(std::function<Vec2(const Vec3&)> handler)
    {
        projectToScreenHandler_ = std::move(handler);
    }

    std::function<void()> quitHandler_{};
    std::function<Vec2(const Vec3&)> projectToScreenHandler_{};
};

} // namespace Eden

#endif // EDEN_ENGINE_SCENE_HPP
