#pragma once

#include "Eden/Systems/RenderSystem/Renderer.hpp"

#include <vector>

namespace Eden::Rendering {

/// Headless Renderer implementation: no window, no GPU, no graphics API
/// calls. Exists for two reasons -- proving the Renderer contract is
/// actually backend-agnostic rather than a one-implementation
/// abstraction, and letting engine logic that goes through a Renderer be
/// unit-tested without a GPU (see tests/). Not wired into RenderSystem;
/// tests construct one directly.
class NullRenderer final : public Renderer {
public:
  NullRenderer() = default;

  MeshHandle createMesh(const MeshDesc &desc) override;
  void destroyMesh(MeshHandle handle) override;
  TextureHandle createTexture(const TextureDesc &desc) override;
  void destroyTexture(TextureHandle handle) override;
  void renderFrame(const RenderFrame &frame) override;
  void requestResize(std::uint32_t width, std::uint32_t height) override;
  ApplyResult applySettings(const RenderSettings &settings) override;
  RendererCapabilities queryCapabilities() const override;

  /// @return The settings passed to the most recent applySettings() call,
  ///         or the default RenderSettings if it's never been called.
  ///         Test-only inspection surface, not part of the Renderer contract.
  const RenderSettings &lastSettings() const noexcept { return settings_; }

  /// @return Number of renderFrame() calls so far. Test-only inspection
  ///         surface, not part of the Renderer contract.
  std::size_t frameCount() const noexcept { return frameCount_; }
  /// @return The most recent frame with the configured light cap applied.
  ///         Only meaningful once frameCount() > 0.
  const RenderFrame &lastFrame() const noexcept { return lastFrame_; }
  /// @return Width passed to the most recent requestResize() call, or 0
  ///         if it's never been called.
  std::uint32_t lastResizeWidth() const noexcept { return lastResizeWidth_; }
  /// @overload
  std::uint32_t lastResizeHeight() const noexcept { return lastResizeHeight_; }

private:
  /// Tracks just enough to validate handle lifetime; no actual geometry
  /// is stored.
  struct NullMesh {
    std::size_t vertexCount{0};
    std::size_t indexCount{0};
    std::uint32_t generation{0};
    bool alive{false};
  };

  // Same slot+generation allocation scheme as VulkanRenderer::GpuMesh --
  // see its comment for why.
  std::vector<NullMesh> meshes_{};
  std::vector<std::uint32_t> freeMeshSlots_{};

  /// Tracks just enough to validate handle lifetime; no actual texel data
  /// is stored.
  struct NullTexture {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t generation{0};
    bool alive{false};
  };

  std::vector<NullTexture> textures_{};
  std::vector<std::uint32_t> freeTextureSlots_{};

  RenderFrame lastFrame_{};
  std::size_t frameCount_{0};
  std::uint32_t lastResizeWidth_{0};
  std::uint32_t lastResizeHeight_{0};
  RenderSettings settings_{};
};

} // namespace Eden::Rendering
