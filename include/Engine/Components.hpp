//#pragma once

#ifndef EDEN_ENGINE_COMPONENTS_HPP
#define EDEN_ENGINE_COMPONENTS_HPP

#include "Engine/Camera.hpp"
#include "Engine/Entity.hpp"
#include "Engine/Material.hpp"

namespace Eden
{

struct Transform
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotationEuler{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};

    // Parent entity used to build world transforms (entt::null = no parent).
    EntityId parent{entt::null};
};

struct Renderable
{
    Material material{};
    PrimitiveShape shape{PrimitiveShape::Triangle};
};

} // namespace Eden

#endif // EDEN_ENGINE_COMPONENTS_HPP
