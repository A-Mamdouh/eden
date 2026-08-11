#pragma once

#include "Eden/Services/ConfigService/Config.hpp"

#include <memory>
#include <vector>

namespace Eden {
class ClockService;
class ConfigService;
class EventService;
class ISystem;
class RenderSystem;
class Scene;
class SceneService;

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
  void loadScene(std::unique_ptr<Scene> scene);

private:
  /// Constructs and initializes EventService, ConfigService,
  /// ClockService, SceneService, TransformSystem, and RenderSystem, in
  /// that order. Called once from the constructor.
  void init();
  /// Shuts down every system (reverse registration order) then every
  /// service. Called from run() after the loop exits, and from the
  /// destructor as a safety net.
  void shutdown();

  Config::ApplicationConfig config_;
  bool running_{false};

  std::shared_ptr<EventService> eventService_{};
  std::unique_ptr<ConfigService> configService_{};
  std::unique_ptr<ClockService> clockService_{};
  std::unique_ptr<SceneService> sceneService_{};

  std::vector<std::unique_ptr<ISystem>> systems_{};
  RenderSystem *renderSystem_{nullptr};
};

} // namespace Eden
