#include "Eden/Systems/RenderSystem/Vulkan/VulkanRenderer.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <string_view>
#include <stdexcept>

#ifndef EDEN_SHADER_DIR
#define EDEN_SHADER_DIR ""
#endif

namespace Eden {
namespace {

constexpr std::array<const char *, 1> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

struct alignas(16) PushConstants {
  Mat4 mvp{Mat4(1.0f)};
  Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
  std::int32_t useVertexColor{0};
};

struct QueueFamilyIndices {
  std::optional<std::uint32_t> graphicsFamily{};
  std::optional<std::uint32_t> presentFamily{};

  bool complete() const noexcept { return graphicsFamily && presentFamily; }
};

struct SwapchainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities{};
  std::vector<vk::SurfaceFormatKHR> formats{};
  std::vector<vk::PresentModeKHR> presentModes{};
};

std::vector<const char *> getRequiredInstanceExtensions(SDL_Window *window,
                                                        bool enableValidation) {
  unsigned int sdlExtensionCount = 0;
  if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr) !=
      SDL_TRUE) {
    throw std::runtime_error(SDL_GetError());
  }

  std::vector<const char *> extensions(sdlExtensionCount);
  if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount,
                                       extensions.data()) != SDL_TRUE) {
    throw std::runtime_error(SDL_GetError());
  }

  if (enableValidation) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

bool checkValidationLayerSupport() {
  const auto available = vk::enumerateInstanceLayerProperties();

  for (const auto *layerName : kValidationLayers) {
    bool found = false;
    for (const auto &props : available) {
      if (std::string_view(props.layerName.data()) == layerName) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device,
                                     vk::SurfaceKHR surface) {
  QueueFamilyIndices indices{};
  const auto families = device.getQueueFamilyProperties();

  for (std::uint32_t i = 0; i < families.size(); ++i) {
    const auto &props = families[i];
    if (props.queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }

    if (device.getSurfaceSupportKHR(i, surface) == VK_TRUE) {
      indices.presentFamily = i;
    }

    if (indices.complete()) {
      break;
    }
  }

  return indices;
}

SwapchainSupportDetails querySwapchainSupport(vk::PhysicalDevice device,
                                             vk::SurfaceKHR surface) {
  SwapchainSupportDetails details{};
  details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
  details.formats = device.getSurfaceFormatsKHR(surface);
  details.presentModes = device.getSurfacePresentModesKHR(surface);
  return details;
}

vk::SurfaceFormatKHR chooseSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &formats) {
  for (const auto &format : formats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb &&
        format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return format;
    }
  }
  return formats.front();
}

vk::PresentModeKHR choosePresentMode(
    const std::vector<vk::PresentModeKHR> &modes) {
  for (const auto mode : modes) {
    if (mode == vk::PresentModeKHR::eMailbox) {
      return mode;
    }
  }
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR &capabilities,
                          SDL_Window *window) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<std::uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  int drawableWidth = 0;
  int drawableHeight = 0;
  SDL_Vulkan_GetDrawableSize(window, &drawableWidth, &drawableHeight);

  vk::Extent2D actual{};
  actual.width = std::clamp<std::uint32_t>(
      static_cast<std::uint32_t>(drawableWidth), capabilities.minImageExtent.width,
      capabilities.maxImageExtent.width);
  actual.height =
      std::clamp<std::uint32_t>(static_cast<std::uint32_t>(drawableHeight),
                                capabilities.minImageExtent.height,
                                capabilities.maxImageExtent.height);

  return actual;
}

} // namespace

VulkanRenderer::VulkanRenderer(const CreateInfo &createInfo)
    : window_{createInfo.window},
      enableValidationLayers_{createInfo.enableValidationLayers} {
  if (!window_) {
    throw std::runtime_error("VulkanRenderer requires a valid SDL_Window");
  }
  initVulkan();
}

