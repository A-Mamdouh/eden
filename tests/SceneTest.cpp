#include <Eden/Services/SceneService/Components.hpp>
#include <Eden/Services/SceneService/Scene.hpp>

#include <gtest/gtest.h>

using namespace Eden::World;

TEST(SceneTest, CreateEntityReturnsValidEntity) {
  Scene scene;
  auto entity = scene.createEntity();

  EXPECT_TRUE(entity.valid());
}

TEST(SceneTest, AddAndGetComponentRoundTrips) {
  Scene scene;
  auto entity = scene.createEntity();

  entity.addComponent<Transform>(Transform{.position = {1.0f, 2.0f, 3.0f}});

  ASSERT_TRUE(entity.hasComponent<Transform>());
  const auto &transform = entity.getComponent<Transform>();
  EXPECT_FLOAT_EQ(transform.position.x, 1.0f);
  EXPECT_FLOAT_EQ(transform.position.y, 2.0f);
  EXPECT_FLOAT_EQ(transform.position.z, 3.0f);
}

TEST(SceneTest, HasComponentIsFalseWhenNeverAdded) {
  Scene scene;
  auto entity = scene.createEntity();

  EXPECT_FALSE(entity.hasComponent<WorldTransform>());
}

TEST(SceneTest, DestroyEntityInvalidatesIt) {
  Scene scene;
  auto entity = scene.createEntity();

  scene.destroyEntity(&entity);

  EXPECT_FALSE(entity.valid());
}

TEST(SceneTest, TransformLocalMatrixAppliesTranslation) {
  Transform transform{};
  transform.position = {2.0f, 3.0f, 4.0f};

  const auto origin = transform.localMatrix() * Eden::Vec4{0.0f, 0.0f, 0.0f, 1.0f};

  EXPECT_FLOAT_EQ(origin.x, 2.0f);
  EXPECT_FLOAT_EQ(origin.y, 3.0f);
  EXPECT_FLOAT_EQ(origin.z, 4.0f);
}

TEST(SceneTest, DefaultTransformLocalMatrixIsIdentity) {
  const Transform transform{};
  const auto point = transform.localMatrix() * Eden::Vec4{5.0f, 6.0f, 7.0f, 1.0f};

  EXPECT_FLOAT_EQ(point.x, 5.0f);
  EXPECT_FLOAT_EQ(point.y, 6.0f);
  EXPECT_FLOAT_EQ(point.z, 7.0f);
}
