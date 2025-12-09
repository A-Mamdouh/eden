#include <memory>
#include "Engine/Eden.hpp"
#include "Engine/Log.hpp"
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
    DemoApplication app(config);

    EDEN_CORE_INFO("Demo application initialized; entering run loop");
    return app.run();
}
