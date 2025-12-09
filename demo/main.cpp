#include <memory>
#include <spdlog/spdlog.h>
#include <Engine/Eden.hpp>
#include "scenes/demo.cpp"

class DemoApplication : public Eden::Application
{
public:
    using Eden::Application::Application;

protected:
    std::unique_ptr<Eden::Scene> createInitialScene() override
    {
        return std::make_unique<DemoScene>(getEngine().getInput());
    }
};

int main()
{
    Eden::EngineConfig config;
    config.title = "Eden Demo";
    DemoApplication app(config);

    spdlog::info("Demo application initialized; entering run loop");
    return app.run();
}
