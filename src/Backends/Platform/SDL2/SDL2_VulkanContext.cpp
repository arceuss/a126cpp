// SDL2 platform backend - Vulkan context creation
// Creates SDL2 window (Vulkan), creates Vulkan instance/device/swapchain/render pass.

#include "lwjgl/GLContext.h"
#include "Backends/Shared/SDL2.h"
#include "Backends/Shared/Vulkan.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <set>

#include <vulkan/vulkan.h>

#include "external/SDLException.h"

#include "SDL.h"
#include "SDL_vulkan.h"
#include "SDL_syswm.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#define IDI_ICON1 1
#endif

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

// ============================================================================
// Vulkan_Shared storage
// ============================================================================

namespace Vulkan_Shared
{
    static VkInstance s_instance = VK_NULL_HANDLE;
    static VkPhysicalDevice s_physicalDevice = VK_NULL_HANDLE;
    static VkDevice s_device = VK_NULL_HANDLE;
    static VkQueue s_graphicsQueue = VK_NULL_HANDLE;
    static uint32_t s_graphicsQueueFamily = 0;

    static VkSwapchainKHR s_swapchain = VK_NULL_HANDLE;
    static VkRenderPass s_renderPass = VK_NULL_HANDLE;
    static VkCommandPool s_commandPool = VK_NULL_HANDLE;

    static VkFormat s_swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    static VkExtent2D s_swapchainExtent = {854, 480};

    static std::vector<VkImageView> s_swapchainImageViews;
    static std::vector<VkFramebuffer> s_swapchainFramebuffers;

    static VkImage s_depthImage = VK_NULL_HANDLE;
    static VkDeviceMemory s_depthMemory = VK_NULL_HANDLE;
    static VkImageView s_depthImageView = VK_NULL_HANDLE;

    static int s_bbWidth = 0;
    static int s_bbHeight = 0;

    static VkCommandBuffer s_currentCommandBuffer = VK_NULL_HANDLE;
    static uint32_t s_currentImageIndex = 0;
    static int s_currentFrame = 0;
    static bool s_renderPassActive = false;
    static uint32_t s_currentAcquireSemaphoreIndex = 0;

    VkInstance getInstance() { return s_instance; }
    VkPhysicalDevice getPhysicalDevice() { return s_physicalDevice; }
    VkDevice getDevice() { return s_device; }
    VkQueue getGraphicsQueue() { return s_graphicsQueue; }
    uint32_t getGraphicsQueueFamily() { return s_graphicsQueueFamily; }

    VkSwapchainKHR getSwapchain() { return s_swapchain; }
    VkRenderPass getRenderPass() { return s_renderPass; }
    VkCommandPool getCommandPool() { return s_commandPool; }

    VkFormat getSwapchainFormat() { return s_swapchainFormat; }
    VkExtent2D getSwapchainExtent() { return s_swapchainExtent; }

    const std::vector<VkImageView>& getSwapchainImageViews() { return s_swapchainImageViews; }
    const std::vector<VkFramebuffer>& getSwapchainFramebuffers() { return s_swapchainFramebuffers; }

    VkImageView getDepthImageView() { return s_depthImageView; }

    int getBackbufferWidth() { return s_bbWidth; }
    int getBackbufferHeight() { return s_bbHeight; }
    void setBackbufferSize(int w, int h) { s_bbWidth = w; s_bbHeight = h; }

