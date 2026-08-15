#include "Eden/Systems/ISystem.hpp"

#include "Eden/Services/EventService/EventService.hpp"
#include "Eden/Systems/CommonSystemEvents.hpp"

namespace Eden {

void ISystem::init(std::weak_ptr<EventService> eventService) {
  eventService_ = std::move(eventService);
  getEventService()->publish<Events::SystemStartedEvent>(
      Events::SystemStartedEvent{.systemName = this->getName(), .system = this});
  onInit();
}

EventService *ISystem::getEventService() {
  const auto es = eventService_.lock();
  if (!es) {
    // TODO: panic
  }
  return es.get();
}

} // namespace Eden
