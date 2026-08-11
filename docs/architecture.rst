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
  event bus, clock, scene ownership, background jobs). A Service holds
  state; it has no per-frame behavior of its own.
- :cpp:class:`Eden::ISystem` -- ticked once per frame via ``update(dt)``.
  A System is the active per-frame computation, often *over* a Service's
  state (e.g. :cpp:class:`Eden::TransformSystem` reads/writes the active
  Scene's registry, but the Scene itself never ticks).

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
4. ``SceneService``
5. ``ScriptSystem``
6. ``TransformSystem``
7. ``RenderSystem``

Systems run in registration order each frame: ``ScriptSystem`` before
``TransformSystem`` so a script's ``Transform``/``Renderable`` writes are
already in place when world transforms get computed, and
``TransformSystem`` before ``RenderSystem`` so rendering always reads
this frame's fresh world transforms, never last frame's. Each frame,
``Engine::run()`` calls ``ClockService::tick()`` for delta time, then
``update(dt)`` on every registered system in that order.

Built, but not wired into Engine
---------------------------------

- ``JobService`` -- compiles into the ``Eden`` library, but
  ``Engine::init()`` never constructs it, and it wouldn't actually run
  jobs yet even if it did: nothing spawns worker threads or invokes its
  ``workerLoop()``, so jobs passed to ``submit()`` would queue forever.

Scene
-----

:cpp:class:`Eden::Scene` wraps an ``entt::registry``; entities are
created via :cpp:func:`Eden::Scene::createEntity`, which returns an
:cpp:class:`Eden::Entity` wrapper for ergonomic
``addComponent``/``getComponent`` calls. :cpp:class:`Eden::SceneService`
owns the single active Scene and publishes ``SceneLoadedEvent`` when
:cpp:func:`Eden::Engine::loadScene` swaps it -- the embedding application
calls that, never ``SceneService`` directly, matching how it never
touches any other subsystem.

Components today: ``Transform`` (local position/rotation/scale),
``EntityHierarchy`` (optional parent link), ``WorldTransform`` (computed,
see below), and ``Renderable`` (a symbolic ``PrimitiveShape`` plus tint --
entities reference a shape, not a raw mesh handle, so scene-authored
content doesn't need to know a mesh was already uploaded to the GPU).

:cpp:class:`Eden::TransformSystem` is the only writer of
``WorldTransform``: every frame it walks entities with a ``Transform``,
composing each one with its ``EntityHierarchy`` parent chain (cached
per-frame to avoid recomputing shared ancestors). Everything else,
``RenderSystem`` included, only ever reads ``WorldTransform`` -- nothing
recomputes world placement on its own.

Scripting
---------

An entity gets custom per-frame behavior by attaching a
:cpp:class:`Eden::ScriptComponent` holding a
:cpp:class:`Eden::ScriptBehaviour`. :cpp:class:`Eden::ScriptSystem` walks
every ``ScriptComponent`` each frame: the first tick calls ``onStart()``,
every tick (including that first one) calls ``onUpdate(entity, dt)``.
Both hooks receive the owning :cpp:class:`Eden::Entity`, so a script
reads/writes its own components the same way any other code does --
``entity.getComponent<Renderable>().tint = ...``, for example.

Deliberately minimal for now: a script only ever sees its own entity and
``dt``, nothing else (no input, no querying other entities, no access to
Renderer/EventService). There's no ``InputSystem`` yet for a script to
react to, so this hasn't been a real limitation so far; when one is
needed, it can be added as a further argument to ``onUpdate`` without
breaking existing scripts.

``demo/scripts/PulseTint.hpp`` is the reference example: it animates a
``Renderable``'s tint through a ``sin(time)`` pulse, which is what the
demo's quad used to do as hardcoded logic inside ``RenderSystem`` before
the scene system existed.

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
``Renderer`` instance, pumps SDL events each frame, and walks the active
Scene's ``Renderable``/``WorldTransform`` entities to build each
``RenderFrame`` -- ``Renderer`` itself never sees ECS/entt types. It also
owns a tiny built-in primitive mesh library (currently a triangle and a
quad) that ``Renderable::shape`` resolves against; there is no real
asset-loading system yet.

The Vulkan backend is implemented against this contract; see the
Vulkan-specific header/source under ``Systems/RenderSystem/Vulkan/`` for
current status rather than trusting this page to stay in sync on backend
internals.

Known gaps
----------

- ``tests/smoke_test.cpp`` is a single ``SUCCEED()`` -- no real coverage
  yet.
- No real asset loading: ``RenderSystem``'s primitive mesh library is
  hardcoded C++, not loaded from a file format.
- No camera component yet -- ``RenderSystem`` currently renders with an
  identity view/projection regardless of scene content.
- No ``InputSystem`` yet, so scripts can't react to keyboard/mouse --
  see the Scripting section above.
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
