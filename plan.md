# Engine Development Plan

High-level TODO list for building out the engine around the existing `Scene` class.

## 1. Core Application & Loop
- [x] Create an `Application` class that owns the main window and active `Scene`.
- [x] Implement the main loop (init → run → shutdown).
- [x] Add a time system (delta time, fixed timestep option).
- [x] Integrate basic error logging / assertions.

## 2. Platform & Windowing
- [x] Choose / integrate a windowing library (e.g. SDL, GLFW, etc. if not already chosen).
- [x] Implement a `Window` abstraction (create, resize, close, vsync).
- [x] Add an event pump for OS events (close, resize, focus).
- [x] Connect window events to the `Application` / `Scene`.

## 3. Input System
- [x] Implement keyboard input handling (key down/up, held state).
- [x] Implement mouse input handling (position, buttons, scroll).
- [x] Provide a simple input API (e.g. `IsKeyDown`, `GetMousePosition`).
- [ ] Add an input mapping layer (actions/axes) for gameplay code (optional for later).

## 4. Rendering System
- [x] Choose / integrate a graphics API backend (e.g. OpenGL, Vulkan, DirectX, etc.).
- [x] Define a renderer interface (clear, set viewport, draw).
- [x] Implement a basic render pipeline (clear screen, draw a triangle/quad).
- [x] Add a `Camera` abstraction (view/projection matrices).
- [x] Add basic material/texture support.

## 5. Scene & Entities
- [x] Decide on entity model (ECS vs. simple GameObject/Component hierarchy).
- [x] Implement an `Entity` representation and ID system.
- [x] Implement a `Transform` component (position, rotation, scale, hierarchy).
- [x] Integrate entities/components with the `Scene` class.
- [x] Provide update hooks for game logic (per-frame update on entities/components).

## 6. Physics / Collision (Basic)
- [ ] Implement simple 2D/3D colliders (AABB, circle/sphere, etc.).
- [ ] Implement basic collision detection (overlap tests).
- [ ] Add a simple physics/update step (movement, basic responses).
- [ ] Expose physics data to gameplay code.

### Immediate plan for Physics / Collision
1. Define foundational physics components:
   - `Rigidbody` (mass, velocity, damping, flags) for anything that moves.
   - Shape-specific collider components (`AabbCollider`, `CircleCollider`) storing extents/offsets.
2. Author a lightweight `PhysicsWorld` service:
   - Maintains a list of active bodies/colliders pulled from the ECS each frame.
   - Steps bodies with semi-implicit Euler integration (apply gravity + velocity).
3. Collision detection routines:
   - Overlap tests for AABB-vs-AABB and AABB-vs-Circle (2D focus first).
   - Produce simple contact info (penetration vector) for resolution.
4. Collision resolution system:
   - Separate overlapping bodies by penetration.
   - Basic response hooks (`onCollisionEnter/Stay` callbacks on scripts or events).
5. ECS integration:
   - A `PhysicsSystem` that runs before rendering, syncing transforms back to the ECS.
   - Optional `PhysicsDebugRenderer` to visualize collider bounds via the renderer.

## 7. Asset & Resource Management
- [ ] Implement a `ResourceManager` for loading/caching textures, shaders, meshes.
- [ ] Decide on asset file formats and directory structure.
- [ ] Add asynchronous or batched loading (optional for later).
- [ ] Implement basic error handling for missing/invalid assets.

## 8. Serialization & Scenes
- [ ] Choose a serialization format (JSON, YAML, or custom).
- [ ] Implement save/load for `Scene` data (entities, components, transforms).
- [ ] Add the ability to switch scenes within `Application`.
- [ ] Optionally implement a simple level loader from disk.

## 9. Audio System
- [ ] Integrate an audio backend (e.g. SDL_mixer, OpenAL, etc.).
- [ ] Implement sound playback (effects, music).
- [ ] Add volume control and simple mixer abstraction.
- [ ] Expose audio controls to gameplay code.

## 10. Debugging & Tooling
- [ ] Add a debug overlay (FPS, frame time, entity count).
- [ ] Add logging categories for core, rendering, input, etc.
- [ ] Integrate an in-engine debug UI (e.g. ImGui) for inspecting scenes and entities (optional).
- [ ] Set up basic profiling hooks or timers.

## 11. Build & Project Structure
- [ ] Organize source into clear modules (core, renderer, platform, game).
- [ ] Update `CMakeLists.txt` to build engine library and game executable separately, if desired.
- [ ] Add configuration options (Debug/Release, platform toggles, etc.).
- [ ] Document build instructions in `README.md`.

## 12. Polishing & Samples
- [ ] Create one or more sample scenes demonstrating key features (input, rendering, physics).
- [ ] Add a small demo game to validate the engine workflow end-to-end.
- [ ] Iterate on API ergonomics based on usage in the demo.

## 13. Gameplay / Scripting
- [x] Define a lightweight scripting interface (e.g. `ScriptBehaviour` with `onStart`, `onUpdate`, `onRender`, `onEnd`).
- [x] Add a script component/system so entities can attach custom behaviours.
- [x] Expose engine services (input, renderer, scene) to scripts.