    VkCommandBuffer getCurrentCommandBuffer() { return s_currentCommandBuffer; }
    uint32_t getCurrentImageIndex() { return s_currentImageIndex; }
    int getCurrentFrame() { return s_currentFrame; }
    bool isRenderPassActive() { return s_renderPassActive; }
    void setRenderPassActive(bool active) { s_renderPassActive = active; }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(s_physicalDevice, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

    void setInstance(VkInstance instance) { s_instance = instance; }
    void setPhysicalDevice(VkPhysicalDevice physicalDevice) { s_physicalDevice = physicalDevice; }
    void setDevice(VkDevice device) { s_device = device; }
    void setGraphicsQueue(VkQueue queue) { s_graphicsQueue = queue; }
    void setGraphicsQueueFamily(uint32_t family) { s_graphicsQueueFamily = family; }
    void setSwapchain(VkSwapchainKHR swapchain) { s_swapchain = swapchain; }
    void setRenderPass(VkRenderPass renderPass) { s_renderPass = renderPass; }
    void setCommandPool(VkCommandPool commandPool) { s_commandPool = commandPool; }
    void setSwapchainFormat(VkFormat format) { s_swapchainFormat = format; }
    void setSwapchainExtent(VkExtent2D extent) { s_swapchainExtent = extent; }
    void setSwapchainImageViews(std::vector<VkImageView> views) { s_swapchainImageViews = std::move(views); }
    void setSwapchainFramebuffers(std::vector<VkFramebuffer> framebuffers) { s_swapchainFramebuffers = std::move(framebuffers); }
    void setDepthImageView(VkImageView view) { s_depthImageView = view; }
    void setCurrentCommandBuffer(VkCommandBuffer cb) { s_currentCommandBuffer = cb; }
    void setCurrentImageIndex(uint32_t index) { s_currentImageIndex = index; }
    void setCurrentFrame(int frame) { s_currentFrame = frame; }
}

// ============================================================================
// SDL2_Shared storage
// ============================================================================

namespace SDL2_Shared
{
    static SDL_Window* s_window = nullptr;

    SDL_Window* getWindow() { return s_window; }
    SDL_GLContext getGLContext() { return nullptr; }
    void setWindow(SDL_Window* window) { s_window = window; }
    void setGLContext(SDL_GLContext) {}
}

// ============================================================================
// Vulkan context implementation
// ============================================================================

namespace
{

static VkSurfaceKHR s_surface = VK_NULL_HANDLE;
static std::vector<VkImage> s_swapchainImages;
static std::vector<VkCommandBuffer> s_commandBuffers;
static std::vector<VkFence> s_inFlightFences;
static std::vector<VkSemaphore> s_imageAvailableSemaphores; // per swapchain image
static std::vector<VkSemaphore> s_renderFinishedSemaphores; // one per swapchain image, indexed by image index
static uint32_t s_acquireSemaphoreIndex = 0; // rotating index into s_imageAvailableSemaphores

static VkFormat findDepthFormat()
{
    VkFormat candidates[] = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT };
    for (auto fmt : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(Vulkan_Shared::s_physicalDevice, fmt, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return fmt;
    }
    return VK_FORMAT_D24_UNORM_S8_UINT;
}

static void createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();

    VkImageCreateInfo imageCI = {};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = depthFormat;
    imageCI.extent = { Vulkan_Shared::s_swapchainExtent.width, Vulkan_Shared::s_swapchainExtent.height, 1 };
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    vkCreateImage(Vulkan_Shared::s_device, &imageCI, nullptr, &Vulkan_Shared::s_depthImage);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(Vulkan_Shared::s_device, Vulkan_Shared::s_depthImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = Vulkan_Shared::findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(Vulkan_Shared::s_device, &allocInfo, nullptr, &Vulkan_Shared::s_depthMemory);
    vkBindImageMemory(Vulkan_Shared::s_device, Vulkan_Shared::s_depthImage, Vulkan_Shared::s_depthMemory, 0);

    VkImageViewCreateInfo viewCI = {};
    viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCI.image = Vulkan_Shared::s_depthImage;
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = depthFormat;
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depthFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT)
        viewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    viewCI.subresourceRange.baseMipLevel = 0;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.baseArrayLayer = 0;
    viewCI.subresourceRange.layerCount = 1;

