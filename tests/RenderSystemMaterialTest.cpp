#include <Eden/Services/SceneService/SceneService.hpp>
#include <Eden/Systems/RenderSystem/RenderSystem.hpp>

#include <gtest/gtest.h>

// Deliberately never calls RenderSystem::init(): that would run onInit()
// and try to create a real SDL window/Vulkan renderer, which these tests
// don't need -- createMaterial()/destroyMaterial() are pure bookkeeping
// over RenderSystem's own material slots, untouched by renderer_.

TEST(RenderSystemMaterialTest, CreateMaterialReturnsValidHandle) {
  Eden::SceneService sceneService;
  Eden::RenderSystem renderSystem{Eden::Config::RenderConfig{}, sceneService};

  const auto handle = renderSystem.createMaterial(Eden::Material{});

  EXPECT_TRUE(handle.valid());
}

TEST(RenderSystemMaterialTest, DestroyMaterialThenDestroyAgainIsSafe) {
  Eden::SceneService sceneService;
  Eden::RenderSystem renderSystem{Eden::Config::RenderConfig{}, sceneService};
  const auto handle = renderSystem.createMaterial(Eden::Material{});

  renderSystem.destroyMaterial(handle);

  EXPECT_NO_THROW(renderSystem.destroyMaterial(handle));
}

TEST(RenderSystemMaterialTest, DestroyingAnInvalidHandleIsSafe) {
  Eden::SceneService sceneService;
  Eden::RenderSystem renderSystem{Eden::Config::RenderConfig{}, sceneService};

  EXPECT_NO_THROW(renderSystem.destroyMaterial(Eden::MaterialHandle{}));
}

TEST(RenderSystemMaterialTest, HandleGenerationChangesWhenSlotIsReused) {
  Eden::SceneService sceneService;
  Eden::RenderSystem renderSystem{Eden::Config::RenderConfig{}, sceneService};

  const auto first = renderSystem.createMaterial(Eden::Material{});
  renderSystem.destroyMaterial(first);
  const auto second = renderSystem.createMaterial(Eden::Material{});

  EXPECT_EQ(first.id, second.id);
  EXPECT_NE(first.generation, second.generation);
}
