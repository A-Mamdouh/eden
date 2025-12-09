#include "Engine/Camera.hpp"
#include "Engine/Renderer.hpp"
#include "Engine/Window.hpp"
#include "Log.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#ifndef EDEN_SHADER_DIR
#define EDEN_SHADER_DIR ""
#endif

#ifdef EDEN_ENABLE_VULKAN

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.hpp>

namespace Eden {

namespace {

Mat4 makeClipSpaceCorrection() {
  return glm::scale(Mat4(1.0f), Vec3{1.0f, -1.0f, 1.0f});
}

struct Vertex {
  float pos[3];
  float color[3];
};

struct DrawCommand {
  PrimitiveShape shape{PrimitiveShape::Triangle};
  DrawParams params{};
};

struct alignas(16) PushConstants {
  Mat4 mvp{Mat4(1.0f)};
  Color color{};
  int useVertexColor{0};
  float padding[3]{};
};

struct QueueFamilyIndices {
  std::uint32_t graphicsFamily = UINT32_MAX;
  std::uint32_t presentFamily = UINT32_MAX;

  bool isComplete() const noexcept {
    return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
  }
};

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device,
                                     vk::SurfaceKHR surface) {
  QueueFamilyIndices indices;

  const auto families = device.getQueueFamilyProperties();

  for (std::uint32_t i = 0; i < families.size(); ++i) {
    const auto &props = families[i];

    if (props.queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }

    const bool presentSupport =
        device.getSurfaceSupportKHR(i, surface) == VK_TRUE;
    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }
  }

  return indices;
}

vk::SurfaceFormatKHR
chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats) {
  for (const auto &fmt : formats) {
    if (fmt.format == vk::Format::eB8G8R8A8Srgb &&
        fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return fmt;
    }
  }

  return formats.front();
}

vk::PresentModeKHR
choosePresentMode(const std::vector<vk::PresentModeKHR> &modes) {
  for (auto mode : modes) {
    if (mode == vk::PresentModeKHR::eMailbox) {
      return mode;
    }
  }

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &caps,
                              unsigned int width, unsigned int height) {
  if (caps.currentExtent.width != UINT32_MAX) {
    return caps.currentExtent;
  }

  vk::Extent2D actual{static_cast<std::uint32_t>(width),
                      static_cast<std::uint32_t>(height)};

  actual.width = std::max(caps.minImageExtent.width,
                          std::min(caps.maxImageExtent.width, actual.width));
  actual.height = std::max(caps.minImageExtent.height,
                           std::min(caps.maxImageExtent.height, actual.height));

  return actual;
}

class VulkanRenderer final : public Renderer {
public:
  explicit VulkanRenderer(Window &window) : window_(window) { init(); }

  ~VulkanRenderer() override {
    if (device_) {
      device_.waitIdle();
    }

    cleanupSwapchain();

    if (graphicsPipeline_) {
      device_.destroyPipeline(graphicsPipeline_);
    }

    if (pipelineLayout_) {
      device_.destroyPipelineLayout(pipelineLayout_);
    }

    if (vertexBuffer_) {
      device_.destroyBuffer(vertexBuffer_);
    }

    if (vertexBufferMemory_) {
      device_.freeMemory(vertexBufferMemory_);
    }

    if (commandPool_) {
      device_.destroyCommandPool(commandPool_);
    }

    if (imageAvailableSemaphore_) {
      device_.destroySemaphore(imageAvailableSemaphore_);
    }

    if (renderFinishedSemaphore_) {
      device_.destroySemaphore(renderFinishedSemaphore_);
    }

    if (inFlightFence_) {
      device_.destroyFence(inFlightFence_);
    }

    if (device_) {
      device_.destroy();
    }

    if (surface_) {
      instance_.destroySurfaceKHR(surface_);
    }

    if (instance_) {
      instance_.destroy();
    }
  }

  void beginFrame() override {
    if (framebufferResized_) {
      recreateSwapchain();
      framebufferResized_ = false;
    }

    drawCommands_.clear();
    pipelineBound_ = false;

    static_cast<void>(
        device_.waitForFences(inFlightFence_, VK_TRUE, UINT64_MAX));
    device_.resetFences(inFlightFence_);

    auto acquireResult = device_.acquireNextImageKHR(
        swapchain_, UINT64_MAX, imageAvailableSemaphore_, nullptr);

    if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
      recreateSwapchain();
      return;
    }

    currentImageIndex_ = acquireResult.value;

    commandBuffers_[currentImageIndex_].reset();

    vk::CommandBufferBeginInfo beginInfo{};

    commandBuffers_[currentImageIndex_].begin(beginInfo);