    vkCreateImageView(Vulkan_Shared::s_device, &viewCI, nullptr, &Vulkan_Shared::s_depthImageView);
}

static void createRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = Vulkan_Shared::s_swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef = {};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };

    VkRenderPassCreateInfo renderPassCI = {};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = 2;
    renderPassCI.pAttachments = attachments;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpass;
    renderPassCI.dependencyCount = 1;
    renderPassCI.pDependencies = &dependency;

    if (vkCreateRenderPass(Vulkan_Shared::s_device, &renderPassCI, nullptr, &Vulkan_Shared::s_renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass");
}

static void createSwapchain(int width, int height)
{
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan_Shared::s_physicalDevice, s_surface, &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan_Shared::s_physicalDevice, s_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan_Shared::s_physicalDevice, s_surface, &formatCount, formats.data());

    // Prefer B8G8R8A8_UNORM with SRGB_NONLINEAR
    VkSurfaceFormatKHR chosen = formats[0];
    for (auto& f : formats)
    {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosen = f;
            break;
        }
    }

    VkExtent2D extent;
    if (caps.currentExtent.width != UINT32_MAX)
    {
        extent = caps.currentExtent;
    }
    else
    {
        extent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, (uint32_t)width));
        extent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, (uint32_t)height));
    }

    uint32_t imageCount = std::max(caps.minImageCount, (uint32_t)MAX_FRAMES_IN_FLIGHT);
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR swapCI = {};
    swapCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapCI.surface = s_surface;
    swapCI.minImageCount = imageCount;
    swapCI.imageFormat = chosen.format;
    swapCI.imageColorSpace = chosen.colorSpace;
    swapCI.imageExtent = extent;
    swapCI.imageArrayLayers = 1;
    swapCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapCI.preTransform = caps.currentTransform;
    swapCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapCI.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapCI.clipped = VK_TRUE;
    swapCI.oldSwapchain = Vulkan_Shared::s_swapchain;

    VkSwapchainKHR newSwapchain;
    if (vkCreateSwapchainKHR(Vulkan_Shared::s_device, &swapCI, nullptr, &newSwapchain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain");

    if (Vulkan_Shared::s_swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(Vulkan_Shared::s_device, Vulkan_Shared::s_swapchain, nullptr);

    Vulkan_Shared::s_swapchain = newSwapchain;
    Vulkan_Shared::s_swapchainFormat = chosen.format;
    Vulkan_Shared::s_swapchainExtent = extent;
    Vulkan_Shared::s_bbWidth = extent.width;
    Vulkan_Shared::s_bbHeight = extent.height;

    // Get swapchain images
    uint32_t swapImageCount = 0;
    vkGetSwapchainImagesKHR(Vulkan_Shared::s_device, Vulkan_Shared::s_swapchain, &swapImageCount, nullptr);
    s_swapchainImages.resize(swapImageCount);
    vkGetSwapchainImagesKHR(Vulkan_Shared::s_device, Vulkan_Shared::s_swapchain, &swapImageCount, s_swapchainImages.data());

    // Create image views
    Vulkan_Shared::s_swapchainImageViews.resize(swapImageCount);
    for (uint32_t i = 0; i < swapImageCount; i++)
    {
        VkImageViewCreateInfo viewCI = {};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.image = s_swapchainImages[i];
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = chosen.format;
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.levelCount = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount = 1;

        vkCreateImageView(Vulkan_Shared::s_device, &viewCI, nullptr, &Vulkan_Shared::s_swapchainImageViews[i]);
    }
}

static void createFramebuffers()
{
    auto& imageViews = Vulkan_Shared::s_swapchainImageViews;
    Vulkan_Shared::s_swapchainFramebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++)
    {
        VkImageView attachments[] = { imageViews[i], Vulkan_Shared::s_depthImageView };

        VkFramebufferCreateInfo fbCI = {};
        fbCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCI.renderPass = Vulkan_Shared::s_renderPass;
        fbCI.attachmentCount = 2;
        fbCI.pAttachments = attachments;
        fbCI.width = Vulkan_Shared::s_swapchainExtent.width;
        fbCI.height = Vulkan_Shared::s_swapchainExtent.height;
        fbCI.layers = 1;

        vkCreateFramebuffer(Vulkan_Shared::s_device, &fbCI, nullptr, &Vulkan_Shared::s_swapchainFramebuffers[i]);
    }
}

static void cleanupSwapchainResources()
{
    for (auto fb : Vulkan_Shared::s_swapchainFramebuffers)
        vkDestroyFramebuffer(Vulkan_Shared::s_device, fb, nullptr);
    Vulkan_Shared::s_swapchainFramebuffers.clear();

    for (auto iv : Vulkan_Shared::s_swapchainImageViews)
        vkDestroyImageView(Vulkan_Shared::s_device, iv, nullptr);
    Vulkan_Shared::s_swapchainImageViews.clear();

    if (Vulkan_Shared::s_depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Vulkan_Shared::s_device, Vulkan_Shared::s_depthImageView, nullptr);
        Vulkan_Shared::s_depthImageView = VK_NULL_HANDLE;
    }
    if (Vulkan_Shared::s_depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(Vulkan_Shared::s_device, Vulkan_Shared::s_depthImage, nullptr);
        Vulkan_Shared::s_depthImage = VK_NULL_HANDLE;
    }
    if (Vulkan_Shared::s_depthMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Vulkan_Shared::s_device, Vulkan_Shared::s_depthMemory, nullptr);
        Vulkan_Shared::s_depthMemory = VK_NULL_HANDLE;
    }
}

