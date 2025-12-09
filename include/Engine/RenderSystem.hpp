//#pragma once

#ifndef EDEN_ENGINE_RENDER_SYSTEM_HPP
#define EDEN_ENGINE_RENDER_SYSTEM_HPP

#include "Engine/Components.hpp"
#include "Engine/Renderer.hpp"

#include <unordered_map>

namespace Eden
{

/**
 * Simple rendering system that walks the ECS registry and pushes
 * draw commands to the active renderer.
 *
 * The system is intentionally stateless for now; future revisions
 * can extend it to handle batching, materials, etc.
 */
class RenderSystem
{
public:
    RenderSystem() = default;

    void render(Registry& registry, Renderer& renderer) const
    {
        std::unordered_map<EntityId, Mat4> worldCache;
        auto view = registry.view<Transform, Renderable>();
        for (auto entityId : view)
        {
            const auto& renderable = view.template get<Renderable>(entityId);

            DrawParams params{};
            params.color = renderable.material.baseColor;
            params.useVertexColor = renderable.material.useVertexColor;
            params.modelMatrix = buildWorldMatrix(entityId, registry, worldCache);

            renderer.submitDrawCommand(renderable.shape, params);
        }
    }

private:
    Mat4 buildWorldMatrix(
        EntityId entityId,
        Registry& registry,
        std::unordered_map<EntityId, Mat4>& cache) const
    {
        if (auto it = cache.find(entityId); it != cache.end())
        {
            return it->second;
        }

        const auto& transform = registry.get<Transform>(entityId);
        const Mat4 local =
            composeTransform(transform.position, transform.rotationEuler, transform.scale);

        Mat4 world = local;
        if (transform.parent != entt::null &&
            transform.parent != entityId &&
            registry.valid(transform.parent) &&
            registry.any_of<Transform>(transform.parent))
        {
            world = buildWorldMatrix(transform.parent, registry, cache) * local;
        }

        cache.emplace(entityId, world);
        return world;
    }
};

} // namespace Eden

#endif // EDEN_ENGINE_RENDER_SYSTEM_HPP
