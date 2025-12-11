#pragma once

#ifndef EDEN_ENGINE_RENDERER_HPP
#define EDEN_ENGINE_RENDERER_HPP

#include "Eden/core/Math.hpp"
#include "Eden/ecs/CameraComponent.hpp"

namespace Eden
{

class Scene;

struct Color
{
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};
};

struct Viewport
{
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
};

struct DrawParams
{
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
    bool useVertexColor{false};
    Mat4 modelMatrix{Mat4(1.0f)};
};

enum class PrimitiveShape
{
    Triangle,
    Quad
};

/**
 * High-level rendering interface used by scenes.
 *
 * The concrete implementation is owned by the Engine and may use Vulkan
 * internally, but no Vulkan types are exposed here.
 */
class Renderer
{
public:
    Renderer() = default;
    virtual ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) noexcept = delete;
    Renderer& operator=(Renderer&&) noexcept = delete;

    /**
     * Begin rendering a new frame.
     *
     * Backends may acquire swapchain images or command buffers here.
     */
    virtual void beginFrame() = 0;

    /**
     * Fetch current scene and fill command buffers
     */
    virtual void renderScene(const Scene& scene) = 0;

    /**
     * Finish rendering the current frame.
     *
     * Backends may submit command buffers and present here.
     */
    virtual void endFrame() = 0;

    /**
     * Notify the renderer that the underlying framebuffer has changed size.
     * This is typically called when the window is resized.
     */
    virtual void resize(unsigned int width, unsigned int height) = 0;

    /**
     * Set the active viewport for subsequent draw calls.
     */
    virtual void setViewport(const Viewport& viewport) = 0;

    /**
     * Set the active camera used for rendering.
     */
    virtual void setCamera(const Camera& camera) = 0;

    /**
     * Query whether a camera is currently set/valid.
     */
    virtual bool hasCamera() const = 0;

    /**
     * Get the currently active camera (undefined if hasCamera() is false).
     */
    virtual const Camera& getCamera() const = 0;

    /**
     * Clear the current frame with the given color.
     * Actual drawing backends will implement this.
     */
    virtual void clear(const Color& color) = 0;

    /**
     * Submit a draw command for the current frame.
     *
     * Implementations are expected to record the necessary GPU commands
     * during endFrame().
     */
    virtual void submitDrawCommand(PrimitiveShape shape, const DrawParams& params) = 0;
};

} // namespace Eden

#endif // EDEN_ENGINE_RENDERER_HPP
