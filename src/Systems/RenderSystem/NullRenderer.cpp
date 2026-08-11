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

void NullRenderer::renderFrame(const RenderFrame &frame) {
  lastFrame_ = frame;
  ++frameCount_;
}

void NullRenderer::requestResize(std::uint32_t width, std::uint32_t height) {
  lastResizeWidth_ = width;
  lastResizeHeight_ = height;
}

} // namespace Eden
