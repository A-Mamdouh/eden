#include <memory>
#include <spdlog/spdlog.h>
#include <Eden/Eden.hpp>

int main()
{
    Eden::AppConfig config;
    config.engine.window.title = "Eden Demo";
    config.engine.render.enableValidationLayers = true;
    config.engine.render.targetFrameRate = 144;

    Eden::Engine engine(config);
    spdlog::info("Demo application initialized; entering run loop");
    return engine.run();
}
