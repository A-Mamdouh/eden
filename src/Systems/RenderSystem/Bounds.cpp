#include "Eden/Systems/RenderSystem/Bounds.hpp"

namespace Eden {

AABB computeBounds(std::span<const Vertex> vertices) {
  if (vertices.empty()) {
    return AABB{};
  }

  Vec3 min = vertices[0].position;
  Vec3 max = vertices[0].position;
  for (const Vertex &vertex : vertices) {
    min = glm::min(min, vertex.position);
    max = glm::max(max, vertex.position);
  }

  return AABB{min, max};
}

} // namespace Eden
