#pragma once

#include "Eden/Systems/RenderSystem/Material.hpp"
#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

namespace Eden {

/// Marks an entity for RenderSystem to draw each frame, using its
/// WorldTransform (computed by TransformSystem) as the model matrix.
struct Renderable {
  /// Mesh to draw; created via Engine::createMesh() (or a model loader).
  MeshHandle mesh{};
  /// Shading parameters; created via Engine::createMaterial(). An invalid
  /// handle draws with Material's defaults (white tint, vertex color).
  MaterialHandle material{};
};

/// Optional per-entity override for Material::tint, and forces
/// useVertexColor off while present. Lets a script animate one entity's
/// color (e.g. a pulse) without creating a new Material or mutating the
/// shared one other entities may reference.
struct TintOverride {
  Color tint{1.0f, 1.0f, 1.0f, 1.0f};
};

} // namespace Eden
