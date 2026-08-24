#pragma once

#include "Eden/Core/Math.hpp"
#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

#include <entt/entt.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace Eden::World {

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

} // namespace Eden::World

namespace Eden::Rendering::Components {

/// Placement and lens parameters for viewing the scene -- pure data, no
/// notion of being "the" camera. RenderSystem renders from whichever
/// entity is selected via RenderSystem::setActiveCamera(); a Camera
/// component on an entity that isn't selected has no effect on its own,
/// which is what lets several coexist (e.g. first-person/third-person
/// views as sibling entities under a player, or one per viewport for
/// split-screen later) without any of them needing to know about the
/// others. Deliberately standalone (not driven by Transform/WorldTransform)
/// for now -- a free-fly camera will likely reshape this (yaw/pitch or a
/// forward vector instead of a fixed target), so it isn't worth coupling
/// to the hierarchy system yet.
struct Camera {
  Vec3 position{0.0f, 0.0f, 3.0f};
  /// World-space point the camera looks toward.
  Vec3 target{0.0f, 0.0f, 0.0f};
  Vec3 up{0.0f, 1.0f, 0.0f};
  /// Vertical field of view, in degrees.
  float fovDegrees{60.0f};
  float nearPlane{0.1f};
  float farPlane{100.0f};

  /// @return World-to-view matrix looking from position toward target.
  Mat4 viewMatrix() const { return glm::lookAt(position, target, up); }

  /// @param aspectRatio Viewport width / height.
  /// @return View-to-clip perspective projection matrix.
  Mat4 projectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fovDegrees), aspectRatio, nearPlane, farPlane);
  }
};

/// Punctual light, placed via the entity's WorldTransform -- unlike
/// Camera, a light has no reason to need its own position/orientation
/// fields when TransformSystem/EntityHierarchy already gives scripts and
/// parenting for free. RenderSystem reads position/direction out of
/// WorldTransform::matrix directly (translation column for position, the
/// local -Z axis for direction), matching glTF's KHR_lights_punctual
/// convention.
struct Light {
  Rendering::LightType type{Rendering::LightType::Directional};
  /// Linear color; not gamma-corrected, same convention as Color.
  Vec3 color{1.0f, 1.0f, 1.0f};
  /// Radiometric-ish scale, not physically calibrated -- tune by eye.
  float intensity{1.0f};
  /// Point only: distance at which attenuation reaches zero. 0 = no cutoff.
  float range{0.0f};
};

} // namespace Eden::Rendering::Components
