#pragma once

#include "Eden/Core/Math.hpp"
#include "Eden/Systems/RenderSystem/Bounds.hpp"

#include <array>

namespace Eden {

/// A camera's view frustum, as six inward-facing planes extracted from a
/// combined view-projection matrix. Used to skip draw commands for
/// geometry that can't be visible this frame -- RenderSystem builds one
/// per frame from CameraDesc::view * CameraDesc::projection.
class Frustum {
public:
  /// @param viewProjection Combined `projection * view` matrix, under
  ///        Eden's Vulkan-style [0, 1] NDC depth convention
  ///        (GLM_FORCE_DEPTH_ZERO_TO_ONE) -- every projection matrix built
  ///        via Camera::projectionMatrix() already satisfies this.
  explicit Frustum(const Mat4 &viewProjection);

  /// @param box Bounding box in some local space.
  /// @param transform That local space's transform to world space.
  /// @return False only if `box` (transformed to world space, then
  ///         re-enclosed in an axis-aligned box) is entirely on the outer
  ///         side of some plane -- true for both partially- and
  ///         fully-inside cases, so this never culls something that's
  ///         actually visible, only what's fully off-screen.
  bool intersects(const AABB &box, const Mat4 &transform) const;

private:
  /// left, right, bottom, top, near, far; each Vec4(normal, distance)
  /// with the normal pointing into the frustum.
  std::array<Vec4, 6> planes_{};
};

} // namespace Eden
