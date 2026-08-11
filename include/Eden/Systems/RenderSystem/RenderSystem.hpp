#pragma once

#include "Eden/Systems/ISystem.hpp"
#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

struct SDL_Window;

namespace Eden {

class Renderer;

/// Owns the SDL window and the active Renderer backend; polls window/OS
/// events and drives one Renderer::renderFrame() per update().
class RenderSystem : public ISystem {

  public:
  /// @param windowConfig SDL window creation parameters.
  /// @param renderConfig Rendering backend parameters (validation layers,
  ///        frame rate target).
  RenderSystem(Config::WindowConfig windowConfig, Config::RenderConfig renderConfig);
  ~RenderSystem() override;

  std::string getName() override { return "Render System"; }
  void update(double dt) override;
  void shutdown() override;

  /// True once SDL_QUIT or a window-close event has been observed.
  bool quitRequested() const noexcept { return quitRequested_; }

  private:
  /// Creates the SDL window and the Vulkan renderer, then calls
  /// createDemoMeshes().
  void onInit() override;

  // Temporary stand-in for scene traversal: RenderSystem doesn't have a
  // real Scene to walk yet, so it owns a couple of meshes directly and
  // builds a RenderFrame by hand each tick to exercise the Renderer
  // contract end to end.
  /// Uploads the demo triangle and quad via Renderer::createMesh(),
  /// storing their handles in triangleMesh_/quadMesh_.
  void createDemoMeshes();
  /// @param elapsedTime Total time since RenderSystem::onInit(), in
  ///        seconds; drives the quad's pulsing tint.
  /// @return A frame with an identity camera and one draw command per
  ///         demo mesh.
  RenderFrame buildDemoFrame(double elapsedTime) const;

  Config::WindowConfig windowConfig_;
  Config::RenderConfig renderConfig_;

  SDL_Window *window_{nullptr};
  std::unique_ptr<Renderer> renderer_{nullptr};
  bool quitRequested_{false};

  MeshHandle triangleMesh_{};
  MeshHandle quadMesh_{};
  /// Accumulated dt passed to update(); see buildDemoFrame().
  double elapsedTime_{0.0};

};

} // namespace Eden
