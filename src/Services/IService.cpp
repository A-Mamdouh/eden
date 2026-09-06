#include "Eden/Services/CommonServiceEvents.hpp"
#include "Eden/Services/EventService/EventService.hpp"

#include <stdexcept>

namespace Eden
{

  void IService::init(std::weak_ptr<Services::EventService> eventService)
  {
    if (initialized_)
    {
      throw std::logic_error("Service is already initialized");
    }

    eventService_ = eventService;
    bool loggerRegistered = false;
    try
    {
      const auto defaultLogger = spdlog::default_logger();
      if (!defaultLogger)
      {
        throw std::runtime_error("spdlog default logger is unavailable");
      }

      logger_ = defaultLogger->clone(getName());
      spdlog::initialize_logger(logger_);
      loggerRegistered = true;
      const auto maybeEventService = getEventService();
      if (maybeEventService.has_value())
      {
        maybeEventService.value()->publish<Events::ServiceStartedEvent>(
            Events::ServiceStartedEvent{.serviceName = this->getName(),
                                        .service = this});
      }
      else
      {
        logger_->warn("Failed to send Service Start Event. Event Service not available.");
      }
      onInit();
      initialized_ = true;
    }
    catch (...)
    {
      if (loggerRegistered)
      {
        spdlog::drop(logger_->name());
      }
      logger_.reset();
      eventService_.reset();
      throw;
    }
  }

  std::optional<Services::EventService *> IService::getEventService()
  {
    const auto es = eventService_.lock();
    if (!es)
    {
      return std::nullopt;
    }
    return es.get();
  }

  void IService::requireInitialized() const
  {
    if (!initialized_)
    {
      throw std::logic_error("Service is not initialized");
    }
  }

  IService::~IService() {
    if(logger_) {
      spdlog::drop(logger_->name());
    }
  }

} // namespace Eden
