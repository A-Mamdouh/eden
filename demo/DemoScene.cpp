#include "DemoScene.hpp"

#include "scripts/FreeFlyCamera.hpp"
#include "scripts/PulseTint.hpp"

#include <memory>

namespace Demo
{
    namespace
    {

        using namespace Eden::World;
        using namespace Eden::Rendering;
        using namespace Eden::Rendering::Components;

        /// Room extents: a square plaza the camera starts inside, walled on all
        /// four sides but open on top -- flying up and over the walls is a
        /// deliberate way to exercise FreeFlyCamera's E/Q vertical movement.
        constexpr float kRoomHalfExtent = 8.0f;
        constexpr float kWallHeight = 3.0f;
        constexpr float kWallThickness = 0.4f;

        Entity spawnBox(Scene &scene, MeshHandle cubeMesh, MaterialHandle material,
                        const Eden::Vec3 &position, const Eden::Vec3 &scale)
        {
            auto entity = scene.createEntity();
            entity.addComponent<Transform>(Transform{.position = position, .scale = scale});
            entity.addComponent<Renderable>(Renderable{.mesh = cubeMesh, .material = material});
            return entity;
        }

        /// Free-fly camera: W/A/S/D to move, mouse to look, Space/Ctrl to rise/descend,
        /// Escape to release the cursor. Camera and FreeFlyCamera live on the same
        /// entity -- the Camera component holds position/target/up, the script
        /// drives them from input each frame -- so there's exactly one entity for
        /// setActiveCamera() to point at.
        Entity spawnFreeFlyCamera(Scene &scene, const Eden::Vec3 &position)
        {
            auto entity = scene.createEntity();
            entity.addComponent<Camera>(Camera{.position = position});
            entity.addComponent<Eden::Scripting::Components::ScriptComponent>(entity, scene,
                                                                              std::make_unique<FreeFlyCamera>());
            return entity;
        }

    } // namespace

    std::unique_ptr<Eden::World::Scene> buildDemoScene(Eden::Engine &engine, const DemoMeshes &meshes,
                                                       Eden::Rendering::Model signModel)
    {
        auto scene = std::make_unique<Scene>();

        const MaterialHandle floorMaterial =
            engine.createMaterial(Material{.shadingModel = ShadingModel::PBR, .baseColorFactor = {0.55f, 0.53f, 0.5f, 1.0f}, .useVertexColor = false});
        const MaterialHandle wallMaterial =
            engine.createMaterial(Material{.shadingModel = ShadingModel::PBR, .baseColorFactor = {0.35f, 0.4f, 0.5f, 1.0f}, .useVertexColor = false});
        // PBR, mid-metallic and fairly smooth: an A/B reference against its
        // still-Unlit neighbors below now that the renderer has real lighting.
        const MaterialHandle pillarMaterialA = engine.createMaterial(Material{.shadingModel = ShadingModel::PBR,
                                                                              .baseColorFactor = {0.8f, 0.3f, 0.25f, 1.0f},
                                                                              .useVertexColor = false,
                                                                              .metallicFactor = 0.6f,
                                                                              .roughnessFactor = 0.35f});
        const MaterialHandle pillarMaterialB =
            engine.createMaterial(Material{.shadingModel = ShadingModel::PBR, .baseColorFactor = {0.3f, 0.7f, 0.35f, 1.0f}, .useVertexColor = false});
        const MaterialHandle pillarMaterialC =
            engine.createMaterial(Material{.shadingModel = ShadingModel::PBR, .baseColorFactor = {0.85f, 0.75f, 0.25f, 1.0f}, .useVertexColor = false});
        const MaterialHandle pillarMaterialD =
            engine.createMaterial(Material{.shadingModel = ShadingModel::PBR, .baseColorFactor = {0.55f, 0.35f, 0.75f, 1.0f}, .useVertexColor = false});
        const MaterialHandle beaconMaterial = engine.createMaterial(Material{});
        const MaterialHandle lightBoxMaterial = engine.createMaterial(Material{.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f}, .useVertexColor = false});

        // Floor: the quad's own vertices lie in its local XY plane, so a 90
        // degree rotation about X lays it flat; scale becomes its world-space
        // width (X) and depth (Z).
        auto floor = scene->createEntity();
        floor.addComponent<Transform>(Transform{
            .rotationEuler = {90.0f, 0.0f, 0.0f}, .scale = {2.0f * kRoomHalfExtent, 2.0f * kRoomHalfExtent, 1.0f}});
        floor.addComponent<Renderable>(Renderable{.mesh = meshes.quad, .material = floorMaterial});

