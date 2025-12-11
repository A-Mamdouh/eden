#pragma once

#ifndef EDEN_ENGINE_TRANSFORM_COMPONENT_HPP
#define EDEN_ENGINE_TRANSFORM_COMPONENT_HPP

#include "Component.hpp"
#include "Eden/core/Math.hpp"

namespace Eden {
struct Transform : public Component {
  Vec3 position{0.0f, 0.0f, 0.0f};
  Vec3 rotationEuler{0.0f, 0.0f, 0.0f};
  Vec3 scale{1.0f, 1.0f, 1.0f};
};
} // namespace Eden

#endif