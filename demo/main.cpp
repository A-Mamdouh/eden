#include <memory>
#include <spdlog/spdlog.h>
#include <Eden/Eden.hpp>
#include "scenes/demo.hpp"

class DemoApplication : public Eden::Application
{
public:
    using Eden::Application::Application;

protected:
    std::unique_ptr<Eden::Scene> createInitialScene() override
    {
        return std::make_unique<Eden::DemoScene>();
    }
};

int main()
{
    Eden::AppConfig config;
    config.engine.window.title = "Eden Demo";
    config.engine.render.enableValidationLayers = true;
    config.engine.render.targetFrameRate = 144;

    DemoApplication app(config);

    spdlog::info("Demo application initialized; entering run loop");
    return app.run();
}
