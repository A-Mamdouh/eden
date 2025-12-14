#include "Eden/Systems/RenderSystem/Vulkan/VulkanRenderer.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <string_view>
#include <stdexcept>

namespace Eden {
namespace {

constexpr std::array<const char *, 1> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
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

  if (inFlightFence_) {
    device_.destroyFence(inFlightFence_);
  }
  if (renderFinishedSemaphore_) {
    device_.destroySemaphore(renderFinishedSemaphore_);
  }
  if (imageAvailableSemaphore_) {
    device_.destroySemaphore(imageAvailableSemaphore_);
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
  renderFinishedSemaphore_ = device_.createSemaphore(semaphoreInfo);
  inFlightFence_ = device_.createFence(fenceInfo);
}

void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer,
                                        std::uint32_t imageIndex) {
  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandBuffer.begin(beginInfo);

  const std::array<float, 4> clearColor = {clearColor_.r, clearColor_.g,
                                          clearColor_.b, clearColor_.a};
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
  commandBuffer.endRenderPass();
  commandBuffer.end();
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
  createFramebuffers();
  allocateCommandBuffers();
}

void VulkanRenderer::drawFrame() {
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
  recordCommandBuffer(commandBuffers_[imageIndex], imageIndex);

  vk::SubmitInfo submitInfo{};

  vk::Semaphore waitSemaphores[] = {imageAvailableSemaphore_};
  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];

  vk::Semaphore signalSemaphores[] = {renderFinishedSemaphore_};
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
