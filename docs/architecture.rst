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
  state (e.g. :cpp:class:`Eden::Systems::TransformSystem` reads/writes
  the active Scene's registry, but the Scene itself never ticks).

Both share the same lifecycle shape: constructed with whatever config
slice they need, then ``init()`` is called once with a weak reference to
the shared :cpp:class:`Eden::Services::EventService`, which is the only
channel subsystems use to talk to each other -- there is no other
cross-service dependency injection. A subsystem publishes an
``IEvent``-derived struct; anything else can subscribe to that type
without knowing who publishes it.

Every non-foundational type lives under a namespace matching its domain
-- ``Eden::Systems``/``Eden::Services`` for the System/Service
implementation classes themselves, ``Eden::Rendering``, ``Eden::World``
(Scene/Entity/ECS components), ``Eden::Scripting``, ``Eden::Input``, and
``Eden::Config`` (further split per-domain, e.g.
``Eden::Config::Rendering``) for configuration. Only what an embedding
application is expected to name directly --
:cpp:class:`Eden::Engine`, :cpp:class:`Eden::World::Scene`/
:cpp:class:`Eden::World::Entity`, the ECS components, ``Model``,
``ScriptBehaviour``, ``InputState``/``Key``, and the ``Vec2``/``Vec3``/
``Vec4``/``Mat4`` math aliases -- stays flat at ``Eden::`` or is
re-exported by the ``Eden.hpp`` umbrella header.

What ``Engine::init()`` actually constructs, in order:

1. ``EventService``
2. ``ConfigService``
3. ``ClockService``
4. ``SceneService``
5. ``InputSystem``
6. ``ScriptSystem``
7. ``TransformSystem``
8. ``RenderSystem``

Systems run in registration order each frame: ``InputSystem`` before
``ScriptSystem`` so a script sees this frame's fresh keyboard/mouse
state rather than last frame's; ``ScriptSystem`` before
``TransformSystem`` so a script's ``Transform``/``Renderable`` writes are
already in place when world transforms get computed; and
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

:cpp:class:`Eden::World::Scene` wraps an ``entt::registry``; entities
are created via :cpp:func:`Eden::World::Scene::createEntity`, which
returns an :cpp:class:`Eden::World::Entity` wrapper for ergonomic
``addComponent``/``getComponent`` calls.
:cpp:class:`Eden::Services::SceneService` owns the single active Scene
and publishes ``SceneLoadedEvent`` when
:cpp:func:`Eden::Engine::loadScene` swaps it -- the embedding application
calls that, never ``SceneService`` directly, matching how it never
touches any other subsystem.

Components today: ``Transform`` (local position/rotation/scale,
``Eden::World``), ``EntityHierarchy`` (optional parent link,
``Eden::World``), ``WorldTransform`` (computed, see below,
``Eden::World``), ``Camera`` (view/projection source for rendering,
``Eden::Rendering::Components``), and ``Renderable`` (a ``MeshHandle``
plus a ``MaterialHandle`` -- both created via
:cpp:func:`Eden::Engine::createMesh` /
:cpp:func:`Eden::Engine::createMaterial`, also
``Eden::Rendering::Components``), for single-mesh content. Transform/
EntityHierarchy/WorldTransform stay in ``Eden::World`` rather than a
domain namespace since every domain reads or writes them; Camera and
Renderable live in ``Eden::Rendering::Components`` since only rendering
ever touches them. A loaded asset instead uses ``Model``, holding a list
of ``ModelPart`` (mesh + material + a transform local to the Model, see
Rendering below) returned by :cpp:func:`Eden::Engine::loadModel`. Either
way, one ``Transform``/``WorldTransform`` on the owning entity places the
whole thing. An optional ``TintOverride`` component
(``Eden::Rendering::Components``) lets a script animate one entity's
color without mutating a Material other entities may share.

:cpp:class:`Eden::Systems::TransformSystem` is the only writer of
``WorldTransform``: every frame it walks entities with a ``Transform``,
composing each one with its ``EntityHierarchy`` parent chain (cached
per-frame to avoid recomputing shared ancestors). Everything else,
``RenderSystem`` included, only ever reads ``WorldTransform`` -- nothing
recomputes world placement on its own.

Scripting
---------

An entity gets custom per-frame behavior by attaching a
:cpp:class:`Eden::Scripting::Components::ScriptComponent` holding a
:cpp:class:`Eden::Scripting::ScriptBehaviour`.
:cpp:class:`Eden::Systems::ScriptSystem` walks every ``ScriptComponent``
each frame: the first tick calls ``onStart()``, every tick (including
that first one) calls ``onUpdate(entity, dt)``. Both hooks receive the
owning :cpp:class:`Eden::World::Entity`, so a script reads/writes its own
components the same way any other code does --
``entity.getComponent<TintOverride>().tint = ...``, for example.

Both hooks also receive an :cpp:class:`Eden::Input::InputState`,
non-const because scripts legitimately mutate it too (e.g. releasing
mouse capture), not just query it -- see Input below. Beyond that, still
deliberately minimal: no querying other entities, no access to
Renderer/EventService, and no :cpp:class:`Eden::Systems::InputSystem`
either -- :cpp:class:`Eden::Input::InputState` is the entire
query/request surface a script needs.

``demo/scripts/PulseTint.hpp`` is the reference example for reading
components: it animates an entity's color through a ``sin(time)`` pulse
by writing a ``TintOverride``, which is what the demo's quad used to do
as hardcoded logic inside ``RenderSystem`` before the scene system
existed. ``demo/scripts/FreeFlyCamera.hpp`` is the reference example for
reading input: a WASD-plus-mouselook camera controller, driven entirely
through the same ``ScriptBehaviour`` hooks -- see Input below.

Input
-----

:cpp:class:`Eden::Input::InputState` is the data half of input --
keyboard/mouse state plus the small query/request API over it
(:cpp:func:`Eden::Input::InputState::isKeyDown`,
:cpp:func:`Eden::Input::InputState::isKeyPressed` for the up-to-down edge,
:cpp:func:`Eden::Input::InputState::mouseDelta`,
:cpp:func:`Eden::Input::InputState::mousePosition` for absolute window
coordinates, e.g. UI hit-testing, and
:cpp:func:`Eden::Input::InputState::setMouseCaptured` for mouselook,
which only records the request here) -- the same role
:cpp:class:`Eden::World::Scene` plays for
:cpp:class:`Eden::Services::SceneService`. Keys are identified by the
vendor-neutral :cpp:enum:`Eden::Input::Key` rather than an SDL scancode,
keeping SDL out of every public header the way ``Vec3``/``Mat4`` keep
glm's types out.

:cpp:class:`Eden::Systems::InputSystem` owns one ``InputState`` and is
the only thing that actually touches SDL: each ``update()`` polls
``SDL_GetKeyboardState()``/``SDL_GetRelativeMouseState()``/
``SDL_GetMouseState()`` into it, reconciles any pending
``setMouseCaptured()`` request against real SDL relative-mouse-mode
(applying ``SDL_SetRelativeMouseMode()`` only on a transition), then
publishes ``Events::InputStateUpdatedEvent`` carrying a pointer to that
``InputState``. This is how :cpp:class:`Eden::Systems::ScriptSystem`
reaches it -- subscribing once in its own ``onInit()`` and caching the
pointer -- rather than depending on ``InputSystem`` directly, matching
every other subsystem's rule of talking only through ``EventService``.
``InputSystem::update()`` always runs before ``ScriptSystem::update()``
in Engine's per-frame system order, so by the time a script runs, the
``InputState`` it was handed already reflects this frame's fresh input,
not last frame's.

Deliberately polling-style rather than a discrete event stream: that
covers everything a movement/mouselook script actually needs, without
fanning per-keystroke events out to listeners. ``InputSystem`` doesn't
own SDL's event queue either -- ``RenderSystem`` still pumps that
directly for quit/resize, exactly as before; ``SDL_GetKeyboardState``/
``SDL_GetRelativeMouseState``/``SDL_GetMouseState`` read input-device
state that ``SDL_PumpEvents`` (called internally by whichever of the two
runs first each frame) keeps current, without draining the same queue
``RenderSystem`` polls, so the two coexist safely.

Rendering
---------

:cpp:class:`Eden::Rendering::Renderer` is the backend-agnostic contract:
:cpp:func:`Eden::Rendering::Renderer::createMesh`,
:cpp:func:`Eden::Rendering::Renderer::createTexture`,
:cpp:func:`Eden::Rendering::Renderer::renderFrame`,
:cpp:func:`Eden::Rendering::Renderer::requestResize`.
No Vulkan/Metal/D3D12 type may appear in this interface or in
:cpp:struct:`Eden::Rendering::RenderFrame` /
:cpp:struct:`Eden::Rendering::DrawCommand` -- a backend receives a plain
frame description (camera + draw commands referencing mesh/texture
handles) and knows nothing about how the scene was built. An
invalid/unset texture handle on a ``DrawCommand`` draws with the
backend's own 1x1 white texture, so untextured and textured draws go
through the same shader path. ``Renderer`` also exposes
``applySettings()``/``queryCapabilities()`` for runtime graphics settings
(anti-aliasing, vsync) -- see Graphics settings below.

:cpp:class:`Eden::Systems::RenderSystem` owns the SDL window and the
active ``Renderer`` instance, pumps SDL events each frame, and walks the
active Scene's ``Renderable``/``WorldTransform`` and
``Model``/``WorldTransform`` entities to build each ``RenderFrame`` --
``Renderer`` itself never sees ECS/entt types. It also owns Material
storage: :cpp:func:`Eden::Engine::createMaterial` stores a ``Material`` and
hands back a ``MaterialHandle`` a ``Renderable`` or ``ModelPart``
references; RenderSystem resolves it into a ``DrawCommand``'s shading
fields each frame. A ``Material``'s ``shadingModel`` picks between
``Unlit`` (flat texture times vertex color or ``baseColorFactor``, no
lighting) and ``PBR`` (metallic-roughness Cook-Torrance, lit by the
frame's ``Light`` entities; see below). A ``Model`` entity draws one
``DrawCommand`` per part, each part's transform composed as the entity's
``WorldTransform`` times that part's own ``ModelPart::localTransform``.

RenderSystem renders from whichever entity is selected via
:cpp:func:`Eden::Engine::setActiveCamera` (forwarded to
:cpp:func:`Eden::Systems::RenderSystem::setActiveCamera`). It stores
just that entity's id and re-resolves its ``Camera`` component against
the active scene each frame rather than caching a reference -- EnTT's
component-pool references aren't safe to hold onto between frames, since
any structural change to that pool (anywhere, not just this entity) can
invalidate them, where an id just fails to resolve cleanly instead. A
``Camera`` component on an entity that isn't selected has no effect on
its own -- there's no per-component "active" flag -- which is what lets
several coexist (e.g. sibling first-person/third-person camera entities
under a player) without needing to know about each other. If no camera
is set, or the selected entity doesn't resolve to a live ``Camera`` in
the active scene, RenderSystem falls back to an aspect-corrected
orthographic projection (identity view) so camera-less scenes still
render undistorted regardless of window size.

Unlike ``Camera``, a ``Light`` component has no position/direction fields
of its own -- RenderSystem collects every ``Light``/``WorldTransform`` pair
in the scene each frame (up to ``render.graphics.maxLights``, or all lights
when it is 0) and reads a
Directional light's travel direction, or a Point light's position, straight
out of that entity's ``WorldTransform``, so a light moves/parents for free
through the same ``Transform``/``EntityHierarchy`` machinery every other
spatial entity uses.

:cpp:func:`Eden::Engine::loadModel` is the real asset-loading path: it
parses a glTF/GLB file (via the vendored ``cgltf``/``stb_image``
single-header libraries), uploads its meshes and textures through
``RenderSystem``, creates a ``Material`` per glTF material, and returns a
``Model`` -- one ``ModelPart`` per mesh primitive in the file, each
carrying its glTF node's world transform *within the file* (i.e.
relative to whatever the caller treats as the model's origin) as a plain
matrix. ``loadModel`` doesn't touch the scene at all; like
``createMesh``/``createMaterial`` it's a pure resource call, so the
caller attaches the result to whichever entity should own it --
``entity.addComponent<Model>(std::move(model))`` -- alongside a
``Transform`` to place it, exactly like any other component. Because
each part's transform is a baked matrix rather than Eden's Euler-angle
``Transform``, there's no quaternion-to-Euler precision loss for a
model's internal node structure; only the one ``Transform`` the caller
puts on the owning entity uses Euler angles. The demo still uploads a
couple of hand-built primitive meshes directly via
:cpp:func:`Eden::Engine::createMesh` alongside a loaded model, showing
both paths side by side.

The Vulkan backend is implemented against this contract; see the
Vulkan-specific header/source under ``src/Systems/RenderSystem/Vulkan/``
for current status rather than trusting this page to stay in sync on
backend internals -- at a glance, it has a depth buffer (device-local
image, second render-pass attachment), an optional multisampled color
target for anti-aliasing (resolved into the swapchain image, added/
removed as anti-aliasing settings change), and a full texture pipeline
(staging buffer, device-local image, sampler, one descriptor set per
texture). :cpp:class:`Eden::Rendering::NullRenderer` is a second,
headless implementation -- no window, no GPU, no graphics API calls --
that exists to prove the contract is genuinely backend-agnostic and to
give the test suite (below) something to construct a ``Renderer``
against without a GPU. It isn't wired into ``RenderSystem``; only tests
use it directly.

Graphics settings
------------------

Every field of ``Eden::Config::Rendering::RenderConfig`` (backend
choice, validation layers, window chrome, display settings, graphics
settings) is live rather than construction-only: an embedding
application reads the current config via ``Engine::config()``, builds a
modified copy, and calls ``Engine::updateConfig()`` to apply it --
forwarded to ``ConfigService::update()``, which publishes
``Events::ConfigUpdatedEvent``. ``RenderSystem`` subscribes to that event
and routes each changed field to wherever it actually applies: window
chrome and screen mode/resolution go straight to SDL, anti-aliasing/
vsync go through ``Renderer::applySettings()`` (which reports back
``Applied`` or ``RequiresRecreate``), and a backend or validation-layer
change rebuilds the ``Renderer`` entirely via a small backend factory
table. This is the same mechanism whether the change is as small as an
anti-aliasing level or as large as swapping the active ``Renderer``
backend -- ``RenderSystem`` doesn't special-case either. Resource
handles (``MeshHandle``/``TextureHandle``) are scoped to the ``Renderer``
instance that issued them, so a backend swap invalidates anything
uploaded before it; nothing recreates that content automatically today,
matching Eden's general stance of exposing the capability rather than
guarding against its misuse.

Culling
-------

:cpp:class:`Eden::Systems::RenderSystem` skips draw commands for
geometry the active camera's frustum can't see, rather than submitting
everything in the scene to the backend every frame. Each mesh gets an
object-space :cpp:struct:`Eden::Rendering::AABB` (min/max over its
vertex positions) computed once in ``createMesh()`` -- the only point
that ever sees the mesh's raw vertex data, since the backend owns the
uploaded buffer afterward and there's no readback path.
``buildFrameFromScene()`` builds one :cpp:class:`Eden::Rendering::Frustum`
per frame from ``projection * view``, extracting its six planes via the
standard Gribb/Hartmann method (adjusted for Eden's Vulkan-style [0, 1]
NDC depth range rather than OpenGL's [-1, 1] -- see ``Frustum.cpp`` for
the derivation), then tests each ``Renderable``/``ModelPart`` mesh's AABB
against it -- transformed into world space by that entity's
``WorldTransform`` and, for a ``ModelPart``, its own ``localTransform``
too -- before building a ``DrawCommand``. The test re-encloses the
transformed box in a new axis-aligned box rather than testing eight
rotated corners against each plane directly, so it's conservative under
rotation: it never culls something actually visible, only ever something
fully outside every plane. A mesh whose bounds can't be resolved (stale
or invalid handle) is never culled -- absence of data means "don't
know", not "not visible".

Backend header firewall
------------------------

No Vulkan (or, eventually, Metal/D3D12/GL) type is reachable from
``include/``. ``VulkanRenderer.hpp`` lives under ``src/``, not
``include/`` -- a plain file-move, since CMake already treats ``src/``
as a private include path (``PRIVATE`` in ``target_include_directories``)
that only the ``Eden`` library's own ``.cpp`` files can see. That alone
keeps Vulkan out of anything a consumer includes.

It also solves a sharper problem than public-API leakage: two real
graphics SDKs' headers landing in the *same translation unit* inside the
engine itself, which is exactly how backends "clash" (macro/type
collisions, especially on Windows where both often drag in
``windows.h``). ``RenderSystem.cpp`` used to construct ``VulkanRenderer``
directly, which meant it had to see the complete ``vk::``-typed class
just to know its size. Now it calls
``createVulkanRenderer(SDL_Window*, bool, const RenderSettings&)`` --
declared in ``VulkanRendererFactory.hpp``, a header with zero Vulkan
types in its signature -- and only ``VulkanRenderer.cpp`` itself ever
includes the real header. A future second backend gets the same shape:
its own private header plus a tiny factory declaration, so no single
file is ever positioned to include two backends' real SDK headers at
once, no matter how many backends exist.

Testing
-------

``tests/`` is GoogleTest-based and runs via ``ctest``. Coverage today:
``EventService`` pub/sub, ``ClockService`` timing/pause/scale behavior,
``Scene``/``Entity``/component round-tripping, ``TransformSystem``'s
hierarchy composition, ``ScriptSystem``'s start/update contract,
``InputSystem``'s ``InputState`` query methods (constructed without
calling ``init()`` either -- ``SDL_GetKeyboardState``/``SDL_PumpEvents``/
``SDL_GetRelativeMouseState`` are all safe to call before ``SDL_Init``,
which is what makes this testable without a window),
``NullRenderer``'s mesh/texture handle lifecycle, ``RenderSystem``'s
Material handle lifecycle (constructed without calling ``init()``, so
no real window/GPU is ever touched), and ``AABB``/``Frustum``'s culling
math (built directly from ``Camera::viewMatrix()``/``projectionMatrix()``
with hand-picked object positions relative to the near/far/side planes,
no ``RenderSystem`` or scene involved) -- all pure logic, none of it
needs a window or GPU, which is what makes it possible
to run in CI without a display or real Vulkan driver (see
``.github/workflows/ci.yml``, which still needs the Vulkan SDK installed
to *build* ``VulkanRenderer.cpp`` and link the loader, just not to run
these tests). The glTF loader itself has no dedicated unit tests yet --
it's exercised end-to-end by loading ``demo/assets/quad.gltf`` in the
demo, not by an automated test.

