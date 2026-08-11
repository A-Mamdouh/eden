#pragma once

#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

namespace Eden {

/// One of RenderSystem's small built-in primitive meshes. Stand-in for
/// real asset loading: scene-authored entities reference a shape
/// symbolically instead of a Renderer MeshHandle, so they don't need to
/// know a mesh was already uploaded via Renderer::createMesh().
enum class PrimitiveShape {
  Triangle,
  Quad,
};

/// Marks an entity for RenderSystem to draw each frame, using its
/// WorldTransform (computed by TransformSystem) as the model matrix.
struct Renderable {
  /// Which of RenderSystem's built-in meshes to draw this entity as.
  PrimitiveShape shape{PrimitiveShape::Triangle};
  /// Flat color used in place of per-vertex color when useVertexColor is false.
  Color tint{1.0f, 1.0f, 1.0f, 1.0f};
  /// True: use the mesh's own per-vertex color. False: use `tint`.
  bool useVertexColor{true};
};

} // namespace Eden
