#pragma once

#include "Entity.hpp"

#include <entt/entt.hpp>

#include <vector>

namespace Eden {

/// Owns the set of active entities and their components for one loaded
/// scene. SceneService owns the active Scene instance; systems that need
/// per-frame behavior over its entities (TransformSystem, RenderSystem)
/// get a reference to it from SceneService, not the other way around.
/// ScriptBehaviour also gets a Scene& directly (via ScriptComponent, see
/// ScriptComponent.hpp) -- createEntity()/destroyEntity() are safe to
/// call from a script's own onStart()/onUpdate(), see destroyEntity()'s
/// doc for the one caveat.
class Scene {
public:
  /// @return A new entity with no components. Synchronous and immediate
  ///         -- doesn't touch any component pool a view might be
  ///         iterating, so it's always safe to call, including from
  ///         inside a script's own onUpdate().
  Entity createEntity() { return Entity{registry_.create(), &registry_}; }

  /// Removes `entity` and all its components. Safe to call for the
  /// entity currently driving the script that calls it -- EnTT's storage
  /// explicitly supports destroying the entity currently being visited
  /// by a view mid-iteration. Destroying a *different* entity than the
  /// one whose script is currently running is the caller's own
  /// responsibility to reason about, same as any other structural ECS
  /// mutation made while something else might be iterating.
  ///
  /// Not a promise about internal timing beyond that -- only that
  /// `*entity` stops resolving immediately. Other copies of the same
  /// Entity held elsewhere aren't touched by this call; re-check
  /// valid() before using those, same discipline as any Entity held
  /// across a frame boundary (see RenderSystem's camera resolution).
  /// @param entity Reset to a default (invalid) Entity after this call.
  void destroyEntity(Entity *entity);

  /// Re-resolves `handle` against this registry right now. Entity ids
  /// aren't safe to hold across frames -- call this again before each
  /// use rather than trusting a stored Entity, same as
  /// RenderSystem::activeCamera().
  /// @return Entity wrapping `handle` if it's currently valid here, or a
  ///         default (invalid) Entity if not.
  Entity getEntity(entt::entity handle) const;

  /// @return Every entity that currently has a T, as Entity handles
  ///         ready for getComponent()/addComponent() -- callers never
  ///         touch entt::entity or entt::registry directly through this.
  template <typename T>
  std::vector<Entity> entitiesWith() const {
    auto &registry = const_cast<entt::registry &>(registry_);
    std::vector<Entity> result;
    for (const auto handle : registry.view<T>()) {
      result.push_back(Entity{handle, &registry});
    }
    return result;
  }

  /// @return The underlying entt registry, for systems that need to
  ///         iterate entities by component (e.g. TransformSystem).
  entt::registry &getRegistry() { return registry_; }
  /// @overload
  const entt::registry &getRegistry() const { return registry_; }

private:
  entt::registry registry_;
};

} // namespace Eden
