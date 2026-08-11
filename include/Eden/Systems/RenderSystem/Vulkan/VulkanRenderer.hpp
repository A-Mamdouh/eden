#pragma once

#include "Eden/Systems/RenderSystem/Renderer.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace Eden {

/// Vulkan implementation of the Renderer contract. Single frame in
/// flight (one command buffer, one fence), host-visible/coherent memory
/// for all mesh buffers -- no staging buffers or async transfer, which
/// keeps this simple but means large uploads block the calling thread.
class VulkanRenderer final : public Renderer {
public:
  /// Backend-specific construction parameters; not part of the shared
  /// Renderer interface, see Renderer.hpp.
  struct CreateInfo {
    /// Window to create a Vulkan surface for; must outlive this renderer.
    SDL_Window *window{nullptr};
    /// Requests the VK_LAYER_KHRONOS_validation layer; silently disabled
    /// with a warning if it isn't installed.
    bool enableValidationLayers{false};
  };

  /// Performs the full Vulkan setup sequence (instance through sync
  /// objects); throws std::runtime_error on any failure.
  /// @param createInfo Window handle and validation-layer preference.
  explicit VulkanRenderer(const CreateInfo &createInfo);
  ~VulkanRenderer() override;

  MeshHandle createMesh(const MeshDesc &desc) override;
  void destroyMesh(MeshHandle handle) override;

  void renderFrame(const RenderFrame &frame) override;
  void requestResize(std::uint32_t width, std::uint32_t height) override;

private:
  /// GPU-side storage for one createMesh() call. `alive` and
  /// `generation` implement the same slot-reuse scheme as MeshHandle.
  struct GpuMesh {
    vk::Buffer vertexBuffer{};
    vk::DeviceMemory vertexMemory{};
    std::uint32_t vertexCount{0};
    /// Null if this mesh was created without indices (non-indexed draw).
    vk::Buffer indexBuffer{};
    vk::DeviceMemory indexMemory{};
    std::uint32_t indexCount{0};
    std::uint32_t generation{0};
    bool alive{false};
  };

  /// Runs every step below in order, once, from the constructor.
  void initVulkan();
  void createInstance();
  /// Creates the SDL/Vulkan surface for window_.
  void createSurface();
  /// Selects the first physical device with graphics+present queue
  /// families, swapchain support, and at least one usable surface
  /// format/present mode.
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapchain();
  void createImageViews();
  void createRenderPass();
  /// (Re)builds the fixed triangle/quad pipeline from the precompiled
  /// triangle.vert/frag SPIR-V under EDEN_SHADER_DIR. Destroys any
  /// existing pipeline/layout first, so it's safe to call again on
  /// swapchain recreation. Leaves pipelineReady_ false (not an error) if
  /// the shader binaries can't be loaded.
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void allocateCommandBuffers();
  /// Creates imageAvailableSemaphore_, inFlightFence_, and the
  /// per-swapchain-image renderFinishedSemaphores_.
  void createSyncObjects();
  /// (Re)creates renderFinishedSemaphores_ sized to swapchainImages_,
  /// destroying any existing ones first.
  void createRenderFinishedSemaphores();
  void destroyRenderFinishedSemaphores();

  void cleanupSwapchain();
  /// Waits out a minimized/zero-size window, then rebuilds every
  /// swapchain-dependent object (swapchain, views, render pass,
  /// pipeline, framebuffers, command buffers, per-image semaphores).
  void recreateSwapchain();
  /// Records the render pass and every frame.commands draw into
  /// commandBuffer for framebuffer imageIndex.
  void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::uint32_t imageIndex,
                           const RenderFrame &frame);

  std::uint32_t findMemoryType(std::uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
  /// Allocates a host-visible/coherent buffer of `size` bytes usable as
  /// `usage`; caller uploads via uploadToBuffer().
  std::pair<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size,
                                                        vk::BufferUsageFlags usage) const;
  void uploadToBuffer(vk::DeviceMemory memory, const void *data, vk::DeviceSize size) const;
  /// @param filename Shader binary name relative to EDEN_SHADER_DIR.
  /// @return SPIR-V words, or empty on any I/O failure (logged, not thrown).
  std::vector<std::uint32_t> loadShaderBinary(const std::string &filename) const;
  vk::ShaderModule createShaderModule(const std::vector<std::uint32_t> &code) const;

  /// @return The live GpuMesh for `handle`, or nullptr if it's invalid,
  ///         out of range, destroyed, or from a reused (stale) slot.
  const GpuMesh *findMesh(MeshHandle handle) const;

  SDL_Window *window_{nullptr};
  bool enableValidationLayers_{false};
  /// Set by requestResize(); consumed (and cleared) at the start of the
  /// next renderFrame().
  bool framebufferResized_{false};

  vk::Instance instance_{};
  vk::SurfaceKHR surface_{};
  vk::PhysicalDevice physicalDevice_{};
  vk::Device device_{};
  vk::Queue graphicsQueue_{};
  vk::Queue presentQueue_{};

  vk::SwapchainKHR swapchain_{};
  vk::Format swapchainImageFormat_{};
  vk::Extent2D swapchainExtent_{};
  std::vector<vk::Image> swapchainImages_{};
  std::vector<vk::ImageView> swapchainImageViews_{};

  vk::RenderPass renderPass_{};
  std::vector<vk::Framebuffer> swapchainFramebuffers_{};

  vk::PipelineLayout pipelineLayout_{};
  vk::Pipeline graphicsPipeline_{};
  /// False if shader loading failed; renderFrame() then clears the
  /// screen but skips drawing (rather than crashing).
  bool pipelineReady_{false};

  vk::CommandPool commandPool_{};
  std::vector<vk::CommandBuffer> commandBuffers_{};

  vk::Semaphore imageAvailableSemaphore_{};
  vk::Fence inFlightFence_{};

  // One per swapchain image, not one shared semaphore: the present engine's
  // consumption of a signal semaphore isn't tracked by inFlightFence_, so a
  // single semaphore can be re-signaled while a prior present is still
  // reading it. Recreated whenever the swapchain image count can change.
  std::vector<vk::Semaphore> renderFinishedSemaphores_{};

  // Slot index (id - 1) into meshes_; freeMeshSlots_ recycles destroyed
  // slots. generation guards against a stale handle referencing a slot
  // that has since been reused by a different mesh.
  std::vector<GpuMesh> meshes_{};
  std::vector<std::uint32_t> freeMeshSlots_{};
};

} // namespace Eden
