#pragma once

// Shared Vulkan state between platform and rendering backends.
// The SDL2+Vulkan platform backend creates the instance/device/swap chain;
// the Vulkan rendering backend uses them for drawing.

#include <vulkan/vulkan.h>
#include <vector>

namespace Vulkan_Shared
{
    VkInstance getInstance();
    VkPhysicalDevice getPhysicalDevice();
    VkDevice getDevice();
    VkQueue getGraphicsQueue();
    uint32_t getGraphicsQueueFamily();

    VkSwapchainKHR getSwapchain();
    VkRenderPass getRenderPass();
    VkRenderPass getRenderPassLoad();
    VkCommandPool getCommandPool();

    VkFormat getSwapchainFormat();
    VkExtent2D getSwapchainExtent();

    const std::vector<VkImageView>& getSwapchainImageViews();
    const std::vector<VkFramebuffer>& getSwapchainFramebuffers();

    VkImageView getDepthImageView();

    int getBackbufferWidth();
    int getBackbufferHeight();
    void setBackbufferSize(int w, int h);

    // Frame management (driven by platform context, consumed by renderer)
    VkCommandBuffer getCurrentCommandBuffer();
    uint32_t getCurrentImageIndex();
    int getCurrentFrame();
    bool isRenderPassActive();
    void setRenderPassActive(bool active);
    bool hasRenderPassBegunThisFrame();
    void setRenderPassBegunThisFrame(bool begun);

    // Memory helper (used by both context and renderer)
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Setters (called by platform context during init)
    void setInstance(VkInstance instance);
    void setPhysicalDevice(VkPhysicalDevice physicalDevice);
    void setDevice(VkDevice device);
    void setGraphicsQueue(VkQueue queue);
    void setGraphicsQueueFamily(uint32_t family);
    void setSwapchain(VkSwapchainKHR swapchain);
    void setRenderPass(VkRenderPass renderPass);
    void setCommandPool(VkCommandPool commandPool);
    void setSwapchainFormat(VkFormat format);
    void setSwapchainExtent(VkExtent2D extent);
    void setSwapchainImageViews(std::vector<VkImageView> views);
    void setSwapchainFramebuffers(std::vector<VkFramebuffer> framebuffers);
    void setDepthImageView(VkImageView view);
    void setCurrentCommandBuffer(VkCommandBuffer cb);
    void setCurrentImageIndex(uint32_t index);
    void setCurrentFrame(int frame);
}
