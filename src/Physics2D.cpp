#include "Engine/Physics.hpp"

#include "Engine/Components.hpp"

#include <chipmunk/chipmunk.h>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Eden
{

namespace
{

cpBodyType toChipmunkBodyType(BodyType2D type)
{
    switch (type)
    {
    case BodyType2D::Static: return CP_BODY_TYPE_STATIC;
    case BodyType2D::Kinematic: return CP_BODY_TYPE_KINEMATIC;
    case BodyType2D::Dynamic:
    default: return CP_BODY_TYPE_DYNAMIC;
    }
}

float defaultDensity(BodyType2D type)
{
    return (type == BodyType2D::Static) ? 0.0f : 1.0f;
}

} // namespace

struct PhysicsWorld2D::Impl
{
    cpSpace* space{nullptr};
    struct BodyRecord
    {
        cpBody* body{nullptr};
        std::vector<cpShape*> shapes{};
    };
    std::unordered_map<EntityId, BodyRecord> bodies{};
    float accumulator{0.0f};

    ~Impl()
    {
        clear();
    }

    void clear()
    {
        for (auto& [_, record] : bodies)
        {
            for (cpShape* shape : record.shapes)
            {
                if (shape)
                {
                    cpShapeFree(shape);
                }
            }
            if (record.body)
            {
                cpBodyFree(record.body);
            }
        }
        bodies.clear();

        if (space)
        {
            cpSpaceFree(space);
            space = nullptr;
        }
        accumulator = 0.0f;
    }

    void rebuild(const PhysicsConfig2D& config)
    {
        clear();
        space = cpSpaceNew();
        cpSpaceSetGravity(space, cpv(config.gravity.x, config.gravity.y));
        cpSpaceSetIterations(space, config.velocityIterations);
        cpSpaceSetCollisionSlop(space, config.collisionSlop);
    }
};

PhysicsWorld2D::PhysicsWorld2D(const PhysicsConfig2D& config)
    : impl_(std::make_unique<Impl>())
    , config_(config)
{
    impl_->rebuild(config_);
}

PhysicsWorld2D::~PhysicsWorld2D() = default;

void PhysicsWorld2D::setConfig(const PhysicsConfig2D& config)
{
    config_ = config;
    impl_->rebuild(config_);
}

void PhysicsWorld2D::syncFromRegistry(Registry& registry)
{
    impl_->rebuild(config_);

    auto view = registry.view<Transform, Rigidbody2D, Collider2D>();
    for (auto entity : view)
    {
        auto& transform = view.get<Transform>(entity);
        auto& rigidbody = view.get<Rigidbody2D>(entity);
        auto& collider = view.get<Collider2D>(entity);

        cpBody* body = nullptr;
        switch (rigidbody.type)
        {
        case BodyType2D::Static:
            body = cpBodyNewStatic();
            break;
        case BodyType2D::Kinematic:
            body = cpBodyNewKinematic();
            break;
        case BodyType2D::Dynamic:
        default:
            body = cpBodyNew(1.0, 1.0);
            break;
        }
        cpBodySetType(body, toChipmunkBodyType(rigidbody.type));
        cpBodySetPosition(body, cpv(transform.position.x, transform.position.y));
        cpBodySetAngle(body, transform.rotationEuler.z);
        cpBodySetVelocity(body, cpv(rigidbody.linearVelocity.x, rigidbody.linearVelocity.y));
        cpBodySetAngularVelocity(body, rigidbody.angularVelocity);
        if (rigidbody.fixedRotation && rigidbody.type == BodyType2D::Dynamic)
        {
            cpBodySetMoment(body, INFINITY);
        }

        cpSpaceAddBody(impl_->space, body);

        PhysicsWorld2D::Impl::BodyRecord record;
        record.body = body;

        const float density = defaultDensity(rigidbody.type);

        switch (collider.shape)
        {
        case ColliderShape2D::Circle:
        {
            cpShape* shape = cpCircleShapeNew(body, collider.radius, cpv(collider.offset.x, collider.offset.y));
            cpShapeSetDensity(shape, density);
            record.shapes.push_back(shape);
            break;
        }
        case ColliderShape2D::Capsule:
        case ColliderShape2D::Polygon:
        case ColliderShape2D::Box:
        default:
        {
            const float width = collider.size.x * 2.0f;
            const float height = collider.size.y * 2.0f;
            const cpBB box = {
                collider.offset.x - collider.size.x,
                collider.offset.y - collider.size.y,
                collider.offset.x + collider.size.x,
                collider.offset.y + collider.size.y};
            cpShape* shape = cpBoxShapeNew2(body, box, 0.0f);
            cpShapeSetDensity(shape, density);
            record.shapes.push_back(shape);
            break;
        }
        }

        for (cpShape* shape : record.shapes)
        {
            cpShapeSetElasticity(shape, collider.material.restitution);
            cpShapeSetFriction(shape, collider.material.friction);
            cpShapeSetSensor(shape, collider.isSensor ? cpTrue : cpFalse);
            cpSpaceAddShape(impl_->space, shape);
        }

        impl_->bodies[entity] = record;
    }
}

void PhysicsWorld2D::step(Registry& registry, float deltaTime)
{
    if (!impl_->space)
    {
        return;
    }

    const float dt = config_.fixedTimestep;
    const int maxSubSteps = std::max(10, config_.maxSubSteps);

    // Push component velocity/pose into Chipmunk bodies before stepping.
    {
        auto view = registry.view<Transform, Rigidbody2D>();
        for (auto entity : view)
        {
            auto it = impl_->bodies.find(entity);
            if (it == impl_->bodies.end())
            {
                continue;
            }

            auto& transform = view.get<Transform>(entity);
            auto& rigidbody = view.get<Rigidbody2D>(entity);
            cpBody* body = it->second.body;
            if (!body)
            {
                continue;
            }

            if (rigidbody.type == BodyType2D::Static)
            {
                cpBodySetPosition(body, cpv(transform.position.x, transform.position.y));
                cpBodySetAngle(body, transform.rotationEuler.z);
                continue;
            }

            if (rigidbody.pendingTranslation.x != 0.0f || rigidbody.pendingTranslation.y != 0.0f)
            {
                const cpVect pos = cpBodyGetPosition(body);
                const cpVect delta = cpv(rigidbody.pendingTranslation.x, rigidbody.pendingTranslation.y);
                const cpVect newPos = cpvadd(pos, delta);
                cpBodySetPosition(body, newPos);
                rigidbody.pendingTranslation = Vec2{0.0f, 0.0f};
            }

            cpBodySetVelocity(body, cpv(rigidbody.linearVelocity.x, rigidbody.linearVelocity.y));
            cpBodySetAngularVelocity(body, rigidbody.angularVelocity);

            if (rigidbody.type == BodyType2D::Kinematic)
            {
                // For kinematic bodies, the transform drives the pose; velocity is still used.
                cpBodySetPosition(body, cpv(transform.position.x, transform.position.y));
                cpBodySetAngle(body, transform.rotationEuler.z);
            }
            else
            {
                // Dynamic: enforce fixed rotation if requested.
                if (rigidbody.fixedRotation)
                {
                    cpBodySetMoment(body, INFINITY);
                    cpBodySetAngularVelocity(body, 0.0);
                    cpBodySetAngle(body, transform.rotationEuler.z);
                }
            }
        }
    }

    impl_->accumulator += deltaTime;

    int steps = 0;
    while (impl_->accumulator >= dt && steps < maxSubSteps)
    {
        cpSpaceStep(impl_->space, dt);
        impl_->accumulator -= dt;
        ++steps;
    }

    auto view = registry.view<Transform, Rigidbody2D>();
    for (auto entity : view)
    {
        auto it = impl_->bodies.find(entity);
        if (it == impl_->bodies.end())
        {
            continue;
        }

        auto& record = it->second;
        cpBody* body = record.body;
        if (!body)
        {
            continue;
        }

        auto& transform = view.get<Transform>(entity);
        auto& rigidbody = view.get<Rigidbody2D>(entity);

        const cpVect position = cpBodyGetPosition(body);
        transform.position.x = static_cast<float>(position.x);
        transform.position.y = static_cast<float>(position.y);
        // keep existing z
        transform.rotationEuler.z =
            rigidbody.fixedRotation ? 0.0f : static_cast<float>(cpBodyGetAngle(body));

        const cpVect velocity = cpBodyGetVelocity(body);
        rigidbody.linearVelocity = Vec2{
            static_cast<float>(velocity.x),
            static_cast<float>(velocity.y)};
        rigidbody.angularVelocity =
            rigidbody.fixedRotation ? 0.0f : static_cast<float>(cpBodyGetAngularVelocity(body));
    }
}

} // namespace Eden
