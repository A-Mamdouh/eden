#pragma once

#include "Eden/Events/IEvent.hpp"
#include "ISystem.hpp"
#include <string>

namespace Eden::Events {
  /// Published by ISystem::init(), just before the derived onInit() runs.
  struct SystemStartedEvent : public IEvent {
    /// Result of the system's getName().
    std::string systemName;
    /// Non-owning pointer to the started system; valid for the system's
    /// lifetime, which outlives this event.
    ISystem *system;
  };
}
