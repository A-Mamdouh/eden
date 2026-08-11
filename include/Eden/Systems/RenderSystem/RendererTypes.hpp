#pragma once

#include "Eden/Core/Math.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace Eden {

/// Opaque, backend-owned resource id. Tag-templated so e.g. a MeshHandle
/// can't be passed where a TextureHandle is expected. `generation` lets a
/// backend detect a stale handle after the id is reused by a new resource.
template <typename Tag>
struct Handle {
  /// 0 means invalid/unset; a live resource's id is always non-zero.
  std::uint32_t id{0};
  /// Bumped by the backend each time id is reused for a new resource.
  std::uint32_t generation{0};

  /// @return True if id is non-zero. Does not confirm the resource is
  ///         still alive on the backend -- only the backend can check
  ///         that, via generation.
  constexpr bool valid() const noexcept { return id != 0; }
  /// Equal only if both id and generation match.
  friend constexpr bool operator==(const Handle &, const Handle &) = default;
};

/// Tag type for MeshHandle; never instantiated.
struct MeshTag {};
/// Handle to a GPU mesh resource created via Renderer::createMesh().
using MeshHandle = Handle<MeshTag>;

/// RGBA color in [0, 1] per channel; not gamma-corrected.
struct Color {
  /// Red channel.
  float r{0.0f};
  /// Green channel.
  float g{0.0f};
  /// Blue channel.
  float b{0.0f};
  /// Opacity; only meaningful where a backend actually blends (the
  /// current Vulkan pipeline has blending disabled, so this is unused
  /// in practice today).
  float a{1.0f};
};

/// Single engine-wide vertex layout; every backend must agree on it.
struct Vertex {
  /// Object-space position.
  Vec3 position{};
  /// Per-vertex color; only used when a DrawCommand sets useVertexColor.
  Color color{};
};

/// Describes a mesh to upload via Renderer::createMesh(). Spans only need
/// to stay valid for that call. Empty `indices` means non-indexed draw.
struct MeshDesc {
  /// Vertex data, in draw order for a non-indexed draw.
  std::span<const Vertex> vertices;
  /// Optional index buffer; empty means draw `vertices` directly in order.
  std::span<const std::uint32_t> indices{};
};

/// View and projection matrices for one frame; glm convention (Y-up),
/// backends are responsible for any Y-flip their clip space needs.
struct CameraDesc {
  /// World-to-camera transform, e.g. from glm::lookAt.
  Mat4 view{1.0f};
  /// Camera-to-clip transform, e.g. built with one of glm's projection helpers.
  Mat4 projection{1.0f};
};

/// One instance to draw. `useVertexColor` picks between the mesh's own
/// per-vertex color and `tint`.
struct DrawCommand {
  /// Mesh to draw; a handle failing Renderer-side validation is skipped.
  MeshHandle mesh{};
  /// Model matrix; combined with the frame's camera as projection * view * transform.
  Mat4 transform{1.0f};
  /// Flat color used in place of per-vertex color when useVertexColor is false.
  Color tint{1.0f, 1.0f, 1.0f, 1.0f};
  /// True: use each vertex's own Vertex::color. False: use `tint` for
  /// the whole mesh.
  bool useVertexColor{true};
};

/// Everything Renderer::renderFrame() needs for one frame. Renderer never
/// sees a Scene, entt::registry, or ECS component type -- only this
/// struct, built externally by whoever walks the scene.
struct RenderFrame {
  /// Camera used for every command in this frame; RenderFrame carries
  /// exactly one, so multi-viewport/multi-camera rendering needs one
  /// RenderFrame (and one renderFrame() call) per camera.
  CameraDesc camera{};
  /// Color the backend clears the frame to before drawing commands.
  Color clearColor{0.0f, 0.0f, 0.0f, 1.0f};
  /// Draw commands in submission order; backends may reorder for
  /// efficiency as long as the visual result is equivalent.
  std::vector<DrawCommand> commands{};
};

} // namespace Eden
