#include <memory>
#include <spdlog/spdlog.h>
#include <Eden/Eden.hpp>

namespace {

std::unique_ptr<Eden::Scene> buildDemoScene() {
  auto scene = std::make_unique<Eden::Scene>();

  auto triangle = scene->createEntity();
  triangle.addComponent<Eden::Transform>(Eden::Transform{.position = {-0.6f, 0.0f, 0.0f}});
  triangle.addComponent<Eden::Renderable>(Eden::Renderable{.shape = Eden::PrimitiveShape::Triangle});

  auto quad = scene->createEntity();
  quad.addComponent<Eden::Transform>(Eden::Transform{.position = {0.6f, 0.0f, 0.0f}});
  quad.addComponent<Eden::Renderable>(Eden::Renderable{
      .shape = Eden::PrimitiveShape::Quad,
      .tint = Eden::Color{0.3f, 0.6f, 1.0f, 1.0f},
      .useVertexColor = false,
  });

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
    engine.loadScene(buildDemoScene());

    spdlog::info("Demo application initialized; entering run loop");
    return engine.run();
}
