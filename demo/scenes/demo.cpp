#include "scenes/demo.hpp"

#include "Eden/core/Math.hpp"
#include "Eden/ecs/CameraComponent.hpp"
#include "Eden/ecs/RenderableComponent.hpp"
#include "Eden/ecs/TransformComponent.hpp"
#include "Eden/script/Script.hpp"

#include "scripts/PlayerInput.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Eden {

void DemoScene::load() {
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

  bool first = true;

  for (const auto &quad : quads) {
    auto entity = createEntity();
    auto &transform = entity.addComponent<Transform>();
    transform.position = quad.position;
    transform.scale = quad.scale;

    auto &renderable = entity.addComponent<Renderable>();
    renderable.shape = PrimitiveShape::Quad;
    renderable.material.baseColor = quad.color;

    if(first) {
      auto  &script = entity.addComponent<ScriptComponent>();
      script.behaviour = std::make_unique<PlayerInput>();
      first = false;
    }
  }
  
}

} // namespace Eden
