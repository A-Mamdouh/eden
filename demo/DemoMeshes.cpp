#include "DemoMeshes.hpp"

#include <array>
#include <cstdint>

namespace Demo {

DemoMeshes createDemoMeshes(Eden::Engine &engine) {
  const std::array<Eden::Vertex, 6> quadVertices{
      Eden::Vertex{Eden::Vec3{-0.5f, 0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, 0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, -0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, -0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{-0.5f, -0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{-0.5f, 0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
  };

  // Unit cube, -0.5..0.5 on every axis; indexed so walls/pillars/floor can
  // all reuse one upload, scaled per-instance via Transform::scale.
  const std::array<Eden::Vertex, 8> cubeVertices{
      Eden::Vertex{Eden::Vec3{-0.5f, -0.5f, -0.5f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, -0.5f, -0.5f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, 0.5f, -0.5f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{-0.5f, 0.5f, -0.5f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{-0.5f, -0.5f, 0.5f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, -0.5f, 0.5f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, 0.5f, 0.5f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{-0.5f, 0.5f, 0.5f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
  };
  const std::array<std::uint32_t, 36> cubeIndices{
      0, 1, 2, 0, 2, 3, // back
      5, 4, 7, 5, 7, 6, // front
      4, 0, 3, 4, 3, 7, // left
      1, 5, 6, 1, 6, 2, // right
      4, 5, 1, 4, 1, 0, // bottom
      3, 2, 6, 3, 6, 7, // top
  };

  return DemoMeshes{
      .quad = engine.createMesh(Eden::MeshDesc{quadVertices}),
      .cube = engine.createMesh(Eden::MeshDesc{cubeVertices, cubeIndices}),
  };
}

} // namespace Demo
