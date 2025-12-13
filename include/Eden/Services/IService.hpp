#pragma once

#include <memory>
#include <string>
#include <type_traits>

namespace Eden {
class EventService;

/**
 * Services are decoupled from the engine main loop.
 */
class IService {
public:
  IService() = default;

  void init(std::weak_ptr<const EventService> eventService);
  virtual std::string getName() = 0;
  virtual void stop() {};

  virtual ~IService() = default;
  IService(const IService &) = delete;
  IService &operator=(const IService &) = delete;
  IService(const IService &&) = delete;
  IService &&operator=(const IService &&) = delete;

protected:
  virtual void onInit() = 0;
  const EventService* getEventService();
private:
  std::weak_ptr<const EventService> eventService_;
};

template <typename T>
concept is_service_type = std::is_base_of_v<IService, T>;

} // namespace Eden