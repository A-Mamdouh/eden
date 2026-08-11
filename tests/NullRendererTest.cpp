#include <Eden/Systems/RenderSystem/NullRenderer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>

TEST(NullRendererTest, CreateMeshReturnsValidHandle) {
  Eden::NullRenderer renderer;
  const std::array<Eden::Vertex, 3> vertices{};

  const auto handle = renderer.createMesh(Eden::MeshDesc{vertices});

  EXPECT_TRUE(handle.valid());
}

TEST(NullRendererTest, DestroyMeshThenDestroyAgainIsSafe) {
  Eden::NullRenderer renderer;
  const std::array<Eden::Vertex, 3> vertices{};
  const auto handle = renderer.createMesh(Eden::MeshDesc{vertices});

  renderer.destroyMesh(handle);

  EXPECT_NO_THROW(renderer.destroyMesh(handle));
}

TEST(NullRendererTest, DestroyingAnInvalidHandleIsSafe) {
  Eden::NullRenderer renderer;

  EXPECT_NO_THROW(renderer.destroyMesh(Eden::MeshHandle{}));
}

TEST(NullRendererTest, HandleGenerationChangesWhenSlotIsReused) {
  Eden::NullRenderer renderer;
  const std::array<Eden::Vertex, 3> vertices{};

  const auto first = renderer.createMesh(Eden::MeshDesc{vertices});
  renderer.destroyMesh(first);
  const auto second = renderer.createMesh(Eden::MeshDesc{vertices});

  EXPECT_EQ(first.id, second.id);
  EXPECT_NE(first.generation, second.generation);
}

TEST(NullRendererTest, CreateTextureReturnsValidHandle) {
  Eden::NullRenderer renderer;
  const std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};

  const auto handle = renderer.createTexture(Eden::TextureDesc{1, 1, pixels});

  EXPECT_TRUE(handle.valid());
}

TEST(NullRendererTest, DestroyTextureThenDestroyAgainIsSafe) {
  Eden::NullRenderer renderer;
  const std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};
  const auto handle = renderer.createTexture(Eden::TextureDesc{1, 1, pixels});

  renderer.destroyTexture(handle);

  EXPECT_NO_THROW(renderer.destroyTexture(handle));
}

TEST(NullRendererTest, DestroyingAnInvalidTextureHandleIsSafe) {
  Eden::NullRenderer renderer;

  EXPECT_NO_THROW(renderer.destroyTexture(Eden::TextureHandle{}));
}

TEST(NullRendererTest, TextureHandleGenerationChangesWhenSlotIsReused) {
  Eden::NullRenderer renderer;
  const std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};

  const auto first = renderer.createTexture(Eden::TextureDesc{1, 1, pixels});
  renderer.destroyTexture(first);
  const auto second = renderer.createTexture(Eden::TextureDesc{1, 1, pixels});

  EXPECT_EQ(first.id, second.id);
  EXPECT_NE(first.generation, second.generation);
}

TEST(NullRendererTest, RenderFrameRecordsTheFrame) {
  Eden::NullRenderer renderer;
  Eden::RenderFrame frame{};
  frame.clearColor = Eden::Color{1.0f, 0.0f, 0.0f, 1.0f};

  renderer.renderFrame(frame);

  EXPECT_EQ(renderer.frameCount(), 1u);
  EXPECT_FLOAT_EQ(renderer.lastFrame().clearColor.r, 1.0f);
}

TEST(NullRendererTest, FrameCountIncrementsPerCall) {
  Eden::NullRenderer renderer;
  Eden::RenderFrame frame{};

  renderer.renderFrame(frame);
  renderer.renderFrame(frame);
  renderer.renderFrame(frame);

  EXPECT_EQ(renderer.frameCount(), 3u);
}

TEST(NullRendererTest, RequestResizeRecordsDimensions) {
  Eden::NullRenderer renderer;

  renderer.requestResize(800, 600);

  EXPECT_EQ(renderer.lastResizeWidth(), 800u);
  EXPECT_EQ(renderer.lastResizeHeight(), 600u);
}

TEST(NullRendererTest, SatisfiesRendererContractPolymorphically) {
  std::unique_ptr<Eden::Renderer> renderer = std::make_unique<Eden::NullRenderer>();
  const std::array<Eden::Vertex, 3> vertices{};

  const auto handle = renderer->createMesh(Eden::MeshDesc{vertices});
  EXPECT_TRUE(handle.valid());

  Eden::RenderFrame frame{};
  EXPECT_NO_THROW(renderer->renderFrame(frame));
}
