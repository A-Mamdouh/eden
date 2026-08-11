#pragma once

#include "Eden/Systems/ISystem.hpp"

namespace Eden {

class SceneService;
class InputSystem;

/// Drives every entity's ScriptComponent each frame: calls
/// ScriptBehaviour::onStart() once, then onUpdate() every frame after.
/// Runs after InputSystem but before TransformSystem/RenderSystem in
/// Engine's system order, so a script sees this frame's fresh input, and
/// its component writes (Transform, Renderable, ...) are visible the
/// same frame they happen.
class ScriptSystem : public ISystem {
public:
  /// @param sceneService Queried each update() for the active scene;
  ///        a null active scene is a no-op, not an error.
  /// @param inputSystem Forwarded to every ScriptBehaviour call this
  ///        drives; must outlive this ScriptSystem.
  ScriptSystem(SceneService &sceneService, InputSystem &inputSystem)
      : sceneService_{sceneService}, inputSystem_{inputSystem} {}

  std::string getName() override { return "Script System"; }
  void update(double dt) override;
  void shutdown() override {}

private:
  void onInit() override {}

  SceneService &sceneService_;
  InputSystem &inputSystem_;
};

} // namespace Eden
