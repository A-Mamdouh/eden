#pragma once

#include "Eden/Systems/RenderSystem/Model.hpp"

#include <string>

namespace Eden {

class RenderSystem;

/// Parses the glTF/GLB file at `path` via cgltf, uploads its meshes and
/// textures through `renderSystem`, creates one Material per glTF
/// material, and returns one ModelPart per mesh primitive in the file --
/// each carrying its node's world transform within the file (i.e.
/// relative to whatever entity the caller attaches the returned Model to)
/// baked in as a plain matrix.
/// @throws std::runtime_error on any parse, buffer-load, image-decode, or
///         unsupported-primitive failure.
Model loadGltfModel(const std::string &path, RenderSystem &renderSystem);

} // namespace Eden
