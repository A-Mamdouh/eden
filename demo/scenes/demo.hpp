#pragma once

#include "Eden/scene/Scene.hpp"
#include "Eden/core/Math.hpp"
#include "Eden/ecs/CameraComponent.hpp"
#include "Eden/ecs/RenderableComponent.hpp"
#include "Eden/ecs/TransformComponent.hpp"
#include "Eden/script/Script.hpp"

#include "scripts/PlayerInput.hpp"
#include "scripts/GameControls.hpp"


namespace Eden {

class DemoScene final : public Scene {
public:
  void load() override {
  // Set up a simple orthographic camera looking at the origin.
  auto cameraEntity = createEntity();
  auto &camera = cameraEntity.addComponent<Camera>();
  camera.active = true;
  camera.view = glm::lookAtRH(Vec3{0.0f, 0.0f, 5.0f}, Vec3{0.0f, 0.0f, 0.0f},
                              Vec3{0.0f, 1.0f, 0.0f});
  camera.projection =
      glm::orthoRH_ZO(-2.0f, 2.0f, -1.5f, 1.5f, 0.1f, 10.0f);

  // Create a few colored quads at different positions.
  const struct {
    Vec3 position;
    Vec3 scale;
    Color color;
  } quads[] = {
      {Vec3{-1.0f, 1.0f, 0.0f}, Vec3{0.6f, 0.6f, 1.0f},
       Color{1.0f, 0.2f, 0.2f, 1.0f}},
      {Vec3{0.0f, 0.5f, 0.0f}, Vec3{0.8f, 0.4f, 1.0f},
       Color{0.2f, 0.8f, 0.3f, 1.0f}},
      {Vec3{1.0f, -0.2f, 0.0f}, Vec3{0.5f, 0.9f, 1.0f},
       Color{0.2f, 0.4f, 1.0f, 1.0f}},
  };

  for (int i = 0; i < 3; ++i) {
    const auto& quad = quads[i];
    auto entity = createEntity();
    auto &transform = entity.addComponent<Transform>();
    transform.position = quad.position;
    transform.scale = quad.scale;

    auto &renderable = entity.addComponent<Renderable>();
    renderable.shape = PrimitiveShape::Quad;
    renderable.material.baseColor = quad.color;

    if(i == 0) {
      auto  &script1 = entity.addComponent<ScriptComponent>();
      script1.behaviour = std::make_unique<PlayerInput>();
    }
    if(i == 1) {
      auto  &script2 = entity.addComponent<ScriptComponent>();
      script2.behaviour = std::make_unique<GameControls>();
    }
  }
  
}
};

} // namespace Eden
