#include <Eden/Services/SceneService/Components.hpp>
#include <Eden/Services/SceneService/SceneService.hpp>
#include <Eden/Systems/TransformSystem.hpp>

#include "EdenTestBase.hpp"

class TransformSystemTest : public EdenTest::EdenTestBase {};

TEST_F(TransformSystemTest, RootEntityWorldTransformEqualsLocalTransform) {
  Eden::SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Eden::Scene>();
  auto entity = scenePtr->createEntity();
  entity.addComponent<Eden::Transform>(Eden::Transform{.position = {1.0f, 0.0f, 0.0f}});
  sceneService.loadScene(std::move(scenePtr));

  Eden::TransformSystem transformSystem{sceneService};
  transformSystem.init(eventService);
  transformSystem.update(0.016);

  ASSERT_TRUE(entity.hasComponent<Eden::WorldTransform>());
  const auto worldPos = entity.getComponent<Eden::WorldTransform>().matrix * Eden::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
  EXPECT_FLOAT_EQ(worldPos.x, 1.0f);
}

TEST_F(TransformSystemTest, ChildWorldTransformComposesWithParent) {
  Eden::SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Eden::Scene>();
  auto &scene = *scenePtr;

  auto parent = scene.createEntity();
  parent.addComponent<Eden::Transform>(Eden::Transform{.position = {10.0f, 0.0f, 0.0f}});

  auto child = scene.createEntity();
  child.addComponent<Eden::Transform>(Eden::Transform{.position = {0.0f, 5.0f, 0.0f}});
  child.addComponent<Eden::EntityHierarchy>(Eden::EntityHierarchy{.parent = parent.handle()});

  sceneService.loadScene(std::move(scenePtr));

  Eden::TransformSystem transformSystem{sceneService};
  transformSystem.init(eventService);
  transformSystem.update(0.016);

  const auto worldPos = child.getComponent<Eden::WorldTransform>().matrix * Eden::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
  EXPECT_FLOAT_EQ(worldPos.x, 10.0f);
  EXPECT_FLOAT_EQ(worldPos.y, 5.0f);
}

TEST_F(TransformSystemTest, EntitiesWithoutTransformAreIgnored) {
  Eden::SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Eden::Scene>();
  auto entity = scenePtr->createEntity();
  sceneService.loadScene(std::move(scenePtr));

  Eden::TransformSystem transformSystem{sceneService};
  transformSystem.init(eventService);

  EXPECT_NO_THROW(transformSystem.update(0.016));
  EXPECT_FALSE(entity.hasComponent<Eden::WorldTransform>());
}

TEST_F(TransformSystemTest, UpdateWithNoActiveSceneIsNoOp) {
  Eden::SceneService sceneService;
  sceneService.init(eventService);

  Eden::TransformSystem transformSystem{sceneService};
  transformSystem.init(eventService);

  EXPECT_NO_THROW(transformSystem.update(0.016));
}
