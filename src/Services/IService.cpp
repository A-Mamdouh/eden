#include "Eden/Services/CommonServiceEvents.hpp"
#include "Eden/Services/EventService/EventService.hpp"

namespace Eden {

void IService::init(std::weak_ptr<Services::EventService> eventService) {
  eventService_ = eventService;
  getEventService()->publish<Events::ServiceStartedEvent>(
      Events::ServiceStartedEvent{.serviceName = this->getName(),
                                  .service = this});
  onInit();
}

Services::EventService *IService::getEventService() {
  const auto es = eventService_.lock();
  if (!es) {
    // TODO: panic
  }
  return es.get();
}

} // namespace Eden