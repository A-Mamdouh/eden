#pragma once

#ifndef EDEN_ENGINE_MATERIAL_HPP
#define EDEN_ENGINE_MATERIAL_HPP

#include <cstdint>

#include "Renderer.hpp"

namespace Eden
{

using TextureId = std::uint32_t;

/**
 * Basic material description used by renderable components.
 *
 * This is intentionally minimal for now. More fields (textures,
 * blending modes, etc.) can be added as the engine grows.
 */
struct Material
{
    Color baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    bool useVertexColor{false};

    // Optional albedo texture handle. A value of 0 means "no texture".
    TextureId albedoTexture{0};
};

} // namespace Eden

#endif // EDEN_ENGINE_MATERIAL_HPP
