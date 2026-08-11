#pragma once

#include "Eden/Events/IEvent.hpp"
#include "IService.hpp"
#include <string>

namespace Eden::Events {
  /// Published by IService::init(), just before the derived onInit() runs.
  struct ServiceStartedEvent : public IEvent {
    /// Result of the service's getName().
    std::string serviceName;
    /// Non-owning pointer to the started service; valid for the service's
    /// lifetime, which outlives this event.
    IService *service;
  };
}
