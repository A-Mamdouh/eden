#include <Eden/Services/SceneService/Components.hpp>
#include <Eden/Services/SceneService/SceneService.hpp>
#include <Eden/Systems/TransformSystem.hpp>

#include "EdenTestBase.hpp"

using namespace Eden::Services;
using namespace Eden::World;
using namespace Eden::Systems;

class TransformSystemTest : public EdenTest::EdenTestBase {};

TEST_F(TransformSystemTest, RootEntityWorldTransformEqualsLocalTransform) {
  SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Scene>();
  auto entity = scenePtr->createEntity();
  entity.addComponent<Transform>(Transform{.position = {1.0f, 0.0f, 0.0f}});
  sceneService.loadScene(std::move(scenePtr));

  TransformSystem transformSystem{sceneService};
  transformSystem.init(eventService);
  transformSystem.update(0.016);

  ASSERT_TRUE(entity.hasComponent<WorldTransform>());
  const auto worldPos = entity.getComponent<WorldTransform>().matrix * Eden::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
  EXPECT_FLOAT_EQ(worldPos.x, 1.0f);
}

TEST_F(TransformSystemTest, ChildWorldTransformComposesWithParent) {
  SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Scene>();
  auto &scene = *scenePtr;

  auto parent = scene.createEntity();
  parent.addComponent<Transform>(Transform{.position = {10.0f, 0.0f, 0.0f}});

  auto child = scene.createEntity();
  child.addComponent<Transform>(Transform{.position = {0.0f, 5.0f, 0.0f}});
  child.addComponent<EntityHierarchy>(EntityHierarchy{.parent = parent.handle()});

  sceneService.loadScene(std::move(scenePtr));

  TransformSystem transformSystem{sceneService};
  transformSystem.init(eventService);
  transformSystem.update(0.016);

  const auto worldPos = child.getComponent<WorldTransform>().matrix * Eden::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
  EXPECT_FLOAT_EQ(worldPos.x, 10.0f);
  EXPECT_FLOAT_EQ(worldPos.y, 5.0f);
}

TEST_F(TransformSystemTest, EntitiesWithoutTransformAreIgnored) {
  SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Scene>();
  auto entity = scenePtr->createEntity();
  sceneService.loadScene(std::move(scenePtr));

  TransformSystem transformSystem{sceneService};
  transformSystem.init(eventService);

  EXPECT_NO_THROW(transformSystem.update(0.016));
  EXPECT_FALSE(entity.hasComponent<WorldTransform>());
}

TEST_F(TransformSystemTest, UpdateWithNoActiveSceneIsNoOp) {
  SceneService sceneService;
  sceneService.init(eventService);

  TransformSystem transformSystem{sceneService};
  transformSystem.init(eventService);

  EXPECT_NO_THROW(transformSystem.update(0.016));
}
