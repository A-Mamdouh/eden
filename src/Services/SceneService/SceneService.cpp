#include "Eden/Services/SceneService/SceneService.hpp"
#include "Eden/Services/SceneService/SceneServiceEvents.hpp"
#include "Eden/Services/EventService/EventService.hpp"

namespace Eden::Services
{
  void SceneService::loadScene(std::unique_ptr<World::Scene> scene)
  {
    requireInitialized();

    const auto eventService = getEventService();
    if(eventService.has_value()) {
      eventService.value()->publish<Events::SceneLoadedEvent>(
          Events::SceneLoadedEvent{.scene = scene.get()});
      activeScene_ = std::move(scene);
    } else {
      logger_->warn("Failed to send Scene loaded event. Event service not available.");
    }
  }

} // namespace Eden::Services