    frameInProgress_ = true;
  }

  void endFrame() override {
    if (!frameInProgress_) {
      EDEN_CORE_WARN("endFrame skipped (no frame in progress)");
      return;
    }

    vk::ClearColorValue clearColorValue{std::array<float, 4>{
        clearColor_.r, clearColor_.g, clearColor_.b, clearColor_.a}};

    vk::ClearValue clearValue{};
    clearValue.color = clearColorValue;

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = framebuffers_[currentImageIndex_];
    renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent_;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    commandBuffers_[currentImageIndex_].beginRenderPass(
        renderPassInfo, vk::SubpassContents::eInline);
    if (pipelineReady_ && graphicsPipeline_ && vertexBuffer_) {
      for (const auto &command : drawCommands_) {
        recordDrawCommand(command);
      }
    }
    drawCommands_.clear();

    commandBuffers_[currentImageIndex_].endRenderPass();
    commandBuffers_[currentImageIndex_].end();

    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphore_};
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphore_};

    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentImageIndex_];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    auto submitResult = graphicsQueue_.submit(1, &submitInfo, inFlightFence_);
    if (submitResult != vk::Result::eSuccess) {
      EDEN_CORE_ERROR("Failed to submit Vulkan draw command buffer");
      return;
    }

    vk::SwapchainKHR swapchains[] = {swapchain_};

    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex_;

    auto result = presentQueue_.presentKHR(presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
      framebufferResized_ = true;
    } else if (result != vk::Result::eSuccess) {
      EDEN_CORE_ERROR("Failed to present Vulkan swapchain image");
    }

    frameInProgress_ = false;
  }

  void resize(unsigned int, unsigned int) override {
    framebufferResized_ = true;
  }

  void setViewport(const Viewport &) override {
    // Viewport is currently set dynamically per-frame from swapchain size.
  }

  void setCamera(const Camera &camera) override {
    camera_ = camera;
    viewProjection_ = camera_.projection * camera_.view;
  }

  void clear(const Color &color) override { clearColor_ = color; }

  void submitDrawCommand(PrimitiveShape shape,
                         const DrawParams &params) override {
    drawCommands_.push_back(DrawCommand{shape, params});
  }

