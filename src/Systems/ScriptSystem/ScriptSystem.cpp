#include "Eden/Systems/ScriptSystem/ScriptSystem.hpp"

#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/InputSystem/InputSystemEvents.hpp"
#include "Eden/Systems/ScriptSystem/ScriptComponent.hpp"

namespace Eden::Systems {

void ScriptSystem::onInit() {
  inputStateListener_ = getEventService()->subscribe<Events::InputStateUpdatedEvent>(
      [this](const Events::InputStateUpdatedEvent &event) { inputState_ = event.state; });
}

void ScriptSystem::shutdown() {
  if (inputStateListener_) {
    getEventService()->unsubscribe<Events::InputStateUpdatedEvent>(*inputStateListener_);
    inputStateListener_.reset();
  }
}

void ScriptSystem::update(double dt) {
  World::Scene *scene = sceneService_.activeScene();
  if (!scene || !inputState_) {
    return;
  }

  auto &registry = scene->getRegistry();
  for (const auto entityHandle : registry.view<Scripting::Components::ScriptComponent>()) {
    auto &scriptComponent = registry.get<Scripting::Components::ScriptComponent>(entityHandle);
    if (!scriptComponent.behaviour) {
      continue;
    }

    if (!scriptComponent.started) {
      scriptComponent.started = true;
      scriptComponent.behaviour->onStart(*inputState_);
    }
    scriptComponent.behaviour->onUpdate(dt, *inputState_);
  }
}

} // namespace Eden::Systems
