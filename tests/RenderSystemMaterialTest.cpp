#include <Eden/Services/SceneService/SceneService.hpp>
#include <Eden/Systems/RenderSystem/RenderSystem.hpp>

#include <gtest/gtest.h>

// Deliberately never calls RenderSystem::init(): that would run onInit()
// and try to create a real SDL window/Vulkan renderer, which these tests
// don't need -- createMaterial()/destroyMaterial() are pure bookkeeping
// over RenderSystem's own material slots, untouched by renderer_.

using namespace Eden::Services;
using namespace Eden::Systems;
using namespace Eden::Rendering;

TEST(RenderSystemMaterialTest, CreateMaterialReturnsValidHandle) {
  SceneService sceneService;
  RenderSystem renderSystem{Eden::Config::Rendering::RenderConfig{}, sceneService};

  const auto handle = renderSystem.createMaterial(Material{});

  EXPECT_TRUE(handle.valid());
}

TEST(RenderSystemMaterialTest, DestroyMaterialThenDestroyAgainIsSafe) {
  SceneService sceneService;
  RenderSystem renderSystem{Eden::Config::Rendering::RenderConfig{}, sceneService};
  const auto handle = renderSystem.createMaterial(Material{});

  renderSystem.destroyMaterial(handle);

  EXPECT_NO_THROW(renderSystem.destroyMaterial(handle));
}

TEST(RenderSystemMaterialTest, DestroyingAnInvalidHandleIsSafe) {
  SceneService sceneService;
  RenderSystem renderSystem{Eden::Config::Rendering::RenderConfig{}, sceneService};

  EXPECT_NO_THROW(renderSystem.destroyMaterial(MaterialHandle{}));
}

TEST(RenderSystemMaterialTest, HandleGenerationChangesWhenSlotIsReused) {
  SceneService sceneService;
  RenderSystem renderSystem{Eden::Config::Rendering::RenderConfig{}, sceneService};

  const auto first = renderSystem.createMaterial(Material{});
  renderSystem.destroyMaterial(first);
  const auto second = renderSystem.createMaterial(Material{});

  EXPECT_EQ(first.id, second.id);
  EXPECT_NE(first.generation, second.generation);
}
