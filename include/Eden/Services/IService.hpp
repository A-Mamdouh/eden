#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <optional>
#include <spdlog/spdlog.h>

namespace Eden::Services
{
  class EventService;
} // namespace Eden::Services

namespace Eden
{

  /// Base class for engine subsystems that are *not* ticked by the main
  /// loop (config, event bus, clock, ...). Contrast with ISystem.
  class IService
  {
  public:
    IService() = default;

    /// Called once by Engine at construction: stores the event service
    /// reference, publishes Events::ServiceStartedEvent, then calls the
    /// derived onInit().
    /// @param eventService Shared event bus, held as a weak reference so
    ///        services don't extend its lifetime.
    void init(std::weak_ptr<Services::EventService> eventService);
    /// Human-readable name used in engine startup/shutdown logging.
    virtual std::string getName() = 0;
    /// Called once by Engine during shutdown.
    virtual void stop() {};

    virtual ~IService();
    IService(const IService &) = delete;
    IService &operator=(const IService &) = delete;
    IService(const IService &&) = delete;
    IService &&operator=(const IService &&) = delete;

  protected:
    /// Derived-class setup, invoked once from init() after eventService_ is set.
    virtual void onInit() = 0;
    /// @return The event bus passed to init(), or nullptr if it has expired.
    std::optional<Services::EventService *> getEventService();
    std::shared_ptr<spdlog::logger> logger_;

  private:
    std::weak_ptr<Services::EventService> eventService_;
  };

  template <typename T>
  concept is_service_type = std::is_base_of_v<IService, T>;

} // namespace Eden
