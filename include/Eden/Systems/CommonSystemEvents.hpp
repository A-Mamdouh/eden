#include "Eden/Events/IEvent.hpp"
#include "ISystem.hpp"
#include <string>

namespace Eden::Events {
  struct ServiceStartedEvent : public IEvent {
    std::string systemName;
    ISystem *system;
  };
}