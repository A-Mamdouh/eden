//#pragma once

#ifndef EDEN_ENGINE_ENTITY_HPP
#define EDEN_ENGINE_ENTITY_HPP

#include <entt/entt.hpp>

namespace Eden
{

using EntityId = entt::entity;
using Registry = entt::registry;

/**
 * Lightweight facade over an ECS entity.
 *
 * Wraps an entt::entity and registry to provide a user-friendly
 * GameObject-style API for adding and querying components.
 */
class Entity
{
public:
    Entity() = default;

    Entity(Registry& registry, EntityId id) noexcept
        : registry_(&registry)
        , id_(id)
    {
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return registry_ && registry_->valid(id_);
    }

    [[nodiscard]] EntityId id() const noexcept
    {
        return id_;
    }

    template <typename Component, typename... Args>
    Component& addComponent(Args&&... args)
    {
        return registry_->emplace<Component>(id_, static_cast<Args&&>(args)...);
    }

    template <typename Component>
    [[nodiscard]] bool hasComponent() const
    {
        return registry_->any_of<Component>(id_);
    }

    template <typename Component>
    [[nodiscard]] Component& getComponent()
    {
        return registry_->get<Component>(id_);
    }

    template <typename Component>
    [[nodiscard]] const Component& getComponent() const
    {
        return registry_->get<Component>(id_);
    }

    template <typename Component>
    void removeComponent()
    {
        registry_->remove<Component>(id_);
    }

private:
    Registry* registry_{nullptr};
    EntityId id_{entt::null};
};

} // namespace Eden

#endif // EDEN_ENGINE_ENTITY_HPP

