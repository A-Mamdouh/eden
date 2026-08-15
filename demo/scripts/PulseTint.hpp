#pragma once

#include <Eden/Eden.hpp>

#include <cmath>

/// Animates its entity's color through a red/blue pulse over time, via a
/// TintOverride -- added on first update if the entity doesn't already
/// have one -- rather than mutating the entity's (possibly shared)
/// Material.
class PulseTint : public Eden::ScriptBehaviour {
public:
  void onUpdate(Eden::Entity entity, double dt, Eden::InputState & /*input*/) override {
    elapsedTime_ += dt;
    const float pulse = static_cast<float>(0.5 + 0.5 * std::sin(elapsedTime_));
    const Eden::Color color{pulse, 0.3f, 1.0f - pulse, 1.0f};

    if (entity.hasComponent<Eden::TintOverride>()) {
      entity.getComponent<Eden::TintOverride>().tint = color;
    } else {
      entity.addComponent<Eden::TintOverride>(Eden::TintOverride{.tint = color});
    }
  }

private:
  double elapsedTime_{0.0};
};
