#pragma once

#include "Eden/Services/ClockService/ClockService.hpp"
#include "Eden/Services/ConfigService/Config.hpp"
#include "Eden/Services/EventService/EventService.hpp"

#include <memory>

namespace Eden {
  class Engine {
    public:
    Engine(const Config::ApplicationConfig &appConfig)
    {

    }

    void run() {
      // Initialize systems
      while(running_)
      {
        clockService_->tick();
        // eventService->process(); ??
        while(clockService_->consumeFixedStep())
        {
          // scriptSystem->fixedUpdate(clockService_->fixedDt());
        }
        // renderSystem_->render(clock.frameDt());
      }
    }

    private:
    bool running_{false};
    // Services
    std::unique_ptr<ClockService> clockService_;
    std::unique_ptr<EventService> eventService_;
    // Systems
    // std::unique_ptr<ScriptSystem> scriptSystem_;
  };
}