#pragma once

#include "Eden/Services/IService.hpp"
#include "Eden/Services/ConfigService/Config.hpp"

#include <chrono>

namespace Eden {

class ClockService : public IService {

public:
  explicit ClockService(Config::ClockConfig clockConfig = {}) : clockConfig_{clockConfig} {}
  std::string getName() override { return "Clock Service"; }

      // Called once per engine loop
    double tick();   // returns frame dt (real time, clamped)

    // Simulation stepping
    bool consumeFixedStep(); // true if one fixed step available
    double fixedDt() const;

    // Query
    double frameDt() const;
    double simTime() const;
    double realTime() const;

    // Control
    void setPaused(bool paused);
    bool isPaused() const;
    void setTimeScale(double scale);

protected:
  void onInit() override;

private:
  using Clock = std::chrono::steady_clock;
  Clock::time_point lastTick_;

  double frameDelta_{0.0f};
  double accumulator_{0.0f};
  double simTimeTotal_ = {0.0f};
  double realTimeTotal_ = {0.0f};
  bool paused_{false};
  Config::ClockConfig clockConfig_;
};

} // namespace Eden
