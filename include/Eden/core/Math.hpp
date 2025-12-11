#pragma once

#ifndef EDEN_CORE_MATH_HPP
#define EDEN_CORE_MATH_HPP

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Eden {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

namespace Math {

inline Mat4 makePerspective(float fovYRadians, float aspect, float nearPlane,
                            float farPlane) noexcept {
  return glm::perspectiveRH_ZO(fovYRadians, aspect, nearPlane, farPlane);
}

inline Mat4 makeLookAt(const Vec3 &eye, const Vec3 &center,
                       const Vec3 &up) noexcept {
  return glm::lookAtRH(eye, center, up);
}

inline Mat4 composeTransform(const Vec3 &position, const Vec3 &rotationEuler,
                             const Vec3 &scale) noexcept {
  Mat4 transform{1.0f};
  transform = glm::translate(transform, position);
  transform = glm::rotate(transform, rotationEuler.z, Vec3{0.0f, 0.0f, 1.0f});
  transform = glm::rotate(transform, rotationEuler.y, Vec3{0.0f, 1.0f, 0.0f});
  transform = glm::rotate(transform, rotationEuler.x, Vec3{1.0f, 0.0f, 0.0f});
  transform = glm::scale(transform, scale);
  return transform;
}

} // namespace Math

} // namespace Eden

#endif // EDEN_CORE_MATH_HPP
