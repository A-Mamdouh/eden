#pragma once

#include "Eden/Events/IEvent.hpp"
#include "Eden/Services/IService.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Eden {

class EventService;

/// Opaque token returned by EventService::subscribe(), needed to
/// unsubscribe(). Only constructible by EventService itself.
class ListenerId {
private:
  friend EventService;
  ListenerId(std::size_t id) : id{id} {}
  std::size_t id;
};

/// Type-indexed pub/sub bus. Every Service and System holds a weak
/// reference to the single instance Engine owns, so any subsystem can
/// publish or subscribe to any IEvent-derived type without a direct
/// dependency on the publisher.
class EventService : public IService {
public:
  virtual std::string getName() override { return "EventType Service"; }

  /// Registers `listener` for every future publish<EventType>() call.
  /// @param listener Callback invoked synchronously on each matching publish().
  /// @return Token to pass to unsubscribe<EventType>(); not valid for
  ///         any other EventType.
  template <typename EventType>
    requires is_event_type<EventType>
  ListenerId subscribe(std::function<void(const EventType &)> listener);

  /// @param id Token previously returned by subscribe<EventType>() for
  ///        this same EventType; a mismatched EventType silently no-ops.
  template <typename EventType>
    requires is_event_type<EventType>
  void unsubscribe(ListenerId id);

  /// Synchronously invokes every listener subscribed to EventType.
  /// @param event Payload passed by const reference to each listener.
  template <typename EventType>
    requires is_event_type<EventType>
  void publish(const EventType &event) const;

private:
  void onInit() override {}
  struct Listener {
    ListenerId id;
    std::function<void(const void *)> invoker;
  };

  std::unordered_map<std::type_index, std::vector<Listener>> listeners_;
  std::size_t nextListenerId_{1};
};

template <typename EventType>
  requires is_event_type<EventType>
ListenerId
EventService::subscribe(std::function<void(const EventType &)> listener) {
  const auto id = nextListenerId_++;
  auto &bucket = listeners_[std::type_index(typeid(EventType))];
  bucket.push_back(
      Listener{id, [fn = std::move(listener)](const void *payload) {
                 fn(*static_cast<const EventType *>(payload));
               }});
  return ListenerId{id};
}

template <typename EventType>
  requires is_event_type<EventType>
void EventService::unsubscribe(ListenerId id) {
  auto it = listeners_.find(std::type_index(typeid(EventType)));
  if (it == listeners_.end()) {
    return;
  }

  auto &bucket = it->second;
  bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
                              [id](const Listener &listener) {
                                return listener.id.id == id.id;
                              }),
               bucket.end());
}

template <typename EventType>
  requires is_event_type<EventType>
void EventService::publish(const EventType &event) const {
  auto it = listeners_.find(std::type_index(typeid(EventType)));
  if (it == listeners_.end()) {
    return;
  }
  for (const auto &listener : it->second) {
    listener.invoker(&event);
  }
}

} // namespace Eden
