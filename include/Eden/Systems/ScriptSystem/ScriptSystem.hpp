#pragma once

#include "Eden/Systems/ISystem.hpp"

namespace Eden {

class SceneService;

/// Drives every entity's ScriptComponent each frame: calls
/// ScriptBehaviour::onStart() once, then onUpdate() every frame after.
/// Runs before TransformSystem/RenderSystem in Engine's system order, so
/// a script's component writes (Transform, Renderable, ...) are visible
/// the same frame they happen.
class ScriptSystem : public ISystem {
public:
  /// @param sceneService Queried each update() for the active scene;
  ///        a null active scene is a no-op, not an error.
  explicit ScriptSystem(SceneService &sceneService) : sceneService_{sceneService} {}

  std::string getName() override { return "Script System"; }
  void update(double dt) override;
  void shutdown() override {}

private:
  void onInit() override {}

  SceneService &sceneService_;
};

} // namespace Eden
