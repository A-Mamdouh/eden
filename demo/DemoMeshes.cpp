#include "DemoMeshes.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>

namespace Demo {

using namespace Eden::Rendering;

DemoMeshes createDemoMeshes(Eden::Engine &engine) {
  // Lies flat in its local XY plane; local +Z is the front face's normal.
  constexpr Eden::Vec3 quadNormal{0.0f, 0.0f, 1.0f};
  const std::array<Vertex, 6> quadVertices{
      Vertex{.position = {-0.5f, 0.5f, 0.0f}, .normal = quadNormal, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {0.5f, 0.5f, 0.0f}, .normal = quadNormal, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {0.5f, -0.5f, 0.0f}, .normal = quadNormal, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {0.5f, -0.5f, 0.0f}, .normal = quadNormal, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-0.5f, -0.5f, 0.0f}, .normal = quadNormal, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-0.5f, 0.5f, 0.0f}, .normal = quadNormal, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
  };

  // Unit cube, -0.5..0.5 on every axis, scaled per-instance via
  // Transform::scale. 24 vertices (4 per face) rather than 8 shared
  // corners: a shared corner touches 3 differently-oriented faces, so it
  // can't carry one flat per-face normal -- each face needs its own copy.
  constexpr float h = 0.5f;
  const std::array<Vertex, 24> cubeVertices{
      // back (z=-h)
      Vertex{.position = {-h, -h, -h}, .normal = {0.0f, 0.0f, -1.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, -h, -h}, .normal = {0.0f, 0.0f, -1.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, h, -h}, .normal = {0.0f, 0.0f, -1.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-h, h, -h}, .normal = {0.0f, 0.0f, -1.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      // front (z=+h)
      Vertex{.position = {h, -h, h}, .normal = {0.0f, 0.0f, 1.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-h, -h, h}, .normal = {0.0f, 0.0f, 1.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-h, h, h}, .normal = {0.0f, 0.0f, 1.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, h, h}, .normal = {0.0f, 0.0f, 1.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      // left (x=-h)
      Vertex{.position = {-h, -h, h}, .normal = {-1.0f, 0.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-h, -h, -h}, .normal = {-1.0f, 0.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-h, h, -h}, .normal = {-1.0f, 0.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-h, h, h}, .normal = {-1.0f, 0.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      // right (x=+h)
      Vertex{.position = {h, -h, -h}, .normal = {1.0f, 0.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, -h, h}, .normal = {1.0f, 0.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, h, h}, .normal = {1.0f, 0.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, h, -h}, .normal = {1.0f, 0.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      // bottom (y=-h)
      Vertex{.position = {-h, -h, h}, .normal = {0.0f, -1.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, -h, h}, .normal = {0.0f, -1.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, -h, -h}, .normal = {0.0f, -1.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-h, -h, -h}, .normal = {0.0f, -1.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      // top (y=+h)
      Vertex{.position = {-h, h, -h}, .normal = {0.0f, 1.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, h, -h}, .normal = {0.0f, 1.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {h, h, h}, .normal = {0.0f, 1.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Vertex{.position = {-h, h, h}, .normal = {0.0f, 1.0f, 0.0f}, .color = Color{1.0f, 1.0f, 1.0f, 1.0f}},
  };

  // Same "0,1,2,0,2,3" diagonal split per face as the old 8-vertex cube,
  // just re-based onto each face's own 4-vertex group.
  std::array<std::uint32_t, 36> cubeIndices{};
  for (std::uint32_t face = 0; face < 6; ++face) {
    const std::uint32_t base = face * 4;
    const std::uint32_t triangles[6] = {base, base + 1, base + 2, base, base + 2, base + 3};
    std::copy(std::begin(triangles), std::end(triangles), cubeIndices.begin() + face * 6);
  }

  return DemoMeshes{
      .quad = engine.createMesh(MeshDesc{quadVertices}),
      .cube = engine.createMesh(MeshDesc{cubeVertices, cubeIndices}),
  };
}

} // namespace Demo
