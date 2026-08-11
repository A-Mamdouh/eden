#include "Eden/Systems/ScriptSystem/ScriptSystem.hpp"

#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Systems/ScriptSystem/ScriptComponent.hpp"

namespace Eden {

void ScriptSystem::update(double dt) {
  Scene *scene = sceneService_.activeScene();
  if (!scene) {
    return;
  }

  auto &registry = scene->getRegistry();
  for (const auto entityHandle : registry.view<ScriptComponent>()) {
    auto &scriptComponent = registry.get<ScriptComponent>(entityHandle);
    if (!scriptComponent.behaviour) {
      continue;
    }

    const Entity entity{entityHandle, &registry};
    if (!scriptComponent.started) {
      scriptComponent.started = true;
      scriptComponent.behaviour->onStart(entity, inputSystem_);
    }
    scriptComponent.behaviour->onUpdate(entity, dt, inputSystem_);
  }
}

} // namespace Eden