private:
  void init() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createCommandBuffers();
    createSyncObjects();
  }
  

  void createInstance() {
    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Eden Demo";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = EDEN_ENGINE_NAME;
    appInfo.engineVersion =
        VK_MAKE_VERSION(EDEN_ENGINE_VERSION_MAJOR, EDEN_ENGINE_VERSION_MINOR,
                        EDEN_ENGINE_VERSION_PATCH);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    SDL_Window *sdlWindow = static_cast<SDL_Window *>(window_.nativeHandle());
    EDEN_ASSERT(sdlWindow != nullptr, "SDL window handle is null");

    unsigned int extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(sdlWindow, &extensionCount, nullptr) !=
        SDL_TRUE) {
      EDEN_CORE_CRITICAL("SDL_Vulkan_GetInstanceExtensions (count) failed: {}",
                         SDL_GetError());
      std::abort();
    }

    std::vector<const char *> extensions(extensionCount);
    if (SDL_Vulkan_GetInstanceExtensions(sdlWindow, &extensionCount,
                                         extensions.data()) != SDL_TRUE) {
      EDEN_CORE_CRITICAL("SDL_Vulkan_GetInstanceExtensions (names) failed: {}",
                         SDL_GetError());
      std::abort();
    }

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount =
        static_cast<std::uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    instance_ = vk::createInstance(createInfo);
  }

  void createSurface() {
    SDL_Window *sdlWindow = static_cast<SDL_Window *>(window_.nativeHandle());
    EDEN_ASSERT(sdlWindow != nullptr, "SDL window handle is null");

    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    if (SDL_Vulkan_CreateSurface(sdlWindow, static_cast<VkInstance>(instance_),
                                 &rawSurface) != SDL_TRUE) {
      EDEN_CORE_CRITICAL("SDL_Vulkan_CreateSurface failed: {}", SDL_GetError());
      std::abort();
    }

    surface_ = rawSurface;
  }

  void pickPhysicalDevice() {
    const auto devices = instance_.enumeratePhysicalDevices();
    EDEN_ASSERT(!devices.empty(), "No Vulkan-compatible GPUs found");

    for (auto device : devices) {
      QueueFamilyIndices indices = findQueueFamilies(device, surface_);
      if (indices.isComplete()) {
        physicalDevice_ = device;
        queueIndices_ = indices;
        break;
      }
    }

    EDEN_ASSERT(physicalDevice_, "Failed to find suitable Vulkan GPU");
  }

  void createLogicalDevice() {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::vector<std::uint32_t> uniqueQueues;
    uniqueQueues.push_back(queueIndices_.graphicsFamily);
    if (queueIndices_.presentFamily != queueIndices_.graphicsFamily) {
      uniqueQueues.push_back(queueIndices_.presentFamily);
    }

    float queuePriority = 1.0f;
    for (std::uint32_t queueFamily : uniqueQueues) {
      vk::DeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};

    vk::DeviceCreateInfo createInfo{};
    createInfo.queueCreateInfoCount =
        static_cast<std::uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    device_ = physicalDevice_.createDevice(createInfo);

    graphicsQueue_ = device_.getQueue(queueIndices_.graphicsFamily, 0);
    presentQueue_ = device_.getQueue(queueIndices_.presentFamily, 0);
  }

  void createSwapchain() {
    const auto caps = physicalDevice_.getSurfaceCapabilitiesKHR(surface_);
    const auto formats = physicalDevice_.getSurfaceFormatsKHR(surface_);
    const auto presentModes =
        physicalDevice_.getSurfacePresentModesKHR(surface_);

    const auto surfaceFormat = chooseSurfaceFormat(formats);
    const auto presentMode = choosePresentMode(presentModes);

    unsigned int width = window_.width();
    unsigned int height = window_.height();
    swapchainExtent_ = chooseSwapExtent(caps, width, height);

    std::uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
      imageCount = caps.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    std::uint32_t queueFamilyIndices[] = {queueIndices_.graphicsFamily,
                                          queueIndices_.presentFamily};

    if (queueIndices_.graphicsFamily != queueIndices_.presentFamily) {
      createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
      createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    swapchain_ = device_.createSwapchainKHR(createInfo);

    swapchainImages_ = device_.getSwapchainImagesKHR(swapchain_);
    swapchainImageFormat_ = surfaceFormat.format;
  }

  void createImageViews() {
    swapchainImageViews_.resize(swapchainImages_.size());

    for (std::size_t i = 0; i < swapchainImages_.size(); ++i) {
      vk::ImageViewCreateInfo createInfo{};
      createInfo.image = swapchainImages_[i];
      createInfo.viewType = vk::ImageViewType::e2D;
      createInfo.format = swapchainImageFormat_;
      createInfo.components = vk::ComponentMapping{
          vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
          vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity};
      createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      swapchainImageViews_[i] = device_.createImageView(createInfo);
    }
  }

  void createRenderPass() {
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat_;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    renderPass_ = device_.createRenderPass(renderPassInfo);
  }

  void createGraphicsPipeline() {
    if (graphicsPipeline_) {
      device_.destroyPipeline(graphicsPipeline_);
      graphicsPipeline_ = vk::Pipeline{};
    }
    if (pipelineLayout_) {
      device_.destroyPipelineLayout(pipelineLayout_);
      pipelineLayout_ = vk::PipelineLayout{};
    }

    const auto vertCode = loadShaderBinary("triangle.vert.spv");
    const auto fragCode = loadShaderBinary("triangle.frag.spv");

    if (vertCode.empty() || fragCode.empty()) {
      EDEN_CORE_ERROR("Failed to load precompiled shader binaries; skipping "
                      "pipeline creation");
      pipelineReady_ = false;
      return;
    }

    vk::ShaderModule vertModule = createShaderModule(vertCode);
    vk::ShaderModule fragModule = createShaderModule(fragCode);

    vk::PipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertStageInfo.module = vertModule;
    vertStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragStageInfo.module = fragModule;
    fragStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo,
                                                        fragStageInfo};

    vk::VertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;

    vk::VertexInputAttributeDescription attributeDescriptions[2]{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.sampleShadingEnable = VK_FALSE;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport,
                                        vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;

    auto result = device_.createGraphicsPipeline(nullptr, pipelineInfo);

    if (result.result != vk::Result::eSuccess) {
      EDEN_CORE_ERROR("Failed to create Vulkan graphics pipeline");
      device_.destroyShaderModule(fragModule);
      device_.destroyShaderModule(vertModule);
      return;
    }

    graphicsPipeline_ = result.value;

    device_.destroyShaderModule(fragModule);
    device_.destroyShaderModule(vertModule);

    pipelineReady_ = true;
  }

  void createFramebuffers() {
    framebuffers_.resize(swapchainImageViews_.size());

    for (std::size_t i = 0; i < swapchainImageViews_.size(); ++i) {
      vk::ImageView attachments[] = {swapchainImageViews_[i]};

      vk::FramebufferCreateInfo framebufferInfo{};
      framebufferInfo.renderPass = renderPass_;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = swapchainExtent_.width;
      framebufferInfo.height = swapchainExtent_.height;
      framebufferInfo.layers = 1;

      framebuffers_[i] = device_.createFramebuffer(framebufferInfo);
    }
  }

  void createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = queueIndices_.graphicsFamily;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    commandPool_ = device_.createCommandPool(poolInfo);
  }

  void createVertexBuffer() {
    std::vector<Vertex> geometry;
    geometry.reserve(triangleTemplate_.size() + quadTemplate_.size());

    triangleVertexOffset_ = 0;
    geometry.insert(geometry.end(), triangleTemplate_.begin(), triangleTemplate_.end());

    quadVertexOffset_ = static_cast<uint32_t>(geometry.size());
    geometry.insert(geometry.end(), quadTemplate_.begin(), quadTemplate_.end());

    const vk::DeviceSize bufferSize =
        sizeof(Vertex) * static_cast<vk::DeviceSize>(geometry.size());

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vertexBuffer_ = device_.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements =
        device_.getBufferMemoryRequirements(vertexBuffer_);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits,
                       vk::MemoryPropertyFlagBits::eHostVisible |
                           vk::MemoryPropertyFlagBits::eHostCoherent);

    vertexBufferMemory_ = device_.allocateMemory(allocInfo);

    device_.bindBufferMemory(vertexBuffer_, vertexBufferMemory_, 0);

    void *data = device_.mapMemory(vertexBufferMemory_, 0, bufferSize);
    std::memcpy(data, geometry.data(), static_cast<std::size_t>(bufferSize));
    device_.unmapMemory(vertexBufferMemory_);
  }

  void createCommandBuffers() {
    commandBuffers_.resize(framebuffers_.size());

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = commandPool_;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount =
        static_cast<std::uint32_t>(commandBuffers_.size());
    commandBuffers_ = device_.allocateCommandBuffers(allocInfo);
  }

  void createSyncObjects() {
    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    imageAvailableSemaphore_ = device_.createSemaphore(semaphoreInfo);
    renderFinishedSemaphore_ = device_.createSemaphore(semaphoreInfo);
    inFlightFence_ = device_.createFence(fenceInfo);
  }

  void cleanupSwapchain() {
    if (!commandBuffers_.empty()) {
      device_.freeCommandBuffers(commandPool_, commandBuffers_);
      commandBuffers_.clear();
    }

    for (auto framebuffer : framebuffers_) {
      if (framebuffer) {
        device_.destroyFramebuffer(framebuffer);
      }
    }
    framebuffers_.clear();

    if (renderPass_) {
      device_.destroyRenderPass(renderPass_);
      renderPass_ = nullptr;
    }

    for (auto imageView : swapchainImageViews_) {
      if (imageView) {
        device_.destroyImageView(imageView);
      }
    }
    swapchainImageViews_.clear();

    if (swapchain_) {
      device_.destroySwapchainKHR(swapchain_);
      swapchain_ = nullptr;
    }
  }

  void recreateSwapchain() {
    if (!device_) {
      return;
    }

    device_.waitIdle();
    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
  }

  vk::ShaderModule
  createShaderModule(const std::vector<std::uint32_t> &code) const {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size() * sizeof(std::uint32_t);
    createInfo.pCode = code.data();

    return device_.createShaderModule(createInfo);
  }

  std::uint32_t findMemoryType(std::uint32_t typeFilter,
                               vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties memProperties =
        physicalDevice_.getMemoryProperties();

    for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
      if ((typeFilter & (1u << i)) != 0u &&
          (memProperties.memoryTypes[i].propertyFlags & properties) ==
              properties) {
        return i;
      }
    }

    EDEN_CORE_CRITICAL("Failed to find suitable Vulkan memory type");
    std::abort();
  }

  void recordDrawCommand(const DrawCommand &command) {
    if (!pipelineBound_) {
      commandBuffers_[currentImageIndex_].bindPipeline(
          vk::PipelineBindPoint::eGraphics, graphicsPipeline_);

      vk::Viewport viewport{};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(swapchainExtent_.width);
      viewport.height = static_cast<float>(swapchainExtent_.height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      vk::Rect2D scissor{};
      scissor.offset = vk::Offset2D{0, 0};
      scissor.extent = swapchainExtent_;

      commandBuffers_[currentImageIndex_].setViewport(0, viewport);
      commandBuffers_[currentImageIndex_].setScissor(0, scissor);

      pipelineBound_ = true;
    }

    PushConstants pushConstants{};

    const Mat4 viewProjModel = viewProjection_ * command.params.modelMatrix;
    pushConstants.mvp = clipSpaceCorrection_ * viewProjModel;
    pushConstants.color = command.params.color;
    pushConstants.useVertexColor = command.params.useVertexColor ? 1 : 0;

    commandBuffers_[currentImageIndex_].pushConstants(
        pipelineLayout_,
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(PushConstants),
        &pushConstants);

    const auto [vertexOffset, vertexCount] = shapeVertexInfo(command.shape);

    vk::Buffer vertexBuffers[] = {vertexBuffer_};
    vk::DeviceSize offsets[] = {0};
    commandBuffers_[currentImageIndex_].bindVertexBuffers(0, 1, vertexBuffers,
                                                          offsets);

    commandBuffers_[currentImageIndex_].draw(vertexCount, 1, vertexOffset, 0);
  }

  std::pair<uint32_t, uint32_t> shapeVertexInfo(PrimitiveShape shape) const {
    if (shape == PrimitiveShape::Triangle) {
      return {triangleVertexOffset_, static_cast<uint32_t>(triangleTemplate_.size())};
    }
    return {quadVertexOffset_, static_cast<uint32_t>(quadTemplate_.size())};
  }

  std::vector<std::uint32_t>
  loadShaderBinary(const std::string &filename) const {
    const std::filesystem::path fullPath =
        std::filesystem::path(EDEN_SHADER_DIR) / filename;

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file) {
      EDEN_CORE_ERROR("Failed to open shader file: {}", fullPath.string());
      return {};
    }

    const std::streamsize size = file.tellg();
    if (size <= 0 || (size % sizeof(std::uint32_t)) != 0) {
      EDEN_CORE_ERROR("Shader file has invalid size: {}", fullPath.string());
      return {};
    }

    std::vector<std::uint32_t> buffer(static_cast<std::size_t>(size) /
                                      sizeof(std::uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), size);
    if (!file) {
      EDEN_CORE_ERROR("Failed to read shader file: {}", fullPath.string());
      return {};
    }

    return buffer;
  }

