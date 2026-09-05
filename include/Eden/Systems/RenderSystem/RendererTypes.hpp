#pragma once

#include "Eden/Core/Math.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Eden::Rendering {

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

/// Tag type for TextureHandle; never instantiated.
struct TextureTag {};
/// Handle to a GPU texture resource created via Renderer::createTexture().
using TextureHandle = Handle<TextureTag>;

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
  /// Object-space normal, expected unit-length. Left zero-initialized
  /// rather than defaulted to (0,0,1): a mesh that forgot to supply one
  /// should shade visibly wrong, not plausible-looking.
  Vec3 normal{0.0f, 0.0f, 0.0f};
  /// Per-vertex color; only used when a DrawCommand sets useVertexColor.
  Color color{};
  /// Texture coordinates; origin top-left, u right, v down.
  Vec2 uv{};
};

/// Describes a mesh to upload via Renderer::createMesh(). Spans only need
/// to stay valid for that call. Empty `indices` means non-indexed draw.
struct MeshDesc {
  /// Vertex data, in draw order for a non-indexed draw.
  std::span<const Vertex> vertices;
  /// Optional index buffer; empty means draw `vertices` directly in order.
  std::span<const std::uint32_t> indices{};
};

/// Describes a texture to upload via Renderer::createTexture(). `pixels`
/// only needs to stay valid for that call.
struct TextureDesc {
  /// Width in texels.
  std::uint32_t width{0};
  /// Height in texels.
  std::uint32_t height{0};
  /// Tightly packed RGBA8 texel data, row-major top-to-bottom;
  /// `width * height * 4` bytes.
  std::span<const std::uint8_t> pixels;
};

/// View and projection matrices for one frame; glm convention (Y-up),
/// backends are responsible for any Y-flip their clip space needs.
struct CameraDesc {
  /// World-to-camera transform, e.g. from glm::lookAt.
  Mat4 view{1.0f};
  /// Camera-to-clip transform, e.g. built with one of glm's projection helpers.
  Mat4 projection{1.0f};
  /// World-space camera position; PBR shading's view vector needs this
  /// directly rather than deriving it back out of `view`.
  Vec3 position{0.0f};
};

/// Selects which lighting math a DrawCommand's fragments go through.
enum class ShadingModel : std::uint8_t {
  /// texture * (vertex color or baseColorFactor), no lighting at all --
  /// today's behavior, kept for hand-authored/flat-tinted geometry.
  Unlit,
  /// Metallic-roughness Cook-Torrance BRDF, lit by RenderFrame::lights.
  PBR,
};

/// Punctual light kind; see Rendering::Components::Light for the
/// scene-facing component this is collected from.
enum class LightType : std::uint8_t { Directional, Point };

/// One light's worth of shading data, resolved from a
/// Rendering::Components::Light + WorldTransform pair.
struct LightDesc {
  LightType type{LightType::Directional};
  /// World-space position; meaningless for Directional.
  Vec3 position{0.0f};
  /// Normalized world-space direction the light travels; meaningless for Point.
  Vec3 direction{0.0f, -1.0f, 0.0f};
  /// Linear color; not gamma-corrected, same convention as Color.
  Vec3 color{1.0f, 1.0f, 1.0f};
  /// Radiometric-ish scale, not physically calibrated -- tune by eye.
  float intensity{1.0f};
  /// Point only: distance at which attenuation reaches zero. 0 = no cutoff.
  float range{0.0f};
};

/// One instance to draw. `useVertexColor` picks between the mesh's own
/// per-vertex color and `baseColorFactor`; `shadingModel` picks whether
/// the rest of the PBR fields have any effect at all.
struct DrawCommand {
  /// Mesh to draw; a handle failing Renderer-side validation is skipped.
  MeshHandle mesh{};
  /// Model matrix; combined with the frame's camera as projection * view * transform.
  Mat4 transform{1.0f};

  ShadingModel shadingModel{ShadingModel::Unlit};

  /// Flat color used in place of per-vertex color when useVertexColor is
  /// false (Unlit), or the multiplier applied to baseColorTexture (PBR).
  Color baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  /// True: use each vertex's own Vertex::color. False: use
  /// `baseColorFactor` for the whole mesh. PBR draws always have this false.
  bool useVertexColor{true};
  /// Texture to sample; invalid (the default) draws with a backend-owned
  /// 1x1 white texture, so untextured draws still go through the same
  /// texture * (vertex color or baseColorFactor) shading path.
  TextureHandle baseColorTexture{};

  // PBR only; ignored when shadingModel == Unlit.
  /// Green channel = roughness, blue channel = metallic (glTF convention).
  TextureHandle metallicRoughnessTexture{};
  float metallicFactor{1.0f};
  float roughnessFactor{1.0f};
  TextureHandle emissiveTexture{};
  Vec3 emissiveFactor{0.0f, 0.0f, 0.0f};
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
  /// Flat ambient term added to every PBR fragment regardless of light
  /// visibility -- a placeholder for real image-based ambient lighting,
  /// which this engine doesn't have yet. rgb = color, a = intensity.
  Color ambientColor{0.03f, 0.03f, 0.035f, 1.0f};
  /// Lights affecting this frame's PBR draws, subject to RenderSettings::maxLights.
  std::vector<LightDesc> lights{};
  /// Draw commands in submission order; backends may reorder for
  /// efficiency as long as the visual result is equivalent.
  std::vector<DrawCommand> commands{};
};

/// Multisample anti-aliasing level. A backend clamps a request it can't
/// satisfy to whatever RendererCapabilities::maxAntiAliasing reports.
enum class AntiAliasing { None, MSAA2x, MSAA4x, MSAA8x };

/// Present-mode preference. Adaptive falls back to On on hardware without
/// relaxed present support; a backend never fails a vsync request outright.
enum class VsyncMode { Off, On, Adaptive };

/// The subset of graphics settings a Renderer itself acts on -- everything
/// else a settings menu exposes (screen mode, resolution, window chrome)
/// is handled by whoever owns the window, never by Renderer.
struct RenderSettings {
  /// Anti-aliasing level to render with.
  AntiAliasing antiAliasing{AntiAliasing::MSAA2x};
  /// Present-mode preference.
  VsyncMode vsync{VsyncMode::On};
  /// Maximum lights to use per frame; 0 means no configured cap.
  /// The backend's available memory and storage-buffer limits still apply.
  std::uint32_t maxLights{16};
};

/// What a Renderer backend can actually do on the current hardware; a
/// settings menu should read this to know which choices are meaningful
/// before offering them, rather than discovering a clamp after the fact.
struct RendererCapabilities {
  /// Highest anti-aliasing level this backend's device actually supports.
  AntiAliasing maxAntiAliasing{AntiAliasing::None};
};

/// Result of Renderer::applySettings().
enum class ApplyResult {
  /// The backend updated itself in place; no caller action needed.
  Applied,
  /// The backend can't satisfy this request without being destroyed and
  /// reconstructed; the caller (RenderSystem) is responsible for that.
  RequiresRecreate,
};

} // namespace Eden::Rendering
