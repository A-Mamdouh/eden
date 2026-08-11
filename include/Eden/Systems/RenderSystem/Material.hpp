#pragma once

#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

namespace Eden {

/// Tag type for MaterialHandle; never instantiated.
struct MaterialTag {};
/// Handle to a Material created via Engine::createMaterial(). Unlike
/// MeshHandle/TextureHandle, this isn't a Renderer-owned GPU resource --
/// RenderSystem stores the Material and resolves the handle into a
/// DrawCommand's texture/tint/useVertexColor fields each frame.
using MaterialHandle = Handle<MaterialTag>;

/// Shading parameters for a Renderable, created via Engine::createMaterial().
struct Material {
  /// Texture to sample; invalid draws with the backend's default white
  /// texture (see Renderer::createTexture()'s contract).
  TextureHandle texture{};
  /// Flat color used in place of per-vertex color when useVertexColor is false.
  Color tint{1.0f, 1.0f, 1.0f, 1.0f};
  /// True: use the mesh's own per-vertex color. False: use `tint`.
  bool useVertexColor{true};
};

} // namespace Eden
