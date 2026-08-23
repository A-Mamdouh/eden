#include <Eden/Services/SceneService/Components.hpp>
#include <Eden/Services/SceneService/Scene.hpp>

#include <gtest/gtest.h>

TEST(SceneTest, CreateEntityReturnsValidEntity) {
  Eden::Scene scene;
  auto entity = scene.createEntity();

  EXPECT_TRUE(entity.valid());
}

TEST(SceneTest, AddAndGetComponentRoundTrips) {
  Eden::Scene scene;
  auto entity = scene.createEntity();

  entity.addComponent<Eden::Transform>(Eden::Transform{.position = {1.0f, 2.0f, 3.0f}});

  ASSERT_TRUE(entity.hasComponent<Eden::Transform>());
  const auto &transform = entity.getComponent<Eden::Transform>();
  EXPECT_FLOAT_EQ(transform.position.x, 1.0f);
  EXPECT_FLOAT_EQ(transform.position.y, 2.0f);
  EXPECT_FLOAT_EQ(transform.position.z, 3.0f);
}

TEST(SceneTest, HasComponentIsFalseWhenNeverAdded) {
  Eden::Scene scene;
  auto entity = scene.createEntity();

  EXPECT_FALSE(entity.hasComponent<Eden::WorldTransform>());
}

TEST(SceneTest, DestroyEntityInvalidatesIt) {
  Eden::Scene scene;
  auto entity = scene.createEntity();

  scene.destroyEntity(&entity);

  EXPECT_FALSE(entity.valid());
}

TEST(SceneTest, TransformLocalMatrixAppliesTranslation) {
  Eden::Transform transform{};
  transform.position = {2.0f, 3.0f, 4.0f};

  const auto origin = transform.localMatrix() * Eden::Vec4{0.0f, 0.0f, 0.0f, 1.0f};

  EXPECT_FLOAT_EQ(origin.x, 2.0f);
  EXPECT_FLOAT_EQ(origin.y, 3.0f);
  EXPECT_FLOAT_EQ(origin.z, 4.0f);
}

TEST(SceneTest, DefaultTransformLocalMatrixIsIdentity) {
  const Eden::Transform transform{};
  const auto point = transform.localMatrix() * Eden::Vec4{5.0f, 6.0f, 7.0f, 1.0f};

  EXPECT_FLOAT_EQ(point.x, 5.0f);
  EXPECT_FLOAT_EQ(point.y, 6.0f);
  EXPECT_FLOAT_EQ(point.z, 7.0f);
}
