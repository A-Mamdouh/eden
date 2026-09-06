#include "Eden/Systems/ISystem.hpp"

#include "Eden/Services/EventService/EventService.hpp"
#include "Eden/Systems/CommonSystemEvents.hpp"

#include <stdexcept>

namespace Eden {

void ISystem::init(std::weak_ptr<Services::EventService> eventService) {
  logger_ = spdlog::default_logger()->clone(getName());
  spdlog::initialize_logger(logger_);
  eventService_ = std::move(eventService);
  const auto maybeEventService = getEventService();
  if (maybeEventService.has_value())
  {
    maybeEventService.value()->publish<Events::SystemStartedEvent>(
        Events::SystemStartedEvent{.systemName = this->getName(), .system = this});
    } else {
      logger_->warn("Failed to send System Start Event. Event Service not available.");
    }
  onInit();
}

std::optional<Services::EventService *> ISystem::getEventService() {
  const auto es = eventService_.lock();
  if (!es) {
    return std::nullopt;
  }
  return es.get();
}

ISystem::~ISystem()
{
  if(logger_) {
    spdlog::drop(logger_->name());
  }
}

} // namespace Eden
