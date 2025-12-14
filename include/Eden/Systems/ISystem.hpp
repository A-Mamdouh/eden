#pragma once

#include <memory>
#include <string>
#include <type_traits>

namespace Eden {
class EventService;

/**
 * Systems are driven by the engine main loop.
 */
class ISystem {
public:
  ISystem() = default;

  void init(std::weak_ptr<const EventService> eventService);
  virtual std::string getName() = 0;
  virtual void update(double dt) = 0;
  virtual void shutdown() = 0;

  virtual ~ISystem() = default;
  ISystem(const ISystem &) = delete;
  ISystem &operator=(const ISystem &) = delete;
  ISystem(const ISystem &&) = delete;
  ISystem &&operator=(const ISystem &&) = delete;

protected:
  virtual void onInit() = 0;
  const EventService* getEventService();
private:
  std::weak_ptr<const EventService> eventService_;
};

template <typename T>
concept is_system_type = std::is_base_of_v<ISystem, T>;

} // namespace Eden