        // Perimeter walls, extended slightly past the room's corners so they
        // overlap instead of leaving gaps.
        const float wallSpan = 2.0f * kRoomHalfExtent + kWallThickness;
        spawnBox(*scene, meshes.cube, wallMaterial, {0.0f, kWallHeight * 0.5f, -kRoomHalfExtent},
                 {wallSpan, kWallHeight, kWallThickness});
        spawnBox(*scene, meshes.cube, wallMaterial, {0.0f, kWallHeight * 0.5f, kRoomHalfExtent},
                 {wallSpan, kWallHeight, kWallThickness});
        spawnBox(*scene, meshes.cube, wallMaterial, {kRoomHalfExtent, kWallHeight * 0.5f, 0.0f},
                 {kWallThickness, kWallHeight, wallSpan});
        spawnBox(*scene, meshes.cube, wallMaterial, {-kRoomHalfExtent, kWallHeight * 0.5f, 0.0f},
                 {kWallThickness, kWallHeight, wallSpan});

        // Corner pillars, one per material, as landmarks to navigate by. A is
        // PBR-shaded (see its Material above) while B/C/D stay flat-tinted
        // Unlit, so orbiting the room shows both shading paths side by side.
        spawnBox(*scene, meshes.cube, pillarMaterialA, {-4.0f, 1.0f, -4.0f}, {1.0f, 2.0f, 1.0f});
        spawnBox(*scene, meshes.cube, pillarMaterialB, {4.0f, 1.0f, -4.0f}, {1.0f, 2.0f, 1.0f});
        spawnBox(*scene, meshes.cube, pillarMaterialC, {-4.0f, 1.0f, 4.0f}, {1.0f, 2.0f, 1.0f});
        spawnBox(*scene, meshes.cube, pillarMaterialD, {4.0f, 1.0f, 4.0f}, {1.0f, 2.0f, 1.0f});

        // Central beacon: taller pillar with PulseTint animating its color each
        // frame via a TintOverride, same script hook FreeFlyCamera drives its
        // own entity through.
        auto beacon =
            spawnBox(*scene, meshes.cube, beaconMaterial, {0.0f, 1.25f, 0.0f}, {1.2f, 2.5f, 1.2f});
        beacon.addComponent<Eden::Scripting::Components::ScriptComponent>(beacon, *scene, std::make_unique<PulseTint>());

        // Proves Engine::loadModel() alongside the hand-built entities above,
        // mounted as a sign against the north wall's interior face. The gltf
        // file bakes a local (0, -1.3, 0) offset into its own node (see
        // demo/assets/quad.gltf), so this entity's Y sits 1.3 above the sign's
        // intended world-space center to compensate.
        auto sign = scene->createEntity();
        sign.addComponent<Transform>(Transform{.position = {-3.0f, 2.8f, -kRoomHalfExtent + 0.4f}});
        sign.addComponent<Model>(std::move(signModel));

        // Directional "sun": Light has no position/direction fields of its own
        // (see Light's doc comment), so a Directional light's travel direction
        // is read off this entity's Transform rotation -- local -Z rotated by
        // rotationEuler, here pointing down and off to one side.
        auto sun = scene->createEntity();
        sun.addComponent<Transform>(Transform{.rotationEuler = {-60.0f, 15.0f, 0.0f}});
        sun.addComponent<Light>(Light{.type = LightType::Directional, .color = {1.0f, 1.0f, 1.0f}, .intensity = 3.0f});

        // Point light above the beacon, warm-tinted against the sun's neutral
        // white -- lets range-based falloff (and the point-vs-directional shading
        // difference) be checked against something already familiar in the scene.
        auto beaconLight = spawnBox(*scene, meshes.cube, lightBoxMaterial, {7.0f, 1.0f, 7.0f}, {.5f, .5f, .5f});
        // auto beaconLight = scene->createEntity();
        // auto &blLightTransform = beaconLight.getComponent<Transform>();
        // blLightTransform.position = {10.0f, 10.0f, 10.0f};
        beaconLight.addComponent<Light>(
            Light{.type = LightType::Point, .color = {1.0f, 0.85f, 0.6f}, .intensity = 10.0f, .range = 5.0f});

        // Starts just inside the south wall, facing the beacon. setActiveCamera()
        // just needs this entity's id, so it's fine to call before loadScene()
        // takes ownership of the scene below.
        auto camera = spawnFreeFlyCamera(*scene, {0.0f, 1.6f, 6.0f});
        engine.setActiveCamera(camera);

        return scene;
    }

} // namespace Demo
