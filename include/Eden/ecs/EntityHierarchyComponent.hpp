#pragma once

#ifndef EDEN_ENGINE_ENTITY_HIERARCHY_COMPONENT_HPP
#define EDEN_ENGINE_ENTITY_HIERARCHY_COMPONENT_HPP

#include "Component.hpp"
#include "Base.hpp"

namespace Eden {
struct EntityHierarchy : public Component {
  EntityId id{entt::null};
  EntityId parent{entt::null};
  EntityId firstChild{entt::null};
  EntityId nextSibling{entt::null};
};
} // namespace Eden

#endif