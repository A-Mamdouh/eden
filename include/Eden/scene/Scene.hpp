#pragma once

#ifndef EDEN_ENGINE_SCENE_HPP
#define EDEN_ENGINE_SCENE_HPP

#include "Eden/ecs/Entity.hpp"

namespace Eden {

/**
 * High-level scene abstraction.
 *
 * Users subclass Scene to implement their game logic.
 * The engine owns the active Scene instance and calls its lifecycle methods.
 */
class Scene {
public:
  Scene();
  virtual ~Scene();

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;

  Scene(Scene &&) noexcept = delete;
  Scene &operator=(Scene &&) noexcept = delete;

  virtual void load() {}
  virtual void unload() {
    registry_.clear();
  }

  void moveEntity(const EntityId entityId,
                    const EntityId parentId = entt::null);

  /**
   * Create a new entity in this scene and return a façade for it.
   */
  Entity createEntity(const EntityId parent = entt::null);

  /**
   * Destroy an existing entity by identifier.
   */
  void destroyEntity(EntityId id);

  Registry &getRegistry() noexcept { return registry_; }
  const Registry& getRegistry() const { return registry_; }

protected:
  /**
   * Direct access to the underlying registry for systems
   * that need more advanced queries.
   */
  Registry &registry() noexcept { return registry_; }
  const Registry &registry() const noexcept { return registry_; }
private:
  Registry registry_;
protected:
  Entity rootEntity_;

};

} // namespace Eden

#endif // EDEN_ENGINE_SCENE_HPP
