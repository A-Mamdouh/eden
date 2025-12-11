#pragma once

#ifndef EDEN_ENGINE_RENDERABLE_COMPONENT_HPP
#define EDEN_ENGINE_RENDERABLE_COMPONENT_HPP

#include "Component.hpp"
#include "Eden/render/Material.hpp"

namespace Eden {

struct Renderable : public Component {
  Material material{};
  PrimitiveShape shape{PrimitiveShape::Triangle};
};
} // namespace Eden

#endif