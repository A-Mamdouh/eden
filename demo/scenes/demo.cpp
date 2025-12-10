#include <Engine/Eden.hpp>
#include "../scripts/PlayerController.cpp"
#include "../scripts/PlayerController2.cpp"

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
        

        Eden::Entity rootEntity_ = createEntity();
        // Camera entity.
        cameraEntity_ = createEntity();
        auto& camTransform = cameraEntity_.addComponent<Eden::Transform>();
        camTransform.position = Eden::Vec3{0.0f, 0.0f, 3.0f};
        // camTransform.rotationEuler = Eden::Vec3{0.0f, 0.0f, 0.0f};
        auto& camComp = cameraEntity_.addComponent<Eden::CameraComponent>();
        camComp.primary = true;

        // Triangle entity.
        auto triangle = createEntity();
        auto& triTransform = triangle.addComponent<Eden::Transform>();
        triTransform.position = Eden::Vec3{-0.75f, 1.0f, 0.0f};
        triTransform.scale = Eden::Vec3{1.0f, 1.0f, 1.0f};
        auto& triRenderable = triangle.addComponent<Eden::Renderable>();
        triRenderable.material.baseColor = Eden::Color{1.0f, 0.0f, 0.0f, 1.0f};
        triRenderable.shape = Eden::PrimitiveShape::Quad;
        auto& triBody = triangle.addComponent<Eden::Rigidbody2D>();
        triBody.type = Eden::BodyType2D::Dynamic;
        triBody.fixedRotation = true;
        triBody.enableCCD = true;
        auto& triCollider = triangle.addComponent<Eden::Collider2D>();
        triCollider.shape = Eden::ColliderShape2D::Box;
        triCollider.size = Eden::Vec2{0.5f, 0.5f};
        triCollider.material.friction = 1.0f;
        triCollider.material.restitution = 0.0f;
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
        auto& squareBody = square.addComponent<Eden::Rigidbody2D>();
        squareBody.type = Eden::BodyType2D::Dynamic;
        squareBody.fixedRotation = true;
        auto& squareCollider = square.addComponent<Eden::Collider2D>();
        squareCollider.shape = Eden::ColliderShape2D::Box;
        squareCollider.size = Eden::Vec2{0.5f, 0.5f};
        squareCollider.material.friction = 0.9f;
        squareCollider.material.restitution = 0.0f;
        auto& playerScript2 = square.addComponent<Eden::ScriptComponent>();
        playerScript2.behaviour = std::make_unique<PlayerControllerScript2>();

        // Configure physics for this scene (no global gravity).
        Eden::PhysicsConfig2D physicsConfig{};
        physicsConfig.gravity = Eden::Vec2{0.0f, 0.0f};
        physics_.setConfig(physicsConfig);
        physics_.syncFromRegistry(registry());

        updateCamera();
        updateScriptContext();
    }

    void onDetach() override
    {
        scriptSystem_.shutdown(registry());
    }

    void onResize(unsigned int width, unsigned int height) override
    {
        lastViewportWidth_ = width;
        lastViewportHeight_ = height;

        updateCamera();
        updateScriptContext();
    }

    void onUpdate(float deltaTime) override
    {
        updateCamera();
        updateScriptContext();
        physics_.step(registry(), deltaTime);

        // Example per-frame ECS-driven game logic hook.
        auto view = registry().view<Eden::Transform, Eden::Renderable>();
        for (auto entityId : view)
        {
            auto& transform = view.get<Eden::Transform>(entityId);
            (void)transform;
            // In a real game we would update transform, scripts, etc.
        }
        scriptSystem_.updateScripts(registry(), deltaTime);
        if(input_.isKeyDown(Eden::KeyCode::Escape)) {
            requestQuit();
        }
    }

    void onRender(Eden::Renderer& renderer) override
    {
        if (auto* camera = getPrimaryCamera())
        {
            renderer.setCamera(*camera);
        }

        Eden::Color clearColor{0.1f, 0.1f, 0.1f, 1.0f};
        renderer.setViewport(Eden::Viewport{0.0f, 0.0f, 100.0f, 100.0f});
        renderer.clear(clearColor);

        scriptSystem_.renderScripts(registry(), renderer);
        renderSystem_.render(registry(), renderer);
    }

