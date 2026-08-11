#include <Eden/Systems/RenderSystem/Bounds.hpp>

#include <gtest/gtest.h>

#include <array>

TEST(BoundsTest, EmptyVerticesGivesZeroBox) {
  const Eden::AABB bounds = Eden::computeBounds({});

  EXPECT_EQ(bounds.min, Eden::Vec3(0.0f));
  EXPECT_EQ(bounds.max, Eden::Vec3(0.0f));
}

TEST(BoundsTest, EnclosesEveryVertexPosition) {
  const std::array<Eden::Vertex, 4> vertices{
      Eden::Vertex{.position = {-1.0f, 2.0f, 0.0f}},
      Eden::Vertex{.position = {3.0f, -0.5f, 1.0f}},
      Eden::Vertex{.position = {0.0f, 0.0f, -4.0f}},
      Eden::Vertex{.position = {1.0f, 1.0f, 1.0f}},
  };

  const Eden::AABB bounds = Eden::computeBounds(vertices);

  EXPECT_EQ(bounds.min, Eden::Vec3(-1.0f, -0.5f, -4.0f));
  EXPECT_EQ(bounds.max, Eden::Vec3(3.0f, 2.0f, 1.0f));
}

TEST(BoundsTest, SingleVertexGivesDegenerateBoxAtThatPoint) {
  const std::array<Eden::Vertex, 1> vertices{Eden::Vertex{.position = {2.0f, -3.0f, 5.0f}}};

  const Eden::AABB bounds = Eden::computeBounds(vertices);

  EXPECT_EQ(bounds.min, Eden::Vec3(2.0f, -3.0f, 5.0f));
  EXPECT_EQ(bounds.max, Eden::Vec3(2.0f, -3.0f, 5.0f));
}