private:
  Window &window_;

  vk::Instance instance_{};
  vk::SurfaceKHR surface_{};
  vk::PhysicalDevice physicalDevice_{};
  vk::Device device_{};

  QueueFamilyIndices queueIndices_{};
  vk::Queue graphicsQueue_{};
  vk::Queue presentQueue_{};

  vk::SwapchainKHR swapchain_{};
  std::vector<vk::Image> swapchainImages_;
  vk::Format swapchainImageFormat_{vk::Format::eUndefined};
  vk::Extent2D swapchainExtent_{};
  std::vector<vk::ImageView> swapchainImageViews_;
  vk::RenderPass renderPass_{};
  std::vector<vk::Framebuffer> framebuffers_;

  vk::CommandPool commandPool_{};
  std::vector<vk::CommandBuffer> commandBuffers_;

  vk::Semaphore imageAvailableSemaphore_{};
  vk::Semaphore renderFinishedSemaphore_{};
  vk::Fence inFlightFence_{};

  std::uint32_t currentImageIndex_{0};
  bool framebufferResized_{false};
  bool frameInProgress_{false};

  Color clearColor_{0.0f, 0.0f, 0.0f, 1.0f};
  Camera camera_{};
  Mat4 viewProjection_{Mat4(1.0f)};
  Mat4 clipSpaceCorrection_{makeClipSpaceCorrection()};
  vk::PipelineLayout pipelineLayout_{};
  vk::Pipeline graphicsPipeline_{};
  vk::Buffer vertexBuffer_{};
  vk::DeviceMemory vertexBufferMemory_{};
  bool pipelineReady_{false};
  bool pipelineBound_{false};
  std::vector<DrawCommand> drawCommands_;
  uint32_t triangleVertexOffset_{0};
  uint32_t quadVertexOffset_{0};
  const std::array<Vertex, 3> triangleTemplate_{
      Vertex{{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
      Vertex{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
      Vertex{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
  };
  const std::array<Vertex, 6> quadTemplate_{
      Vertex{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
      Vertex{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
      Vertex{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},

      Vertex{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
      Vertex{{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
      Vertex{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
  };
};
} // namespace

std::unique_ptr<Renderer> createVulkanRenderer(Window &window) {
  return std::make_unique<VulkanRenderer>(window);
}

} // namespace Eden

#else // EDEN_ENABLE_VULKAN

namespace Eden {

std::unique_ptr<Renderer> createVulkanRenderer(Window &) {
  EDEN_CORE_WARN("Vulkan support is not enabled; using null renderer");
  return nullptr;
}

} // namespace Eden

#endif // EDEN_ENABLE_VULKAN
