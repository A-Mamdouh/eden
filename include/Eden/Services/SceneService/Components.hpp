#pragma once

#include "Eden/Core/Math.hpp"

#include <entt/entt.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace Eden {

/// Local-space transform. Composed into a matrix via localMatrix();
/// world-space placement is computed separately by TransformSystem,
/// which accounts for EntityHierarchy.
struct Transform {
  /// Local-space (parent-relative if EntityHierarchy is present) position.
  Vec3 position{0.0f};
  /// Euler angles in degrees, applied X then Y then Z.
  Vec3 rotationEuler{0.0f};
  /// Per-axis scale; {1,1,1} is unscaled.
  Vec3 scale{1.0f, 1.0f, 1.0f};

  /// @return This transform's local matrix: translate * rotate * scale.
  Mat4 localMatrix() const {
    Mat4 m = glm::translate(Mat4(1.0f), position);
    m = glm::rotate(m, glm::radians(rotationEuler.x), Vec3{1.0f, 0.0f, 0.0f});
    m = glm::rotate(m, glm::radians(rotationEuler.y), Vec3{0.0f, 1.0f, 0.0f});
    m = glm::rotate(m, glm::radians(rotationEuler.z), Vec3{0.0f, 0.0f, 1.0f});
    m = glm::scale(m, scale);
    return m;
  }
};

/// Optional parent link. Entities without this component (or with
/// parent == entt::null) are roots.
struct EntityHierarchy {
  /// entt::null for a root entity.
  entt::entity parent{entt::null};
};

/// World-space matrix computed by TransformSystem from Transform and the
/// EntityHierarchy chain; consumers (RenderSystem, ...) only ever read
/// this, never Transform directly, so they don't need to know about
/// hierarchy at all.
struct WorldTransform {
  /// Local-to-world matrix, already accounting for the parent chain.
  Mat4 matrix{1.0f};
};

} // namespace Eden
