#include "Eden/Services/CommonServiceEvents.hpp"
#include "Eden/Services/EventService/EventService.hpp"

namespace Eden
{

  void IService::init(std::weak_ptr<Services::EventService> eventService)
  {
    eventService_ = eventService;
    logger_ = spdlog::default_logger()->clone(getName());
    spdlog::initialize_logger(logger_);
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

  IService::~IService() {
    if(logger_) {
      spdlog::drop(logger_->name());
    }
  }

} // namespace Eden