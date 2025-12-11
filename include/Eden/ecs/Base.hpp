#pragma once
#ifndef EDEN_ENGINE_ECS_BASE_HPP
#define EDEN_ENGINE_ECS_BASE_HPP
#ifndef ENTT_DISABLE_ASSERT
#define ENTT_DISABLE_ASSERT
#endif
#include <entt/entt.hpp>

namespace Eden {
    using EntityId = entt::entity;
using Registry = entt::registry;
}

#endif
