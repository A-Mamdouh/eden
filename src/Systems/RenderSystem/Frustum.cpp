#include "Eden/Systems/RenderSystem/Frustum.hpp"

#include <limits>

namespace Eden {
namespace {

/// @return Row `row` of `m`, gathered across glm's column-major storage.
Vec4 matrixRow(const Mat4 &m, int row) { return Vec4{m[0][row], m[1][row], m[2][row], m[3][row]}; }

Vec4 normalizePlane(const Vec4 &plane) {
  const float length = glm::length(Vec3{plane});
  return length > 0.0f ? plane / length : plane;
}

/// @return An axis-aligned box in world space that fully encloses `box`
///         (transformed by `transform`) -- exact only if `transform` has
///         no rotation, conservative (never smaller than the true bounds)
///         otherwise.
AABB worldSpaceBounds(const AABB &box, const Mat4 &transform) {
  Vec3 worldMin{std::numeric_limits<float>::max()};
  Vec3 worldMax{std::numeric_limits<float>::lowest()};

  for (int i = 0; i < 8; ++i) {
    const Vec3 corner{
        (i & 1) ? box.max.x : box.min.x,
        (i & 2) ? box.max.y : box.min.y,
        (i & 4) ? box.max.z : box.min.z,
    };
    const Vec3 worldCorner = Vec3{transform * Vec4{corner, 1.0f}};
    worldMin = glm::min(worldMin, worldCorner);
    worldMax = glm::max(worldMax, worldCorner);
  }

  return AABB{worldMin, worldMax};
}

} // namespace

Frustum::Frustum(const Mat4 &viewProjection) {
  const Vec4 row0 = matrixRow(viewProjection, 0);
  const Vec4 row1 = matrixRow(viewProjection, 1);
  const Vec4 row2 = matrixRow(viewProjection, 2);
  const Vec4 row3 = matrixRow(viewProjection, 3);

  planes_[0] = normalizePlane(row3 + row0); // left
  planes_[1] = normalizePlane(row3 - row0); // right
  planes_[2] = normalizePlane(row3 + row1); // bottom
  planes_[3] = normalizePlane(row3 - row1); // top
  // Near/far follow Eden's [0, 1] NDC depth convention
  // (GLM_FORCE_DEPTH_ZERO_TO_ONE, see src/CMakeLists.txt): near is
  // clip.z >= 0, i.e. row2 itself, not row3 + row2 as in the classic
  // OpenGL [-1, 1] derivation.
  planes_[4] = normalizePlane(row2);        // near
  planes_[5] = normalizePlane(row3 - row2); // far
}

bool Frustum::intersects(const AABB &box, const Mat4 &transform) const {
  const AABB worldBox = worldSpaceBounds(box, transform);

  for (const Vec4 &plane : planes_) {
    const Vec3 positiveVertex{
        plane.x >= 0.0f ? worldBox.max.x : worldBox.min.x,
        plane.y >= 0.0f ? worldBox.max.y : worldBox.min.y,
        plane.z >= 0.0f ? worldBox.max.z : worldBox.min.z,
    };
    if (glm::dot(Vec3{plane}, positiveVertex) + plane.w < 0.0f) {
      return false;
    }
  }

  return true;
}

} // namespace Eden
