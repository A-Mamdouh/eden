# Eden

[![CI](https://github.com/A-Mamdouh/eden/actions/workflows/ci.yml/badge.svg)](https://github.com/A-Mamdouh/eden/actions/workflows/ci.yml)
[![Docs](https://github.com/A-Mamdouh/eden/actions/workflows/docs.yml/badge.svg)](https://a-mamdouh.github.io/eden/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A C++20 graphics engine built around a small, explicit **Service/System**
architecture and a **backend-agnostic renderer contract** — Vulkan is the
only implementation today, but nothing above the renderer boundary knows
that.

**[Full documentation and API reference →](https://a-mamdouh.github.io/eden/)**

![Eden demo screenshot](docs/_static/demo-screenshot.png)

## What this is

Eden is a from-scratch engine project, not a game built on an existing
engine. The interesting parts, architecturally:

- A **`Renderer` interface** with zero Vulkan/Metal/D3D types in it —
  retained GPU resources via handles, one `renderFrame()` call per frame.
  `NullRenderer` is a second, headless implementation that exists purely
  to prove the contract is real and to let engine logic be unit-tested
  without a GPU.
- An **entt-backed scene** (`Scene`/`Entity`/components) with a small,
  explicit set of engine **Systems** (`ScriptSystem` → `TransformSystem`
  → `RenderSystem`) that Engine ticks in a fixed order each frame, each
  one only reading what the previous one wrote.
- A **script system** for attaching per-entity behavior
  (`ScriptBehaviour`/`ScriptComponent`) without touching engine internals.
- **Every public type is documented** (Doxygen + Breathe + Sphinx,
  published from CI) and the [architecture page](https://a-mamdouh.github.io/eden/architecture.html)
  is kept honest about what's actually wired up versus what's aspirational
  — no feature is claimed that isn't there.

## Building

Requires the [Vulkan SDK](https://vulkan.lunarg.com/) (for `glslc` and the
Vulkan loader) and CMake 3.21+. Every other dependency (SDL2, EnTT, glm,
spdlog, GoogleTest) is fetched automatically if not already installed.

```sh
cmake -B build -G Ninja
cmake --build build
./build/demo/Eden_Demo
```

```sh
ctest --test-dir build --output-on-failure   # run the test suite
```

## Project layout

```
include/Eden/   Public headers (the only thing an embedding app includes)
src/            Implementation
demo/           A small demo application built against the public API
tests/          Unit tests (GoogleTest)
docs/           Doxygen + Sphinx documentation source
```

## Extending Eden

- **New component** — a plain struct, no base class required (see
  `include/Eden/Services/SceneService/Components.hpp`).
- **New system** — derive from `ISystem`, register it in `Engine::init()`
  in the order it needs to run relative to the existing ones.
- **New script** — derive from `ScriptBehaviour`, override `onUpdate()`,
  attach via `ScriptComponent`. See `demo/scripts/PulseTint.hpp` for a
  minimal example.
- **New renderer backend** — implement the `Renderer` interface
  (`include/Eden/Systems/RenderSystem/Renderer.hpp`); `NullRenderer` is
  the smallest real example of what that takes.

## Current limitations

This is an active portfolio project, not a finished product. Documented
honestly rather than glossed over:

- No camera component yet — rendering uses an identity view/projection.
- No input system yet, so scripts can't react to keyboard/mouse.
- No real asset loading — `RenderSystem` draws from a tiny built-in
  primitive mesh library (a triangle and a quad).
- Vulkan is the only renderer backend actually driving pixels;
  `NullRenderer` exists for tests, not for rendering.

## License

[MIT](LICENSE)
