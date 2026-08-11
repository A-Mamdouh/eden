#pragma once

#include "Eden/Core/Math.hpp"
#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

#include <span>

namespace Eden {

/// Axis-aligned bounding box in a mesh's local object space.
struct AABB {
  Vec3 min{0.0f};
  Vec3 max{0.0f};
};

/// @param vertices A mesh's vertex data, as passed to Renderer::createMesh().
/// @return The AABB enclosing every vertex position; {0,0,0}-{0,0,0} if
///         `vertices` is empty.
AABB computeBounds(std::span<const Vertex> vertices);

} // namespace Eden
