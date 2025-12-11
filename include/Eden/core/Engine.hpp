#pragma once

#ifndef EDEN_ENGINE_ENGINE_HPP
#define EDEN_ENGINE_ENGINE_HPP

#include "Config.hpp"
#include "Configuration.hpp"
#include "System.hpp"

#include <memory>
#include <vector>

namespace Eden {

class Scene;
class Renderer;
class Input;
class Platform;
class RenderSystem;
class ScriptSystem;
class SceneManager;

/**
 * Main entry point for running a game using the Eden engine.
 *
 * Implemented as a singleton so the rest of the systems can reach it
 * without passing references around.
 */
class Engine {
public:
  static Engine &getInstance();

  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;

  Engine(Engine &&) noexcept = delete;
  Engine &operator=(Engine &&) noexcept = delete;

  void run();
  void stop() noexcept;
  ConfigurationService &getConfigService() noexcept;
  const ConfigurationService &getConfigService() const noexcept;
  Platform &platform() noexcept;
  RenderSystem &renderSystem() noexcept;
  ScriptSystem &scriptSystem() noexcept;
  SceneManager &sceneManager() noexcept;

private:
  explicit Engine(const EngineConfig &config);
  void registerSystem(ISystem &system);

  static std::unique_ptr<Engine> instance_;
  ConfigurationService *configService_{nullptr};
  Platform *platform_{nullptr};
  RenderSystem *renderSystem_{nullptr};
  ScriptSystem *scriptSystem_{nullptr};
  SceneManager *sceneManager_{nullptr};
  std::vector<ISystem *> systems_{};
  bool running_{false};
  float accumulator_{0.0f};
};

} // namespace Eden

#endif // EDEN_ENGINE_ENGINE_HPP
