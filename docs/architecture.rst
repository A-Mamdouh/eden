Architecture
============

This page describes what is actually wired up today, and separately what
exists in code but isn't connected yet. Treat it as the map to read
before opening source files.

Engine, Services, and Systems
------------------------------

:cpp:class:`Eden::Engine` is the composition root. It owns every
subsystem and drives the main loop. Subsystems come in two kinds:

- :cpp:class:`Eden::IService` -- not ticked by the main loop (config,
  event bus, clock, scene ownership, background jobs).
- :cpp:class:`Eden::ISystem` -- ticked once per frame via ``update(dt)``
  (currently only rendering).

Both share the same lifecycle shape: constructed with whatever config
slice they need, then ``init()`` is called once with a weak reference to
the shared :cpp:class:`Eden::EventService`, which is the only channel
subsystems use to talk to each other -- there is no other cross-service
dependency injection. A subsystem publishes an ``IEvent``-derived struct;
anything else can subscribe to that type without knowing who publishes
it.

What ``Engine::init()`` actually constructs, in order:

1. ``EventService``
2. ``ConfigService``
3. ``ClockService``
4. ``RenderSystem`` (the only ``ISystem`` today)

Each frame, ``Engine::run()`` calls ``ClockService::tick()`` for delta
time, then ``update(dt)`` on every registered system.

Built, but not wired into Engine
---------------------------------

These compile into the ``Eden`` library but ``Engine::init()`` never
constructs them:

- ``SceneService`` -- and ``Scene`` itself is currently an empty stub
  struct, not an ECS registry wrapper.
- ``JobService`` -- a worker-thread pool with no callers yet.
- EnTT is fetched by CMake as a dependency but isn't used anywhere in the
  active source tree.

Rendering
---------

:cpp:class:`Eden::Renderer` is the backend-agnostic contract:
:cpp:func:`Eden::Renderer::createMesh`, :cpp:func:`Eden::Renderer::renderFrame`,
:cpp:func:`Eden::Renderer::requestResize`. No Vulkan/Metal/D3D12 type may
appear in this interface or in :cpp:struct:`Eden::RenderFrame` /
:cpp:struct:`Eden::DrawCommand` -- a backend receives a plain frame
description (camera + draw commands referencing mesh handles) and knows
nothing about how the scene was built.

:cpp:class:`Eden::RenderSystem` owns the SDL window and the active
``Renderer`` instance, pumps SDL events each frame, and is the only place
that will eventually walk a ``Scene`` to build a ``RenderFrame`` --
``Renderer`` itself never sees ECS/entt types.

The Vulkan backend is being rewritten against this contract; see the
Vulkan-specific header/source under ``Systems/RenderSystem/Vulkan/`` for
current status rather than trusting this page to stay in sync on backend
internals.

Known gaps
----------

- ``tests/smoke_test.cpp`` is a single ``SUCCEED()`` -- no real coverage
  yet.
- No scene-graph traversal exists yet to build a ``RenderFrame`` from a
  real scene; today's demo constructs one by hand.
- Windowing goes through SDL2 (already cross-platform: Windows, Linux,
  and Apple Silicon macOS). Vulkan itself has no native macOS driver and
  requires MoltenVK via the LunarG Vulkan SDK; ``CMakeLists.txt``'s
  fallback when the system Vulkan SDK isn't found only fetches
  Vulkan-Hpp's headers, not a linkable loader, so a from-scratch build
  currently assumes the Vulkan SDK is already installed.

Building
--------

::

   cmake -B build -G Ninja
   cmake --build build

Dependencies (SDL2, spdlog, EnTT, glm, GoogleTest) are fetched
automatically via CMake ``FetchContent`` if not already installed
system-wide. The Vulkan SDK (providing ``glslc`` and the Vulkan loader)
must be installed separately.

To build this documentation locally, without needing the Vulkan SDK or
any other engine dependency::

   pip install -r docs/requirements.txt
   cmake -B build-docs -G Ninja -DEDEN_BUILD_ENGINE=OFF -DEDEN_BUILD_DOCS=ON
   cmake --build build-docs --target Sphinx

CI builds and, on pushes to ``main``, publishes these docs to GitHub
Pages via ``.github/workflows/docs.yml`` using the same
``EDEN_BUILD_ENGINE=OFF`` path.
