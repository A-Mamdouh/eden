#pragma once

#ifndef EDEN_ENGINE_EVENT_SYSTEM_HPP
#define EDEN_ENGINE_EVENT_SYSTEM_HPP

#include "System.hpp"
#include "Eden/platform/Input.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Eden {

/**
 * Central event hub containing input state and a lightweight pub/sub bus.
 *
 * Implemented as a singleton so gameplay and engine code can fetch it
 * anywhere without threading references through APIs.
 */
class EventSystem : public ISystem {
public:
  static EventSystem &getInstance();

  EventSystem(const EventSystem &) = delete;
  EventSystem &operator=(const EventSystem &) = delete;

  EventSystem(EventSystem &&) noexcept = delete;
  EventSystem &operator=(EventSystem &&) noexcept = delete;

  void beginFrame();
  void update(float deltaTime) override;

  Input &getInput() noexcept;
  const Input &getInput() const noexcept;

  template <typename Event>
  std::size_t subscribe(std::function<void(const Event &)> listener);

  template <typename Event> void unsubscribe(std::size_t id);

  template <typename Event> void publish(const Event &event) const;

private:
  EventSystem() = default;

  struct Listener {
    std::size_t id;
    std::function<void(const void *)> invoker;
  };

  template <typename Event> static std::type_index typeKey();

  static std::unique_ptr<EventSystem> instance_;
  Input input_{};
  mutable std::unordered_map<std::type_index, std::vector<Listener>> listeners_;
  std::size_t nextListenerId_{1};
};

template <typename Event> std::type_index EventSystem::typeKey() {
  return std::type_index(typeid(Event));
}

template <typename Event>
std::size_t
EventSystem::subscribe(std::function<void(const Event &)> listener) {
  const auto id = nextListenerId_++;
  auto &bucket = listeners_[typeKey<Event>()];
  bucket.push_back(
      Listener{id, [fn = std::move(listener)](const void *payload) {
                 fn(*static_cast<const Event *>(payload));
               }});
  return id;
}

template <typename Event> void EventSystem::unsubscribe(std::size_t id) {
  auto it = listeners_.find(typeKey<Event>());
  if (it == listeners_.end()) {
    return;
  }

  auto &bucket = it->second;
  bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
                              [id](const Listener &listener) {
                                return listener.id == id;
                              }),
               bucket.end());
}

template <typename Event> void EventSystem::publish(const Event &event) const {
  auto it = listeners_.find(typeKey<Event>());
  if (it == listeners_.end()) {
    return;
  }

  for (const auto &listener : it->second) {
    listener.invoker(&event);
  }
}

} // namespace Eden

#endif // EDEN_ENGINE_EVENT_SYSTEM_HPP
