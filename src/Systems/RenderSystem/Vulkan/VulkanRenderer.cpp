#include "VulkanRenderer.hpp"
#include "VulkanRendererFactory.hpp"

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

namespace Eden::Rendering {
namespace {

constexpr std::array<const char *, 1> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

/// Upper bound on live textures at once; sized generously for a demo/asset
/// scene, not meant to scale to a large open-world texture budget. Fixed
/// so descriptorPool_ can be allocated once and never resized.
constexpr std::uint32_t kMaxTextures = 256;

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

bool supportsPresentMode(const std::vector<vk::PresentModeKHR> &modes, vk::PresentModeKHR mode) {
  return std::find(modes.begin(), modes.end(), mode) != modes.end();
}

/// eFifo is guaranteed by the Vulkan spec to always be supported, so it's
/// the safe fallback whenever a preferred mode isn't available -- On maps
/// straight to it, Off/Adaptive fall back to it rather than failing.
vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> &modes,
                                     VsyncMode vsync) {
  switch (vsync) {
  case VsyncMode::Off:
    return supportsPresentMode(modes, vk::PresentModeKHR::eImmediate)
               ? vk::PresentModeKHR::eImmediate
               : vk::PresentModeKHR::eFifo;
  case VsyncMode::Adaptive:
    return supportsPresentMode(modes, vk::PresentModeKHR::eFifoRelaxed)
               ? vk::PresentModeKHR::eFifoRelaxed
               : vk::PresentModeKHR::eFifo;
  case VsyncMode::On:
  default:
    return vk::PresentModeKHR::eFifo;
  }
}

vk::SampleCountFlagBits toVkSampleCount(AntiAliasing aa) {
  switch (aa) {
  case AntiAliasing::MSAA2x:
    return vk::SampleCountFlagBits::e2;
  case AntiAliasing::MSAA4x:
    return vk::SampleCountFlagBits::e4;
  case AntiAliasing::MSAA8x:
    return vk::SampleCountFlagBits::e8;
  case AntiAliasing::None:
  default:
    return vk::SampleCountFlagBits::e1;
  }
}

AntiAliasing toAntiAliasing(vk::SampleCountFlagBits samples) {
  switch (samples) {
  case vk::SampleCountFlagBits::e8:
    return AntiAliasing::MSAA8x;
  case vk::SampleCountFlagBits::e4:
    return AntiAliasing::MSAA4x;
  case vk::SampleCountFlagBits::e2:
    return AntiAliasing::MSAA2x;
  default:
    return AntiAliasing::None;
  }
}

/// Compares as plain integers rather than relying on operator<= existing
/// for a Vulkan flag-bits enum (it generally doesn't).
vk::SampleCountFlagBits clampSampleCount(vk::SampleCountFlagBits requested,
                                        vk::SampleCountFlagBits max) {
  return static_cast<std::uint32_t>(requested) <= static_cast<std::uint32_t>(max) ? requested : max;
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
      enableValidationLayers_{createInfo.enableValidationLayers},
      currentSettings_{createInfo.initialSettings} {
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

  for (auto &texture : textures_) {
    if (!texture.alive) {
      continue;
    }
    device_.destroyImageView(texture.view);
    device_.destroyImage(texture.image);
    device_.freeMemory(texture.memory);
  }

  // Destroying the pool implicitly frees every descriptor set allocated
  // from it, so the per-texture sets above don't need individual frees.
  if (descriptorPool_) {
    device_.destroyDescriptorPool(descriptorPool_);
  }
  if (textureSampler_) {
    device_.destroySampler(textureSampler_);
  }
  if (descriptorSetLayout_) {
    device_.destroyDescriptorSetLayout(descriptorSetLayout_);
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

ApplyResult VulkanRenderer::applySettings(const RenderSettings &settings) {
  currentSettings_ = settings;
  // Anti-aliasing changes the render pass/pipeline's attachment layout and
  // vsync changes the swapchain's present mode -- both are already exactly
  // what recreateSwapchain() rebuilds, without touching instance_/device_.
  recreateSwapchain();
  return ApplyResult::Applied;
}

RendererCapabilities VulkanRenderer::queryCapabilities() const {
  return RendererCapabilities{toAntiAliasing(maxSampleCount_)};
}

vk::SampleCountFlagBits VulkanRenderer::sampleCount() const {
  return clampSampleCount(toVkSampleCount(currentSettings_.antiAliasing), maxSampleCount_);
}

void VulkanRenderer::initVulkan() {
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandPool();
  createSwapchain();
  createImageViews();
  createDepthResources();
  createColorResources();
  createRenderPass();
  createDescriptorSetLayout();
  createTextureSampler();
  createDescriptorPool();
  createDefaultTexture();
  createGraphicsPipeline();
  createFramebuffers();
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

  // Highest count both color and depth attachments can agree on; capped by
  // the candidates list at e8 since AntiAliasing itself has no higher
  // level to request. Hardware reporting less than e2 for either just
  // means maxSampleCount_ stays e1 -- anti-aliasing genuinely isn't
  // available, not a policy choice.
  const auto limits = physicalDevice_.getProperties().limits;
  const vk::SampleCountFlags supported =
      limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
  maxSampleCount_ = vk::SampleCountFlagBits::e1;
  for (const auto candidate : {vk::SampleCountFlagBits::e8, vk::SampleCountFlagBits::e4,
                               vk::SampleCountFlagBits::e2}) {
    if (supported & candidate) {
      maxSampleCount_ = candidate;
      break;
    }
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
  const auto presentMode = choosePresentMode(details.presentModes, currentSettings_.vsync);
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

vk::Format VulkanRenderer::findDepthFormat() const {
  const std::array<vk::Format, 3> candidates = {
      vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint};

  for (const auto format : candidates) {
    const auto props = physicalDevice_.getFormatProperties(format);
    if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
        vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
      return format;
    }
  }

  throw std::runtime_error("Failed to find a supported depth format");
}

void VulkanRenderer::createDepthResources() {
  depthFormat_ = findDepthFormat();

  vk::ImageCreateInfo imageInfo{};
  imageInfo.imageType = vk::ImageType::e2D;
  imageInfo.extent = vk::Extent3D{swapchainExtent_.width, swapchainExtent_.height, 1};
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = depthFormat_;
  imageInfo.tiling = vk::ImageTiling::eOptimal;
  imageInfo.initialLayout = vk::ImageLayout::eUndefined;
  imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
  imageInfo.samples = sampleCount();
  imageInfo.sharingMode = vk::SharingMode::eExclusive;

  depthImage_ = device_.createImage(imageInfo);

  const vk::MemoryRequirements memRequirements = device_.getImageMemoryRequirements(depthImage_);
  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

  depthImageMemory_ = device_.allocateMemory(allocInfo);
  device_.bindImageMemory(depthImage_, depthImageMemory_, 0);

  vk::ImageViewCreateInfo viewInfo{};
  viewInfo.image = depthImage_;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = depthFormat_;
  viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  depthImageView_ = device_.createImageView(viewInfo);
}

void VulkanRenderer::createColorResources() {
  if (sampleCount() == vk::SampleCountFlagBits::e1) {
    return;
  }

  vk::ImageCreateInfo imageInfo{};
  imageInfo.imageType = vk::ImageType::e2D;
  imageInfo.extent = vk::Extent3D{swapchainExtent_.width, swapchainExtent_.height, 1};
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = swapchainImageFormat_;
  imageInfo.tiling = vk::ImageTiling::eOptimal;
  imageInfo.initialLayout = vk::ImageLayout::eUndefined;
  imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment;
  imageInfo.samples = sampleCount();
  imageInfo.sharingMode = vk::SharingMode::eExclusive;

  colorImage_ = device_.createImage(imageInfo);

  const vk::MemoryRequirements memRequirements = device_.getImageMemoryRequirements(colorImage_);
  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

  colorImageMemory_ = device_.allocateMemory(allocInfo);
  device_.bindImageMemory(colorImage_, colorImageMemory_, 0);

  vk::ImageViewCreateInfo viewInfo{};
  viewInfo.image = colorImage_;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = swapchainImageFormat_;
  viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  colorImageView_ = device_.createImageView(viewInfo);
}

void VulkanRenderer::createRenderPass() {
  const vk::SampleCountFlagBits samples = sampleCount();
  const bool multisampled = samples != vk::SampleCountFlagBits::e1;

  vk::AttachmentDescription colorAttachment{};
  colorAttachment.format = swapchainImageFormat_;
  colorAttachment.samples = samples;
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  // Multisampled: the resolve attachment below is what actually gets
  // presented, so this attachment's own contents don't need to survive
  // past the subpass.
  colorAttachment.storeOp =
      multisampled ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout =
      multisampled ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentDescription depthAttachment{};
  depthAttachment.format = depthFormat_;
  depthAttachment.samples = samples;
  depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
  depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  // Only used/attached when multisampled; single-sample rendering writes
  // straight to the swapchain image via colorAttachment above instead.
  vk::AttachmentDescription resolveAttachment{};
  resolveAttachment.format = swapchainImageFormat_;
  resolveAttachment.samples = vk::SampleCountFlagBits::e1;
  resolveAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
  resolveAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  resolveAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  resolveAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  resolveAttachment.initialLayout = vk::ImageLayout::eUndefined;
  resolveAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::AttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentReference resolveAttachmentRef{};
  resolveAttachmentRef.attachment = 2;
  resolveAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  if (multisampled) {
    subpass.pResolveAttachments = &resolveAttachmentRef;
  }

  vk::SubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                            vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.srcAccessMask = {};
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                            vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite |
                             vk::AccessFlagBits::eDepthStencilAttachmentWrite;

  std::vector<vk::AttachmentDescription> attachments = {colorAttachment, depthAttachment};
  if (multisampled) {
    attachments.push_back(resolveAttachment);
  }

  vk::RenderPassCreateInfo renderPassInfo{};
  renderPassInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  renderPass_ = device_.createRenderPass(renderPassInfo);
}

void VulkanRenderer::createDescriptorSetLayout() {
  vk::DescriptorSetLayoutBinding samplerBinding{};
  samplerBinding.binding = 0;
  samplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  samplerBinding.descriptorCount = 1;
  samplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &samplerBinding;

  descriptorSetLayout_ = device_.createDescriptorSetLayout(layoutInfo);
}

void VulkanRenderer::createTextureSampler() {
  vk::SamplerCreateInfo samplerInfo{};
  samplerInfo.magFilter = vk::Filter::eLinear;
  samplerInfo.minFilter = vk::Filter::eLinear;
  samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = vk::CompareOp::eAlways;
  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

  textureSampler_ = device_.createSampler(samplerInfo);
}

void VulkanRenderer::createDescriptorPool() {
  vk::DescriptorPoolSize poolSize{};
  poolSize.type = vk::DescriptorType::eCombinedImageSampler;
  poolSize.descriptorCount = kMaxTextures;

  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = kMaxTextures;
  poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

  descriptorPool_ = device_.createDescriptorPool(poolInfo);
}

void VulkanRenderer::createDefaultTexture() {
  const std::array<std::uint8_t, 4> whitePixel = {255, 255, 255, 255};

  TextureDesc desc{};
  desc.width = 1;
  desc.height = 1;
  desc.pixels = whitePixel;

  defaultTexture_ = createTexture(desc);
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

  const auto vertCode = loadShaderBinary("mesh.vert.spv");
  const auto fragCode = loadShaderBinary("mesh.frag.spv");

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

  vk::VertexInputAttributeDescription attributeDescriptions[3]{};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
  attributeDescriptions[0].offset = offsetof(Vertex, position);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
  attributeDescriptions[1].offset = offsetof(Vertex, color);

  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
  attributeDescriptions[2].offset = offsetof(Vertex, uv);

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 3;
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
  multisampling.rasterizationSamples = sampleCount();
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

  vk::PipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = vk::CompareOp::eLess;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStates;

  vk::PushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstants);

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
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
  pipelineInfo.pDepthStencilState = &depthStencil;
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
  const bool multisampled = sampleCount() != vk::SampleCountFlagBits::e1;

  swapchainFramebuffers_.clear();
  swapchainFramebuffers_.reserve(swapchainImageViews_.size());

  for (const auto view : swapchainImageViews_) {
    // Attachment order must match createRenderPass()'s attachment indices:
    // color=0, depth=1, resolve=2 when multisampled.
    const std::vector<vk::ImageView> attachments =
        multisampled ? std::vector<vk::ImageView>{colorImageView_, depthImageView_, view}
                     : std::vector<vk::ImageView>{view, depthImageView_};

    vk::FramebufferCreateInfo framebufferInfo{};
    framebufferInfo.renderPass = renderPass_;
    framebufferInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
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

vk::CommandBuffer VulkanRenderer::beginSingleTimeCommands() const {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandPool = commandPool_;
  allocInfo.commandBufferCount = 1;

  vk::CommandBuffer commandBuffer = device_.allocateCommandBuffers(allocInfo).front();

  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
  commandBuffer.begin(beginInfo);

  return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(vk::CommandBuffer commandBuffer) const {
  commandBuffer.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  static_cast<void>(graphicsQueue_.submit(1, &submitInfo, nullptr));
  graphicsQueue_.waitIdle();

  device_.freeCommandBuffers(commandPool_, commandBuffer);
}

void VulkanRenderer::transitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image,
                                           vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const {
  vk::ImageMemoryBarrier barrier{};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vk::PipelineStageFlags srcStage{};
  vk::PipelineStageFlags dstStage{};

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    srcStage = vk::PipelineStageFlagBits::eTransfer;
    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else {
    throw std::runtime_error("Unsupported Vulkan image layout transition");
  }

  commandBuffer.pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);
}

void VulkanRenderer::copyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer buffer,
                                       vk::Image image, std::uint32_t width,
                                       std::uint32_t height) const {
  vk::BufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = vk::Offset3D{0, 0, 0};
  region.imageExtent = vk::Extent3D{width, height, 1};

  commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

TextureHandle VulkanRenderer::createTexture(const TextureDesc &desc) {
  const vk::DeviceSize imageSize =
      static_cast<vk::DeviceSize>(desc.width) * static_cast<vk::DeviceSize>(desc.height) * 4;

  const auto [stagingBuffer, stagingMemory] =
      createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc);
  uploadToBuffer(stagingMemory, desc.pixels.data(), imageSize);

  GpuTexture texture{};

  vk::ImageCreateInfo imageInfo{};
  imageInfo.imageType = vk::ImageType::e2D;
  imageInfo.extent = vk::Extent3D{desc.width, desc.height, 1};
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  // sRGB: glTF/authoring tools store base-color textures gamma-encoded;
  // this format tells the sampler to linearize on read.
  imageInfo.format = vk::Format::eR8G8B8A8Srgb;
  imageInfo.tiling = vk::ImageTiling::eOptimal;
  imageInfo.initialLayout = vk::ImageLayout::eUndefined;
  imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
  imageInfo.samples = vk::SampleCountFlagBits::e1;
  imageInfo.sharingMode = vk::SharingMode::eExclusive;

  texture.image = device_.createImage(imageInfo);

  const vk::MemoryRequirements memRequirements = device_.getImageMemoryRequirements(texture.image);
  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

  texture.memory = device_.allocateMemory(allocInfo);
  device_.bindImageMemory(texture.image, texture.memory, 0);

  vk::CommandBuffer commandBuffer = beginSingleTimeCommands();
  transitionImageLayout(commandBuffer, texture.image, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal);
  copyBufferToImage(commandBuffer, stagingBuffer, texture.image, desc.width, desc.height);
  transitionImageLayout(commandBuffer, texture.image, vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal);
  endSingleTimeCommands(commandBuffer);

  device_.destroyBuffer(stagingBuffer);
  device_.freeMemory(stagingMemory);

  vk::ImageViewCreateInfo viewInfo{};
  viewInfo.image = texture.image;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = vk::Format::eR8G8B8A8Srgb;
  viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  texture.view = device_.createImageView(viewInfo);

  vk::DescriptorSetAllocateInfo dsAllocInfo{};
  dsAllocInfo.descriptorPool = descriptorPool_;
  dsAllocInfo.descriptorSetCount = 1;
  dsAllocInfo.pSetLayouts = &descriptorSetLayout_;

  texture.descriptorSet = device_.allocateDescriptorSets(dsAllocInfo).front();

  vk::DescriptorImageInfo imageDescInfo{};
  imageDescInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  imageDescInfo.imageView = texture.view;
  imageDescInfo.sampler = textureSampler_;

  vk::WriteDescriptorSet descriptorWrite{};
  descriptorWrite.dstSet = texture.descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageDescInfo;

  device_.updateDescriptorSets(descriptorWrite, {});

  texture.alive = true;

  std::uint32_t slot{};
  if (!freeTextureSlots_.empty()) {
    slot = freeTextureSlots_.back();
    freeTextureSlots_.pop_back();
    texture.generation = textures_[slot].generation + 1;
    textures_[slot] = texture;
  } else {
    slot = static_cast<std::uint32_t>(textures_.size());
    texture.generation = 1;
    textures_.push_back(texture);
  }

  return TextureHandle{slot + 1, textures_[slot].generation};
}

void VulkanRenderer::destroyTexture(TextureHandle handle) {
  if (!handle.valid()) {
    return;
  }
  const std::uint32_t slot = handle.id - 1;
  if (slot >= textures_.size()) {
    return;
  }

  GpuTexture &texture = textures_[slot];
  if (!texture.alive || texture.generation != handle.generation) {
    return;
  }

  static_cast<void>(device_.freeDescriptorSets(descriptorPool_, texture.descriptorSet));
  device_.destroyImageView(texture.view);
  device_.destroyImage(texture.image);
  device_.freeMemory(texture.memory);

  texture.alive = false;
  freeTextureSlots_.push_back(slot);
}

const VulkanRenderer::GpuTexture *VulkanRenderer::findTexture(TextureHandle handle) const {
  if (!handle.valid()) {
    return nullptr;
  }
  const std::uint32_t slot = handle.id - 1;
  if (slot >= textures_.size()) {
    return nullptr;
  }
  const GpuTexture &texture = textures_[slot];
  if (!texture.alive || texture.generation != handle.generation) {
    return nullptr;
  }
  return &texture;
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

  if (depthImageView_) {
    device_.destroyImageView(depthImageView_);
    depthImageView_ = nullptr;
  }
  if (depthImage_) {
    device_.destroyImage(depthImage_);
    depthImage_ = nullptr;
  }
  if (depthImageMemory_) {
    device_.freeMemory(depthImageMemory_);
    depthImageMemory_ = nullptr;
  }

  if (colorImageView_) {
    device_.destroyImageView(colorImageView_);
    colorImageView_ = nullptr;
  }
  if (colorImage_) {
    device_.destroyImage(colorImage_);
    colorImage_ = nullptr;
  }
  if (colorImageMemory_) {
    device_.freeMemory(colorImageMemory_);
    colorImageMemory_ = nullptr;
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
  createDepthResources();
  createColorResources();
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
  std::array<vk::ClearValue, 2> clearValues{};
  clearValues[0].color = vk::ClearColorValue(clearColor);
  clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = renderPass_;
  renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
  renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
  renderPassInfo.renderArea.extent = swapchainExtent_;
  renderPassInfo.clearValueCount = static_cast<std::uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

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

      const GpuTexture *texture = findTexture(draw.texture);
      if (!texture) {
        texture = findTexture(defaultTexture_);
      }
      if (texture) {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout_, 0,
                                         texture->descriptorSet, {});
      }

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

std::unique_ptr<Renderer> createVulkanRenderer(SDL_Window *window, bool enableValidationLayers,
                                               const RenderSettings &initialSettings) {
  return std::make_unique<VulkanRenderer>(VulkanRenderer::CreateInfo{
      .window = window, .enableValidationLayers = enableValidationLayers, .initialSettings = initialSettings});
}

} // namespace Eden::Rendering
