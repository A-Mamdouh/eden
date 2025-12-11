#include "Eden/scene/Scene.hpp"
#include "Log.hpp"
#include "Eden/ecs/EntityHierarchyComponent.hpp"

// TODO: After implementing serialization. The scene should just be a data container containing the scene tree (entities and registry). Nothing else should be done here

namespace Eden {
Scene::Scene() :
  registry_{},
  rootEntity_{registry_, registry_.create()}
  {
    rootEntity_.addComponent<EntityHierarchy>();
  }

void Scene::moveEntity(const EntityId entityId, const EntityId parentId) {
  EDEN_ASSERT((entityId != rootEntity_.id()), "Cannot move root node");

  EntityHierarchy &entEHC = registry_.get_or_emplace<EntityHierarchy>(entityId);
  if (parentId == entt::null) {
    return;
  }
  entEHC.parent = parentId;
  auto &parentEHC = Entity{registry_, parentId}.getComponent<EntityHierarchy>();
  entEHC.nextSibling = parentEHC.firstChild;
  parentEHC.firstChild = entityId;
}

Entity Scene::createEntity(const EntityId parent) {
  const EntityId id = registry_.create();
  moveEntity(id, parent == entt::null ? rootEntity_.id() : parent);
  return Entity{registry_, id};
}

void Scene::destroyEntity(EntityId id) {
  if (registry_.valid(id)) {
    registry_.destroy(id);
  }
}

Scene::~Scene(){
  unload();
}

} // namespace Eden