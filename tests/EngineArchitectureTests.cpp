#include <gtest/gtest.h>

#include "Eden.hpp"

#include <type_traits>

namespace
{

class DummyScene final : public Eden::Scene
{
};

class DummyApplication final : public Eden::Application
{
public:
    using Eden::Application::Application;

private:
    std::unique_ptr<Eden::Scene> createInitialScene() override
    {
        return std::make_unique<DummyScene>();
    }
};

} // namespace

TEST(EngineArchitecture, EngineConfigDefaults)
{
    Eden::EngineConfig config;

    EXPECT_EQ(config.window.width, 1280u);
    EXPECT_EQ(config.window.height, 720u);
    EXPECT_EQ(config.window.title, "Eden");
    EXPECT_TRUE(config.window.resizable);
    EXPECT_FALSE(config.render.enableValidationLayers);
    EXPECT_FLOAT_EQ(config.render.targetFrameRate, 60.0f);
    EXPECT_FALSE(config.render.fixedTimestep);
}

TEST(EngineArchitecture, SceneIsNonCopyableAndNonMovable)
{
    static_assert(!std::is_copy_constructible_v<Eden::Scene>);
    static_assert(!std::is_copy_assignable_v<Eden::Scene>);
    static_assert(!std::is_move_constructible_v<Eden::Scene>);
    static_assert(!std::is_move_assignable_v<Eden::Scene>);
}

TEST(EngineArchitecture, RendererIsNonCopyableAndNonMovable)
{
    static_assert(!std::is_copy_constructible_v<Eden::Renderer>);
    static_assert(!std::is_copy_assignable_v<Eden::Renderer>);
    static_assert(!std::is_move_constructible_v<Eden::Renderer>);
    static_assert(!std::is_move_assignable_v<Eden::Renderer>);
}

TEST(EngineArchitecture, EngineIsNonCopyableAndNonMovable)
{
    static_assert(!std::is_copy_constructible_v<Eden::Engine>);
    static_assert(!std::is_copy_assignable_v<Eden::Engine>);
    static_assert(!std::is_move_constructible_v<Eden::Engine>);
    static_assert(!std::is_move_assignable_v<Eden::Engine>);
}

TEST(EngineArchitecture, ApplicationIsNonCopyableAndNonMovable)
{
    static_assert(!std::is_copy_constructible_v<Eden::Application>);
    static_assert(!std::is_copy_assignable_v<Eden::Application>);
    static_assert(!std::is_move_constructible_v<Eden::Application>);
    static_assert(!std::is_move_assignable_v<Eden::Application>);
}

TEST(EngineArchitecture, WindowIsNonCopyableAndNonMovable)
{
    static_assert(!std::is_copy_constructible_v<Eden::Window>);
    static_assert(!std::is_copy_assignable_v<Eden::Window>);
    static_assert(!std::is_move_constructible_v<Eden::Window>);
    static_assert(!std::is_move_assignable_v<Eden::Window>);
}

TEST(EngineArchitecture, WindowConfigDefaults)
{
    Eden::WindowConfig config;

    EXPECT_EQ(config.width, 1280u);
    EXPECT_EQ(config.height, 720u);
    EXPECT_EQ(config.title, "Eden");
    EXPECT_TRUE(config.resizable);
}

TEST(EngineArchitecture, ApplicationWindowResizeOverrideCompiles)
{
    struct ResizeApplication final : Eden::Application
    {
        using Eden::Application::Application;

        void onWindowResized(unsigned int /*width*/, unsigned int /*height*/) override {}

    private:
        std::unique_ptr<Eden::Scene> createInitialScene() override
        {
            return std::make_unique<DummyScene>();
        }
    };

    static_assert(std::is_constructible_v<ResizeApplication, const Eden::EngineConfig&>);
}
