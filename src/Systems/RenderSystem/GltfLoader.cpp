#include "GltfLoader.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Eden/Systems/RenderSystem/Material.hpp"
#include "Eden/Systems/RenderSystem/Model.hpp"
#include "Eden/Systems/RenderSystem/RenderSystem.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace Eden::Rendering {
namespace {

/// RAII owner for a cgltf_data*, so early throws don't leak it.
struct GltfDataDeleter {
  void operator()(cgltf_data *data) const noexcept {
    if (data) {
      cgltf_free(data);
    }
  }
};
using GltfDataPtr = std::unique_ptr<cgltf_data, GltfDataDeleter>;

std::vector<std::uint8_t> readFile(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }

  const std::streamsize size = file.tellg();
  std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), size);
  if (!file) {
    throw std::runtime_error("Failed to read file: " + path.string());
  }

  return buffer;
}

/// @return `image`'s texel data decoded to RGBA8, from its embedded
/// bufferView if present, else from a file resolved as `basePath /
/// image.uri`. Throws if neither source is present or decoding fails.
TextureDesc decodeImage(const cgltf_image &image, const std::filesystem::path &basePath,
                        std::vector<std::uint8_t> &pixelStorage) {
  int width = 0;
  int height = 0;
  int sourceChannels = 0;
  stbi_uc *decoded = nullptr;

  if (image.buffer_view) {
    const auto *bufferData = static_cast<const std::uint8_t *>(image.buffer_view->buffer->data) +
                             image.buffer_view->offset;
    decoded = stbi_load_from_memory(bufferData, static_cast<int>(image.buffer_view->size), &width,
                                    &height, &sourceChannels, 4);
  } else if (image.uri) {
    const std::vector<std::uint8_t> fileBytes = readFile(basePath / image.uri);
    decoded = stbi_load_from_memory(fileBytes.data(), static_cast<int>(fileBytes.size()), &width,
                                    &height, &sourceChannels, 4);
  } else {
    throw std::runtime_error("glTF image has neither a bufferView nor a uri");
  }

  if (!decoded) {
    throw std::runtime_error("Failed to decode glTF image: " + std::string(stbi_failure_reason()));
  }

  const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
  pixelStorage.assign(decoded, decoded + byteCount);
  stbi_image_free(decoded);

  TextureDesc desc{};
  desc.width = static_cast<std::uint32_t>(width);
  desc.height = static_cast<std::uint32_t>(height);
  desc.pixels = pixelStorage;
  return desc;
}

/// @return `primitive`'s vertices/indices in Eden's layout. Throws if it
/// isn't a triangle list or is missing POSITION.
struct PrimitiveGeometry {
  std::vector<Vertex> vertices;
  std::vector<std::uint32_t> indices;
};

PrimitiveGeometry extractGeometry(const cgltf_primitive &primitive) {
  if (primitive.type != cgltf_primitive_type_triangles) {
    throw std::runtime_error("Only triangle-list glTF primitives are supported");
  }

  const cgltf_accessor *positions = nullptr;
  const cgltf_accessor *normals = nullptr;
  const cgltf_accessor *texcoords = nullptr;
  for (cgltf_size i = 0; i < primitive.attributes_count; ++i) {
    const cgltf_attribute &attribute = primitive.attributes[i];
    if (attribute.type == cgltf_attribute_type_position) {
      positions = attribute.data;
    } else if (attribute.type == cgltf_attribute_type_normal) {
      normals = attribute.data;
    } else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0) {
      texcoords = attribute.data;
    }
  }

  if (!positions) {
    throw std::runtime_error("glTF primitive is missing a POSITION attribute");
  }
  // Every loaded glTF material shades via the PBR path (see the material
  // loop below), which needs a real normal -- silently substituting a
  // placeholder would shade the mesh plausibly-but-wrong, so this fails
  // loudly instead, same contract as the POSITION check above.
  if (!normals) {
    throw std::runtime_error("glTF primitive is missing a NORMAL attribute");
  }

  const std::size_t vertexCount = positions->count;

  std::vector<float> positionFloats(vertexCount * 3);
  cgltf_accessor_unpack_floats(positions, positionFloats.data(), positionFloats.size());

  std::vector<float> normalFloats(vertexCount * 3);
  cgltf_accessor_unpack_floats(normals, normalFloats.data(), normalFloats.size());

  std::vector<float> texcoordFloats;
  if (texcoords) {
    texcoordFloats.resize(vertexCount * 2);
    cgltf_accessor_unpack_floats(texcoords, texcoordFloats.data(), texcoordFloats.size());
  }

  PrimitiveGeometry geometry{};
  geometry.vertices.resize(vertexCount);
  for (std::size_t i = 0; i < vertexCount; ++i) {
    Vertex &vertex = geometry.vertices[i];
    vertex.position =
        Vec3{positionFloats[i * 3 + 0], positionFloats[i * 3 + 1], positionFloats[i * 3 + 2]};
    vertex.normal = Vec3{normalFloats[i * 3 + 0], normalFloats[i * 3 + 1], normalFloats[i * 3 + 2]};
    // glTF meshes shade via material (texture/factors), not per-vertex
    // color; RenderSystem's Material for this primitive sets
    // useVertexColor = false, so this value is never actually sampled.
    vertex.color = Color{1.0f, 1.0f, 1.0f, 1.0f};
    if (texcoords) {
      vertex.uv = Vec2{texcoordFloats[i * 2 + 0], texcoordFloats[i * 2 + 1]};
    }
  }

  if (primitive.indices) {
    geometry.indices.resize(primitive.indices->count);
    cgltf_accessor_unpack_indices(primitive.indices, geometry.indices.data(), sizeof(std::uint32_t),
                                  geometry.indices.size());
  }

  return geometry;
}

} // namespace

