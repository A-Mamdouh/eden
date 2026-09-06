#include "Eden/Services/ClockService/ClockService.hpp"

#include <algorithm>

namespace Eden::Services {

void ClockService::onInit() { lastTick_ = Clock::now(); }

double ClockService::tick() {
  requireInitialized();

  auto now = Clock::now();
  std::chrono::duration<double> dt = now - lastTick_;
  lastTick_ = now;

  double rawDt = dt.count();
  rawDt = std::min(rawDt, clockConfig_.maxFrameDt);

  realTimeTotal_ += rawDt;

  if (!paused_) {
    frameDelta_ = rawDt * clockConfig_.timeScale;
    accumulator_ += frameDelta_;
  } else {
    frameDelta_ = 0.0;
  }

  return frameDelta_;
}

bool ClockService::consumeFixedStep() {
  requireInitialized();

  if (accumulator_ >= clockConfig_.fixedDt) {
    accumulator_ -= clockConfig_.fixedDt;
    simTimeTotal_ += clockConfig_.fixedDt;
    return true;
  }
  return false;
}

double ClockService::fixedDt() const { return clockConfig_.fixedDt; }

double ClockService::frameDt() const { return frameDelta_; }

double ClockService::simTime() const { return simTimeTotal_; }

double ClockService::realTime() const { return realTimeTotal_; }

void ClockService::setPaused(bool paused) { paused_ = paused; }

bool ClockService::isPaused() const { return paused_; }

void ClockService::setTimeScale(double scale) { clockConfig_.timeScale = scale; }
} // namespace Eden::Services
