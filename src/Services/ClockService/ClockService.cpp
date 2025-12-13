#include "Eden/Services/ClockService/ClockService.hpp"

namespace Eden {

double ClockService::tick() {
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
  if (accumulator_ >= clockConfig_.fixedDt) {
    accumulator_ -= clockConfig_.fixedDt;
    simTimeTotal_ += clockConfig_.fixedDt;
    return true;
  }
  return false;
}
} // namespace Eden