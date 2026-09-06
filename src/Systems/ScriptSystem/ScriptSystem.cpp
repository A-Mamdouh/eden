#include "Eden/Systems/ScriptSystem/ScriptSystem.hpp"

#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/InputSystem/InputSystemEvents.hpp"
#include "Eden/Systems/ScriptSystem/ScriptComponent.hpp"

namespace Eden::Systems {

void ScriptSystem::onInit() {
  auto eventService = getEventService();
  if(eventService.has_value()) {
    inputStateListener_ = eventService.value()->subscribe<Events::InputStateUpdatedEvent>(
        [this](const Events::InputStateUpdatedEvent &event) { inputState_ = event.state; });
  } else {
    logger_->warn("Could not subscribe to input system. Event service not available.");
  }
}

void ScriptSystem::shutdown() {
  if (inputStateListener_) {
    auto eventService = getEventService();
    if(eventService.has_value()) {
      eventService.value()->unsubscribe<Events::InputStateUpdatedEvent>(*inputStateListener_);
    } else {
      logger_->error("Could not unsubscribe from input system. Event system not available.");
    }
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
