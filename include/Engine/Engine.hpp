//#pragma once

#ifndef EDEN_ENGINE_ENGINE_HPP
#define EDEN_ENGINE_ENGINE_HPP

#include <functional>
#include <memory>
#include <string>

#include "Engine/EdenConfig.hpp"

namespace Eden
{

class Scene;
class Renderer;
class Input;

struct EngineConfig
{
    unsigned int width{1280};
    unsigned int height{720};
    std::string title{"Platformer"};
    bool enableValidationLayers{false};

    // Time-step configuration
    // When fixedTimestep is true, the engine will update at a fixed
    // rate of targetFrameRate Hz and render as fast as possible.
    // When false, the engine uses a variable timestep.
    float targetFrameRate{60.0f};
    bool fixedTimestep{false};
};

/**
 * Main entry point for running a game using the Eden engine.
 *
 * The Engine owns the rendering backend, window, and active scene.
 * Users interact with it at a high level and never touch Vulkan directly.
 */
class Engine
{
public:
    explicit Engine(const EngineConfig& config);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    Engine(Engine&&) noexcept = delete;
    Engine& operator=(Engine&&) noexcept = delete;

    /**
     * Set the active scene. Ownership of the scene is transferred to the engine.
     */
    void setScene(std::unique_ptr<Scene> scene);

    /**
     * Access the currently active scene, or nullptr if none is set.
     */
    Scene* getScene() noexcept;
    const Scene* getScene() const noexcept;

    /**
     * Start the main loop. This will block until the engine is stopped.
     */
    void run();

    /**
     * Request the engine to stop at the end of the current frame.
     */
    void stop() noexcept;

    /**
     * Access the renderer used by the engine.
     * This does not expose any Vulkan types.
     */
    Renderer& getRenderer() noexcept;
    const Renderer& getRenderer() const noexcept;

    /**
     * Access the current input state.
     */
    Input& getInput() noexcept;
    const Input& getInput() const noexcept;

    /**
     * Register a callback that is invoked when the window is resized.
     */
    void setWindowResizeCallback(
        std::function<void(unsigned int width, unsigned int height)> callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Eden

#endif // EDEN_ENGINE_ENGINE_HPP
