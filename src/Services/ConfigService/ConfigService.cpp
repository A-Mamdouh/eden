#include "Eden/Services/ConfigService/ConfigService.hpp"
#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Services/ConfigService/ConfigServiceEvents.hpp"
#include "Eden/Services/EventService/EventService.hpp"

namespace Eden::Services {
void ConfigService::update(const Config::ApplicationConfig &newConfig) {
  const auto eventService = getEventService();
  if(eventService.has_value()) {
    eventService.value()->publish<Events::ConfigUpdatedEvent>(
        Events::ConfigUpdatedEvent{.oldConfig = &config_,
                                   .newConfig = &newConfig});
  } else {
    logger_->warn("Failed to send config updated event. Event service not available.");
  }
  config_ = newConfig;
}
} // namespace Eden::Services