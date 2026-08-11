#include <Eden/Services/SceneService/Components.hpp>
#include <Eden/Systems/RenderSystem/Frustum.hpp>

#include <gtest/gtest.h>

namespace {

// Camera at (0, 0, 5) looking down -Z at the origin, 60-degree vertical
// FOV, square aspect ratio -- matches how RenderSystem builds a Frustum
// from CameraDesc::projection * CameraDesc::view each frame.
Eden::Frustum makeTestFrustum() {
  const Eden::Camera camera{.position = {0.0f, 0.0f, 5.0f}, .target = {0.0f, 0.0f, 0.0f}};
  return Eden::Frustum{camera.projectionMatrix(1.0f) * camera.viewMatrix()};
}

Eden::AABB unitBoxAt(const Eden::Vec3 &center) {
  return Eden::AABB{center - Eden::Vec3(0.5f), center + Eden::Vec3(0.5f)};
}

} // namespace

TEST(FrustumTest, ObjectAtTargetIsVisible) {
  const Eden::Frustum frustum = makeTestFrustum();

  EXPECT_TRUE(frustum.intersects(unitBoxAt({0.0f, 0.0f, 0.0f}), Eden::Mat4{1.0f}));
}

TEST(FrustumTest, ObjectFarOffToTheSideIsCulled) {
  const Eden::Frustum frustum = makeTestFrustum();

  // Same depth as the origin, but far outside the ~2.9-unit half-width
  // the 60-degree FOV allows at that distance.
  EXPECT_FALSE(frustum.intersects(unitBoxAt({1000.0f, 0.0f, 0.0f}), Eden::Mat4{1.0f}));
}

TEST(FrustumTest, ObjectBehindTheCameraIsCulled) {
  const Eden::Frustum frustum = makeTestFrustum();

  // Camera looks toward -Z from Z=5; Z=20 is behind the eye.
  EXPECT_FALSE(frustum.intersects(unitBoxAt({0.0f, 0.0f, 20.0f}), Eden::Mat4{1.0f}));
}

TEST(FrustumTest, ObjectBeyondTheFarPlaneIsCulled) {
  const Eden::Frustum frustum = makeTestFrustum();

  // Far plane defaults to 100 units in front of the camera, i.e. Z = -95.
  EXPECT_FALSE(frustum.intersects(unitBoxAt({0.0f, 0.0f, -195.0f}), Eden::Mat4{1.0f}));
}

TEST(FrustumTest, ObjectInsideTheNearPlaneIsCulled) {
  const Eden::Frustum frustum = makeTestFrustum();

  // Near plane defaults to 0.1 units in front of the camera (Z = 4.9) and
  // the eye itself is at Z = 5 -- that's only a 0.1-unit gap, so this box
  // is sized to fit entirely inside it rather than straddling either edge.
  const Eden::AABB nearlyAtTheEye{{-0.01f, -0.01f, 4.91f}, {0.01f, 0.01f, 4.99f}};
  EXPECT_FALSE(frustum.intersects(nearlyAtTheEye, Eden::Mat4{1.0f}));
}

TEST(FrustumTest, ObjectStraddlingThePlaneBoundaryIsVisible) {
  const Eden::Frustum frustum = makeTestFrustum();

  // Spans Z = -90 to Z = -110, straddling the far plane at Z = -95:
  // partially inside, so this must not be culled.
  const Eden::AABB straddling{{-0.5f, -0.5f, -110.0f}, {0.5f, 0.5f, -90.0f}};
  EXPECT_TRUE(frustum.intersects(straddling, Eden::Mat4{1.0f}));
}

TEST(FrustumTest, TransformIsAppliedBeforeTheVisibilityTest) {
  const Eden::Frustum frustum = makeTestFrustum();

  // Local-space box sits at object-space X=10, well outside the frustum
  // on its own; a translation bringing it back to world-space X=0 must
  // make it visible again.
  const Eden::AABB localBox = unitBoxAt({10.0f, 0.0f, 0.0f});
  const Eden::Mat4 recenter = glm::translate(Eden::Mat4{1.0f}, Eden::Vec3{-10.0f, 0.0f, 0.0f});

  EXPECT_FALSE(frustum.intersects(localBox, Eden::Mat4{1.0f}));
  EXPECT_TRUE(frustum.intersects(localBox, recenter));
}
