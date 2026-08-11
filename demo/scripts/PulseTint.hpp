#pragma once

#include <Eden/Eden.hpp>

#include <cmath>

/// Animates its entity's Renderable tint through a red/blue pulse over
/// time. Assumes the entity already has a Renderable with
/// useVertexColor = false -- see Renderable::tint.
class PulseTint : public Eden::ScriptBehaviour {
public:
  void onUpdate(Eden::Entity entity, double dt) override {
    elapsedTime_ += dt;
    const float pulse = static_cast<float>(0.5 + 0.5 * std::sin(elapsedTime_));
    entity.getComponent<Eden::Renderable>().tint = Eden::Color{pulse, 0.3f, 1.0f - pulse, 1.0f};
  }

private:
  double elapsedTime_{0.0};
};
