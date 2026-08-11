#pragma once

#include "Eden/Systems/ISystem.hpp"
#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Systems/RenderSystem/RenderableComponent.hpp"
#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

struct SDL_Window;

namespace Eden {

class Renderer;
class SceneService;

/// Owns the SDL window and the active Renderer backend; polls window/OS
/// events and drives one Renderer::renderFrame() per update(), built
/// from the active Scene's Renderable/WorldTransform entities.
class RenderSystem : public ISystem {

  public:
  /// @param windowConfig SDL window creation parameters.
  /// @param renderConfig Rendering backend parameters (validation layers,
  ///        frame rate target).
  /// @param sceneService Queried each update() for the active scene to
  ///        draw; a null active scene renders just the clear color.
  RenderSystem(Config::WindowConfig windowConfig, Config::RenderConfig renderConfig,
              SceneService &sceneService);
  ~RenderSystem() override;

  std::string getName() override { return "Render System"; }
  void update(double dt) override;
  void shutdown() override;

  /// True once SDL_QUIT or a window-close event has been observed.
  bool quitRequested() const noexcept { return quitRequested_; }

  private:
  /// Creates the SDL window and the Vulkan renderer, then calls
  /// createPrimitiveMeshes().
  void onInit() override;

  /// Uploads RenderSystem's small built-in primitive library (currently
  /// just a triangle and a quad) via Renderer::createMesh(). Stand-in
  /// for real asset loading -- see RenderableComponent.hpp.
  void createPrimitiveMeshes();
  /// @return The mesh handle backing a PrimitiveShape.
  MeshHandle resolvePrimitive(PrimitiveShape shape) const;
  /// Walks the active scene's Renderable+WorldTransform entities into a
  /// RenderFrame. @return A frame with just the clear color and no draw
  /// commands if there's no active scene.
  RenderFrame buildFrameFromScene() const;

  Config::WindowConfig windowConfig_;
  Config::RenderConfig renderConfig_;
  SceneService &sceneService_;

  SDL_Window *window_{nullptr};
  std::unique_ptr<Renderer> renderer_{nullptr};
  bool quitRequested_{false};

  MeshHandle triangleMesh_{};
  MeshHandle quadMesh_{};

};

} // namespace Eden
