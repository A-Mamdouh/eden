#pragma once
#include "Eden/Events/IEvent.hpp"

#include "Config.hpp"

namespace Eden::Events {

struct ConfigUpdatedEvent : public IEvent {
  const Config::ApplicationConfig * oldConfig;
  const Config::ApplicationConfig * newConfig;
};

} // namespace Eden::Events