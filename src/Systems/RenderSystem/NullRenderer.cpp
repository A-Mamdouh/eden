#include "Eden/Systems/RenderSystem/NullRenderer.hpp"

namespace Eden {

MeshHandle NullRenderer::createMesh(const MeshDesc &desc) {
  NullMesh mesh{};
  mesh.vertexCount = desc.vertices.size();
  mesh.indexCount = desc.indices.size();
  mesh.alive = true;

  std::uint32_t slot{};
  if (!freeMeshSlots_.empty()) {
    slot = freeMeshSlots_.back();
    freeMeshSlots_.pop_back();
    mesh.generation = meshes_[slot].generation + 1;
    meshes_[slot] = mesh;
  } else {
    slot = static_cast<std::uint32_t>(meshes_.size());
    mesh.generation = 1;
    meshes_.push_back(mesh);
  }

  return MeshHandle{slot + 1, meshes_[slot].generation};
}

void NullRenderer::destroyMesh(MeshHandle handle) {
  if (!handle.valid()) {
    return;
  }
  const std::uint32_t slot = handle.id - 1;
  if (slot >= meshes_.size()) {
    return;
  }

  NullMesh &mesh = meshes_[slot];
  if (!mesh.alive || mesh.generation != handle.generation) {
    return;
  }

  mesh.alive = false;
  freeMeshSlots_.push_back(slot);
}

TextureHandle NullRenderer::createTexture(const TextureDesc &desc) {
  NullTexture texture{};
  texture.width = desc.width;
  texture.height = desc.height;
  texture.alive = true;

  std::uint32_t slot{};
  if (!freeTextureSlots_.empty()) {
    slot = freeTextureSlots_.back();
    freeTextureSlots_.pop_back();
    texture.generation = textures_[slot].generation + 1;
    textures_[slot] = texture;
  } else {
    slot = static_cast<std::uint32_t>(textures_.size());
    texture.generation = 1;
    textures_.push_back(texture);
  }

  return TextureHandle{slot + 1, textures_[slot].generation};
}

void NullRenderer::destroyTexture(TextureHandle handle) {
  if (!handle.valid()) {
    return;
  }
  const std::uint32_t slot = handle.id - 1;
  if (slot >= textures_.size()) {
    return;
  }

  NullTexture &texture = textures_[slot];
  if (!texture.alive || texture.generation != handle.generation) {
    return;
  }

  texture.alive = false;
  freeTextureSlots_.push_back(slot);
}

void NullRenderer::renderFrame(const RenderFrame &frame) {
  lastFrame_ = frame;
  ++frameCount_;
}

void NullRenderer::requestResize(std::uint32_t width, std::uint32_t height) {
  lastResizeWidth_ = width;
  lastResizeHeight_ = height;
}

ApplyResult NullRenderer::applySettings(const RenderSettings &settings) {
  settings_ = settings;
  return ApplyResult::Applied;
}

RendererCapabilities NullRenderer::queryCapabilities() const {
  // No real hardware limit to report; claims the highest level so callers
  // exercising the full settings range against this backend aren't
  // artificially clamped.
  return RendererCapabilities{AntiAliasing::MSAA8x};
}

} // namespace Eden