static void recreateSwapchain()
{
    int w, h;
    SDL_GetWindowSize(SDL2_Shared::s_window, &w, &h);

    // Wait until the window has a nonzero size (e.g. un-minimized)
    while (w == 0 || h == 0)
    {
        SDL_Event e;
        SDL_WaitEvent(&e);
        SDL_GetWindowSize(SDL2_Shared::s_window, &w, &h);
    }

    vkDeviceWaitIdle(Vulkan_Shared::s_device);

    cleanupSwapchainResources();
    createSwapchain(w, h);
    createDepthResources();

    // Render pass stays the same (format doesn't change)
    if (Vulkan_Shared::s_renderPass == VK_NULL_HANDLE)
        createRenderPass();

    createFramebuffers();
}

} // anonymous namespace

// ============================================================================
// lwjgl::GLContext implementation
// ============================================================================

namespace lwjgl
{
namespace GLContext
{

namespace detail
{

class VulkanContext
{
private:
    SDL_Window* window = nullptr;
    GLCapabilities capabilities;
    int lastWidth = 0;
    int lastHeight = 0;

public:
    VulkanContext()
    {
        // Create SDL window with Vulkan flag
        window = SDL_CreateWindow("Minecraft Alpha v1.2.6",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            854, 480,
            SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
        if (window == nullptr)
            throw SDLException();

        // Load and set window icon
#ifdef _WIN32
        {
            HICON hIcon = NULL;
            bool fromResource = false;

            hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
            if (hIcon != NULL)
            {
                fromResource = true;
            }
            else
            {
                const char* iconPaths[] = {
                    "src/mc.ico",
                    "mc.ico",
                    "../src/mc.ico",
                    "../../src/mc.ico"
                };

                for (int i = 0; i < 4 && hIcon == NULL; i++)
                {
                    hIcon = (HICON)LoadImageA(NULL, iconPaths[i], IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
                }
            }

            if (hIcon != NULL)
            {
                ICONINFO iconInfo;
                if (GetIconInfo(hIcon, &iconInfo))
                {
                    BITMAP bmp;
                    if (GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp))
                    {
                        int width = bmp.bmWidth;
                        int height = bmp.bmHeight;

                        SDL_Surface* iconSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_BGRA32);
                        if (iconSurface != nullptr)
                        {
                            HDC hDC = CreateCompatibleDC(NULL);
                            HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, iconInfo.hbmColor);

                            BITMAPINFO bmi;
                            ZeroMemory(&bmi, sizeof(BITMAPINFO));
                            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                            bmi.bmiHeader.biWidth = width;
                            bmi.bmiHeader.biHeight = -height;
                            bmi.bmiHeader.biPlanes = 1;
                            bmi.bmiHeader.biBitCount = 32;
                            bmi.bmiHeader.biCompression = BI_RGB;

                            if (GetDIBits(hDC, iconInfo.hbmColor, 0, height, iconSurface->pixels, &bmi, DIB_RGB_COLORS))
                            {
                                SDL_SetWindowIcon(window, iconSurface);
                            }

                            SelectObject(hDC, hOldBmp);
                            DeleteDC(hDC);
                            SDL_FreeSurface(iconSurface);
                        }
                    }

                    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
                    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
                }

                if (!fromResource)
                {
                    DestroyIcon(hIcon);
                }
            }
        }
#endif

