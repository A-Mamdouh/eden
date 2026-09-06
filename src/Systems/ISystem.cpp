#include "Eden/Systems/ISystem.hpp"

#include "Eden/Services/EventService/EventService.hpp"
#include "Eden/Systems/CommonSystemEvents.hpp"

#include <stdexcept>

namespace Eden {

void ISystem::init(std::weak_ptr<Services::EventService> eventService) {
  if (initialized_) {
    throw std::logic_error("System is already initialized");
  }

  eventService_ = std::move(eventService);
  bool loggerRegistered = false;
  try {
    const auto defaultLogger = spdlog::default_logger();
    if (!defaultLogger) {
      throw std::runtime_error("spdlog default logger is unavailable");
    }

    logger_ = defaultLogger->clone(getName());
    spdlog::initialize_logger(logger_);
    loggerRegistered = true;
    const auto maybeEventService = getEventService();
    if (maybeEventService.has_value()) {
      maybeEventService.value()->publish<Events::SystemStartedEvent>(
          Events::SystemStartedEvent{.systemName = this->getName(), .system = this});
    } else {
      logger_->warn("Failed to send System Start Event. Event Service not available.");
    }
    onInit();
    initialized_ = true;
  } catch (...) {
    if (loggerRegistered) {
      spdlog::drop(logger_->name());
    }
    logger_.reset();
    eventService_.reset();
    throw;
  }
}

std::optional<Services::EventService *> ISystem::getEventService() {
  const auto es = eventService_.lock();
  if (!es) {
    return std::nullopt;
  }
  return es.get();
}

void ISystem::requireInitialized() const {
  if (!initialized_) {
    throw std::logic_error("System is not initialized");
  }
}

ISystem::~ISystem()
{
  if(logger_) {
    spdlog::drop(logger_->name());
  }
}

} // namespace Eden
