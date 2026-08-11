#pragma once

#include "Eden/Core/Math.hpp"
#include "Eden/Systems/RenderSystem/Material.hpp"
#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

#include <vector>

namespace Eden {

/// One mesh+material piece of a Model, positioned relative to the Model's
/// owning entity by localTransform (baked from the source asset's node
/// hierarchy at load time -- see Engine::loadModel()).
struct ModelPart {
  MeshHandle mesh{};
  MaterialHandle material{};
  Mat4 localTransform{1.0f};
};

/// A multi-part loaded asset, attached to an entity like any other
/// component (paired with a Transform to place it in the world). The
/// owning entity's WorldTransform places every part rigidly as one unit;
/// there's no per-part entity or independent movement (no
/// skinning/animation support yet).
struct Model {
  std::vector<ModelPart> parts;
};

} // namespace Eden