        // Create Vulkan instance
        {
            unsigned int sdlExtCount = 0;
            SDL_Vulkan_GetInstanceExtensions(window, &sdlExtCount, nullptr);
            std::vector<const char*> extensions(sdlExtCount);
            SDL_Vulkan_GetInstanceExtensions(window, &sdlExtCount, extensions.data());

            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Minecraft Alpha v1.2.6";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 6);
            appInfo.pEngineName = "a126cpp";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_1;

            VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledExtensionCount = (uint32_t)extensions.size();
            createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef MC_DEBUG_VK
            const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
            createInfo.enabledLayerCount = 1;
            createInfo.ppEnabledLayerNames = validationLayers;
#endif

            if (vkCreateInstance(&createInfo, nullptr, &Vulkan_Shared::s_instance) != VK_SUCCESS)
                throw std::runtime_error("Failed to create Vulkan instance");
        }

        // Create surface
        if (!SDL_Vulkan_CreateSurface(window, Vulkan_Shared::s_instance, &s_surface))
            throw std::runtime_error("Failed to create Vulkan surface: " + std::string(SDL_GetError()));

        // Pick physical device
        {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(Vulkan_Shared::s_instance, &deviceCount, nullptr);
            if (deviceCount == 0)
                throw std::runtime_error("No Vulkan-capable GPU found");

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(Vulkan_Shared::s_instance, &deviceCount, devices.data());

            // Pick first device with graphics queue and swapchain support
            for (auto& dev : devices)
            {
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
                std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

                int graphicsFamily = -1;
                for (uint32_t i = 0; i < queueFamilyCount; i++)
                {
                    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    {
                        VkBool32 presentSupport = VK_FALSE;
                        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, s_surface, &presentSupport);
                        if (presentSupport)
                        {
                            graphicsFamily = i;
                            break;
                        }
                    }
                }

                if (graphicsFamily >= 0)
                {
                    // Check swapchain extension
                    uint32_t extCount = 0;
                    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
                    std::vector<VkExtensionProperties> exts(extCount);
                    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, exts.data());

                    bool hasSwapchain = false;
                    for (auto& e : exts)
                    {
                        if (strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
                        {
                            hasSwapchain = true;
                            break;
                        }
                    }

                    if (hasSwapchain)
                    {
                        Vulkan_Shared::s_physicalDevice = dev;
                        Vulkan_Shared::s_graphicsQueueFamily = (uint32_t)graphicsFamily;
                        break;
                    }
                }
            }

            if (Vulkan_Shared::s_physicalDevice == VK_NULL_HANDLE)
                throw std::runtime_error("No suitable Vulkan GPU found");
        }

        // Create logical device
        {
            float queuePriority = 1.0f;
            VkDeviceQueueCreateInfo queueCI = {};
            queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCI.queueFamilyIndex = Vulkan_Shared::s_graphicsQueueFamily;
            queueCI.queueCount = 1;
            queueCI.pQueuePriorities = &queuePriority;

            VkPhysicalDeviceFeatures deviceFeatures = {};
            deviceFeatures.fillModeNonSolid = VK_FALSE;
            deviceFeatures.wideLines = VK_FALSE;
            deviceFeatures.logicOp = VK_TRUE; // For glLogicOp

            const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

            VkDeviceCreateInfo deviceCI = {};
            deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceCI.queueCreateInfoCount = 1;
            deviceCI.pQueueCreateInfos = &queueCI;
            deviceCI.enabledExtensionCount = 1;
            deviceCI.ppEnabledExtensionNames = deviceExtensions;
            deviceCI.pEnabledFeatures = &deviceFeatures;

            if (vkCreateDevice(Vulkan_Shared::s_physicalDevice, &deviceCI, nullptr, &Vulkan_Shared::s_device) != VK_SUCCESS)
                throw std::runtime_error("Failed to create logical device");

            vkGetDeviceQueue(Vulkan_Shared::s_device, Vulkan_Shared::s_graphicsQueueFamily, 0, &Vulkan_Shared::s_graphicsQueue);
        }

