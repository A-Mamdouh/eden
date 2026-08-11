#include "Eden/Systems/TransformSystem.hpp"

#include "Eden/Services/SceneService/Components.hpp"
#include "Eden/Services/SceneService/Scene.hpp"
#include "Eden/Services/SceneService/SceneService.hpp"

#include <unordered_map>

namespace Eden {
namespace {

Mat4 computeWorldMatrix(entt::entity entity, entt::registry &registry,
                        std::unordered_map<entt::entity, Mat4> &cache) {
  if (auto it = cache.find(entity); it != cache.end()) {
    return it->second;
  }

  const auto &transform = registry.get<Transform>(entity);
  Mat4 world = transform.localMatrix();

  if (const auto *hierarchy = registry.try_get<EntityHierarchy>(entity)) {
    if (hierarchy->parent != entt::null && hierarchy->parent != entity &&
        registry.valid(hierarchy->parent) && registry.all_of<Transform>(hierarchy->parent)) {
      world = computeWorldMatrix(hierarchy->parent, registry, cache) * world;
    }
  }

  cache.emplace(entity, world);
  return world;
}

} // namespace

void TransformSystem::update(double /*dt*/) {
  Scene *scene = sceneService_.activeScene();
  if (!scene) {
    return;
  }

  auto &registry = scene->getRegistry();
  std::unordered_map<entt::entity, Mat4> cache;

  for (const auto entity : registry.view<Transform>()) {
    registry.emplace_or_replace<WorldTransform>(entity, WorldTransform{computeWorldMatrix(entity, registry, cache)});
  }
}

} // namespace Eden
