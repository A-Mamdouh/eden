#pragma once

#ifndef EDEN_ENGINE_PHYSICS_HPP
#define EDEN_ENGINE_PHYSICS_HPP

#include "Eden/core/Math.hpp"
#include "Eden/ecs/Entity.hpp"

namespace Eden
{

enum class BodyType2D
{
    Static,
    Dynamic,
    Kinematic
};

struct Rigidbody2D
{
    BodyType2D type{BodyType2D::Static};
    bool fixedRotation{false};
    bool enableCCD{false};
};

struct ColliderMaterial2D
{
    float friction{0.0f};
    float restitution{0.0f};
};

enum class ColliderShape2D
{
    Box,
    Circle
};

struct Collider2D
{
    ColliderShape2D shape{ColliderShape2D::Box};
    Vec2 size{1.0f, 1.0f};
    ColliderMaterial2D material{};
};

struct PhysicsConfig2D
{
    Vec2 gravity{0.0f, -9.81f};
};

class PhysicsWorld2D
{
public:
    void setConfig(const PhysicsConfig2D&) {}
    void syncFromRegistry(Registry&) {}
    void step(Registry&, float) {}
};

} // namespace Eden

#endif // EDEN_ENGINE_PHYSICS_HPP
