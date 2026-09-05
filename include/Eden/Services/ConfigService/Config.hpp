#pragma once

#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

#include <cstdint>
#include <string>

/// Plain-data configuration structs. No behavior lives here; ConfigService
/// owns the live instance and Engine hands slices of it to each
/// Service/System constructor. Every field here is live: RenderSystem
/// reacts to a ConfigService::update() call the same way regardless of
/// which field changed, including backend/validation-layer choices --
/// there's no separate "construction-only" config, since a caller that
/// wants to e.g. swap Renderer backends at runtime should be able to.
/// Grouped by domain, mirroring Eden's other namespaces: only the two
/// aggregates that necessarily span every domain (EngineConfig,
/// ApplicationConfig) live directly in Eden::Config.
namespace Eden::Config::Rendering {

/// Which Renderer implementation to construct.
enum class RendererBackend { Vulkan, Null };

/// How the OS window occupies the screen.
enum class ScreenMode { Windowed, Borderless, Fullscreen };

/// SDL window chrome, owned by RenderSystem. Not something a settings
/// menu would ever expose -- contrast with DisplaySettings.
struct WindowConfig {
  /// Whether the OS window can be resized after creation.
  bool resizable{true};
  /// Text shown in the OS window title bar.
  std::string title{"Eden"};
};

/// The "Display" tab of a settings menu, in the naming games conventionally
/// use for it -- everything here is a candidate for a runtime settings UI.
struct DisplaySettings {
  /// Windowed, borderless-fullscreen, or exclusive fullscreen.
  ScreenMode screenMode{ScreenMode::Windowed};
  /// Window/display width in pixels.
  std::uint32_t width{1280};
  /// Window/display height in pixels.
  std::uint32_t height{720};
  /// Present-mode preference; see Renderer::applySettings().
  Eden::Rendering::VsyncMode vsync{Eden::Rendering::VsyncMode::On};
  /// Target frames per second; 0 means uncapped. Not currently enforced
  /// anywhere (no frame limiter exists yet), reserved for when one is added.
  float targetFrameRate{0.0f};
};

/// The "Graphics" tab of a settings menu -- quality settings that go
/// through Renderer::applySettings(), as opposed to DisplaySettings'
/// window-level fields.
struct GraphicsSettings {
  /// Anti-aliasing level; see Renderer::applySettings().
  Eden::Rendering::AntiAliasing antiAliasing{Eden::Rendering::AntiAliasing::None};
  /// Maximum lights per frame; 0 removes the configured cap. GPU memory and
  /// storage-buffer limits still apply. Changes take effect on the next frame.
  std::uint32_t maxLights{16};
};

/// Rendering backend parameters, owned by RenderSystem -- everything
/// RenderSystem needs to create its window and Renderer.
struct RenderConfig {
  /// Which Renderer implementation to construct.
  RendererBackend backend{RendererBackend::Vulkan};
  /// Requests Vulkan validation layers if available; falls back to off
  /// with a warning if the layer isn't installed.
  bool enableValidationLayers{false};
  WindowConfig window{};
  DisplaySettings display{};
  GraphicsSettings graphics{};
};

} // namespace Eden::Config::Rendering

namespace Eden::Config::Clock {

/// ClockService fixed-timestep and time-scaling parameters.
struct ClockConfig {
  /// Fixed simulation step consumed by ClockService::consumeFixedStep(), in seconds.
  double fixedDt{1.0 / 60.0};
  /// Upper bound on a single frame's real delta time, to avoid a spiral
  /// of death after a stall (e.g. breakpoint, window drag).
  double maxFrameDt{0.25};
  /// Multiplier applied to simulation time; 1.0 is real-time, 0.0 is
  /// equivalent to ClockService::setPaused(true).
  double timeScale{1.0};
};

} // namespace Eden::Config::Clock

namespace Eden::Config::Jobs {

/// JobService worker-pool sizing. Currently unused: JobService's
/// constructor doesn't take this config, and nothing wires it up.
struct JobServiceConfig {
  /// Worker thread count. See struct doc: not wired up yet.
  int numWorkers{4};
};

} // namespace Eden::Config::Jobs

namespace Eden::Config {

/// Aggregate of every subsystem's configuration; one instance per Engine.
struct EngineConfig {
  /// Passed to RenderSystem's constructor.
  Rendering::RenderConfig render{};
  /// Passed to ClockService's constructor.
  Clock::ClockConfig clock{};
  /// See Jobs::JobServiceConfig -- not currently consumed by anything.
  Jobs::JobServiceConfig jobs{};
};

/// Top-level config an application supplies to Engine's constructor.
struct ApplicationConfig {
  /// The only config Engine currently reads; see EngineConfig.
  EngineConfig engine{};
};

} // namespace Eden::Config