private:
    Eden::Input& input_;
    Eden::RenderSystem renderSystem_;
    Eden::ScriptSystem scriptSystem_;
    Eden::PhysicsWorld2D physics_;
    unsigned int lastViewportWidth_{1280};
    unsigned int lastViewportHeight_{720};
    Eden::Entity cameraEntity_{};
    Eden::Entity rootEntity_{};

    Eden::Camera* getPrimaryCamera()
    {
        if (!cameraEntity_.valid() || !registry().all_of<Eden::CameraComponent, Eden::Transform>(cameraEntity_.id()))
        {
            return nullptr;
        }
        return &registry().get<Eden::CameraComponent>(cameraEntity_.id()).camera;
    }

    void updateCamera()
    {
        if (!cameraEntity_.valid() || !registry().all_of<Eden::CameraComponent, Eden::Transform>(cameraEntity_.id()))
        {
            return;
        }

        auto& camComp = registry().get<Eden::CameraComponent>(cameraEntity_.id());
        const auto& camTransform = registry().get<Eden::Transform>(cameraEntity_.id());

        const float aspect =
            (lastViewportHeight_ > 0)
                ? static_cast<float>(lastViewportWidth_) / static_cast<float>(lastViewportHeight_)
                : 1.0f;

        camComp.camera.projection = Eden::makePerspective(
            60.0f * M_PI / 180.0f,
            aspect,
            0.1f,
            100.0f);

        const Eden::Vec3 eye = camTransform.position;
        const Eden::Vec3 center = Eden::Vec3{0.0f, 0.0f, 0.0f};
        const Eden::Vec3 up = Eden::Vec3{0.0f, 1.0f, 0.0f};
        camComp.camera.view = Eden::makeLookAt(eye, center, up);
    }

    void updateScriptContext()
    {
        Eden::Viewport vp{
            0.0f,
            0.0f,
            static_cast<float>(lastViewportWidth_),
            static_cast<float>(lastViewportHeight_)};

        auto* camera = getPrimaryCamera();
        if (camera)
        {
            setProjectToScreenContext(*camera, vp);
        }

        // Precompute inverse VP for screenToWorld.
        std::optional<Eden::Mat4> invVP{};
        if (camera)
        {
            const Eden::Mat4 vpMat = camera->projection * camera->view;
            invVP = glm::inverse(vpMat);
        }

        Eden::ScriptContext ctx{};
        ctx.input = &input_;
        ctx.renderer = nullptr;
        ctx.scene = this;
        ctx.projectToScreen = [this](const Eden::Vec3& world) { return projectToScreen(world); };
        ctx.viewportPixels = Eden::Vec2{
            static_cast<float>(lastViewportWidth_),
            static_cast<float>(lastViewportHeight_)};

        if (invVP)
        {
            const auto inv = *invVP;
            ctx.screenToWorld = [inv, vp](const Eden::Vec2& screenPx, float targetZ) -> Eden::Vec3 {
                if (vp.width == 0.0f || vp.height == 0.0f)
                {
                    return {};
                }

                const float ndcX = (screenPx.x / vp.width) * 2.0f - 1.0f;
                const float ndcY = 1.0f - (screenPx.y / vp.height) * 2.0f;

                const Eden::Vec4 nearClip{ndcX, ndcY, 0.0f, 1.0f};
                const Eden::Vec4 farClip{ndcX, ndcY, 1.0f, 1.0f};

                Eden::Vec4 nearWorld = inv * nearClip;
                Eden::Vec4 farWorld = inv * farClip;
                nearWorld /= nearWorld.w;
                farWorld /= farWorld.w;

                const Eden::Vec3 dir = Eden::Vec3{farWorld} - Eden::Vec3{nearWorld};
                if (std::abs(dir.z) < 1e-4f)
                {
                    return Eden::Vec3{nearWorld};
                }

                const float t = (targetZ - nearWorld.z) / dir.z;
                return Eden::Vec3{nearWorld} + dir * t;
            };
        }

        scriptSystem_.setContext(ctx);
    }
};
