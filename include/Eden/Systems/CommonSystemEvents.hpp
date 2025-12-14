#pragma once

#include "Eden/Events/IEvent.hpp"
#include "ISystem.hpp"
#include <string>

namespace Eden::Events {
  struct SystemStartedEvent : public IEvent {
    std::string systemName;
    ISystem *system;
  };
}
