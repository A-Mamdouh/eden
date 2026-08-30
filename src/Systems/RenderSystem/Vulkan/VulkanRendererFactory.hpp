#pragma once

#include "Eden/Systems/RenderSystem/RendererTypes.hpp"

#include <memory>

struct SDL_Window;

namespace Eden::Rendering {

class Renderer;

/// Constructs the Vulkan Renderer implementation. Declared here instead
/// of requiring callers to include VulkanRenderer.hpp so that no
/// translation unit outside VulkanRenderer.cpp itself ever needs to see
/// a vk:: type -- the seam that keeps this backend's headers from ever
/// coexisting with a future second backend's in the same file.
/// @param window Window to create a Vulkan surface for; must outlive
///        the returned Renderer.
/// @param enableValidationLayers Requests the VK_LAYER_KHRONOS_validation
///        layer; silently disabled with a warning if it isn't installed.
/// @param initialSettings Anti-aliasing/vsync to start with; clamped to
///        whatever the picked device actually supports.
std::unique_ptr<Renderer> createVulkanRenderer(SDL_Window *window, bool enableValidationLayers,
                                               const RenderSettings &initialSettings);

} // namespace Eden::Rendering
