# Engine Refactor Plan

## Goals
- Centralize configuration so every system reads from the same authoritative data (and some systems can write back for runtime settings like resolution).
- Let the engine own all systems, initialize them automatically from configuration, and drive them through a uniform `system->update()` loop.
- Keep user code lightweight: they supply configuration once, derive their application/game, and never touch subsystems or resources directly.
- Expose resources only through their owning system (e.g., `RenderSystem` manages the renderer, `PlatformSystem` manages the window/input) to allow swaps without leaking ownership.

## Proposed Architecture

1. **Global EngineConfig Service**
   - Promote the existing `EngineConfig` into a globally accessible service (or keep a single instance managed by `Engine`) that stores nested structs (`window`, `render`, `input`, etc.).
   - Provide read/write APIs with change notifications so systems can react when another subsystem adjusts settings (window resize triggering renderer + scene updates, etc.).

2. **Engine-Owned Service Container**
   - `Engine` becomes the composition root. During construction it:
     - Stores the config instance.
     - Creates and registers every subsystem (Platform, Render, Event, Scene, Script, future audio/physics).
     - Injects dependencies explicitly (e.g., `RenderSystem` receives references to `Platform` + config slices).
   - Keep a lightweight registry (`std::vector<ISystem*>` or similar) so the main loop can simply iterate and call `update(deltaTime)` (or skip for systems without per-frame work).

3. **System Interfaces & Resource Access**
   - Each system exposes the small API it owns:
     - `RenderSystem::renderer()` returns the renderer reference.
     - `PlatformSystem::window()` / `PlatformSystem::input()` expose window/input handles.
     - Other systems follow the same pattern.
   - No other code holds the raw resources; swapping implementations happens via system methods (e.g., `RenderSystem::reloadRenderer(newConfig)`).

4. **Application Entry Flow**
   - User creates an `Application` subclass, fills out `AppConfig`/`EngineConfig`, and hands it to `Application::Application`.
   - `Application::run()` passes the config to `Engine`, which initializes every system once and starts the loop.
   - When the app quits, `Engine` shuts systems down in reverse order and persists any config mutations if desired.

## Implementation Steps

1. **Configuration Service**
   - Expand `EngineConfig` into nested structs (window, render, input, scene, etc.).
   - Add a `ConfigurationService` (singleton or Engine-owned object) with read/write accessors + change callbacks.
2. **System Interface Cleanup**
   - Introduce a base `ISystem` with `init(const ConfigurationService&)`, `update(float dt)`, and `shutdown()` hooks.
   - Update every existing subsystem to inherit from `ISystem`, accept dependencies in constructors, and expose resource getters instead of globals.
3. **Engine Composition Root**
   - Give `Engine` a `std::vector<std::unique_ptr<ISystem>> systems_` plus typed references for direct use.
   - In `Engine::Engine(const EngineConfig&)`, instantiate systems (Platform first, then Render, etc.), register them, and call their `init`.
   - Rewrite the main loop to iterate `systems_` for `update` calls; specialized per-frame work (poll events, begin/end frame) stays inside each system.
4. **Resource Access Migration**
   - Refactor callers (window, renderer, scripts, demo scenes) to fetch resources from their owning system instead of touching globals (`RenderSystem::getInstance()` etc.).
   - Remove ad-hoc singletons once all call sites use Engine-managed references.
5. **Runtime Config Updates**
   - Wire systems that mutate config (window resize, graphics options) to notify the configuration service so other systems can respond.
   - Expose user-facing APIs to tweak config at runtime (e.g., `Engine::getConfigService().setWindowSize(...)`).

## Follow-Up Tasks / Questions
- Decide whether the configuration service needs thread safety for future tooling.
- Determine persistence format (e.g., write config changes to disk) and ownership (Engine vs Application).
- Consider dependency injection/testing helpers to instantiate single systems with mock configs for unit tests.
