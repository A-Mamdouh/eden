#pragma once

#include <Eden/Eden.hpp>

#include <algorithm>
#include <cmath>

/// WASD + mouselook free-fly camera, driven by whatever Camera component
/// is on its own entity. Yaw/pitch live here, not on Camera -- Camera
/// stays position/target/up so RenderSystem doesn't need to know this
/// script exists; it just recomputes target each frame from position
/// plus a forward vector derived from yaw/pitch.
class FreeFlyCamera : public Eden::Scripting::ScriptBehaviour {
public:
  void onStart(Eden::Input::InputState &input) override { input.setMouseCaptured(true); }

  void onUpdate(double dt, Eden::Input::InputState &input) override {
    if (input.isKeyPressed(Eden::Input::Key::Escape)) {
      input.setMouseCaptured(!input.mouseCaptured());
    }

    if (input.mouseCaptured()) {
      const Eden::Vec2 mouseDelta = input.mouseDelta();
      yawDegrees_ += mouseDelta.x * kMouseSensitivity;
      pitchDegrees_ = std::clamp(pitchDegrees_ - mouseDelta.y * kMouseSensitivity, -89.0f, 89.0f);
    }

    const float yaw = glm::radians(yawDegrees_);
    const float pitch = glm::radians(pitchDegrees_);
    const Eden::Vec3 forward = glm::normalize(
        Eden::Vec3{std::cos(pitch) * std::cos(yaw), std::sin(pitch), std::cos(pitch) * std::sin(yaw)});
    const Eden::Vec3 worldUp{0.0f, 1.0f, 0.0f};
    const Eden::Vec3 right = glm::normalize(glm::cross(forward, worldUp));

    Eden::Vec3 movement{0.0f};
    if (input.isKeyDown(Eden::Input::Key::W)) {
      movement += forward;
    }
    if (input.isKeyDown(Eden::Input::Key::S)) {
      movement -= forward;
    }
    if (input.isKeyDown(Eden::Input::Key::D)) {
      movement += right;
    }
    if (input.isKeyDown(Eden::Input::Key::A)) {
      movement -= right;
    }
    if (input.isKeyDown(Eden::Input::Key::Space)) {
      movement += worldUp;
    }
    if (input.isKeyDown(Eden::Input::Key::LeftControl)) {
      movement -= worldUp;
    }

    auto &camera = entity().getComponent<Eden::Rendering::Components::Camera>();
    if (glm::length(movement) > 0.0f) {
      const float speed = input.isKeyDown(Eden::Input::Key::LeftShift) ? kFastSpeed : kSpeed;
      camera.position += glm::normalize(movement) * speed * static_cast<float>(dt);
    }
    camera.target = camera.position + forward;
    camera.up = worldUp;
  }

private:
  static constexpr float kMouseSensitivity = 0.15f;
  static constexpr float kSpeed = 2.5f;
  static constexpr float kFastSpeed = 6.0f;

  float yawDegrees_{-90.0f};
  float pitchDegrees_{0.0f};
};
