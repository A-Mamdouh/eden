#pragma once

#include <glm/glm.hpp>

/// Engine-wide aliases over glm types, so call sites and headers don't
/// depend on glm directly.
namespace Eden {

/// 2D vector.
using Vec2 = glm::vec2;
/// 3D vector; used for positions in Vertex and DrawCommand's world space.
using Vec3 = glm::vec3;
/// 4D vector.
using Vec4 = glm::vec4;
/// 4x4 matrix; used for transforms and camera view/projection.
using Mat4 = glm::mat4;

} // namespace Eden