VulkanRenderer::~VulkanRenderer() {
  if (device_) {
    device_.waitIdle();
  }

  for (auto &mesh : meshes_) {
    if (!mesh.alive) {
      continue;
    }
    device_.destroyBuffer(mesh.vertexBuffer);
    device_.freeMemory(mesh.vertexMemory);
    if (mesh.indexBuffer) {
      device_.destroyBuffer(mesh.indexBuffer);
      device_.freeMemory(mesh.indexMemory);
    }
  }

  if (inFlightFence_) {
    device_.destroyFence(inFlightFence_);
  }
  destroyRenderFinishedSemaphores();
  if (imageAvailableSemaphore_) {
    device_.destroySemaphore(imageAvailableSemaphore_);
  }

  if (graphicsPipeline_) {
    device_.destroyPipeline(graphicsPipeline_);
  }
  if (pipelineLayout_) {
    device_.destroyPipelineLayout(pipelineLayout_);
  }

  cleanupSwapchain();

  if (commandPool_) {
    device_.destroyCommandPool(commandPool_);
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

void VulkanRenderer::requestResize(std::uint32_t /*width*/, std::uint32_t /*height*/) {
  framebufferResized_ = true;
}

void VulkanRenderer::initVulkan() {
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
  allocateCommandBuffers();
  createSyncObjects();
}

void VulkanRenderer::createInstance() {
  if (enableValidationLayers_ && !checkValidationLayerSupport()) {
    spdlog::warn("Validation layers requested but not available");
    enableValidationLayers_ = false;
  }

  vk::ApplicationInfo appInfo{};
  appInfo.pApplicationName = "Eden";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  appInfo.pEngineName = "Eden";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  const auto extensions =
      getRequiredInstanceExtensions(window_, enableValidationLayers_);

  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount =
      static_cast<std::uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (enableValidationLayers_) {
    createInfo.enabledLayerCount =
        static_cast<std::uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();
  }

  instance_ = vk::createInstance(createInfo);
}

void VulkanRenderer::createSurface() {
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if (SDL_Vulkan_CreateSurface(window_, static_cast<VkInstance>(instance_),
                               &surface) != SDL_TRUE) {
    throw std::runtime_error(SDL_GetError());
  }
  surface_ = surface;
}

void VulkanRenderer::pickPhysicalDevice() {
  const auto devices = instance_.enumeratePhysicalDevices();
  if (devices.empty()) {
    throw std::runtime_error("No Vulkan physical devices found");
  }

  for (const auto &device : devices) {
    const auto indices = findQueueFamilies(device, surface_);

    const auto extensions = device.enumerateDeviceExtensionProperties();
    bool hasSwapchain = false;
    for (const auto &ext : extensions) {
      if (std::string_view(ext.extensionName.data()) ==
          VK_KHR_SWAPCHAIN_EXTENSION_NAME) {
        hasSwapchain = true;
        break;
      }
    }

    bool swapchainAdequate = false;
    if (hasSwapchain) {
      const auto details = querySwapchainSupport(device, surface_);
      swapchainAdequate = !details.formats.empty() && !details.presentModes.empty();
    }

    if (indices.complete() && hasSwapchain && swapchainAdequate) {
      physicalDevice_ = device;
      break;
    }
  }

  if (!physicalDevice_) {
    throw std::runtime_error("Failed to find a suitable GPU");
  }
}

void VulkanRenderer::createLogicalDevice() {
  const auto indices = findQueueFamilies(physicalDevice_, surface_);
  std::set<std::uint32_t> uniqueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  queueCreateInfos.reserve(uniqueFamilies.size());

  const float queuePriority = 1.0f;
  for (const auto family : uniqueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.queueFamilyIndex = family;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  vk::PhysicalDeviceFeatures features{};

  const std::array<const char *, 1> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  vk::DeviceCreateInfo createInfo{};
  createInfo.queueCreateInfoCount =
      static_cast<std::uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &features;
  createInfo.enabledExtensionCount =
      static_cast<std::uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  device_ = physicalDevice_.createDevice(createInfo);

  graphicsQueue_ = device_.getQueue(indices.graphicsFamily.value(), 0);
  presentQueue_ = device_.getQueue(indices.presentFamily.value(), 0);
}

void VulkanRenderer::createSwapchain() {
  const auto details = querySwapchainSupport(physicalDevice_, surface_);

  const auto surfaceFormat = chooseSurfaceFormat(details.formats);
  const auto presentMode = choosePresentMode(details.presentModes);
  const auto extent = chooseExtent(details.capabilities, window_);

  std::uint32_t imageCount = details.capabilities.minImageCount + 1;
  if (details.capabilities.maxImageCount > 0 &&
      imageCount > details.capabilities.maxImageCount) {
    imageCount = details.capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo{};
  createInfo.surface = surface_;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

  const auto indices = findQueueFamilies(physicalDevice_, surface_);
  std::array<std::uint32_t, 2> queueFamilyIndices = {
      indices.graphicsFamily.value(),
      indices.presentFamily.value(),
  };

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount =
        static_cast<std::uint32_t>(queueFamilyIndices.size());
    createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  } else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
  }

  createInfo.preTransform = details.capabilities.currentTransform;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  swapchain_ = device_.createSwapchainKHR(createInfo);

  swapchainImages_ = device_.getSwapchainImagesKHR(swapchain_);
  swapchainImageFormat_ = surfaceFormat.format;
  swapchainExtent_ = extent;
}

void VulkanRenderer::createImageViews() {
  swapchainImageViews_.clear();
  swapchainImageViews_.reserve(swapchainImages_.size());

  for (const auto image : swapchainImages_) {
    vk::ImageViewCreateInfo createInfo{};
    createInfo.image = image;
    createInfo.viewType = vk::ImageViewType::e2D;
    createInfo.format = swapchainImageFormat_;
    createInfo.components.r = vk::ComponentSwizzle::eIdentity;
    createInfo.components.g = vk::ComponentSwizzle::eIdentity;
    createInfo.components.b = vk::ComponentSwizzle::eIdentity;
    createInfo.components.a = vk::ComponentSwizzle::eIdentity;
    createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    swapchainImageViews_.push_back(device_.createImageView(createInfo));
  }
}

void VulkanRenderer::createRenderPass() {
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
  dependency.srcAccessMask = {};
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

std::vector<std::uint32_t> VulkanRenderer::loadShaderBinary(const std::string &filename) const {
  const std::filesystem::path fullPath = std::filesystem::path(EDEN_SHADER_DIR) / filename;

  std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
  if (!file) {
    spdlog::error("Failed to open shader file: {}", fullPath.string());
    return {};
  }

  const std::streamsize size = file.tellg();
  if (size <= 0 || (size % sizeof(std::uint32_t)) != 0) {
    spdlog::error("Shader file has invalid size: {}", fullPath.string());
    return {};
  }

  std::vector<std::uint32_t> buffer(static_cast<std::size_t>(size) / sizeof(std::uint32_t));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), size);
  if (!file) {
    spdlog::error("Failed to read shader file: {}", fullPath.string());
    return {};
  }

  return buffer;
}

vk::ShaderModule VulkanRenderer::createShaderModule(const std::vector<std::uint32_t> &code) const {
  vk::ShaderModuleCreateInfo createInfo{};
  createInfo.codeSize = code.size() * sizeof(std::uint32_t);
  createInfo.pCode = code.data();
  return device_.createShaderModule(createInfo);
}

void VulkanRenderer::createGraphicsPipeline() {
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
    spdlog::error("Failed to load precompiled shader binaries; skipping pipeline creation");
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

  vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

  vk::VertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = vk::VertexInputRate::eVertex;

  vk::VertexInputAttributeDescription attributeDescriptions[2]{};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
  attributeDescriptions[0].offset = offsetof(Vertex, position);

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
  rasterizer.cullMode = vk::CullModeFlagBits::eNone;
  rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.lineWidth = 1.0f;

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

  vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
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

  device_.destroyShaderModule(fragModule);
  device_.destroyShaderModule(vertModule);

  if (result.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create Vulkan graphics pipeline");
  }

  graphicsPipeline_ = result.value;
  pipelineReady_ = true;
}

void VulkanRenderer::createFramebuffers() {
  swapchainFramebuffers_.clear();
  swapchainFramebuffers_.reserve(swapchainImageViews_.size());

  for (const auto view : swapchainImageViews_) {
    vk::FramebufferCreateInfo framebufferInfo{};
    framebufferInfo.renderPass = renderPass_;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &view;
    framebufferInfo.width = swapchainExtent_.width;
    framebufferInfo.height = swapchainExtent_.height;
    framebufferInfo.layers = 1;

    swapchainFramebuffers_.push_back(device_.createFramebuffer(framebufferInfo));
  }
}

void VulkanRenderer::createCommandPool() {
  const auto indices = findQueueFamilies(physicalDevice_, surface_);

  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

  commandPool_ = device_.createCommandPool(poolInfo);
}

void VulkanRenderer::allocateCommandBuffers() {
  if (!commandBuffers_.empty()) {
    device_.freeCommandBuffers(commandPool_, commandBuffers_);
    commandBuffers_.clear();
  }

  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.commandPool = commandPool_;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount =
      static_cast<std::uint32_t>(swapchainFramebuffers_.size());

  commandBuffers_ = device_.allocateCommandBuffers(allocInfo);
}

void VulkanRenderer::createSyncObjects() {
  vk::SemaphoreCreateInfo semaphoreInfo{};
  vk::FenceCreateInfo fenceInfo{};
  fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

  imageAvailableSemaphore_ = device_.createSemaphore(semaphoreInfo);
  inFlightFence_ = device_.createFence(fenceInfo);

  createRenderFinishedSemaphores();
}

void VulkanRenderer::createRenderFinishedSemaphores() {
  destroyRenderFinishedSemaphores();

  renderFinishedSemaphores_.resize(swapchainImages_.size());
  vk::SemaphoreCreateInfo semaphoreInfo{};
  for (auto &semaphore : renderFinishedSemaphores_) {
    semaphore = device_.createSemaphore(semaphoreInfo);
  }
}

void VulkanRenderer::destroyRenderFinishedSemaphores() {
  for (const auto semaphore : renderFinishedSemaphores_) {
    if (semaphore) {
      device_.destroySemaphore(semaphore);
    }
  }
  renderFinishedSemaphores_.clear();
}

std::uint32_t VulkanRenderer::findMemoryType(std::uint32_t typeFilter,
                                             vk::MemoryPropertyFlags properties) const {
  const vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice_.getMemoryProperties();

  for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
    if ((typeFilter & (1u << i)) != 0u &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable Vulkan memory type");
}

std::pair<vk::Buffer, vk::DeviceMemory> VulkanRenderer::createBuffer(
    vk::DeviceSize size, vk::BufferUsageFlags usage) const {
  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  vk::Buffer buffer = device_.createBuffer(bufferInfo);

  const vk::MemoryRequirements memRequirements = device_.getBufferMemoryRequirements(buffer);

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memRequirements.memoryTypeBits,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

  vk::DeviceMemory memory = device_.allocateMemory(allocInfo);
  device_.bindBufferMemory(buffer, memory, 0);

  return {buffer, memory};
}

void VulkanRenderer::uploadToBuffer(vk::DeviceMemory memory, const void *data,
                                    vk::DeviceSize size) const {
  void *mapped = device_.mapMemory(memory, 0, size);
  std::memcpy(mapped, data, static_cast<std::size_t>(size));
  device_.unmapMemory(memory);
}

MeshHandle VulkanRenderer::createMesh(const MeshDesc &desc) {
  GpuMesh mesh{};
  mesh.vertexCount = static_cast<std::uint32_t>(desc.vertices.size());

  const vk::DeviceSize vertexBytes = sizeof(Vertex) * desc.vertices.size();
  std::tie(mesh.vertexBuffer, mesh.vertexMemory) =
      createBuffer(vertexBytes, vk::BufferUsageFlagBits::eVertexBuffer);
  uploadToBuffer(mesh.vertexMemory, desc.vertices.data(), vertexBytes);

  if (!desc.indices.empty()) {
    mesh.indexCount = static_cast<std::uint32_t>(desc.indices.size());
    const vk::DeviceSize indexBytes = sizeof(std::uint32_t) * desc.indices.size();
    std::tie(mesh.indexBuffer, mesh.indexMemory) =
        createBuffer(indexBytes, vk::BufferUsageFlagBits::eIndexBuffer);
    uploadToBuffer(mesh.indexMemory, desc.indices.data(), indexBytes);
  }

  mesh.alive = true;

  std::uint32_t slot{};
  if (!freeMeshSlots_.empty()) {
    slot = freeMeshSlots_.back();
    freeMeshSlots_.pop_back();
    mesh.generation = meshes_[slot].generation + 1;
    meshes_[slot] = mesh;
  } else {
    slot = static_cast<std::uint32_t>(meshes_.size());
    mesh.generation = 1;
    meshes_.push_back(mesh);
  }

  return MeshHandle{slot + 1, meshes_[slot].generation};
}

void VulkanRenderer::destroyMesh(MeshHandle handle) {
  if (!handle.valid()) {
    return;
  }
  const std::uint32_t slot = handle.id - 1;
  if (slot >= meshes_.size()) {
    return;
  }

  GpuMesh &mesh = meshes_[slot];
  if (!mesh.alive || mesh.generation != handle.generation) {
    return;
  }

  device_.destroyBuffer(mesh.vertexBuffer);
  device_.freeMemory(mesh.vertexMemory);
  if (mesh.indexBuffer) {
    device_.destroyBuffer(mesh.indexBuffer);
    device_.freeMemory(mesh.indexMemory);
  }

  mesh.alive = false;
  freeMeshSlots_.push_back(slot);
}

const VulkanRenderer::GpuMesh *VulkanRenderer::findMesh(MeshHandle handle) const {
  if (!handle.valid()) {
    return nullptr;
  }
  const std::uint32_t slot = handle.id - 1;
  if (slot >= meshes_.size()) {
    return nullptr;
  }
  const GpuMesh &mesh = meshes_[slot];
  if (!mesh.alive || mesh.generation != handle.generation) {
    return nullptr;
  }
  return &mesh;
}

void VulkanRenderer::cleanupSwapchain() {
  if (!device_) {
    return;
  }

  for (const auto fb : swapchainFramebuffers_) {
    device_.destroyFramebuffer(fb);
  }
  swapchainFramebuffers_.clear();

  if (renderPass_) {
    device_.destroyRenderPass(renderPass_);
    renderPass_ = nullptr;
  }

  for (const auto view : swapchainImageViews_) {
    device_.destroyImageView(view);
  }
  swapchainImageViews_.clear();

  if (swapchain_) {
    device_.destroySwapchainKHR(swapchain_);
    swapchain_ = nullptr;
  }
}

void VulkanRenderer::recreateSwapchain() {
  int width = 0;
  int height = 0;
  SDL_Vulkan_GetDrawableSize(window_, &width, &height);
  while (width == 0 || height == 0) {
    SDL_Event event{};
    SDL_WaitEventTimeout(&event, 16);
    SDL_Vulkan_GetDrawableSize(window_, &width, &height);
  }

  device_.waitIdle();
  cleanupSwapchain();

  createSwapchain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  allocateCommandBuffers();
  createRenderFinishedSemaphores();
}

void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, std::uint32_t imageIndex,
                                         const RenderFrame &frame) {
  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandBuffer.begin(beginInfo);

  const std::array<float, 4> clearColor = {frame.clearColor.r, frame.clearColor.g,
                                          frame.clearColor.b, frame.clearColor.a};
  vk::ClearValue clearValue{};
  clearValue.color = vk::ClearColorValue(clearColor);

  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = renderPass_;
  renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
  renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
  renderPassInfo.renderArea.extent = swapchainExtent_;
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue;

  commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

  if (pipelineReady_ && !frame.commands.empty()) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline_);

    // Negative-height viewport flips Vulkan's Y-down NDC back to the Y-up
    // convention glm::perspective/ortho/lookAt produce, without a matrix.
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(swapchainExtent_.height);
    viewport.width = static_cast<float>(swapchainExtent_.width);
    viewport.height = -static_cast<float>(swapchainExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, viewport);

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = swapchainExtent_;
    commandBuffer.setScissor(0, scissor);

    const Mat4 viewProjection = frame.camera.projection * frame.camera.view;

    for (const auto &draw : frame.commands) {
      const GpuMesh *mesh = findMesh(draw.mesh);
      if (!mesh) {
        continue;
      }

      PushConstants pushConstants{};
      pushConstants.mvp = viewProjection * draw.transform;
      pushConstants.color = Vec4{draw.tint.r, draw.tint.g, draw.tint.b, draw.tint.a};
      pushConstants.useVertexColor = draw.useVertexColor ? 1 : 0;

      commandBuffer.pushConstants(pipelineLayout_, vk::ShaderStageFlagBits::eVertex, 0,
                                  sizeof(PushConstants), &pushConstants);

      vk::DeviceSize offsets[] = {0};
      commandBuffer.bindVertexBuffers(0, 1, &mesh->vertexBuffer, offsets);

      if (mesh->indexBuffer) {
        commandBuffer.bindIndexBuffer(mesh->indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(mesh->indexCount, 1, 0, 0, 0);
      } else {
        commandBuffer.draw(mesh->vertexCount, 1, 0, 0);
      }
    }
  }

  commandBuffer.endRenderPass();
  commandBuffer.end();
}

