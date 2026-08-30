#include "Eden/Services/ConfigService/ConfigService.hpp"
#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Services/ConfigService/ConfigServiceEvents.hpp"
#include "Eden/Services/EventService/EventService.hpp"

namespace Eden::Services {
void ConfigService::update(const Config::ApplicationConfig &newConfig) {
  getEventService()->publish<Events::ConfigUpdatedEvent>(
      Events::ConfigUpdatedEvent{.oldConfig = &config_,
                                 .newConfig = &newConfig});
  config_ = newConfig;
}
} // namespace Eden::Services