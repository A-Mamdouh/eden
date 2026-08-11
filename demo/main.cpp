#include <array>
#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <Eden/Eden.hpp>

#include "scripts/PulseTint.hpp"

namespace {

/// Demo's own small mesh library, uploaded via Engine::createMesh() --
/// stand-in for real asset loading until the engine loads glTF models.
struct DemoMeshes {
  Eden::MeshHandle triangle{};
  Eden::MeshHandle quad{};
};

DemoMeshes createDemoMeshes(Eden::Engine &engine) {
  const std::array<Eden::Vertex, 3> triangleVertices{
      Eden::Vertex{Eden::Vec3{0.0f, 0.5f, 0.0f}, Eden::Color{1.0f, 0.0f, 0.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, -0.5f, 0.0f}, Eden::Color{0.0f, 1.0f, 0.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{-0.5f, -0.5f, 0.0f}, Eden::Color{0.0f, 0.0f, 1.0f, 1.0f}},
  };

  const std::array<Eden::Vertex, 6> quadVertices{
      Eden::Vertex{Eden::Vec3{-0.5f, 0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, 0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, -0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{0.5f, -0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{-0.5f, -0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
      Eden::Vertex{Eden::Vec3{-0.5f, 0.5f, 0.0f}, Eden::Color{1.0f, 1.0f, 1.0f, 1.0f}},
  };

  return DemoMeshes{
      .triangle = engine.createMesh(Eden::MeshDesc{triangleVertices}),
      .quad = engine.createMesh(Eden::MeshDesc{quadVertices}),
  };
}

std::unique_ptr<Eden::Scene> buildDemoScene(Eden::Engine &engine, const DemoMeshes &meshes) {
  auto scene = std::make_unique<Eden::Scene>();

  const Eden::MaterialHandle defaultMaterial = engine.createMaterial(Eden::Material{});
  const Eden::MaterialHandle quadMaterial = engine.createMaterial(Eden::Material{
      .tint = Eden::Color{0.3f, 0.6f, 1.0f, 1.0f},
      .useVertexColor = false,
  });

  auto triangle = scene->createEntity();
  triangle.addComponent<Eden::Transform>(Eden::Transform{.position = {-0.6f, 0.0f, 0.0f}});
  triangle.addComponent<Eden::Renderable>(
      Eden::Renderable{.mesh = meshes.triangle, .material = defaultMaterial});

  auto quad = scene->createEntity();
  quad.addComponent<Eden::Transform>(Eden::Transform{.position = {0.6f, 0.0f, 0.0f}});
  quad.addComponent<Eden::Renderable>(Eden::Renderable{.mesh = meshes.quad, .material = quadMaterial});
  quad.addComponent<Eden::ScriptComponent>(Eden::ScriptComponent{.behaviour = std::make_unique<PulseTint>()});

  return scene;
}

} // namespace

int main()
{
    Eden::AppConfig config;
    config.engine.window.title = "Eden Demo";
    config.engine.render.enableValidationLayers = true;
    config.engine.render.targetFrameRate = 144;

    Eden::Engine engine(config);
    const DemoMeshes meshes = createDemoMeshes(engine);
    engine.loadScene(buildDemoScene(engine, meshes));
    // Proves Engine::loadModel() alongside the hand-built entities above --
    // a hand-authored two-node glTF asset (external .bin + checker .png)
    // exercising mesh/material/texture import and node hierarchy.
    engine.loadModel(std::string(EDEN_DEMO_ASSETS_DIR) + "/quad.gltf");

    spdlog::info("Demo application initialized; entering run loop");
    return engine.run();
}
