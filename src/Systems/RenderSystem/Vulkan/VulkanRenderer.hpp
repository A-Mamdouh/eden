#pragma once

#include "Eden/Systems/RenderSystem/Renderer.hpp"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace Eden::Rendering {

/// Vulkan implementation of the Renderer contract. Single frame in
/// flight (one active command buffer, one fence), host-visible/coherent memory
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
    /// Initial anti-aliasing/vsync/light limit;
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
  /// One acquired swapchain image and the command buffer shared by all passes.
  struct FrameContext {
    std::uint32_t imageIndex{0};
    vk::CommandBuffer commandBuffer{};
  };

  /// Scene-owned resources, rebuilt together when the swapchain changes.
  /// Swapchain image views and shared descriptors are borrowed from the renderer.
  struct SceneRenderPass {
    vk::RenderPass renderPass{};
    vk::PipelineLayout pipelineLayout{};
    vk::Pipeline pipeline{};
    /// Missing shaders leave a valid clear-only pass.
    bool pipelineReady{false};
    std::vector<vk::Framebuffer> framebuffers{};

    vk::Format depthFormat{};
    vk::Image depthImage{};
    vk::DeviceMemory depthMemory{};
    vk::ImageView depthView{};

    /// Multisampled color attachment; null when anti-aliasing is off.
    vk::Image colorImage{};
    vk::DeviceMemory colorMemory{};
    vk::ImageView colorView{};
  };

  /// Waits/acquires and begins recording; nullopt skips the rest of the frame.
  std::optional<FrameContext> beginFrame();
  /// Uploads camera data and capped lights once, growing the light buffer as
  /// needed after the previous frame has completed.
  void prepareFrameResources(const RenderFrame &frame);
  /// Records only this pass; the enclosing command buffer stays open for others.
  void recordScenePass(const SceneRenderPass &pass, const RenderFrame &frame,
                       const FrameContext &context);
  /// Ends recording, submits all passes together, and presents the image.
  void endFrame(const FrameContext &context);

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
  /// Creates a complete scene pass using the current swapchain and settings.
  /// The caller must ensure previous GPU uses have completed before rebuilding.
  void createScenePass(SceneRenderPass &pass);
  /// Releases a complete or partially created pass and resets all its state.
  /// The caller must ensure previous GPU uses have completed.
  void destroyScenePass(SceneRenderPass &pass);
  /// @return A depth format supported by physicalDevice_
  ///         for optimal-tiling depth-stencil attachments; throws if none is.
  vk::Format findDepthFormat() const;
  /// Creates the pass's depth attachment, sized to swapchainExtent_.
  /// Device-local: only cleared/tested by the GPU each frame.
  void createDepthResources(SceneRenderPass &pass);
  /// Creates the pass's multisampled color target, resolved into the swapchain
  /// image. Leaves the handles null when sampleCount() is 1 (anti-aliasing off).
  void createColorResources(SceneRenderPass &pass);
  /// Creates the scene render pass: color + depth attachments, plus a resolve
  /// attachment when sampleCount() > 1.
  void createRenderPass(SceneRenderPass &pass);
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
  /// Creates set 0: camera/ambient/count UBO at binding 0 (vertex + fragment),
  /// and a runtime-sized light storage buffer at binding 1 (fragment only).
  void createFrameDescriptorSetLayout();
  /// Allocates frameUniformBuffer_/frameUniformBufferMemory_, sized for
  /// one FrameUBO; host-visible/coherent like every other buffer this
  /// renderer creates (see createBuffer()). Contents are (re)written every
  /// frame by prepareFrameResources(), never resized.
  void createFrameUniformBuffer();
  /// Grows the light storage buffer geometrically and updates its descriptor.
  /// Keeps at least one entry allocated for an empty scene. Requires previous
  /// frame completion; throws if the requested data exceeds the device range.
  void ensureLightBufferCapacity(std::size_t lightCount);
  /// Creates frameDescriptorPool_, sized for exactly the one set
  /// frameDescriptorSet_ needs.
  void createFrameDescriptorPool();
  /// Allocates frameDescriptorSet_ and binds the uniform and light buffers.
  void createFrameDescriptorSet();
  /// Refreshes buffer bindings after creation or light-buffer growth.
  void updateFrameDescriptorSet();
  /// Builds the mesh pipeline from mesh.vert/frag SPIR-V under EDEN_SHADER_DIR.
  /// Called for an empty pass by createScenePass(). Leaves pipelineReady false
  /// (not an error) if the shader binaries can't be loaded.
  void createGraphicsPipeline(SceneRenderPass &pass);
  /// Creates one framebuffer per entry in swapchainImageViews_.
  void createFramebuffers(SceneRenderPass &pass);
  /// Creates commandPool_ for the graphics queue family.
  void createCommandPool();
  /// (Re)allocates one primary command buffer per swapchainImages_ entry.
  void allocateCommandBuffers();
  /// Creates imageAvailableSemaphore_, inFlightFence_, and the
  /// per-swapchain-image renderFinishedSemaphores_.
  void createSyncObjects();
  /// (Re)creates renderFinishedSemaphores_ sized to swapchainImages_,
  /// destroying any existing ones first.
  void createRenderFinishedSemaphores();
  void destroyRenderFinishedSemaphores();

  /// Destroys the scene pass, command buffers, and swapchain images/views.
  /// Requires an idle device; safe to call repeatedly.
  void cleanupSwapchain();
  /// Waits out a minimized/zero-size window, then rebuilds every
  /// swapchain-dependent object (swapchain, views, render pass,
  /// pipeline, framebuffers, command buffers, per-image semaphores).
  void recreateSwapchain();

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
  /// Wraps precompiled SPIR-V `code` in an automatically released shader module.
  vk::UniqueShaderModule createShaderModule(const std::vector<std::uint32_t> &code) const;

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

  /// Live anti-aliasing/vsync/light limit. Updated by applySettings().
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

  SceneRenderPass scenePass_{};

  vk::DescriptorSetLayout descriptorSetLayout_{};
  vk::Sampler textureSampler_{};
  // Fixed-size pool (see createDescriptorPool()'s comment); not resized as
  // textures come and go.
  vk::DescriptorPool descriptorPool_{};

  /// Shared frame descriptors and camera/ambient/count UBO. Independent of
  /// the swapchain, as is the growable light storage buffer below.
  vk::DescriptorSetLayout frameDescriptorSetLayout_{};
  vk::DescriptorPool frameDescriptorPool_{};
  vk::DescriptorSet frameDescriptorSet_{};
  vk::Buffer frameUniformBuffer_{};
  vk::DeviceMemory frameUniformBufferMemory_{};

  vk::Buffer lightBuffer_{};
  vk::DeviceMemory lightBufferMemory_{};
  std::size_t lightBufferCapacity_{0};
  /// Physical device's maxStorageBufferRange; checked before allocation.
  vk::DeviceSize maxLightBufferSize_{0};

  vk::CommandPool commandPool_{};
  /// Indexed by swapchain image index; each buffer records all passes.
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
  /// 1x1 white texture created by createDefaultTexture(); recordScenePass()
  /// falls back to this when a DrawCommand's texture handle doesn't resolve.
  TextureHandle defaultTexture_{};
};

} // namespace Eden::Rendering
