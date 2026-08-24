#pragma once

#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

namespace Eden::Rendering {

/// Tag type for MaterialHandle; never instantiated.
struct MaterialTag {};
/// Handle to a Material created via Engine::createMaterial(). Unlike
/// MeshHandle/TextureHandle, this isn't a Renderer-owned GPU resource --
/// RenderSystem stores the Material and resolves the handle into a
/// DrawCommand's texture/tint/useVertexColor fields each frame.
using MaterialHandle = Handle<MaterialTag>;

/// Shading parameters for a Renderable, created via Engine::createMaterial().
struct Material {
  /// Unlit (default): today's flat texture*(vertex color or
  /// baseColorFactor) shading, no lighting involved. PBR: metallic-roughness
  /// Cook-Torrance, lit by whatever Light entities are in the scene.
  ShadingModel shadingModel{ShadingModel::Unlit};

  /// Texture to sample; invalid draws with the backend's default white
  /// texture (see Renderer::createTexture()'s contract).
  TextureHandle baseColorTexture{};
  /// Flat color used in place of per-vertex color when useVertexColor is
  /// false (Unlit), or the multiplier applied to baseColorTexture (PBR).
  Color baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  /// True: use the mesh's own per-vertex color. False: use
  /// `baseColorFactor`. Meaningless (and always false in practice) for PBR.
  bool useVertexColor{true};

  // PBR only; ignored when shadingModel == Unlit.
  /// Green channel = roughness, blue channel = metallic (glTF convention).
  TextureHandle metallicRoughnessTexture{};
  float metallicFactor{1.0f};
  float roughnessFactor{1.0f};
  TextureHandle emissiveTexture{};
  Vec3 emissiveFactor{0.0f, 0.0f, 0.0f};
};

} // namespace Eden::Rendering