Model loadGltfModel(const std::string &path, Systems::RenderSystem &renderSystem) {
  cgltf_options options{};
  cgltf_data *rawData = nullptr;

  if (cgltf_parse_file(&options, path.c_str(), &rawData) != cgltf_result_success) {
    throw std::runtime_error("Failed to parse glTF file: " + path);
  }
  const GltfDataPtr data{rawData};

  if (cgltf_load_buffers(&options, data.get(), path.c_str()) != cgltf_result_success) {
    throw std::runtime_error("Failed to load glTF buffers for: " + path);
  }

  const std::filesystem::path basePath = std::filesystem::path(path).parent_path();

  // One Eden TextureHandle per cgltf_image actually referenced by a
  // material, decoded (and uploaded) at most once even if several
  // materials share it.
  std::unordered_map<const cgltf_image *, TextureHandle> textures;
  // One Eden MaterialHandle per cgltf_material.
  std::unordered_map<const cgltf_material *, MaterialHandle> materials;

  // Resolves a cgltf_texture_view to an Eden TextureHandle, decoding and
  // uploading through `textures` at most once per cgltf_image even when
  // several material slots (or several materials) reference the same one.
  // Returns an invalid (default) handle for an unset texture view.
  auto resolveTexture = [&](const cgltf_texture_view &view) -> TextureHandle {
    const cgltf_texture *texture = view.texture;
    if (!texture || !texture->image) {
      return TextureHandle{};
    }
    const cgltf_image *image = texture->image;
    auto it = textures.find(image);
    if (it == textures.end()) {
      std::vector<std::uint8_t> pixelStorage;
      const TextureDesc desc = decodeImage(*image, basePath, pixelStorage);
      it = textures.emplace(image, renderSystem.createTexture(desc)).first;
    }
    return it->second;
  };

  for (cgltf_size i = 0; i < data->materials_count; ++i) {
    const cgltf_material &gltfMaterial = data->materials[i];

    Material material{};
    material.shadingModel = ShadingModel::PBR;
    material.useVertexColor = false;

    if (gltfMaterial.has_pbr_metallic_roughness) {
      const cgltf_pbr_metallic_roughness &pbr = gltfMaterial.pbr_metallic_roughness;
      material.baseColorFactor = Color{pbr.base_color_factor[0], pbr.base_color_factor[1],
                                       pbr.base_color_factor[2], pbr.base_color_factor[3]};
      material.baseColorTexture = resolveTexture(pbr.base_color_texture);
      material.metallicFactor = pbr.metallic_factor;
      material.roughnessFactor = pbr.roughness_factor;
      material.metallicRoughnessTexture = resolveTexture(pbr.metallic_roughness_texture);
    }

    material.emissiveFactor =
        Vec3{gltfMaterial.emissive_factor[0], gltfMaterial.emissive_factor[1], gltfMaterial.emissive_factor[2]};
    material.emissiveTexture = resolveTexture(gltfMaterial.emissive_texture);

    materials.emplace(&gltfMaterial, renderSystem.createMaterial(material));
  }

  // One Eden MeshHandle per cgltf_primitive -- a glTF mesh with several
  // primitives (each with its own material) becomes several ModelParts.
  struct PrimitiveHandles {
    MeshHandle mesh{};
    MaterialHandle material{};
  };
  std::unordered_map<const cgltf_primitive *, PrimitiveHandles> primitiveHandles;

  for (cgltf_size i = 0; i < data->meshes_count; ++i) {
    const cgltf_mesh &gltfMesh = data->meshes[i];
    for (cgltf_size p = 0; p < gltfMesh.primitives_count; ++p) {
      const cgltf_primitive &primitive = gltfMesh.primitives[p];
      const PrimitiveGeometry geometry = extractGeometry(primitive);

      PrimitiveHandles handles{};
      handles.mesh = renderSystem.createMesh(MeshDesc{geometry.vertices, geometry.indices});
      if (primitive.material) {
        handles.material = materials.at(primitive.material);
      }

      primitiveHandles.emplace(&primitive, handles);
    }
  }

  // One ModelPart per node's mesh primitive, positioned by the node's
  // world transform within the file -- i.e. relative to whatever entity
  // the caller attaches the returned Model to.
  Model model{};
  for (cgltf_size i = 0; i < data->nodes_count; ++i) {
    const cgltf_node &node = data->nodes[i];
    if (!node.mesh) {
      continue;
    }

    float matrixValues[16];
    cgltf_node_transform_world(&node, matrixValues);
    const Mat4 nodeTransform = glm::make_mat4(matrixValues);

    for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p) {
      const PrimitiveHandles &handles = primitiveHandles.at(&node.mesh->primitives[p]);
      model.parts.push_back(
          ModelPart{.mesh = handles.mesh, .material = handles.material, .localTransform = nodeTransform});
    }
  }

  return model;
}

} // namespace Eden::Rendering
