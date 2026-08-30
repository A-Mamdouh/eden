#pragma once

#include "Eden/Services/IService.hpp"
#include "Scene.hpp"

#include <memory>

namespace Eden::Services {

/// Owns the active Scene and publishes SceneLoadedEvent on change. Holds
/// data only -- TransformSystem and RenderSystem read/write its Scene's
/// registry as part of their own per-frame work; SceneService itself has
/// no per-frame behavior.
class SceneService : public IService {
public:
  std::string getName() override { return "Scene Service"; }

  /// Publishes Events::SceneLoadedEvent, then takes ownership of `scene`.
  /// @param scene New active scene; the previous one, if any, is destroyed.
  void loadScene(std::unique_ptr<World::Scene> scene);

  /// @return The active scene, or nullptr if loadScene() hasn't been
  ///         called yet.
  World::Scene *activeScene() const { return activeScene_.get(); }

private:
  void onInit() override {}
  std::unique_ptr<World::Scene> activeScene_{nullptr};
};

} // namespace Eden::Services