Known gaps
----------

- The glTF loader supports triangle-list primitives with POSITION,
  NORMAL, and TEXCOORD_0, and reads the full metallic-roughness material
  (base color, metallic/roughness, emissive, each factor+texture) -- no
  skinning/animation, tangents/normal maps, occlusion texture, or multi-UV
  materials yet. It also reads every node in the file rather than
  respecting glTF's ``scene``/``scenes`` selection, which only matters for
  multi-scene files (uncommon).
- Rendering supports two shading models per ``Material``: flat ``Unlit``
  (texture times vertex color or a flat tint, no lighting) and metallic-
  roughness PBR (Cook-Torrance BRDF) lit by any number of directional/point
  ``Light`` entities, limited by ``render.graphics.maxLights`` (default 16;
  0 removes the configured cap). Vulkan uses a growable light storage buffer,
  subject to GPU memory and storage-buffer limits. There's no image-based ambient lighting yet
  -- PBR materials get a single flat ambient constant instead of a real
  environment/irradiance map -- and no spot lights, shadows, or HDR/
  tonemapping.
- A ``Model``'s parts are a flat, rigid list under one entity -- there's
  no way to reference or move an individual part independently (e.g. one
  node of a loaded rig), matching the lack of skinning/animation support
  above.
- ``DisplaySettings::targetFrameRate`` is live and threaded through
  config, but nothing enforces it yet -- no frame limiter exists in
  Engine's main loop, so it's a no-op until one is added.
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
   ctest --test-dir build --output-on-failure

Dependencies (SDL2, spdlog, EnTT, glm, GoogleTest) are fetched
automatically via CMake ``FetchContent`` if not already installed
system-wide. The Vulkan SDK (providing ``glslc`` and the Vulkan loader)
must be installed separately. ``.github/workflows/ci.yml`` runs this same
build-and-test sequence on every push and pull request.

To build this documentation locally, without needing the Vulkan SDK or
any other engine dependency::

   pip install -r docs/requirements.txt
   cmake -B build-docs -G Ninja -DEDEN_BUILD_LIBRARY=OFF -DEDEN_BUILD_DEMO=OFF -DEDEN_BUILD_TESTS=OFF -DEDEN_BUILD_DOCS=ON
   cmake --build build-docs --target Sphinx

CI builds and, on pushes to ``main``, publishes these docs to GitHub
Pages via ``.github/workflows/docs.yml`` using the same
``EDEN_BUILD_LIBRARY=OFF`` path.
