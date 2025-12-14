#pragma once

#include "Eden/Systems/RenderSystem/Renderer.hpp"

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace Eden {

class VulkanRenderer final : public Renderer {
public:
  struct CreateInfo {
    SDL_Window *window{nullptr};
    bool enableValidationLayers{false};
  };

  explicit VulkanRenderer(const CreateInfo &createInfo);
  ~VulkanRenderer() override;

  void drawFrame() override;
  void requestResize(std::uint32_t width, std::uint32_t height) override;
  void setClearColor(ClearColor color) override { clearColor_ = color; }

private:
  void initVulkan();
  void createInstance();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapchain();
  void createImageViews();
  void createRenderPass();
  void createFramebuffers();
  void createCommandPool();
  void allocateCommandBuffers();
  void createSyncObjects();

  void cleanupSwapchain();
  void recreateSwapchain();
  void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::uint32_t imageIndex);

  SDL_Window *window_{nullptr};
  bool enableValidationLayers_{false};

  bool framebufferResized_{false};
  ClearColor clearColor_{0.05f, 0.05f, 0.08f, 1.0f};

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

  vk::CommandPool commandPool_{};
  std::vector<vk::CommandBuffer> commandBuffers_{};

  vk::Semaphore imageAvailableSemaphore_{};
  vk::Semaphore renderFinishedSemaphore_{};
  vk::Fence inFlightFence_{};
};

} // namespace Eden

