#pragma once

#include "Eden/Services/IService.hpp"
#include "Eden/Services/ConfigService/Config.hpp"

#include <chrono>

namespace Eden::Services {

/// Real-time clock with an optional fixed-timestep accumulator for
/// simulation stepping (consumeFixedStep()), independent of frame rate.
class ClockService : public IService {

public:
  /// @param clockConfig Fixed-step size, frame-delta clamp, and initial
  ///        time scale; defaults to 60Hz simulation, uncapped real time.
  explicit ClockService(Config::Clock::ClockConfig clockConfig = {}) : clockConfig_{clockConfig} {}
  std::string getName() override { return "Clock Service"; }

    /// Called once per Engine::run() iteration.
    /// @return Frame delta time in seconds: real elapsed time clamped to
    ///         clockConfig_.maxFrameDt and scaled by setTimeScale(), or 0
    ///         while paused.
    double tick();

    /// True if the fixed-timestep accumulator has at least one
    /// clockConfig_.fixedDt worth of time banked; consumes it if so.
    bool consumeFixedStep();
    /// @return The configured fixed simulation step, in seconds.
    double fixedDt() const;

    /// @return The most recent tick()'s return value.
    double frameDt() const;
    /// @return Accumulated time consumed by consumeFixedStep(), in
    ///         seconds; advances in fixedDt() increments and only while
    ///         something calls consumeFixedStep() each frame.
    double simTime() const;
    /// @return Total real (wall-clock) time elapsed since onInit(), in
    ///         seconds -- unaffected by pause/time scale.
    double realTime() const;

    /// @param paused If true, tick() returns 0 and the fixed-step
    ///        accumulator stops growing; realTime() keeps advancing.
    void setPaused(bool paused);
    /// @return The value last passed to setPaused(); false initially.
    bool isPaused() const;
    /// @param scale Multiplier applied to simulation time; see
    ///        Config::Clock::ClockConfig::timeScale.
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
  Config::Clock::ClockConfig clockConfig_;
};

} // namespace Eden::Services