        // Create swapchain
        createSwapchain(854, 480);

        // Create render pass
        createRenderPass();

        // Create depth resources
        createDepthResources();

        // Create framebuffers
        createFramebuffers();

        // Create command pool
        {
            VkCommandPoolCreateInfo poolCI = {};
            poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolCI.queueFamilyIndex = Vulkan_Shared::s_graphicsQueueFamily;

            if (vkCreateCommandPool(Vulkan_Shared::s_device, &poolCI, nullptr, &Vulkan_Shared::s_commandPool) != VK_SUCCESS)
                throw std::runtime_error("Failed to create command pool");
        }

        // Allocate command buffers
        {
            s_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = Vulkan_Shared::s_commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

            vkAllocateCommandBuffers(Vulkan_Shared::s_device, &allocInfo, s_commandBuffers.data());
        }

        // Create sync objects
        {
            s_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            // Create enough acquire and render-finished semaphores to cover all swapchain images.
            // This prevents reusing a semaphore that's still referenced by a presented-but-not-re-acquired image.
            uint32_t swapImageCount = (uint32_t)s_swapchainImages.size();
            uint32_t semCount = std::max(swapImageCount, (uint32_t)MAX_FRAMES_IN_FLIGHT);
            s_imageAvailableSemaphores.resize(semCount);
            s_renderFinishedSemaphores.resize(semCount);

            VkFenceCreateInfo fenceCI = {};
            fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VkSemaphoreCreateInfo semCI = {};
            semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                vkCreateFence(Vulkan_Shared::s_device, &fenceCI, nullptr, &s_inFlightFences[i]);
            }
            for (uint32_t i = 0; i < semCount; i++)
            {
                vkCreateSemaphore(Vulkan_Shared::s_device, &semCI, nullptr, &s_imageAvailableSemaphores[i]);
                vkCreateSemaphore(Vulkan_Shared::s_device, &semCI, nullptr, &s_renderFinishedSemaphores[i]);
            }
        }

        // Store window
        SDL2_Shared::setWindow(window);
        SDL_GetWindowSize(window, &lastWidth, &lastHeight);

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(Vulkan_Shared::s_physicalDevice, &props);
        std::cout << "Vulkan initialized: " << props.deviceName
                  << " (API " << VK_VERSION_MAJOR(props.apiVersion)
                  << "." << VK_VERSION_MINOR(props.apiVersion) << ")" << std::endl;
    }

    SDL_Window* getWindow() const { return window; }
    const GLCapabilities& getCapabilities() const { return capabilities; }

    void beginFrame()
    {
        int frame = Vulkan_Shared::s_currentFrame;

        vkWaitForFences(Vulkan_Shared::s_device, 1, &s_inFlightFences[frame], VK_TRUE, UINT64_MAX);

        // Use a separate rotating index for acquire semaphores to avoid reusing one
        // that's still referenced by a presented-but-not-re-acquired swapchain image.
        uint32_t semIdx = s_acquireSemaphoreIndex;
        s_acquireSemaphoreIndex = (s_acquireSemaphoreIndex + 1) % (uint32_t)s_imageAvailableSemaphores.size();

        VkResult result = vkAcquireNextImageKHR(Vulkan_Shared::s_device, Vulkan_Shared::s_swapchain,
            UINT64_MAX, s_imageAvailableSemaphores[semIdx], VK_NULL_HANDLE, &Vulkan_Shared::s_currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapchain();
            result = vkAcquireNextImageKHR(Vulkan_Shared::s_device, Vulkan_Shared::s_swapchain,
                UINT64_MAX, s_imageAvailableSemaphores[semIdx], VK_NULL_HANDLE, &Vulkan_Shared::s_currentImageIndex);
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            Vulkan_Shared::s_currentCommandBuffer = VK_NULL_HANDLE;
            Vulkan_Shared::s_renderPassActive = false;
            return;
        }

        // Store which acquire semaphore was used for this frame, so swapBuffers can wait on it
        Vulkan_Shared::s_currentAcquireSemaphoreIndex = semIdx;

        vkResetFences(Vulkan_Shared::s_device, 1, &s_inFlightFences[frame]);

        VkCommandBuffer cb = s_commandBuffers[frame];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &beginInfo);

