#include <Eden/Systems/RenderSystem/Bounds.hpp>

#include <gtest/gtest.h>

#include <array>

using namespace Eden::Rendering;

TEST(BoundsTest, EmptyVerticesGivesZeroBox) {
  const AABB bounds = computeBounds({});

  EXPECT_EQ(bounds.min, Eden::Vec3(0.0f));
  EXPECT_EQ(bounds.max, Eden::Vec3(0.0f));
}

TEST(BoundsTest, EnclosesEveryVertexPosition) {
  const std::array<Vertex, 4> vertices{
      Vertex{.position = {-1.0f, 2.0f, 0.0f}},
      Vertex{.position = {3.0f, -0.5f, 1.0f}},
      Vertex{.position = {0.0f, 0.0f, -4.0f}},
      Vertex{.position = {1.0f, 1.0f, 1.0f}},
  };

  const AABB bounds = computeBounds(vertices);

  EXPECT_EQ(bounds.min, Eden::Vec3(-1.0f, -0.5f, -4.0f));
  EXPECT_EQ(bounds.max, Eden::Vec3(3.0f, 2.0f, 1.0f));
}

TEST(BoundsTest, SingleVertexGivesDegenerateBoxAtThatPoint) {
  const std::array<Vertex, 1> vertices{Vertex{.position = {2.0f, -3.0f, 5.0f}}};

  const AABB bounds = computeBounds(vertices);

  EXPECT_EQ(bounds.min, Eden::Vec3(2.0f, -3.0f, 5.0f));
  EXPECT_EQ(bounds.max, Eden::Vec3(2.0f, -3.0f, 5.0f));
}
