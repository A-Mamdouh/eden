#pragma once

#include <string>

namespace Eden {

class RenderSystem;
class Scene;

/// Parses the glTF/GLB file at `path` via cgltf, uploads its meshes and
/// textures through `renderSystem`, creates one Material per glTF
/// material, and spawns one entity per glTF node (Transform +
/// EntityHierarchy, mirroring the node hierarchy, plus a Renderable on
/// nodes with a mesh) into `scene`.
/// @throws std::runtime_error on any parse, buffer-load, image-decode, or
///         unsupported-primitive failure.
void loadGltfModel(const std::string &path, RenderSystem &renderSystem, Scene &scene);

} // namespace Eden
