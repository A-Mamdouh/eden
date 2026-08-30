#pragma once

#include "Eden/Services/EventService/EventService.hpp"
#include "Eden/Systems/ISystem.hpp"

#include <optional>

namespace Eden::Services {
class SceneService;
}

namespace Eden::Input {
class InputState;
}

namespace Eden::Systems {

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
  explicit ScriptSystem(Services::SceneService &sceneService) : sceneService_{sceneService} {}

  std::string getName() override { return "Script System"; }
  void update(double dt) override;
  void shutdown() override;

private:
  /// Subscribes to Events::InputStateUpdatedEvent so update() has this
  /// frame's InputState without ScriptSystem depending on InputSystem
  /// directly -- see InputSystemEvents.hpp.
  void onInit() override;

  Services::SceneService &sceneService_;
  /// Latest InputState from Events::InputStateUpdatedEvent; null until
  /// InputSystem's first update() (always before ScriptSystem's own,
  /// per Engine's registration order), forwarded to every ScriptBehaviour
  /// call this drives.
  Input::InputState *inputState_{nullptr};
  std::optional<Services::ListenerId> inputStateListener_{};
};

} // namespace Eden::Systems
