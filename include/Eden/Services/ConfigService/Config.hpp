#pragma once

#include <string>

/// Plain-data configuration structs. No behavior lives here; ConfigService
/// owns the live instance and Engine hands slices of it to each
/// Service/System constructor.
namespace Eden::Config {

/// SDL window creation parameters, owned by RenderSystem.
struct WindowConfig {
  /// Initial window width in pixels.
  int width{1280};
  /// Initial window height in pixels.
  int height{720};
  /// Whether to create the window in borderless fullscreen-desktop mode.
  bool fullscreen{false};
  /// Whether the OS window can be resized after creation.
  bool resizable{true};
  /// Text shown in the OS window title bar.
  std::string title{"Eden"};
};

/// Rendering backend parameters, owned by RenderSystem.
struct RenderConfig {
  /// Requests Vulkan validation layers if available; falls back to off
  /// with a warning if the layer isn't installed.
  bool enableValidationLayers{false};
  /// Target frames per second; not currently enforced anywhere (no
  /// frame limiter exists yet), reserved for when one is added.
  float targetFrameRate{60.0f};
  /// If true, ignore targetFrameRate and run uncapped.
  bool enableMaxFPS{false};
};

/// JobService worker-pool sizing. Currently unused: JobService's
/// constructor doesn't take this config, and nothing wires it up.
struct JobServiceConfig {
  /// Worker thread count. See struct doc: not wired up yet.
  int numWorkers{4};
};

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

/// Aggregate of every subsystem's configuration; one instance per Engine.
struct EngineConfig {
  /// Passed to RenderSystem's constructor.
  WindowConfig window{};
  /// Passed to RenderSystem's constructor.
  RenderConfig render{};
  /// Passed to ClockService's constructor.
  ClockConfig clock{};
  /// See JobServiceConfig -- not currently consumed by anything.
  JobServiceConfig jobs{};
};

/// Top-level config an application supplies to Engine's constructor.
struct ApplicationConfig {
  /// The only config Engine currently reads; see EngineConfig.
  EngineConfig engine{};
};

} // namespace Eden::Config
