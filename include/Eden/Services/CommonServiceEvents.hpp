#include "Eden/Events/IEvent.hpp"
#include "IService.hpp"
#include <string>

namespace Eden::Events {
  struct ServiceStartedEvent : public IEvent {
    std::string serviceName;
    IService *service;
  };
}