#include "DemoMeshes.hpp"
#include "DemoScene.hpp"

#include <spdlog/spdlog.h>
#include <Eden/Eden.hpp>

#include <string>

int main()
{
    Eden::AppConfig config;
    config.engine.render.window.title = "Eden Demo";
    config.engine.render.enableValidationLayers = true;
    config.engine.render.display.targetFrameRate = 60;
    config.engine.render.display.vsync = Eden::VsyncMode::On;
    config.engine.render.display.height = 1080;
    config.engine.render.display.width = 1920;
    config.engine.render.display.screenMode = Eden::Config::ScreenMode::Windowed;
    config.engine.render.graphics.antiAliasing = Eden::AntiAliasing::MSAA4x;

    Eden::Engine engine(config);
    const Demo::DemoMeshes meshes = Demo::createDemoMeshes(engine);
    Eden::Model signModel = engine.loadModel(std::string(EDEN_DEMO_ASSETS_DIR) + "/quad.gltf");
    engine.loadScene(Demo::buildDemoScene(engine, meshes, std::move(signModel)));

    spdlog::info("Demo application initialized; entering run loop");
    return engine.run();
}
