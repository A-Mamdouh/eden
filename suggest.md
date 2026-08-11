Pulling together everything suggested across this session into one prioritized list:

1. ~~Real camera~~ — Done. `Camera` component + `RenderSystem::resolveCamera()`, tracking window aspect ratio on resize. Scenes without a Camera entity fall back to an aspect-corrected orthographic projection (not a bare identity matrix), so camera-less content still renders undistorted.

2. Hierarchy demo scene (cheap, no new engine code) — a parent-child arrangement of entities (e.g. a simple solar-system-style demo). Pure content using EntityHierarchy/TransformSystem, which already exist. The fastest way to make "extensibility" visible rather than argued. (The glTF test asset's two-node hierarchy exercises the mechanism, but there's no dedicated demo content showing it off yet.)

3. InputSystem + interactive demo (medium effort, highest resume payoff) — the single most convincing kind of demo for this project ("look, I can control something"), and the natural next step now that ScriptSystem exists with nothing to react to yet. Also a prerequisite for the free camera below.

4. ~~Depth testing~~ — Done. Device-local depth image/view, second render-pass attachment, `pDepthStencilState` in the pipeline.

5. ~~Textures/materials~~ — Done. `TextureHandle`/`TextureDesc` through the `Renderer` contract (staging buffer, image, sampler, descriptor sets in `VulkanRenderer`; matching bookkeeping in `NullRenderer`), plus a `Material` concept (`RenderSystem`-owned, texture + tint + useVertexColor) that `Renderable` now references via `MeshHandle` + `MaterialHandle` instead of the old symbolic `PrimitiveShape`.

6. ~~Asset loading~~ — Done. cgltf + stb_image vendored; `Engine::loadModel()` parses a glTF/GLB file, uploads its meshes/textures, creates Materials, and spawns entities mirroring the node hierarchy (Transform + EntityHierarchy + Renderable). Verified against a hand-authored two-node textured-quad test asset (`demo/assets/quad.gltf`).

7. "Extending Eden" docs page (cheap, do alongside anything above) — leverages the Sphinx pipeline that already exists.

One cleanup worth naming: JobService sits in the tree fully non-functional (documented honestly as such) — worth either finishing it (spawn the worker threads it's missing, not much code) or removing it, since "here's a subsystem that doesn't work" isn't a great thing for a reviewer to stumble into even when disclosed.

## Next milestone (as scoped when 1/4/5/6 above were picked up)

The actual goal behind camera/depth/textures/materials/asset-loading was prep work for two follow-on milestones, not yet started:

- **Demo scene**: replace the current placeholder triangle/quad/test-quad content with a small `loadModel()`-driven map, plus a free-fly camera (needs InputSystem from #3 above, or at least a minimal keyboard/mouse read path) — the Camera component is deliberately not yet coupled to Transform/WorldTransform, anticipating this.
- **Physics + character**: extend toward a game engine — a physics system and a controllable character, likely needing InputSystem too.

My recommendation: hierarchy demo (#2) is nearly free now that the loader exists — worth doing alongside whichever of InputSystem or the free-camera/map demo comes next, since they reinforce each other. Want to go in that order, or pick something else off the list?
