#pragma once

#include "Eden/Systems/ISystem.hpp"
#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Services/SceneService/Entity.hpp"
#include "Eden/Systems/RenderSystem/Material.hpp"
#include "Eden/Systems/RenderSystem/Model.hpp"
#include "Eden/Systems/RenderSystem/RenderableComponent.hpp"
#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

#include <vector>

struct SDL_Window;

namespace Eden {

class Renderer;
class Scene;
class SceneService;

/// Owns the SDL window and the active Renderer backend; polls window/OS
/// events and drives one Renderer::renderFrame() per update(), built from
/// the active Scene's Renderable/Model entities (each paired with a
/// WorldTransform).
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

  /// Forwards to the owned Renderer; Engine::createMesh() is what the
  /// embedding application actually calls, never this directly.
  /// @param desc Vertex/index data to upload; only needs to stay valid
  ///        for the duration of this call.
  MeshHandle createMesh(const MeshDesc &desc);
  /// @param handle Handle previously returned by createMesh(); a stale
  ///        or already-destroyed handle silently no-ops.
  void destroyMesh(MeshHandle handle);

  /// Forwards to the owned Renderer; see createMesh().
  TextureHandle createTexture(const TextureDesc &desc);
  /// @param handle Handle previously returned by createTexture(); a stale
  ///        or already-destroyed handle silently no-ops.
  void destroyTexture(TextureHandle handle);

  /// Stores `desc`; a Renderable referencing the returned handle resolves
  /// it into a DrawCommand's texture/tint/useVertexColor each frame.
  MaterialHandle createMaterial(const Material &desc);
  /// @param handle Handle previously returned by createMaterial(); a
  ///        stale or already-destroyed handle silently no-ops.
  void destroyMaterial(MaterialHandle handle);

  /// Loads a glTF/GLB file's meshes, textures, and materials, returning
  /// them as a Model. See Engine::loadModel(), the intended entry point.
  /// @throws std::runtime_error on any parse/load failure.
  Model loadModel(const std::string &path);

  /// Selects which entity's Camera component to render from each frame.
  /// Stores just the entity id and re-resolves it against whichever
  /// scene is active at render time (see resolveCamera()) -- safe to
  /// call before the scene it refers to is even loaded, and safe across
  /// scene changes (a stale id simply fails to resolve).
  /// @param camera Entity expected to carry a Camera component; one that
  ///        doesn't is silently skipped at render time, same as no
  ///        camera being set at all.
  void setActiveCamera(Entity camera);
  /// @return The entity passed to the most recent setActiveCamera()
  ///         call, resolved against the current active scene.
  ///         Entity::valid() is false if none was set, there's no active
  ///         scene, or the entity no longer exists in it.
  Entity activeCamera() const;

  private:
  /// Creates the SDL window and the Vulkan renderer.
  void onInit() override;

  /// Walks the active scene's Renderable+WorldTransform and
  /// Model+WorldTransform entities into a RenderFrame. @return A frame
  /// with just the clear color and no draw commands if there's no active
  /// scene.
  RenderFrame buildFrameFromScene() const;
  /// @param scene Scene to resolve the entity set via setActiveCamera()
  ///        against.
  /// @return That camera's view/projection, using the current window
  ///         aspect ratio. If none is set, or it doesn't resolve to a
  ///         live Camera in `scene`, an aspect-corrected orthographic
  ///         projection (identity view) so camera-less scenes still
  ///         render undistorted.
  CameraDesc resolveCamera(Scene &scene) const;
  /// @return The live Material for `handle`, or nullptr if it's invalid,
  ///         out of range, destroyed, or from a reused (stale) slot.
  const Material *resolveMaterial(MaterialHandle handle) const;
  /// Resolves `material` into tint/useVertexColor/texture (Material's
  /// defaults if invalid/destroyed), then applies `tintOverride` if
  /// present. Shared by the Renderable and Model draw-building loops in
  /// buildFrameFromScene().
  DrawCommand buildDrawCommand(MeshHandle mesh, MaterialHandle material, const Mat4 &transform,
                               const TintOverride *tintOverride) const;

  Config::WindowConfig windowConfig_;
  Config::RenderConfig renderConfig_;
  SceneService &sceneService_;

  SDL_Window *window_{nullptr};
  std::unique_ptr<Renderer> renderer_{nullptr};
  bool quitRequested_{false};

  /// Updated on window creation and every resize event; drives the
  /// active Camera's aspect ratio.
  int windowWidth_{0};
  int windowHeight_{0};

  /// Entity id selected via setActiveCamera(); re-resolved against the
  /// active scene each frame in resolveCamera() rather than cached as a
  /// reference, since EnTT component references aren't safe to hold
  /// across frames.
  entt::entity activeCamera_{entt::null};

  /// Tracks just enough to validate handle lifetime, mirroring
  /// VulkanRenderer's mesh/texture slot+generation scheme.
  struct MaterialSlot {
    Material material{};
    std::uint32_t generation{0};
    bool alive{false};
  };
  std::vector<MaterialSlot> materials_{};
  std::vector<std::uint32_t> freeMaterialSlots_{};
};

} // namespace Eden
