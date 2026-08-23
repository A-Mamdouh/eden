#pragma once

#include <Eden/Eden.hpp>

namespace Demo {

/// Demo's own small mesh library, uploaded via Engine::createMesh() --
/// stand-in for real asset loading for the hand-built parts of the scene
/// (walls, pillars, floor); loadModel() supplies the rest.
struct DemoMeshes {
  Eden::Rendering::MeshHandle quad{};
  Eden::Rendering::MeshHandle cube{};
};

DemoMeshes createDemoMeshes(Eden::Engine &engine);

} // namespace Demo
