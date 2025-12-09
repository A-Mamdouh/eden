//#pragma once

#ifndef EDEN_ENGINE_SCENE_HPP
#define EDEN_ENGINE_SCENE_HPP

namespace Eden
{

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
};

} // namespace Eden

#endif // EDEN_ENGINE_SCENE_HPP
