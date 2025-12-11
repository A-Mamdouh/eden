#pragma once

#ifndef EDEN_ENGINE_SCENE_MANAGER_HPP
#define EDEN_ENGINE_SCENE_MANAGER_HPP

#include "Scene.hpp"
#include <memory>

namespace Eden {

/**
 * Singleton responsible for owning and swapping the active scene.
 *
 * Scenes get their lifecycle callbacks (attach/detach/update/render)
 * through this manager so the engine loop can stay lean.
 */
class SceneManager {
public:
  static SceneManager &getInstance();

  SceneManager(const SceneManager &) = delete;
  SceneManager &operator=(const SceneManager &) = delete;

  SceneManager(SceneManager &&) noexcept = delete;
  SceneManager &operator=(SceneManager &&) noexcept = delete;
  ~SceneManager();

  void setScene(std::shared_ptr<Scene> scene);

  std::shared_ptr<Scene> getActiveScene() noexcept;
  const std::shared_ptr<Scene> getActiveScene() const noexcept;
  bool hasActiveScene() const noexcept;

private:
  SceneManager();

  void detachScene();
  void attachScene();

  static std::unique_ptr<SceneManager> instance_;
  std::shared_ptr<Scene> scene_;
  
};

} // namespace Eden

#endif // EDEN_ENGINE_SCENE_MANAGER_HPP
