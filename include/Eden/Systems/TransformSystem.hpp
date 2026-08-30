#pragma once

#include "Eden/Systems/ISystem.hpp"

namespace Eden::Services {
class SceneService;
}

namespace Eden::Systems {

/// Computes each entity's WorldTransform from its Transform and
/// EntityHierarchy parent chain, every frame. The only system that
/// writes WorldTransform; everything else (RenderSystem, eventually
/// physics/audio) only ever reads it.
class TransformSystem : public ISystem {
public:
  /// @param sceneService Queried each update() for the active scene;
  ///        a null active scene is a no-op, not an error.
  explicit TransformSystem(Services::SceneService &sceneService) : sceneService_{sceneService} {}

  std::string getName() override { return "Transform System"; }
  void update(double dt) override;
  void shutdown() override {}

private:
  void onInit() override {}

  Services::SceneService &sceneService_;
};

} // namespace Eden::Systems
