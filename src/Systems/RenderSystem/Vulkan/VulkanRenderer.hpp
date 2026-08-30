#pragma once

#include "Eden/Systems/RenderSystem/Renderer.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace Eden::Rendering {

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
    /// Anti-aliasing/vsync to build the pipeline and swapchain with;
    /// clamped against the picked device's actual limits, same as any
    /// later applySettings() call.
    RenderSettings initialSettings{};
  };

  /// Performs the full Vulkan setup sequence (instance through sync
  /// objects); throws std::runtime_error on any failure.
  /// @param createInfo Window handle and validation-layer preference.
  explicit VulkanRenderer(const CreateInfo &createInfo);
  ~VulkanRenderer() override;

  MeshHandle createMesh(const MeshDesc &desc) override;
  void destroyMesh(MeshHandle handle) override;

  TextureHandle createTexture(const TextureDesc &desc) override;
  void destroyTexture(TextureHandle handle) override;

  void renderFrame(const RenderFrame &frame) override;
  void requestResize(std::uint32_t width, std::uint32_t height) override;
  ApplyResult applySettings(const RenderSettings &settings) override;
  RendererCapabilities queryCapabilities() const override;

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
    /// Mirrors the generation of the MeshHandle that owns this slot.
    std::uint32_t generation{0};
    /// False for a freed slot awaiting reuse by a future createMesh().
    bool alive{false};
  };

  /// GPU-side storage for one createTexture() call. Same slot+generation
  /// reuse scheme as GpuMesh.
  struct GpuTexture {
    vk::Image image{};
    vk::DeviceMemory memory{};
    vk::ImageView view{};
    /// Pre-bound to `image`'s view and the shared textureSampler_; handed
    /// straight to vkCmdBindDescriptorSets at draw time.
    vk::DescriptorSet descriptorSet{};
    /// Mirrors the generation of the TextureHandle that owns this slot.
    std::uint32_t generation{0};
    /// False for a freed slot awaiting reuse by a future createTexture().
    bool alive{false};
  };

  /// Runs every step below in order, once, from the constructor.
  void initVulkan();
  /// Creates instance_, enabling validation layers if requested and available.
  void createInstance();
  /// Creates the SDL/Vulkan surface for window_.
  void createSurface();
  /// Selects the first physical device with graphics+present queue
  /// families, swapchain support, and at least one usable surface
  /// format/present mode.
  void pickPhysicalDevice();
  /// Creates device_ and its graphics/present queues from physicalDevice_.
  void createLogicalDevice();
  /// Creates swapchain_ and populates swapchainImages_/swapchainImageFormat_/swapchainExtent_.
  void createSwapchain();
  /// Creates one image view per entry in swapchainImages_.
  void createImageViews();
  /// @return A depthFormat_-eligible format supported by physicalDevice_
  ///         for optimal-tiling depth-stencil attachments; throws if none is.
  vk::Format findDepthFormat() const;
  /// Creates depthImage_/depthImageMemory_/depthImageView_, sized to
  /// swapchainExtent_. Device-local (not host-visible): never written
  /// from the CPU, only cleared/tested by the GPU each frame.
  void createDepthResources();
  /// Creates colorImage_/colorImageMemory_/colorImageView_, the
  /// multisampled color target the render pass resolves into the
  /// swapchain image. No-ops (leaves them null) when sampleCount() is 1,
  /// i.e. anti-aliasing is off.
  void createColorResources();
  /// Creates renderPass_: color + depth attachments, plus a resolve
  /// attachment when sampleCount() > 1.
  void createRenderPass();
  /// Creates descriptorSetLayout_: one combined-image-sampler binding,
  /// fragment stage, matching `layout(set = N, binding = 0) uniform
  /// sampler2D` in mesh.frag -- reused at three different pipeline-layout
  /// set indices (baseColor/metallicRoughness/emissive), see
  /// createGraphicsPipeline().
  void createDescriptorSetLayout();
  /// Creates textureSampler_: linear filtering, repeat addressing --
  /// reasonable defaults for glTF textures, not currently configurable
  /// per-texture.
  void createTextureSampler();
  /// Creates descriptorPool_, sized for kMaxTextures combined-image-sampler
  /// descriptor sets (see its comment for why a fixed cap).
  void createDescriptorPool();
  /// Uploads a 1x1 opaque white texture and stores it as
  /// defaultTexture_, so draws with no texture set still go through the
  /// texture-sampling path in the fragment shader. Reused as the default
  /// for all three material texture roles (baseColor/metallicRoughness/
  /// emissive): `sampled * factor` is multiplicative-identity-safe for
  /// each of them, so one shared white texture needs no per-role variant.
  void createDefaultTexture();
  /// Creates frameDescriptorSetLayout_: one uniform-buffer binding, both
  /// vertex and fragment stages (view/proj feed the former, everything
  /// else -- camera position, ambient, lights -- feeds the latter),
  /// matching `layout(set = 0, binding = 0) uniform FrameUBO` in both
  /// mesh.vert and mesh.frag.
  void createFrameDescriptorSetLayout();
  /// Allocates frameUniformBuffer_/frameUniformBufferMemory_, sized for
  /// one FrameUBO; host-visible/coherent like every other buffer this
  /// renderer creates (see createBuffer()). Contents are (re)written every
  /// frame by updateFrameUniformBuffer(), never resized.
  void createFrameUniformBuffer();
  /// Creates frameDescriptorPool_, sized for exactly the one set
  /// frameDescriptorSet_ needs.
  void createFrameDescriptorPool();
  /// Allocates frameDescriptorSet_ and points it at frameUniformBuffer_
  /// once; only the buffer's contents change per frame afterward (see
  /// updateFrameUniformBuffer()), never the descriptor set's binding.
  void createFrameDescriptorSet();
  /// Packs `frame`'s camera/ambient/lights into a FrameUBO and uploads it
  /// to frameUniformBuffer_. Called once per frame, before the draw loop,
  /// from recordCommandBuffer() -- safe without per-frame-in-flight
  /// duplication because renderFrame() already waits on inFlightFence_
  /// (i.e. the previous frame's GPU work is done) before recording begins.
  void updateFrameUniformBuffer(const RenderFrame &frame) const;
  /// (Re)builds the mesh pipeline from the precompiled
  /// mesh.vert/frag SPIR-V under EDEN_SHADER_DIR. Destroys any
  /// existing pipeline/layout first, so it's safe to call again on
  /// swapchain recreation. Leaves pipelineReady_ false (not an error) if
  /// the shader binaries can't be loaded.
  void createGraphicsPipeline();
  /// Creates one framebuffer per entry in swapchainImageViews_.
  void createFramebuffers();
  /// Creates commandPool_ for the graphics queue family.
  void createCommandPool();
  /// (Re)allocates one primary command buffer per swapchainFramebuffers_ entry.
  void allocateCommandBuffers();
  /// Creates imageAvailableSemaphore_, inFlightFence_, and the
  /// per-swapchain-image renderFinishedSemaphores_.
  void createSyncObjects();
  /// (Re)creates renderFinishedSemaphores_ sized to swapchainImages_,
  /// destroying any existing ones first.
  void createRenderFinishedSemaphores();
  void destroyRenderFinishedSemaphores();

  /// Destroys the swapchain and everything sized by its image count
  /// (image views, render pass, framebuffers); safe to call repeatedly.
  void cleanupSwapchain();
  /// Waits out a minimized/zero-size window, then rebuilds every
  /// swapchain-dependent object (swapchain, views, render pass,
  /// pipeline, framebuffers, command buffers, per-image semaphores).
  void recreateSwapchain();
  /// Records the render pass and every frame.commands draw into
  /// commandBuffer for framebuffer imageIndex.
  void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::uint32_t imageIndex,
                           const RenderFrame &frame);

  /// @return Index of a physicalDevice_ memory type matching both
  ///         `typeFilter` (a bitmask from a memory-requirements query)
  ///         and `properties`; throws if none qualifies.
  std::uint32_t findMemoryType(std::uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
  /// Allocates a host-visible/coherent buffer of `size` bytes usable as
  /// `usage`; caller uploads via uploadToBuffer().
  std::pair<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size,
                                                        vk::BufferUsageFlags usage) const;
  /// Maps `memory`, copies `size` bytes from `data`, unmaps. `memory`
  /// must be host-visible/coherent (i.e. from createBuffer()).
  void uploadToBuffer(vk::DeviceMemory memory, const void *data, vk::DeviceSize size) const;

  /// Allocates and begins a primary command buffer from commandPool_ for
  /// a one-shot transfer; caller submits it via endSingleTimeCommands().
  vk::CommandBuffer beginSingleTimeCommands() const;
  /// Ends, submits to graphicsQueue_, waits idle, and frees `commandBuffer`.
  /// Simple (blocking) but fine for the low-frequency createTexture() path.
  void endSingleTimeCommands(vk::CommandBuffer commandBuffer) const;
  /// Records a pipeline barrier moving `image` from `oldLayout` to
  /// `newLayout`; only supports the two transitions createTexture() needs
  /// (undefined -> transferDstOptimal, transferDstOptimal -> shaderReadOnlyOptimal).
  void transitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image,
                             vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;
  /// Records a buffer-to-image copy of a full `width` x `height` region.
  void copyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer buffer, vk::Image image,
                         std::uint32_t width, std::uint32_t height) const;
  /// @param filename Shader binary name relative to EDEN_SHADER_DIR.
  /// @return SPIR-V words, or empty on any I/O failure (logged, not thrown).
  std::vector<std::uint32_t> loadShaderBinary(const std::string &filename) const;
  /// Wraps precompiled SPIR-V `code` in a vk::ShaderModule; caller destroys it.
  vk::ShaderModule createShaderModule(const std::vector<std::uint32_t> &code) const;

  /// @return currentSettings_.antiAliasing translated to a sample count
  ///         and clamped to maxSampleCount_ -- the single source of truth
  ///         every swapchain-dependent object builds against.
  vk::SampleCountFlagBits sampleCount() const;

  /// @return The live GpuMesh for `handle`, or nullptr if it's invalid,
  ///         out of range, destroyed, or from a reused (stale) slot.
  const GpuMesh *findMesh(MeshHandle handle) const;
  /// @return The live GpuTexture for `handle`, or nullptr if it's invalid,
  ///         out of range, destroyed, or from a reused (stale) slot.
  const GpuTexture *findTexture(TextureHandle handle) const;

  SDL_Window *window_{nullptr};
  bool enableValidationLayers_{false};
  /// Set by requestResize(); consumed (and cleared) at the start of the
  /// next renderFrame().
  bool framebufferResized_{false};

  /// Live anti-aliasing/vsync request; sampleCount() and createSwapchain()
  /// both read this. Updated by applySettings().
  RenderSettings currentSettings_{};
  /// Highest MSAA sample count physicalDevice_ supports for both color and
  /// depth attachments, capped at e8 (AntiAliasing's own ceiling); set once
  /// in pickPhysicalDevice().
  vk::SampleCountFlagBits maxSampleCount_{vk::SampleCountFlagBits::e1};

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

  vk::Format depthFormat_{};
  vk::Image depthImage_{};
  vk::DeviceMemory depthImageMemory_{};
  vk::ImageView depthImageView_{};

  /// The multisampled color attachment the render pass resolves into the
  /// swapchain image; null when sampleCount() is 1 (anti-aliasing off).
  vk::Image colorImage_{};
  vk::DeviceMemory colorImageMemory_{};
  vk::ImageView colorImageView_{};

  vk::DescriptorSetLayout descriptorSetLayout_{};
  vk::Sampler textureSampler_{};
  // Fixed-size pool (see createDescriptorPool()'s comment); not resized as
  // textures come and go.
  vk::DescriptorPool descriptorPool_{};

  /// Set 0's layout (camera/ambient/lights UBO), the buffer backing it,
  /// and the one descriptor set bound from it every frame. Created once in
  /// initVulkan(), destroyed once in the destructor -- not swapchain-sized
  /// (unlike descriptorSetLayout_'s per-texture sets, this isn't tied to
  /// swapchain image count or format), so cleanupSwapchain()/
  /// recreateSwapchain() never touch these.
  vk::DescriptorSetLayout frameDescriptorSetLayout_{};
  vk::DescriptorPool frameDescriptorPool_{};
  vk::DescriptorSet frameDescriptorSet_{};
  vk::Buffer frameUniformBuffer_{};
  vk::DeviceMemory frameUniformBufferMemory_{};

  vk::PipelineLayout pipelineLayout_{};
  vk::Pipeline graphicsPipeline_{};
  /// False if shader loading failed; renderFrame() then clears the
  /// screen but skips drawing (rather than crashing).
  bool pipelineReady_{false};

  vk::CommandPool commandPool_{};
  /// Indexed by swapchain image index, like swapchainFramebuffers_.
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

  // Same slot+generation scheme, for textures.
  std::vector<GpuTexture> textures_{};
  std::vector<std::uint32_t> freeTextureSlots_{};
  /// 1x1 white texture created by createDefaultTexture(); recordCommandBuffer()
  /// falls back to this when a DrawCommand's texture handle doesn't resolve.
  TextureHandle defaultTexture_{};
};

} // namespace Eden::Rendering
