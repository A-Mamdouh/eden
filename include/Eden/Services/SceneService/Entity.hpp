#pragma once

#include <entt/entt.hpp>

#include <utility>

namespace Eden {

/// Lightweight, copyable handle into a Scene's registry. Doesn't own the
/// entity -- destroying the Scene (or the entity itself) invalidates it.
class Entity {
public:
  Entity() = default;
  Entity(entt::entity handle, entt::registry *registry) : handle_{handle}, registry_{registry} {}

  /// Constructs a T in place on this entity and returns it. Replaces
  /// any existing T -- for that, prefer getComponent().
  template <typename T, typename... Args>
  T &addComponent(Args &&...args) {
    return registry_->emplace<T>(handle_, std::forward<Args>(args)...);
  }

  /// @return Reference to this entity's T; undefined behavior if it
  ///         doesn't have one (check hasComponent() first if unsure).
  template <typename T>
  T &getComponent() {
    return registry_->get<T>(handle_);
  }

  /// @overload
  template <typename T>
  const T &getComponent() const {
    return registry_->get<T>(handle_);
  }

  /// @return True if this entity currently has a T.
  template <typename T>
  bool hasComponent() const {
    return registry_->all_of<T>(handle_);
  }

  /// @return The underlying entt entity id, for code that needs to talk
  ///         to the registry directly (e.g. EntityHierarchy::parent).
  entt::entity handle() const noexcept { return handle_; }

  /// @return False if default-constructed, or if the entity has been
  ///         destroyed since this handle was obtained.
  bool valid() const noexcept { return registry_ != nullptr && registry_->valid(handle_); }

private:
  entt::entity handle_{entt::null};
  entt::registry *registry_{nullptr};
};

} // namespace Eden
