#include <Eden/Systems/RenderSystem/NullRenderer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>

using namespace Eden::Rendering;

TEST(NullRendererTest, CreateMeshReturnsValidHandle) {
  NullRenderer renderer;
  const std::array<Vertex, 3> vertices{};

  const auto handle = renderer.createMesh(MeshDesc{vertices});

  EXPECT_TRUE(handle.valid());
}

TEST(NullRendererTest, DestroyMeshThenDestroyAgainIsSafe) {
  NullRenderer renderer;
  const std::array<Vertex, 3> vertices{};
  const auto handle = renderer.createMesh(MeshDesc{vertices});

  renderer.destroyMesh(handle);

  EXPECT_NO_THROW(renderer.destroyMesh(handle));
}

TEST(NullRendererTest, DestroyingAnInvalidHandleIsSafe) {
  NullRenderer renderer;

  EXPECT_NO_THROW(renderer.destroyMesh(MeshHandle{}));
}

TEST(NullRendererTest, HandleGenerationChangesWhenSlotIsReused) {
  NullRenderer renderer;
  const std::array<Vertex, 3> vertices{};

  const auto first = renderer.createMesh(MeshDesc{vertices});
  renderer.destroyMesh(first);
  const auto second = renderer.createMesh(MeshDesc{vertices});

  EXPECT_EQ(first.id, second.id);
  EXPECT_NE(first.generation, second.generation);
}

TEST(NullRendererTest, CreateTextureReturnsValidHandle) {
  NullRenderer renderer;
  const std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};

  const auto handle = renderer.createTexture(TextureDesc{1, 1, pixels});

  EXPECT_TRUE(handle.valid());
}

TEST(NullRendererTest, DestroyTextureThenDestroyAgainIsSafe) {
  NullRenderer renderer;
  const std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};
  const auto handle = renderer.createTexture(TextureDesc{1, 1, pixels});

  renderer.destroyTexture(handle);

  EXPECT_NO_THROW(renderer.destroyTexture(handle));
}

TEST(NullRendererTest, DestroyingAnInvalidTextureHandleIsSafe) {
  NullRenderer renderer;

  EXPECT_NO_THROW(renderer.destroyTexture(TextureHandle{}));
}

TEST(NullRendererTest, TextureHandleGenerationChangesWhenSlotIsReused) {
  NullRenderer renderer;
  const std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};

  const auto first = renderer.createTexture(TextureDesc{1, 1, pixels});
  renderer.destroyTexture(first);
  const auto second = renderer.createTexture(TextureDesc{1, 1, pixels});

  EXPECT_EQ(first.id, second.id);
  EXPECT_NE(first.generation, second.generation);
}

TEST(NullRendererTest, RenderFrameRecordsTheFrame) {
  NullRenderer renderer;
  RenderFrame frame{};
  frame.clearColor = Color{1.0f, 0.0f, 0.0f, 1.0f};

  renderer.renderFrame(frame);

  EXPECT_EQ(renderer.frameCount(), 1u);
  EXPECT_FLOAT_EQ(renderer.lastFrame().clearColor.r, 1.0f);
}

TEST(NullRendererTest, FrameCountIncrementsPerCall) {
  NullRenderer renderer;
  RenderFrame frame{};

  renderer.renderFrame(frame);
  renderer.renderFrame(frame);
  renderer.renderFrame(frame);

  EXPECT_EQ(renderer.frameCount(), 3u);
}

TEST(NullRendererTest, RequestResizeRecordsDimensions) {
  NullRenderer renderer;

  renderer.requestResize(800, 600);

  EXPECT_EQ(renderer.lastResizeWidth(), 800u);
  EXPECT_EQ(renderer.lastResizeHeight(), 600u);
}

TEST(NullRendererTest, SatisfiesRendererContractPolymorphically) {
  std::unique_ptr<Renderer> renderer = std::make_unique<NullRenderer>();
  const std::array<Vertex, 3> vertices{};

  const auto handle = renderer->createMesh(MeshDesc{vertices});
  EXPECT_TRUE(handle.valid());

  RenderFrame frame{};
  EXPECT_NO_THROW(renderer->renderFrame(frame));
}

TEST(NullRendererTest, ConfiguredLightLimitPreservesOrderAndInputFrame) {
  NullRenderer renderer;
  renderer.applySettings(RenderSettings{.maxLights = 3});
  RenderFrame frame{};
  for (int i = 0; i < 40; ++i) {
    frame.lights.push_back(LightDesc{.intensity = static_cast<float>(i)});
  }

  renderer.renderFrame(frame);

  ASSERT_EQ(renderer.lastFrame().lights.size(), 3u);
  EXPECT_FLOAT_EQ(renderer.lastFrame().lights.back().intensity, 2.0f);
  EXPECT_EQ(frame.lights.size(), 40u);
}

TEST(NullRendererTest, LightLimitCanSwitchBetweenFiniteAndUnlimitedAtRuntime) {
  NullRenderer renderer;
  RenderFrame frame{};
  frame.lights.resize(40);

  renderer.renderFrame(frame);
  EXPECT_EQ(renderer.lastFrame().lights.size(), 16u);

  renderer.applySettings(RenderSettings{.maxLights = 0});
  renderer.renderFrame(frame);
  EXPECT_EQ(renderer.lastFrame().lights.size(), 40u);

  renderer.applySettings(RenderSettings{.maxLights = 24});
  renderer.renderFrame(frame);
  EXPECT_EQ(renderer.lastFrame().lights.size(), 24u);

  frame.lights.clear();
  renderer.renderFrame(frame);
  EXPECT_TRUE(renderer.lastFrame().lights.empty());
}
