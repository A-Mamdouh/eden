#pragma once

#include "RendererTypes.hpp"

#include <cstdint>

namespace Eden::Rendering {

/// Backend-agnostic rendering contract. No Vulkan/Metal/D3D12 type may
/// appear below this line. Backend-specific setup (native window handle,
/// validation toggles, ...) lives in each backend's own CreateInfo +
/// factory function, never in this interface.
class Renderer {
public:
  virtual ~Renderer() = default;

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;
  Renderer(Renderer &&) noexcept = delete;
  Renderer &operator=(Renderer &&) noexcept = delete;

  /// Uploads `desc` to the GPU.
  /// @param desc Vertex/index data to upload; only needs to stay valid
  ///        for the duration of this call.
  /// @return Handle valid until destroyMesh() is called with it.
  virtual MeshHandle createMesh(const MeshDesc &desc) = 0;
  /// @param handle Handle previously returned by createMesh(); a stale
  ///        or already-destroyed handle silently no-ops.
  virtual void destroyMesh(MeshHandle handle) = 0;

  /// Uploads `desc` to the GPU.
  /// @param desc Texel data to upload; only needs to stay valid for the
  ///        duration of this call.
  /// @return Handle valid until destroyTexture() is called with it.
  virtual TextureHandle createTexture(const TextureDesc &desc) = 0;
  /// @param handle Handle previously returned by createTexture(); a stale
  ///        or already-destroyed handle silently no-ops.
  virtual void destroyTexture(TextureHandle handle) = 0;

  /// Draws one frame. Acquire/record/submit/present all happen inside
  /// this one call; no frame-lifecycle state is shared across calls.
  /// @param frame Camera, clear color, and draw commands for this frame.
  virtual void renderFrame(const RenderFrame &frame) = 0;

  /// Notifies the backend that the window/framebuffer size changed.
  /// @param width New framebuffer width in pixels.
  /// @param height New framebuffer height in pixels.
  virtual void requestResize(std::uint32_t width, std::uint32_t height) = 0;

  /// Requests a new anti-aliasing/vsync configuration.
  /// @param settings Requested settings; a value the hardware can't
  ///        satisfy (e.g. an unsupported MSAA level) is clamped rather
  ///        than rejected.
  /// @return Applied if handled internally, RequiresRecreate if the
  ///         caller must destroy and reconstruct this Renderer instead.
  virtual ApplyResult applySettings(const RenderSettings &settings) = 0;

  /// @return What this backend's current device actually supports, so a
  ///         caller can offer only meaningful choices.
  virtual RendererCapabilities queryCapabilities() const = 0;

protected:
  Renderer() = default;
};

} // namespace Eden::Rendering