        Vulkan_Shared::s_currentCommandBuffer = cb;
        Vulkan_Shared::s_renderPassActive = false;
    }

    void swapBuffers()
    {
        int frame = Vulkan_Shared::s_currentFrame;
        VkCommandBuffer cb = s_commandBuffers[frame];

        // If beginFrame failed to acquire an image, skip submit/present
        if (Vulkan_Shared::s_currentCommandBuffer == VK_NULL_HANDLE)
        {
            Vulkan_Shared::s_currentFrame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
            beginFrame();
            return;
        }

        // If no render pass was started this frame, begin one to transition the
        // swapchain image from UNDEFINED to PRESENT_SRC_KHR via the render pass.
        if (!Vulkan_Shared::s_renderPassActive)
        {
            VkClearValue clearValues[2] = {};
            clearValues[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo rpBI = {};
            rpBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpBI.renderPass = Vulkan_Shared::s_renderPass;
            rpBI.framebuffer = Vulkan_Shared::getSwapchainFramebuffers()[Vulkan_Shared::s_currentImageIndex];
            rpBI.renderArea.extent = Vulkan_Shared::getSwapchainExtent();
            rpBI.clearValueCount = 2;
            rpBI.pClearValues = clearValues;
            vkCmdBeginRenderPass(cb, &rpBI, VK_SUBPASS_CONTENTS_INLINE);
        }

        // End render pass
        vkCmdEndRenderPass(cb);
        Vulkan_Shared::s_renderPassActive = false;

        vkEndCommandBuffer(cb);

        // Submit
        // Use the swapchain image index for render-finished semaphore.  Each image
        // can only be acquired after its previous present completes, so the semaphore
        // is guaranteed to have been consumed by the time we signal it again.
        uint32_t renderSemIdx = Vulkan_Shared::s_currentImageIndex;

        VkSemaphore waitSemaphores[] = { s_imageAvailableSemaphores[Vulkan_Shared::s_currentAcquireSemaphoreIndex] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore signalSemaphores[] = { s_renderFinishedSemaphores[renderSemIdx] };

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult submitResult = vkQueueSubmit(Vulkan_Shared::s_graphicsQueue, 1, &submitInfo, s_inFlightFences[frame]);
        if (submitResult != VK_SUCCESS)
        {
            // Submit failed — fence was not enqueued, so it will never signal.
            // Re-signal the fence manually so the next beginFrame doesn't hang.
            std::cerr << "Vulkan: vkQueueSubmit failed (" << submitResult << ")" << std::endl;
            Vulkan_Shared::s_currentFrame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
            beginFrame();
            return;
        }

        // Present
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &Vulkan_Shared::s_swapchain;
        presentInfo.pImageIndices = &Vulkan_Shared::s_currentImageIndex;

        VkResult result = vkQueuePresentKHR(Vulkan_Shared::s_graphicsQueue, &presentInfo);

        // Check for resize
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || w != lastWidth || h != lastHeight)
        {
            lastWidth = w;
            lastHeight = h;
            recreateSwapchain();
        }

        // Advance frame
        Vulkan_Shared::s_currentFrame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;

        // Begin next frame
        beginFrame();
    }
};

static VulkanContext& getContext()
{
    static VulkanContext context;
    return context;
}

SDL_Window* getWindow()
{
    return getContext().getWindow();
}

SDL_GLContext getGLContext()
{
    return nullptr;
}

void swapBuffers()
{
    getContext().swapBuffers();
}

} // namespace detail

void instantiate()
{
    detail::getContext();
    // Start the first frame
    detail::getContext().beginFrame();
}

const detail::GLCapabilities& getCapabilities()
{
    return detail::getContext().getCapabilities();
}

} // namespace GLContext
} // namespace lwjgl
