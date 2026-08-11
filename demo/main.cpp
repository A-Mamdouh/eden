#include "DemoMeshes.hpp"
#include "DemoScene.hpp"

#include <spdlog/spdlog.h>
#include <Eden/Eden.hpp>

#include <string>

int main()
{
    Eden::AppConfig config;
    config.engine.window.title = "Eden Demo";
    config.engine.render.enableValidationLayers = true;
    config.engine.render.targetFrameRate = 144;

    Eden::Engine engine(config);
    const Demo::DemoMeshes meshes = Demo::createDemoMeshes(engine);
    Eden::Model signModel = engine.loadModel(std::string(EDEN_DEMO_ASSETS_DIR) + "/quad.gltf");
    engine.loadScene(Demo::buildDemoScene(engine, meshes, std::move(signModel)));

    spdlog::info("Demo application initialized; entering run loop");
    return engine.run();
}
