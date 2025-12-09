#include <Engine/Eden.hpp>
#include "../scripts/PlayerController.cpp"

class DemoScene : public Eden::EcsScene
{
public:
    explicit DemoScene(Eden::Input& input)
        : input_(input)
    {
    }

    void onAttach() override
    {
        scriptSystem_.setContext(Eden::ScriptContext{&input_, nullptr, this});

        // Triangle entity.
        auto triangle = createEntity();
        auto& triTransform = triangle.addComponent<Eden::Transform>();
        triTransform.position = Eden::Vec3{-0.75f, 1.0f, 0.0f};
        triTransform.scale = Eden::Vec3{1.0f, 1.0f, 1.0f};
        auto& triRenderable = triangle.addComponent<Eden::Renderable>();
        triRenderable.material.baseColor = Eden::Color{1.0f, 0.0f, 0.0f, 1.0f};
        triRenderable.shape = Eden::PrimitiveShape::Triangle;
        auto& playerScript = triangle.addComponent<Eden::ScriptComponent>();
        playerScript.behaviour = std::make_unique<PlayerControllerScript>();

        // Square entity (to be rendered as a quad later).
        auto square = createEntity();
        auto& squareTransform = square.addComponent<Eden::Transform>();
        squareTransform.position = Eden::Vec3{0.75f, 0.0f, 0.0f};
        squareTransform.scale = Eden::Vec3{1.0f, 1.0f, 1.0f};
        auto& sqRenderable = square.addComponent<Eden::Renderable>();
        sqRenderable.material.baseColor = Eden::Color{0.0f, 0.0f, 1.0f, 1.0f};
        sqRenderable.shape = Eden::PrimitiveShape::Quad;
    }

    void onDetach() override
    {
        scriptSystem_.shutdown(registry());
    }

    void onResize(unsigned int width, unsigned int height) override
    {
        const float aspect =
            (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

        camera_.projection = Eden::makePerspective(
            60.0f * M_PI / 180.0f,
            aspect,
            0.1f,
            100.0f);

        camera_.view = Eden::makeLookAt(
            Eden::Vec3{0.0f, 0.0f, 3.0f},
            Eden::Vec3{0.0f, 0.0f, 0.0f},
            Eden::Vec3{0.0f, 1.0f, 0.0f});
    }

    void onUpdate(float deltaTime) override
    {
        (void)deltaTime;

        // Example per-frame ECS-driven game logic hook.
        auto view = registry().view<Eden::Transform, Eden::Renderable>();
        for (auto entityId : view)
        {
            auto& transform = view.get<Eden::Transform>(entityId);
            (void)transform;
            // In a real game we would update transform, scripts, etc.
        }

        scriptSystem_.updateScripts(registry(), deltaTime);
    }

    void onRender(Eden::Renderer& renderer) override
    {
        renderer.setCamera(camera_);

        Eden::Color clearColor{0.1f, 0.1f, 0.1f, 1.0f};
        renderer.setViewport(Eden::Viewport{0.0f, 0.0f, 100.0f, 100.0f});
        renderer.clear(clearColor);

        scriptSystem_.renderScripts(registry(), renderer);
        renderSystem_.render(registry(), renderer);
    }

private:
    Eden::Input& input_;
    Eden::Camera camera_;
    Eden::RenderSystem renderSystem_;
    Eden::ScriptSystem scriptSystem_;
};
