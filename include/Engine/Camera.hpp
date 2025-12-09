//#pragma once

#ifndef EDEN_ENGINE_CAMERA_HPP
#define EDEN_ENGINE_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Eden
{

using Vec3 = glm::vec3;
using Mat4 = glm::mat4;

inline Mat4 makePerspective(float fovYRadians, float aspect, float nearPlane, float farPlane) noexcept
{
    return glm::perspectiveRH_ZO(fovYRadians, aspect, nearPlane, farPlane);
}

inline Mat4 makeLookAt(const Vec3& eye, const Vec3& center, const Vec3& up) noexcept
{
    return glm::lookAtRH(eye, center, up);
}

inline Mat4 composeTransform(
    const Vec3& position,
    const Vec3& rotationEuler,
    const Vec3& scale) noexcept
{
    Mat4 transform{1.0f};
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, rotationEuler.z, Vec3{0.0f, 0.0f, 1.0f});
    transform = glm::rotate(transform, rotationEuler.y, Vec3{0.0f, 1.0f, 0.0f});
    transform = glm::rotate(transform, rotationEuler.x, Vec3{1.0f, 0.0f, 0.0f});
    transform = glm::scale(transform, scale);
    return transform;
}

struct Camera
{
    Mat4 view{Mat4(1.0f)};
    Mat4 projection{Mat4(1.0f)};
};

} // namespace Eden

#endif // EDEN_ENGINE_CAMERA_HPP
