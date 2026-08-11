#pragma once

#include "Entity.hpp"

#include <entt/entt.hpp>

namespace Eden {

/// Owns the set of active entities and their components for one loaded
/// scene. SceneService owns the active Scene instance; systems that need
/// per-frame behavior over its entities (TransformSystem, RenderSystem)
/// get a reference to it from SceneService, not the other way around.
class Scene {
public:
  /// @return A new entity with no components.
  Entity createEntity() { return Entity{registry_.create(), &registry_}; }

  /// @param entity Entity to destroy, along with all of its components.
  void destroyEntity(Entity entity) { registry_.destroy(entity.handle()); }

  /// @return The underlying entt registry, for systems that need to
  ///         iterate entities by component (e.g. TransformSystem).
  entt::registry &getRegistry() { return registry_; }
  /// @overload
  const entt::registry &getRegistry() const { return registry_; }

private:
  entt::registry registry_;
};

} // namespace Eden
