#pragma once

#include <memory>
#include <string>
#include <type_traits>

namespace Eden {
class EventService;

/// Base class for engine subsystems that need per-frame work (rendering,
/// input polling, ...). Contrast with IService, which is not ticked.
class ISystem {
public:
  ISystem() = default;

  /// Called once by Engine before the first update(): stores the event
  /// service reference, publishes Events::SystemStartedEvent, then calls
  /// the derived onInit().
  /// @param eventService Shared event bus, held as a weak reference so
  ///        systems don't extend its lifetime.
  void init(std::weak_ptr<const EventService> eventService);
  /// Human-readable name used in engine startup/shutdown logging.
  virtual std::string getName() = 0;
  /// Called once per Engine::run() iteration.
  /// @param dt Frame delta time in seconds.
  virtual void update(double dt) = 0;
  /// Called once by Engine during shutdown, in reverse registration order.
  virtual void shutdown() = 0;

  virtual ~ISystem() = default;
  ISystem(const ISystem &) = delete;
  ISystem &operator=(const ISystem &) = delete;
  ISystem(const ISystem &&) = delete;
  ISystem &&operator=(const ISystem &&) = delete;

protected:
  /// Derived-class setup, invoked once from init() after eventService_ is set.
  virtual void onInit() = 0;
  /// @return The event bus passed to init(), or nullptr if it has expired.
  const EventService* getEventService();
private:
  std::weak_ptr<const EventService> eventService_;
};

template <typename T>
concept is_system_type = std::is_base_of_v<ISystem, T>;

} // namespace Eden
