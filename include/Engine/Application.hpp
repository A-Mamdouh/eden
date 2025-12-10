//#pragma once

#ifndef EDEN_ENGINE_APPLICATION_HPP
#define EDEN_ENGINE_APPLICATION_HPP

#include "Engine/Engine.hpp"
#include "Engine/Scene.hpp"

#include <memory>

namespace Eden
{

/**
 * High-level application wrapper around the core Engine.
 *
 * Users derive from Application and provide an initial Scene.
 */
class Application
{
public:
    explicit Application(const EngineConfig& config = {});
    virtual ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    Application(Application&&) noexcept = delete;
    Application& operator=(Application&&) noexcept = delete;

    /**
     * Run the application until the engine stops.
     *
     * Returns a process exit code (0 on success).
     */
    int run();

protected:
    /**
     * Called before the engine main loop starts.
     */
    virtual void onInit() {}

    /**
     * Called after the engine main loop exits.
     */
    virtual void onShutdown() {}

    /**
     * Called when the window backing this application is resized.
     */
    virtual void onWindowResized(unsigned int /*width*/, unsigned int /*height*/) {}

    /**
     * Create the initial scene for the application.
     */
    virtual std::unique_ptr<Scene> createInitialScene() = 0;

    /**
     * Access to the underlying engine for advanced use cases.
     */
    Engine& getEngine() noexcept { return engine_; }
    const Engine& getEngine() const noexcept { return engine_; }

    /**
     * Request the application to quit at the end of the current frame.
     */
    void requestQuit() noexcept { engine_.stop(); }

private:
    Engine engine_;
};

} // namespace Eden

#endif // EDEN_ENGINE_APPLICATION_HPP
