#pragma once

#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Services/SceneService/Entity.hpp"
#include "Eden/Systems/RenderSystem/Material.hpp"
#include "Eden/Systems/RenderSystem/Model.hpp"
#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

#include <memory>
#include <vector>

namespace Eden::Services {
class ClockService;
class ConfigService;
class EventService;
class SceneService;
} // namespace Eden::Services

namespace Eden::Systems {
class RenderSystem;
} // namespace Eden::Systems

namespace Eden::World {
class Scene;
} // namespace Eden::World

namespace Eden {

class ISystem;

/// Composition root: owns every Service and System and drives the main
/// loop. Construct one with an ApplicationConfig, then call run().
class Engine {
public:
  /// Constructs every Service and System (see init()); construction is
  /// the only place Engine reads appConfig.
  /// @param appConfig Configuration for every subsystem; copied.
  explicit Engine(const Config::ApplicationConfig &appConfig);
  ~Engine();

  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;

  /// Blocks, ticking every System each frame, until stop() is called or a
  /// System requests quit.
  /// @return Always 0 currently; failures surface as thrown exceptions
  ///         during construction, not as a non-zero exit code here.
  int run();

  /// Requests that the current run() call return after the in-flight
  /// frame finishes.
  void stop();

  /// Forwards to SceneService::loadScene(); the embedding application
  /// never touches SceneService directly.
  /// @param scene New active scene; the previous one, if any, is destroyed.
  void loadScene(std::unique_ptr<World::Scene> scene);

  /// @return The current live configuration; reflects any prior
  ///         updateConfig() call, not just what was passed to the
  ///         constructor.
  const Config::ApplicationConfig &config() const;
  /// Forwards to ConfigService::update(). Every Service/System that reacts
  /// to config (currently just RenderSystem) picks up the change via
  /// Events::ConfigUpdatedEvent -- no restart required. The embedding
  /// application never touches ConfigService directly.
  /// @param newConfig Configuration to become the new live value; read
  ///        config() first if you only want to change one field.
  void updateConfig(const Config::ApplicationConfig &newConfig);

  /// Loads a glTF/GLB file's meshes, textures, and materials, uploading
  /// them through RenderSystem and returning the result as a Model --
  /// attach it to an entity like any other component (paired with a
  /// Transform to place it in the world) to actually put it in a scene.
  /// @param path Path to a .gltf or .glb file.
  /// @throws std::runtime_error on any parse/load failure.
  Rendering::Model loadModel(const std::string &path);

  /// Forwards to RenderSystem's owned Renderer; the embedding application
  /// creates meshes this way instead of touching RenderSystem/Renderer
  /// directly.
  /// @param desc Vertex/index data to upload; only needs to stay valid
  ///        for the duration of this call.
  Rendering::MeshHandle createMesh(const Rendering::MeshDesc &desc);
  /// @param handle Handle previously returned by createMesh(); a stale
  ///        or already-destroyed handle silently no-ops.
  void destroyMesh(Rendering::MeshHandle handle);

  /// Forwards to RenderSystem's owned Renderer.
  /// @param desc Texel data to upload; only needs to stay valid for the
  ///        duration of this call.
  Rendering::TextureHandle createTexture(const Rendering::TextureDesc &desc);
  /// @param handle Handle previously returned by createTexture(); a stale
  ///        or already-destroyed handle silently no-ops.
  void destroyTexture(Rendering::TextureHandle handle);

  /// Forwards to RenderSystem's material storage; see Material.hpp.
  Rendering::MaterialHandle createMaterial(const Rendering::Material &desc);
  /// @param handle Handle previously returned by createMaterial(); a
  ///        stale or already-destroyed handle silently no-ops.
  void destroyMaterial(Rendering::MaterialHandle handle);

  /// Selects which entity's Camera component RenderSystem renders from
  /// each frame; forwards to RenderSystem::setActiveCamera(). Safe to
  /// call before loadScene(), and safe across later scene changes -- see
  /// RenderSystem::setActiveCamera() for why.
  /// @param camera Entity expected to carry a Camera component.
  void setActiveCamera(World::Entity camera);
  /// @return The entity passed to the most recent setActiveCamera()
  ///         call, resolved against the current active scene.
  ///         Entity::valid() is false if none was set or it no longer
  ///         exists in that scene.
  World::Entity activeCamera() const;

private:
  /// Constructs and initializes EventService, ConfigService,
  /// ClockService, SceneService, ScriptSystem, TransformSystem, and
  /// RenderSystem, in that order. Called once from the constructor.
  void init();
  /// Shuts down every system (reverse registration order) then every
  /// service. Called from run() after the loop exits, and from the
  /// destructor as a safety net.
  void shutdown();

  Config::ApplicationConfig config_;
  bool running_{false};

  std::shared_ptr<Services::EventService> eventService_{};
  std::unique_ptr<Services::ConfigService> configService_{};
  std::unique_ptr<Services::ClockService> clockService_{};
  std::unique_ptr<Services::SceneService> sceneService_{};

  std::vector<std::unique_ptr<ISystem>> systems_{};
  Systems::RenderSystem *renderSystem_{nullptr};
};

} // namespace Eden
