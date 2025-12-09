//#pragma once

#ifndef EDEN_ENGINE_ECS_SCENE_HPP
#define EDEN_ENGINE_ECS_SCENE_HPP

#include "Engine/Entity.hpp"
#include "Engine/Scene.hpp"

namespace Eden
{

/**
 * Scene base class backed by an ECS registry.
 *
 * Provides a small, GameObject-style façade over entt so that
 * concrete scenes can create and manage entities without touching
 * the registry directly if they don't want to.
 */
class EcsScene : public Scene
{
public:
    EcsScene() = default;
    ~EcsScene() override = default;

    EcsScene(const EcsScene&) = delete;
    EcsScene& operator=(const EcsScene&) = delete;

    EcsScene(EcsScene&&) noexcept = delete;
    EcsScene& operator=(EcsScene&&) noexcept = delete;

protected:
    /**
     * Create a new entity in this scene and return a façade for it.
     */
    Entity createEntity()
    {
        const EntityId id = registry_.create();
        return Entity{registry_, id};
    }

    /**
     * Destroy an existing entity by identifier.
     */
    void destroyEntity(EntityId id)
    {
        if (registry_.valid(id))
        {
            registry_.destroy(id);
        }
    }

    /**
     * Direct access to the underlying registry for systems
     * that need more advanced queries.
     */
    Registry& registry() noexcept { return registry_; }
    const Registry& registry() const noexcept { return registry_; }

private:
    Registry registry_;
};

} // namespace Eden

#endif // EDEN_ENGINE_ECS_SCENE_HPP

