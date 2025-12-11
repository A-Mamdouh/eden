#pragma once

#ifndef EDEN_ENGINE_WINDOW_HPP
#define EDEN_ENGINE_WINDOW_HPP

#include "Eden/core/Config.hpp"

#include <memory>
#include <string>

namespace Eden
{

class Input;
struct WindowConfig;

/**
 * Abstract window interface used by the engine.
 *
 * Concrete implementations wrap a specific windowing library (e.g. SDL).
 */
class Window
{
public:
    Window() = default;
    virtual ~Window() = default;

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) noexcept = delete;
    Window& operator=(Window&&) noexcept = delete;

    /**
     * Pump OS / window events.
     *
     * Must be called regularly from the main loop.
     */
    virtual void pollEvents() = 0;

    /**
     * Returns true if the user requested the window to close.
     */
    virtual bool shouldClose() const noexcept = 0;

    /**
     * Current window size in pixels.
     */
    virtual unsigned int width() const noexcept = 0;
    virtual unsigned int height() const noexcept = 0;

    /**
     * Opaque native handle used by low-level backends
     * (e.g. to create graphics surfaces).
     *
     * For the SDL backend this is an SDL_Window*.
     */
    virtual void* nativeHandle() noexcept = 0;

    /**
     * Change the window title at runtime.
     */
    virtual void setTitle(const std::string& title) = 0;
};

/**
 * Factory for creating a platform-specific window implementation.
 */
std::shared_ptr<Window> createWindow(const WindowConfig& config);

} // namespace Eden

#endif // EDEN_ENGINE_WINDOW_HPP
