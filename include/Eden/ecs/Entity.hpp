#pragma once

#ifndef EDEN_ENGINE_ENTITY_HPP
#define EDEN_ENGINE_ENTITY_HPP

#include "Component.hpp"
#include "Base.hpp"

namespace Eden
{

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
    requires is_component<Component>
    Component& addComponent(Args&&... args)
    {
        return registry_->emplace<Component>(id_, static_cast<Args&&>(args)...);
    }

    template <typename Component>
    requires is_component<Component>
    [[nodiscard]] bool hasComponent() const
    {
        return registry_->any_of<Component>(id_);
    }

    template <typename Component>
    requires is_component<Component>
    [[nodiscard]] Component& getComponent()
    {
        return registry_->get<Component>(id_);
    }

    template <typename Component>
    requires is_component<Component>
    [[nodiscard]] const Component& getComponent() const
    {
        return registry_->get<Component>(id_);
    }

    template <typename Component>
    requires is_component<Component>
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

