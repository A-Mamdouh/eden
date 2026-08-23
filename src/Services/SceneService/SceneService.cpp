#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Services/SceneService/SceneServiceEvents.hpp"
#include "Eden/Services/EventService/EventService.hpp"

namespace Eden::Services {
void SceneService::loadScene(std::unique_ptr<World::Scene> scene) {
  getEventService()->publish<Events::SceneLoadedEvent>(
      Events::SceneLoadedEvent{.scene = scene.get()});
  activeScene_ = std::move(scene);
}

} // namespace Eden::Services
