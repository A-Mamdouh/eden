#include "Eden/Services/SceneService/Scene.hpp"

namespace Eden::World {

void Scene::destroyEntity(Entity *entity) {
  registry_.destroy(entity->handle());
  *entity = Entity{};
}

Entity Scene::getEntity(entt::entity handle) const {
  auto &registry = const_cast<entt::registry &>(registry_);
  return registry.valid(handle) ? Entity{handle, &registry} : Entity{};
}

} // namespace Eden::World