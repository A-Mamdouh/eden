#pragma once
#include "Eden/Events/IEvent.hpp"

#include "Config.hpp"

namespace Eden::Events {

/// Published by ConfigService::update() before it overwrites the live
/// config. oldConfig aliases the service's own member, so it only reads
/// as the old value during this synchronous listener call -- once
/// update() returns, that same address holds the new value.
struct ConfigUpdatedEvent : public IEvent {
  /// Value before this update; see the class doc re: its lifetime.
  const Config::ApplicationConfig * oldConfig;
  /// Value this update is about to apply.
  const Config::ApplicationConfig * newConfig;
};

} // namespace Eden::Events