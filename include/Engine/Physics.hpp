//#pragma once

#ifndef EDEN_ENGINE_PHYSICS_HPP
#define EDEN_ENGINE_PHYSICS_HPP

#include "Engine/Entity.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace Eden
{

using Vec2 = glm::vec2;

enum class BodyType2D
{
    Static,
    Kinematic,
    Dynamic
};

enum class ColliderShape2D
{
    Box,
    Circle,
    Capsule,
    Polygon
};

struct PhysicsMaterial
{
    float friction{0.8f};     // Surface friction coefficient (0 = no friction).
    float restitution{0.0f};  // Bounciness (0 = inelastic, 1 = perfectly elastic).
};

struct Rigidbody2D
{
    BodyType2D type{BodyType2D::Dynamic};      // Static/kinematic/dynamic body mode.
    Vec2 linearVelocity{0.0f, 0.0f};           // Linear velocity (world units per second).
    float angularVelocity{0.0f};               // Angular velocity (radians per second).
    float linearDamping{0.0f};                 // Linear drag applied each step.
    float angularDamping{0.0f};                // Rotational drag.
    bool fixedRotation{false};                 // Lock rotation (infinite inertia).
    bool enableCCD{true};                      // Enable continuous collision detection.
    // Optional per-frame translation applied directly to the body (teleport-style).
    Vec2 pendingTranslation{0.0f, 0.0f};
};

struct Collider2D
{
    ColliderShape2D shape{ColliderShape2D::Box};
    Vec2 size{0.5f, 0.5f}; // Half-extents for box/capsule shapes.
    float radius{0.25f};   // Radius for circle/capsule shapes.
    Vec2 offset{0.0f, 0.0f}; // Local offset from the entity transform origin.
    bool isSensor{false};  // If true, generates contacts but no physical response.
    PhysicsMaterial material{};
};

struct PhysicsConfig2D
{
    Vec2 gravity{0.0f, -9.81f};         // Global gravity vector.
    float fixedTimestep{1.0f / 165.0f}; // Fixed physics step duration (seconds).
    int velocityIterations{16};         // Solver iterations for velocity constraints.
    int maxSubSteps{8};                 // Max fixed steps per frame (prevents spiral of death).
    float collisionSlop{0.0f};         // Allowed penetration before correction (meters).
};

class PhysicsWorld2D
{
public:
    explicit PhysicsWorld2D(const PhysicsConfig2D& config = {});
    ~PhysicsWorld2D();

    PhysicsWorld2D(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D(PhysicsWorld2D&&) noexcept = delete;
    PhysicsWorld2D& operator=(PhysicsWorld2D&&) noexcept = delete;

    void setConfig(const PhysicsConfig2D& config);
    [[nodiscard]] const PhysicsConfig2D& config() const noexcept { return config_; }

    // Sync bodies/fixtures from ECS state (component add/remove).
    void syncFromRegistry(Registry& registry);

    // Step the physics world at a fixed timestep, syncing transforms back.
    void step(Registry& registry, float deltaTime);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    PhysicsConfig2D config_{};
};

} // namespace Eden

#endif // EDEN_ENGINE_PHYSICS_HPP
