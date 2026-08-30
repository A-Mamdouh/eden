#include <Eden/Eden.hpp>
#include <Eden/EdenConfig.hpp>

int main() {
  Eden::AppConfig config{};
  config.engine.render.backend = Eden::Config::Rendering::RendererBackend::Null;

  Eden::Engine engine{config};
  engine.stop();
  return 0;
}