void VulkanRenderer::renderFrame(const RenderFrame &frame) {
  if (framebufferResized_) {
    framebufferResized_ = false;
    recreateSwapchain();
  }

  static_cast<void>(device_.waitForFences(inFlightFence_, VK_TRUE, UINT64_MAX));

  auto acquireResult = device_.acquireNextImageKHR(
      swapchain_, UINT64_MAX, imageAvailableSemaphore_, nullptr);

  if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapchain();
    return;
  }
  if (acquireResult.result != vk::Result::eSuccess &&
      acquireResult.result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("Failed to acquire swapchain image");
  }

  const std::uint32_t imageIndex = acquireResult.value;

  device_.resetFences(inFlightFence_);

  commandBuffers_[imageIndex].reset();
  recordCommandBuffer(commandBuffers_[imageIndex], imageIndex, frame);

  vk::SubmitInfo submitInfo{};

  vk::Semaphore waitSemaphores[] = {imageAvailableSemaphore_};
  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];

  vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores_[imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  graphicsQueue_.submit(submitInfo, inFlightFence_);

  vk::PresentInfoKHR presentInfo{};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  vk::SwapchainKHR swapchains[] = {swapchain_};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &imageIndex;

  const auto presentResult = presentQueue_.presentKHR(presentInfo);
  if (presentResult == vk::Result::eErrorOutOfDateKHR ||
      presentResult == vk::Result::eSuboptimalKHR || framebufferResized_) {
    framebufferResized_ = false;
    recreateSwapchain();
    return;
  }

  if (presentResult != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to present swapchain image");
  }
}

} // namespace Eden
