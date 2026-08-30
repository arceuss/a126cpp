#include "backends/Backend.h"
#include "backends/Vulkan/Shaders.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "backends/Platform/Platform.h"
#include "legacygl/LegacyGL.h"
#include "legacygl/Sink.h"

#include <vulkan/vulkan.h>

namespace vulkanbackend
{

static const int VULKAN_TEXTURE_LEVELS = 16;
static const VkDeviceSize VULKAN_STREAM_CHUNK_SIZE = 4 * 1024 * 1024;
static const char *VULKAN_PORTABILITY_SUBSET_EXTENSION = "VK_KHR_portability_subset";

struct VulkanGPUVertex
{
	float position[3];
	float color[4];
	float normal[3];
	float texCoord[2];
	float flatPosition[3];
	float flatColor[4];
	float flatNormal[3];
};

struct alignas(16) VulkanGPUMaterial
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float emission[4];
	float shininess[4];
};

struct alignas(16) VulkanGPULight
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float positionEye[4];
	float spotDirectionCutoff[4];
	float attenuationExponent[4];
};

struct alignas(16) VulkanGPUState
{
	float modelView[16];
	float projection[16];
	float texture[16];
	float normal[16];
	float globalAmbient[4];
	VulkanGPUMaterial frontMaterial;
	VulkanGPUMaterial backMaterial;
	VulkanGPULight lights[8];
	float fogColor[4];
	float fogParams[4];
	float textureSize[4];
	float normalParams[4];
	unsigned int flags0[4];
	unsigned int flags1[4];
	unsigned int flags2[4];
	unsigned int flags3[4];
};

static_assert(sizeof(VulkanGPUVertex) == 88, "Vulkan vertex ABI changed");
static_assert(sizeof(unsigned int) == 4, "Vulkan shader flags require 32-bit unsigned int");
static_assert(sizeof(VulkanGPUMaterial) == 80, "Vulkan shader material ABI changed");
static_assert(sizeof(VulkanGPULight) == 96, "Vulkan shader light ABI changed");
static_assert(offsetof(VulkanGPUState, modelView) == 0, "Vulkan shader model-view ABI changed");
static_assert(offsetof(VulkanGPUState, projection) == 64, "Vulkan shader projection ABI changed");
static_assert(offsetof(VulkanGPUState, texture) == 128, "Vulkan shader texture matrix ABI changed");
static_assert(offsetof(VulkanGPUState, normal) == 192, "Vulkan shader normal matrix ABI changed");
static_assert(offsetof(VulkanGPUState, globalAmbient) == 256, "Vulkan shader ambient ABI changed");
static_assert(offsetof(VulkanGPUState, frontMaterial) == 272, "Vulkan shader front material ABI changed");
static_assert(offsetof(VulkanGPUState, backMaterial) == 352, "Vulkan shader back material ABI changed");
static_assert(offsetof(VulkanGPUState, lights) == 432, "Vulkan shader light ABI changed");
static_assert(offsetof(VulkanGPUState, fogColor) == 1200, "Vulkan shader fog color ABI changed");
static_assert(offsetof(VulkanGPUState, fogParams) == 1216, "Vulkan shader fog parameters ABI changed");
static_assert(offsetof(VulkanGPUState, textureSize) == 1232, "Vulkan shader texture size ABI changed");
static_assert(offsetof(VulkanGPUState, normalParams) == 1248, "Vulkan shader normal parameters ABI changed");
static_assert(offsetof(VulkanGPUState, flags0) == 1264, "Vulkan shader flags0 ABI changed");
static_assert(offsetof(VulkanGPUState, flags1) == 1280, "Vulkan shader flags1 ABI changed");
static_assert(offsetof(VulkanGPUState, flags2) == 1296, "Vulkan shader flags2 ABI changed");
static_assert(offsetof(VulkanGPUState, flags3) == 1312, "Vulkan shader flags3 ABI changed");
static_assert(sizeof(VulkanGPUState) == 1328, "Vulkan shader block ABI changed");

struct BufferResource
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize size = 0;
	void *mapped = nullptr;
	bool coherent = false;
};

struct StreamChunk
{
	BufferResource buffer;
	VkDeviceSize used = 0;
	bool dirty = false;
};

struct StreamAllocation
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceSize offset = 0;
	void *mapped = nullptr;
};

struct ImageResource
{
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
};

struct VulkanTextureLevel
{
	int width = 0;
	int height = 0;
	bool defined = false;
	std::vector<unsigned char> rgba;
};

struct VulkanTexture
{
	VulkanTextureLevel levels[VULKAN_TEXTURE_LEVELS];
	ImageResource image;
	bool derivedDirty = true;
	bool derivedHasGutter = false;
	unsigned int derivedWrapS = 0;
	unsigned int derivedWrapT = 0;
	unsigned char derivedBorder[4] = { 0, 0, 0, 0 };
};

struct SamplerKey
{
	unsigned int minFilter = 0;
	unsigned int magFilter = 0;
	unsigned int wrapS = 0;
	unsigned int wrapT = 0;

	bool operator<(const SamplerKey &other) const
	{
		return std::tie(minFilter, magFilter, wrapS, wrapT) <
			std::tie(other.minFilter, other.magFilter, other.wrapS, other.wrapT);
	}
};

struct PipelineKey
{
	legacygl::Topology topology = legacygl::Topology::Triangles;
	bool depthTest = false;
	bool depthWrite = false;
	unsigned int depthFunction = 0;
	bool cullFace = false;
	unsigned int cullFaceMode = 0;
	unsigned int frontFaceMode = 0;
	bool blend = false;
	unsigned int blendSource = 0;
	unsigned int blendDestination = 0;
	bool logicOp = false;
	unsigned int logicOpcode = 0;
	unsigned int colorWriteMask = 0;
	bool depthBias = false;
	bool stencilTest = false;

	bool operator<(const PipelineKey &other) const
	{
		return std::tie(topology, depthTest, depthWrite, depthFunction, cullFace,
			cullFaceMode, frontFaceMode, blend, blendSource, blendDestination,
			logicOp, logicOpcode, colorWriteMask, depthBias, stencilTest) <
			std::tie(other.topology, other.depthTest, other.depthWrite, other.depthFunction,
			other.cullFace, other.cullFaceMode, other.frontFaceMode, other.blend,
			other.blendSource, other.blendDestination, other.logicOp, other.logicOpcode,
			other.colorWriteMask, other.depthBias, other.stencilTest);
	}
};

class LogicalNameAllocator
{
public:
	unsigned int allocate()
	{
		if (names.size() == static_cast<std::size_t>(std::numeric_limits<unsigned int>::max()))
			return 0;
		while (names.find(nextName) != names.end())
			advance();
		const unsigned int name = nextName;
		names.insert(name);
		advance();
		return name;
	}

	void reserve(unsigned int name)
	{
		if (name != 0)
			names.insert(name);
	}

	void release(unsigned int name)
	{
		if (name != 0)
			names.erase(name);
	}

private:
	void advance()
	{
		nextName = nextName == std::numeric_limits<unsigned int>::max() ? 1 : nextName + 1;
	}

	std::set<unsigned int> names;
	unsigned int nextName = 1;
};

struct QueueFamilies
{
	uint32_t graphics = std::numeric_limits<uint32_t>::max();
	uint32_t present = std::numeric_limits<uint32_t>::max();

	bool complete() const
	{
		return graphics != std::numeric_limits<uint32_t>::max() &&
			present != std::numeric_limits<uint32_t>::max();
	}
};

struct SwapchainSupport
{
	VkSurfaceCapabilitiesKHR capabilities = {};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct State
{
	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties physicalProperties = {};
	VkPhysicalDeviceMemoryProperties memoryProperties = {};
	VkPhysicalDeviceFeatures physicalFeatures = {};
	VkDevice device = VK_NULL_HANDLE;
	QueueFamilies queueFamilies;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapchainExtent = {};
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> framebuffers;

	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	ImageResource colorTarget;
	ImageResource depthTarget;
	VkExtent2D targetExtent = {};
	VkRenderPass legacyRenderPass = VK_NULL_HANDLE;
	VkFramebuffer legacyFramebuffer = VK_NULL_HANDLE;
	VkDescriptorSetLayout legacyDescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout legacyPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout clearPipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout presentDescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout presentPipelineLayout = VK_NULL_HANDLE;
	VkPipeline presentPipeline = VK_NULL_HANDLE;
	VkSampler presentSampler = VK_NULL_HANDLE;
	VkShaderModule legacyVertexShader = VK_NULL_HANDLE;
	VkShaderModule legacyFragmentShader = VK_NULL_HANDLE;
	VkShaderModule clearVertexShader = VK_NULL_HANDLE;
	VkShaderModule clearFragmentShader = VK_NULL_HANDLE;
	VkShaderModule presentVertexShader = VK_NULL_HANDLE;
	VkShaderModule presentFragmentShader = VK_NULL_HANDLE;
	std::map<PipelineKey, VkPipeline> pipelines;
	std::map<unsigned int, VkPipeline> clearPipelines;
	std::map<SamplerKey, VkSampler> samplers;
	std::map<unsigned int, VulkanTexture> textures;
	ImageResource fallbackTexture;

	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkSemaphore imageAvailable = VK_NULL_HANDLE;
	VkSemaphore renderingFinished = VK_NULL_HANDLE;
	VkFence frameFence = VK_NULL_HANDLE;
	std::vector<VkDescriptorPool> descriptorPools;
	std::size_t activeDescriptorPool = 0;
	std::vector<StreamChunk> streamChunks;
	std::vector<BufferResource> transientBuffers;
	std::vector<ImageResource> retiredImages;
	bool commandRecording = false;
	bool legacyPassActive = false;
	bool targetsNeedTransition = false;
	bool logicOpSupported = false;
	bool wideLinesSupported = false;
	VkLineRasterizationModeEXT lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT;
	uint32_t lineSubPixelPrecisionBits = 0;
	float lineRasterizationBias = 0.0f;
	bool validationEnabled = false;
	unsigned int validationErrorCount = 0;
	bool initialized = false;
};

static State state;

static void submitAndWait();
static void imageBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout,
	VkImageLayout newLayout, VkAccessFlags sourceAccess, VkAccessFlags destinationAccess,
	VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
static VkPipeline presentPipeline();
static void setFullscreenViewport(VkExtent2D extent);

static void requireSuccess(VkResult result, const char *operation)
{
	if (result == VK_SUCCESS)
		return;
	throw std::runtime_error(std::string(operation) + " failed with VkResult " +
		std::to_string(static_cast<int>(result)));
}

static const char *deviceTypeName(VkPhysicalDeviceType type)
{
	switch (type)
	{
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return "discrete";
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return "integrated";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return "virtual";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return "cpu";
		default:
			return "other";
	}
}

static std::string versionString(uint32_t version)
{
	return std::to_string(VK_VERSION_MAJOR(version)) + "." +
		std::to_string(VK_VERSION_MINOR(version)) + "." +
		std::to_string(VK_VERSION_PATCH(version));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
	void *)
{
	if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
		state.validationErrorCount++;
	std::cerr << "vulkan-validation: " << callbackData->pMessage << '\n';
	return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = &debugCallback;
	return createInfo;
}

static bool instanceLayerAvailable(const char *name)
{
	uint32_t count = 0;
	requireSuccess(vkEnumerateInstanceLayerProperties(&count, nullptr),
		"vkEnumerateInstanceLayerProperties");
	std::vector<VkLayerProperties> layers(count);
	if (count != 0)
		requireSuccess(vkEnumerateInstanceLayerProperties(&count, layers.data()),
			"vkEnumerateInstanceLayerProperties");
	for (const VkLayerProperties &layer : layers)
	{
		if (std::strcmp(layer.layerName, name) == 0)
			return true;
	}
	return false;
}

static bool instanceExtensionAvailable(const char *name)
{
	uint32_t count = 0;
	requireSuccess(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr),
		"vkEnumerateInstanceExtensionProperties");
	std::vector<VkExtensionProperties> extensions(count);
	if (count != 0)
		requireSuccess(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()),
			"vkEnumerateInstanceExtensionProperties");
	for (const VkExtensionProperties &extension : extensions)
	{
		if (std::strcmp(extension.extensionName, name) == 0)
			return true;
	}
	return false;
}

static bool containsExtension(const std::vector<const char *> &extensions, const char *name)
{
	for (const char *extension : extensions)
	{
		if (std::strcmp(extension, name) == 0)
			return true;
	}
	return false;
}

static void createInstance()
{
	uint32_t loaderVersion = VK_API_VERSION_1_0;
	PFN_vkEnumerateInstanceVersion enumerateInstanceVersion =
		reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
			vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
	if (enumerateInstanceVersion != nullptr)
		requireSuccess(enumerateInstanceVersion(&loaderVersion), "vkEnumerateInstanceVersion");
	if (loaderVersion < VK_API_VERSION_1_1)
		throw std::runtime_error("Vulkan 1.1 or newer is required");

	unsigned int extensionCount = 0;
	platform::getRequiredVulkanInstanceExtensions(extensionCount, nullptr);
	std::vector<const char *> extensions(extensionCount);
	if (extensionCount != 0)
		platform::getRequiredVulkanInstanceExtensions(extensionCount, extensions.data());
	extensions.resize(extensionCount);
	for (const char *extension : extensions)
	{
		if (!instanceExtensionAvailable(extension))
			throw std::runtime_error(std::string("required Vulkan instance extension is unavailable: ") + extension);
	}

#if defined(NDEBUG)
	const bool requestValidation = false;
#else
	const bool requestValidation = true;
#endif
	state.validationEnabled = requestValidation &&
		instanceLayerAvailable("VK_LAYER_KHRONOS_validation");
	if (requestValidation && !state.validationEnabled)
		std::cout << "vulkan: VK_LAYER_KHRONOS_validation is unavailable\n";

	const bool enableDebugUtils = state.validationEnabled &&
		instanceExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	if (enableDebugUtils && !containsExtension(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	const bool enablePortabilityEnumeration =
		instanceExtensionAvailable(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	if (enablePortabilityEnumeration &&
		!containsExtension(extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
	{
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	}

	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "Minecraft Alpha v1.2.6";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 6);
	applicationInfo.pEngineName = "a126cpp LegacyGL";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_1;

	const char *validationLayer = "VK_LAYER_KHRONOS_validation";
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = debugMessengerCreateInfo();
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	if (enablePortabilityEnumeration)
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	createInfo.pApplicationInfo = &applicationInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	if (state.validationEnabled)
	{
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &validationLayer;
		if (enableDebugUtils)
			createInfo.pNext = &debugCreateInfo;
	}

	requireSuccess(vkCreateInstance(&createInfo, nullptr, &state.instance), "vkCreateInstance");

	if (enableDebugUtils)
	{
		PFN_vkCreateDebugUtilsMessengerEXT createDebugMessenger =
			reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(state.instance, "vkCreateDebugUtilsMessengerEXT"));
		if (createDebugMessenger != nullptr)
			requireSuccess(createDebugMessenger(state.instance, &debugCreateInfo, nullptr,
				&state.debugMessenger), "vkCreateDebugUtilsMessengerEXT");
	}

	std::cout << "vulkan: loader " << versionString(loaderVersion) <<
		", validation=" << (state.validationEnabled ? "on" : "off") << '\n';
}

static QueueFamilies findQueueFamilies(VkPhysicalDevice device)
{
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
	std::vector<VkQueueFamilyProperties> properties(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

	QueueFamilies result;
	for (uint32_t i = 0; i < count; i++)
	{
		VkBool32 presentSupport = VK_FALSE;
		requireSuccess(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, state.surface,
			&presentSupport), "vkGetPhysicalDeviceSurfaceSupportKHR");
		const bool graphicsSupport = (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
		if (graphicsSupport && presentSupport == VK_TRUE)
		{
			result.graphics = i;
			result.present = i;
			return result;
		}
		if (graphicsSupport && result.graphics == std::numeric_limits<uint32_t>::max())
			result.graphics = i;
		if (presentSupport == VK_TRUE && result.present == std::numeric_limits<uint32_t>::max())
			result.present = i;
	}
	return result;
}

static bool deviceExtensionAvailable(VkPhysicalDevice device, const char *name)
{
	uint32_t count = 0;
	requireSuccess(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr),
		"vkEnumerateDeviceExtensionProperties");
	std::vector<VkExtensionProperties> extensions(count);
	if (count != 0)
		requireSuccess(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()),
			"vkEnumerateDeviceExtensionProperties");
	for (const VkExtensionProperties &extension : extensions)
	{
		if (std::strcmp(extension.extensionName, name) == 0)
			return true;
	}
	return false;
}

static SwapchainSupport querySwapchainSupport(VkPhysicalDevice device)
{
	SwapchainSupport support;
	requireSuccess(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, state.surface,
		&support.capabilities), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

	uint32_t count = 0;
	requireSuccess(vkGetPhysicalDeviceSurfaceFormatsKHR(device, state.surface, &count, nullptr),
		"vkGetPhysicalDeviceSurfaceFormatsKHR");
	support.formats.resize(count);
	if (count != 0)
		requireSuccess(vkGetPhysicalDeviceSurfaceFormatsKHR(device, state.surface, &count,
			support.formats.data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");

	count = 0;
	requireSuccess(vkGetPhysicalDeviceSurfacePresentModesKHR(device, state.surface, &count, nullptr),
		"vkGetPhysicalDeviceSurfacePresentModesKHR");
	support.presentModes.resize(count);
	if (count != 0)
		requireSuccess(vkGetPhysicalDeviceSurfacePresentModesKHR(device, state.surface, &count,
			support.presentModes.data()), "vkGetPhysicalDeviceSurfacePresentModesKHR");
	return support;
}

static bool deviceSuitable(VkPhysicalDevice device, QueueFamilies &queueFamilies,
	VkPhysicalDeviceProperties &properties)
{
	vkGetPhysicalDeviceProperties(device, &properties);
	if (properties.apiVersion < VK_API_VERSION_1_1)
		return false;
	if (!deviceExtensionAvailable(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
		return false;
	VkPhysicalDeviceFeatures features = {};
	vkGetPhysicalDeviceFeatures(device, &features);
	if (features.logicOp != VK_TRUE)
		return false;
	queueFamilies = findQueueFamilies(device);
	if (!queueFamilies.complete())
		return false;
	SwapchainSupport support = querySwapchainSupport(device);
	return !support.formats.empty() && !support.presentModes.empty() &&
		(support.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0;
}

static void selectPhysicalDevice()
{
	uint32_t count = 0;
	requireSuccess(vkEnumeratePhysicalDevices(state.instance, &count, nullptr),
		"vkEnumeratePhysicalDevices");
	if (count == 0)
		throw std::runtime_error("no Vulkan physical devices were found");
	std::vector<VkPhysicalDevice> devices(count);
	requireSuccess(vkEnumeratePhysicalDevices(state.instance, &count, devices.data()),
		"vkEnumeratePhysicalDevices");

	int bestScore = -1;
	VkPhysicalDeviceProperties bestProperties = {};
	for (VkPhysicalDevice device : devices)
	{
		QueueFamilies queueFamilies;
		VkPhysicalDeviceProperties properties = {};
		if (!deviceSuitable(device, queueFamilies, properties))
			continue;
		int score = static_cast<int>(properties.limits.maxImageDimension2D);
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 100000;
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			score += 50000;
		if (score > bestScore)
		{
			bestScore = score;
			state.physicalDevice = device;
			state.queueFamilies = queueFamilies;
			bestProperties = properties;
		}
	}
	if (state.physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("no Vulkan 1.1 device with graphics, presentation, "
			"swapchain, and logic-op support was found");
	state.physicalProperties = bestProperties;
	vkGetPhysicalDeviceMemoryProperties(state.physicalDevice, &state.memoryProperties);
	vkGetPhysicalDeviceFeatures(state.physicalDevice, &state.physicalFeatures);

	std::cout << "vulkan: device " << bestProperties.deviceName << " (" <<
		deviceTypeName(bestProperties.deviceType) << "), api=" <<
		versionString(bestProperties.apiVersion) << ", driver=" <<
		bestProperties.driverVersion << '\n';
}

static void createDevice()
{
	std::vector<uint32_t> queueFamilyIndices = { state.queueFamilies.graphics };
	if (state.queueFamilies.present != state.queueFamilies.graphics)
		queueFamilyIndices.push_back(state.queueFamilies.present);
	const float priority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (uint32_t family : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = family;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &priority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	std::vector<const char *> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	if (deviceExtensionAvailable(state.physicalDevice, VULKAN_PORTABILITY_SUBSET_EXTENSION))
		extensions.push_back(VULKAN_PORTABILITY_SUBSET_EXTENSION);
	VkPhysicalDeviceFeatures features = {};
	features.logicOp = state.physicalFeatures.logicOp;
	features.wideLines = state.physicalFeatures.wideLines;
	VkPhysicalDeviceLineRasterizationFeaturesEXT availableLineFeatures = {};
	availableLineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
	VkPhysicalDeviceFeatures2 availableFeatures = {};
	availableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	availableFeatures.pNext = &availableLineFeatures;
	VkPhysicalDeviceLineRasterizationFeaturesEXT enabledLineFeatures = {};
	enabledLineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
	const char *lineRasterizationExtension = nullptr;
	if (deviceExtensionAvailable(state.physicalDevice, VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME))
		lineRasterizationExtension = VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME;
	else if (deviceExtensionAvailable(state.physicalDevice, VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME))
		lineRasterizationExtension = VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME;
	if (lineRasterizationExtension != nullptr)
	{
		vkGetPhysicalDeviceFeatures2(state.physicalDevice, &availableFeatures);
		if (availableLineFeatures.bresenhamLines == VK_TRUE)
		{
			extensions.push_back(lineRasterizationExtension);
			enabledLineFeatures.bresenhamLines = VK_TRUE;
			state.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT;
			VkPhysicalDeviceLineRasterizationPropertiesEXT lineProperties = {};
			lineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT;
			VkPhysicalDeviceProperties2 extendedProperties = {};
			extendedProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			extendedProperties.pNext = &lineProperties;
			vkGetPhysicalDeviceProperties2(state.physicalDevice, &extendedProperties);
			state.lineSubPixelPrecisionBits = lineProperties.lineSubPixelPrecisionBits;
			state.lineRasterizationBias = std::ldexp(1.0f,
				-static_cast<int>(state.lineSubPixelPrecisionBits));
		}
	}
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = state.lineRasterizationMode != VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT ?
		&enabledLineFeatures : nullptr;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.pEnabledFeatures = &features;
	requireSuccess(vkCreateDevice(state.physicalDevice, &createInfo, nullptr, &state.device),
		"vkCreateDevice");
	vkGetDeviceQueue(state.device, state.queueFamilies.graphics, 0, &state.graphicsQueue);
	vkGetDeviceQueue(state.device, state.queueFamilies.present, 0, &state.presentQueue);
	state.logicOpSupported = features.logicOp == VK_TRUE;
	state.wideLinesSupported = features.wideLines == VK_TRUE;
	if (state.lineRasterizationMode == VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT)
	{
		std::cout << "vulkan: line rasterization=bresenham, subpixelBits=" <<
			state.lineSubPixelPrecisionBits << '\n';
	}
	else
	{
		std::cout << "vulkan: line rasterization=default-fallback\n";
	}
}

static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	for (const VkSurfaceFormatKHR &format : formats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}
	for (const VkSurfaceFormatKHR &format : formats)
	{
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}
	return formats[0];
}

static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	int width = 0;
	int height = 0;
	platform::getDrawableSize(width, height);
	if (width <= 0 || height <= 0)
		return {};
	VkExtent2D extent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};
	extent.width = std::max(capabilities.minImageExtent.width,
		std::min(capabilities.maxImageExtent.width, extent.width));
	extent.height = std::max(capabilities.minImageExtent.height,
		std::min(capabilities.maxImageExtent.height, extent.height));
	return extent;
}

static VkCompositeAlphaFlagBitsKHR chooseCompositeAlpha(VkCompositeAlphaFlagsKHR supported)
{
	const VkCompositeAlphaFlagBitsKHR choices[] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
	};
	for (VkCompositeAlphaFlagBitsKHR choice : choices)
	{
		if ((supported & choice) != 0)
			return choice;
	}
	return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

static uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags required,
	VkMemoryPropertyFlags preferred)
{
	for (uint32_t i = 0; i < state.memoryProperties.memoryTypeCount; i++)
	{
		const VkMemoryPropertyFlags flags = state.memoryProperties.memoryTypes[i].propertyFlags;
		if ((typeBits & (1u << i)) != 0 && (flags & required) == required &&
			(flags & preferred) == preferred)
		{
			return i;
		}
	}
	for (uint32_t i = 0; i < state.memoryProperties.memoryTypeCount; i++)
	{
		const VkMemoryPropertyFlags flags = state.memoryProperties.memoryTypes[i].propertyFlags;
		if ((typeBits & (1u << i)) != 0 && (flags & required) == required)
			return i;
	}
	throw std::runtime_error("no compatible Vulkan memory type was found");
}

static BufferResource createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred, bool map)
{
	BufferResource result;
	result.size = size;
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	requireSuccess(vkCreateBuffer(state.device, &createInfo, nullptr, &result.buffer), "vkCreateBuffer");

	VkMemoryRequirements requirements = {};
	vkGetBufferMemoryRequirements(state.device, result.buffer, &requirements);
	const uint32_t memoryType = findMemoryType(requirements.memoryTypeBits, required, preferred);
	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = requirements.size;
	allocateInfo.memoryTypeIndex = memoryType;
	requireSuccess(vkAllocateMemory(state.device, &allocateInfo, nullptr, &result.memory),
		"vkAllocateMemory(buffer)");
	requireSuccess(vkBindBufferMemory(state.device, result.buffer, result.memory, 0),
		"vkBindBufferMemory");
	result.coherent = (state.memoryProperties.memoryTypes[memoryType].propertyFlags &
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
	if (map)
		requireSuccess(vkMapMemory(state.device, result.memory, 0, requirements.size, 0, &result.mapped),
			"vkMapMemory(buffer)");
	return result;
}

static void destroyBuffer(BufferResource &buffer)
{
	if (buffer.mapped != nullptr)
		vkUnmapMemory(state.device, buffer.memory);
	if (buffer.buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(state.device, buffer.buffer, nullptr);
	if (buffer.memory != VK_NULL_HANDLE)
		vkFreeMemory(state.device, buffer.memory, nullptr);
	buffer = BufferResource();
}

static void flushBuffer(const BufferResource &buffer)
{
	if (buffer.coherent || buffer.memory == VK_NULL_HANDLE)
		return;
	VkMappedMemoryRange range = {};
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.memory = buffer.memory;
	range.offset = 0;
	range.size = VK_WHOLE_SIZE;
	requireSuccess(vkFlushMappedMemoryRanges(state.device, 1, &range), "vkFlushMappedMemoryRanges");
}

static void invalidateBuffer(const BufferResource &buffer)
{
	if (buffer.coherent || buffer.memory == VK_NULL_HANDLE)
		return;
	VkMappedMemoryRange range = {};
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.memory = buffer.memory;
	range.offset = 0;
	range.size = VK_WHOLE_SIZE;
	requireSuccess(vkInvalidateMappedMemoryRanges(state.device, 1, &range),
		"vkInvalidateMappedMemoryRanges");
}

static VkDeviceSize alignDeviceSize(VkDeviceSize value, VkDeviceSize alignment)
{
	if (alignment <= 1)
		return value;
	const VkDeviceSize remainder = value % alignment;
	if (remainder == 0)
		return value;
	const VkDeviceSize addition = alignment - remainder;
	if (value > std::numeric_limits<VkDeviceSize>::max() - addition)
		throw std::runtime_error("Vulkan stream allocation size overflow");
	return value + addition;
}

static StreamAllocation allocateStreamBuffer(VkDeviceSize size, VkDeviceSize alignment)
{
	for (StreamChunk &chunk : state.streamChunks)
	{
		const VkDeviceSize offset = alignDeviceSize(chunk.used, alignment);
		if (offset <= chunk.buffer.size && size <= chunk.buffer.size - offset)
		{
			chunk.used = offset + size;
			chunk.dirty = true;
			StreamAllocation allocation;
			allocation.buffer = chunk.buffer.buffer;
			allocation.offset = offset;
			allocation.mapped = static_cast<unsigned char *>(chunk.buffer.mapped) + offset;
			return allocation;
		}
	}

	StreamChunk chunk;
	chunk.buffer = createBuffer(std::max(VULKAN_STREAM_CHUNK_SIZE,
		alignDeviceSize(size, alignment)), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
	state.streamChunks.push_back(chunk);
	StreamChunk &stored = state.streamChunks.back();
	stored.used = size;
	stored.dirty = true;
	StreamAllocation allocation;
	allocation.buffer = stored.buffer.buffer;
	allocation.mapped = stored.buffer.mapped;
	return allocation;
}

static void flushStreamBuffers()
{
	for (const StreamChunk &chunk : state.streamChunks)
	{
		if (chunk.dirty)
			flushBuffer(chunk.buffer);
	}
}

static void resetStreamBuffers()
{
	for (StreamChunk &chunk : state.streamChunks)
	{
		chunk.used = 0;
		chunk.dirty = false;
	}
}

static ImageResource createImage(uint32_t width, uint32_t height, VkFormat format,
	VkImageUsageFlags usage, VkImageAspectFlags aspect)
{
	ImageResource result;
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = format;
	createInfo.extent = { width, height, 1 };
	createInfo.mipLevels = 1;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	requireSuccess(vkCreateImage(state.device, &createInfo, nullptr, &result.image), "vkCreateImage");

	VkMemoryRequirements requirements = {};
	vkGetImageMemoryRequirements(state.device, result.image, &requirements);
	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = requirements.size;
	allocateInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	requireSuccess(vkAllocateMemory(state.device, &allocateInfo, nullptr, &result.memory),
		"vkAllocateMemory(image)");
	requireSuccess(vkBindImageMemory(state.device, result.image, result.memory, 0),
		"vkBindImageMemory");

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = result.image;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.subresourceRange.aspectMask = aspect;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.layerCount = 1;
	requireSuccess(vkCreateImageView(state.device, &viewCreateInfo, nullptr, &result.view),
		"vkCreateImageView");
	return result;
}

static void destroyImage(ImageResource &image)
{
	if (image.view != VK_NULL_HANDLE)
		vkDestroyImageView(state.device, image.view, nullptr);
	if (image.image != VK_NULL_HANDLE)
		vkDestroyImage(state.device, image.image, nullptr);
	if (image.memory != VK_NULL_HANDLE)
		vkFreeMemory(state.device, image.memory, nullptr);
	image = ImageResource();
}

static VkShaderModule createShaderModule(const std::uint32_t *code, std::size_t byteSize)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = byteSize;
	createInfo.pCode = code;
	VkShaderModule module = VK_NULL_HANDLE;
	requireSuccess(vkCreateShaderModule(state.device, &createInfo, nullptr, &module),
		"vkCreateShaderModule");
	return module;
}

static VkFormat chooseDepthFormat()
{
	const VkFormat choices[] = {
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT
	};
	for (VkFormat format : choices)
	{
		VkFormatProperties properties = {};
		vkGetPhysicalDeviceFormatProperties(state.physicalDevice, format, &properties);
		if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
			return format;
	}
	throw std::runtime_error("no supported Vulkan depth attachment format was found");
}

static void destroySwapchain()
{
	if (state.presentPipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(state.device, state.presentPipeline, nullptr);
	state.presentPipeline = VK_NULL_HANDLE;
	for (VkFramebuffer framebuffer : state.framebuffers)
		vkDestroyFramebuffer(state.device, framebuffer, nullptr);
	state.framebuffers.clear();
	if (state.renderPass != VK_NULL_HANDLE)
		vkDestroyRenderPass(state.device, state.renderPass, nullptr);
	state.renderPass = VK_NULL_HANDLE;
	for (VkImageView imageView : state.swapchainImageViews)
		vkDestroyImageView(state.device, imageView, nullptr);
	state.swapchainImageViews.clear();
	state.swapchainImages.clear();
	if (state.swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(state.device, state.swapchain, nullptr);
	state.swapchain = VK_NULL_HANDLE;
	state.swapchainFormat = VK_FORMAT_UNDEFINED;
	state.swapchainExtent = {};
}

static bool createSwapchain()
{
	SwapchainSupport support = querySwapchainSupport(state.physicalDevice);
	VkExtent2D extent = chooseExtent(support.capabilities);
	if (extent.width == 0 || extent.height == 0)
		return false;
	VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats);

	uint32_t imageCount = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount != 0 && imageCount > support.capabilities.maxImageCount)
		imageCount = support.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = state.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	const uint32_t queueFamilies[] = {
		state.queueFamilies.graphics,
		state.queueFamilies.present
	};
	if (state.queueFamilies.graphics != state.queueFamilies.present)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilies;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	createInfo.preTransform = support.capabilities.currentTransform;
	createInfo.compositeAlpha = chooseCompositeAlpha(support.capabilities.supportedCompositeAlpha);
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.clipped = VK_TRUE;
	requireSuccess(vkCreateSwapchainKHR(state.device, &createInfo, nullptr, &state.swapchain),
		"vkCreateSwapchainKHR");
	state.swapchainFormat = surfaceFormat.format;
	state.swapchainExtent = extent;

	requireSuccess(vkGetSwapchainImagesKHR(state.device, state.swapchain, &imageCount, nullptr),
		"vkGetSwapchainImagesKHR");
	state.swapchainImages.resize(imageCount);
	requireSuccess(vkGetSwapchainImagesKHR(state.device, state.swapchain, &imageCount,
		state.swapchainImages.data()), "vkGetSwapchainImagesKHR");

	for (VkImage image : state.swapchainImages)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = state.swapchainFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		VkImageView imageView = VK_NULL_HANDLE;
		requireSuccess(vkCreateImageView(state.device, &imageViewCreateInfo, nullptr, &imageView),
			"vkCreateImageView");
		state.swapchainImageViews.push_back(imageView);
	}

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = state.swapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;
	requireSuccess(vkCreateRenderPass(state.device, &renderPassCreateInfo, nullptr,
		&state.renderPass), "vkCreateRenderPass");

	for (VkImageView imageView : state.swapchainImageViews)
	{
		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = state.renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &imageView;
		framebufferCreateInfo.width = state.swapchainExtent.width;
		framebufferCreateInfo.height = state.swapchainExtent.height;
		framebufferCreateInfo.layers = 1;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		requireSuccess(vkCreateFramebuffer(state.device, &framebufferCreateInfo, nullptr,
			&framebuffer), "vkCreateFramebuffer");
		state.framebuffers.push_back(framebuffer);
	}

	std::cout << "vulkan: swapchain " << state.swapchainExtent.width << 'x' <<
		state.swapchainExtent.height << ", images=" << state.swapchainImages.size() << '\n';
	return true;
}

static void destroyRenderTargets()
{
	if (state.legacyFramebuffer != VK_NULL_HANDLE)
		vkDestroyFramebuffer(state.device, state.legacyFramebuffer, nullptr);
	state.legacyFramebuffer = VK_NULL_HANDLE;
	for (const std::pair<const PipelineKey, VkPipeline> &entry : state.pipelines)
		vkDestroyPipeline(state.device, entry.second, nullptr);
	state.pipelines.clear();
	for (const std::pair<const unsigned int, VkPipeline> &entry : state.clearPipelines)
		vkDestroyPipeline(state.device, entry.second, nullptr);
	state.clearPipelines.clear();
	if (state.legacyRenderPass != VK_NULL_HANDLE)
		vkDestroyRenderPass(state.device, state.legacyRenderPass, nullptr);
	state.legacyRenderPass = VK_NULL_HANDLE;
	destroyImage(state.depthTarget);
	destroyImage(state.colorTarget);
	state.targetExtent = {};
	state.targetsNeedTransition = false;
}

static void createRenderTargets()
{
	if (state.swapchainExtent.width == 0 || state.swapchainExtent.height == 0)
		return;
	if (state.depthFormat == VK_FORMAT_UNDEFINED)
		state.depthFormat = chooseDepthFormat();

	state.colorTarget = createImage(state.swapchainExtent.width, state.swapchainExtent.height,
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	VkImageAspectFlags depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (state.depthFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
		state.depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT)
	{
		depthAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	state.depthTarget = createImage(state.swapchainExtent.width, state.swapchainExtent.height,
		state.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthAspect);

	VkAttachmentDescription attachments[2] = {};
	attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[1].format = state.depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pDepthStencilAttachment = &depthReference;
	VkSubpassDependency dependencies[2] = {};
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 2;
	renderPassCreateInfo.pDependencies = dependencies;
	requireSuccess(vkCreateRenderPass(state.device, &renderPassCreateInfo, nullptr,
		&state.legacyRenderPass), "vkCreateRenderPass(legacy)");

	const VkImageView views[] = { state.colorTarget.view, state.depthTarget.view };
	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = state.legacyRenderPass;
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.pAttachments = views;
	framebufferCreateInfo.width = state.swapchainExtent.width;
	framebufferCreateInfo.height = state.swapchainExtent.height;
	framebufferCreateInfo.layers = 1;
	requireSuccess(vkCreateFramebuffer(state.device, &framebufferCreateInfo, nullptr,
		&state.legacyFramebuffer), "vkCreateFramebuffer(legacy)");
	state.targetExtent = state.swapchainExtent;
	state.targetsNeedTransition = true;
}

static void recreateSwapchain()
{
	if (state.device == VK_NULL_HANDLE)
		return;
	submitAndWait();
	requireSuccess(vkDeviceWaitIdle(state.device), "vkDeviceWaitIdle");
	destroyRenderTargets();
	destroySwapchain();
	if (createSwapchain())
		createRenderTargets();
}

static bool ensureRenderTargets()
{
	int width = 0;
	int height = 0;
	platform::getDrawableSize(width, height);
	if (width <= 0 || height <= 0)
		return false;
	if (state.swapchain == VK_NULL_HANDLE || state.targetExtent.width != static_cast<uint32_t>(width) ||
		state.targetExtent.height != static_cast<uint32_t>(height))
	{
		recreateSwapchain();
	}
	return state.colorTarget.image != VK_NULL_HANDLE;
}

static void createCommandResources()
{
	VkCommandPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolCreateInfo.queueFamilyIndex = state.queueFamilies.graphics;
	requireSuccess(vkCreateCommandPool(state.device, &poolCreateInfo, nullptr,
		&state.commandPool), "vkCreateCommandPool");

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = state.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	requireSuccess(vkAllocateCommandBuffers(state.device, &allocateInfo, &state.commandBuffer),
		"vkAllocateCommandBuffers");

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	requireSuccess(vkCreateSemaphore(state.device, &semaphoreCreateInfo, nullptr,
		&state.imageAvailable), "vkCreateSemaphore");
	requireSuccess(vkCreateSemaphore(state.device, &semaphoreCreateInfo, nullptr,
		&state.renderingFinished), "vkCreateSemaphore");

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	requireSuccess(vkCreateFence(state.device, &fenceCreateInfo, nullptr, &state.frameFence),
		"vkCreateFence");
}

static void createRendererResources()
{
	VkDescriptorSetLayoutBinding legacyBindings[2] = {};
	legacyBindings[0].binding = 0;
	legacyBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	legacyBindings[0].descriptorCount = 1;
	legacyBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	legacyBindings[1].binding = 1;
	legacyBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	legacyBindings[1].descriptorCount = 1;
	legacyBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
	descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutInfo.bindingCount = 2;
	descriptorLayoutInfo.pBindings = legacyBindings;
	requireSuccess(vkCreateDescriptorSetLayout(state.device, &descriptorLayoutInfo, nullptr,
		&state.legacyDescriptorSetLayout), "vkCreateDescriptorSetLayout(legacy)");

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &state.legacyDescriptorSetLayout;
	requireSuccess(vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr,
		&state.legacyPipelineLayout), "vkCreatePipelineLayout(legacy)");

	VkPushConstantRange clearPushRange = {};
	clearPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	clearPushRange.offset = 0;
	clearPushRange.size = 32;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &clearPushRange;
	requireSuccess(vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr,
		&state.clearPipelineLayout), "vkCreatePipelineLayout(clear)");

	VkDescriptorSetLayoutBinding presentBinding = {};
	presentBinding.binding = 0;
	presentBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	presentBinding.descriptorCount = 1;
	presentBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorLayoutInfo.bindingCount = 1;
	descriptorLayoutInfo.pBindings = &presentBinding;
	requireSuccess(vkCreateDescriptorSetLayout(state.device, &descriptorLayoutInfo, nullptr,
		&state.presentDescriptorSetLayout), "vkCreateDescriptorSetLayout(present)");
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &state.presentDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	requireSuccess(vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr,
		&state.presentPipelineLayout), "vkCreatePipelineLayout(present)");

	std::size_t byteSize = 0;
	const std::uint32_t *code = legacyVertexShaderCode(byteSize);
	state.legacyVertexShader = createShaderModule(code, byteSize);
	code = legacyFragmentShaderCode(byteSize);
	state.legacyFragmentShader = createShaderModule(code, byteSize);
	code = clearVertexShaderCode(byteSize);
	state.clearVertexShader = createShaderModule(code, byteSize);
	code = clearFragmentShaderCode(byteSize);
	state.clearFragmentShader = createShaderModule(code, byteSize);
	code = presentVertexShaderCode(byteSize);
	state.presentVertexShader = createShaderModule(code, byteSize);
	code = presentFragmentShaderCode(byteSize);
	state.presentFragmentShader = createShaderModule(code, byteSize);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxLod = 0.0f;
	requireSuccess(vkCreateSampler(state.device, &samplerInfo, nullptr, &state.presentSampler),
		"vkCreateSampler(present)");
}

static VkDescriptorPool createDescriptorPool()
{
	VkDescriptorPoolSize sizes[2] = {};
	sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sizes[0].descriptorCount = 256;
	sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sizes[1].descriptorCount = 256;
	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.maxSets = 256;
	createInfo.poolSizeCount = 2;
	createInfo.pPoolSizes = sizes;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	requireSuccess(vkCreateDescriptorPool(state.device, &createInfo, nullptr, &pool),
		"vkCreateDescriptorPool");
	return pool;
}

static VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout)
{
	for (;;)
	{
		if (state.activeDescriptorPool == state.descriptorPools.size())
			state.descriptorPools.push_back(createDescriptorPool());
		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = state.descriptorPools[state.activeDescriptorPool];
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &layout;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		const VkResult result = vkAllocateDescriptorSets(state.device, &allocateInfo, &descriptorSet);
		if (result == VK_SUCCESS)
			return descriptorSet;
		if (result != VK_ERROR_OUT_OF_POOL_MEMORY && result != VK_ERROR_FRAGMENTED_POOL)
			requireSuccess(result, "vkAllocateDescriptorSets");
		state.activeDescriptorPool++;
	}
}

static void cleanupSubmittedResources()
{
	for (BufferResource &buffer : state.transientBuffers)
		destroyBuffer(buffer);
	state.transientBuffers.clear();
	for (ImageResource &image : state.retiredImages)
		destroyImage(image);
	state.retiredImages.clear();
	for (VkDescriptorPool pool : state.descriptorPools)
		requireSuccess(vkResetDescriptorPool(state.device, pool, 0), "vkResetDescriptorPool");
	state.activeDescriptorPool = 0;
	resetStreamBuffers();
}

static void beginCommandRecording()
{
	if (state.commandRecording)
		return;
	requireSuccess(vkWaitForFences(state.device, 1, &state.frameFence, VK_TRUE,
		std::numeric_limits<uint64_t>::max()), "vkWaitForFences");
	requireSuccess(vkResetCommandBuffer(state.commandBuffer, 0), "vkResetCommandBuffer");
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	requireSuccess(vkBeginCommandBuffer(state.commandBuffer, &beginInfo), "vkBeginCommandBuffer");
	state.commandRecording = true;

	if (state.targetsNeedTransition)
	{
		VkImageMemoryBarrier barriers[2] = {};
		barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[0].image = state.colorTarget.image;
		barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barriers[0].subresourceRange.levelCount = 1;
		barriers[0].subresourceRange.layerCount = 1;
		barriers[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].image = state.depthTarget.image;
		barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (state.depthFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
			state.depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT)
		{
			barriers[1].subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		barriers[1].subresourceRange.levelCount = 1;
		barriers[1].subresourceRange.layerCount = 1;
		barriers[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		vkCmdPipelineBarrier(state.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);
		state.targetsNeedTransition = false;
	}
}

static void beginLegacyPass()
{
	beginCommandRecording();
	if (state.legacyPassActive)
		return;
	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = state.legacyRenderPass;
	beginInfo.framebuffer = state.legacyFramebuffer;
	beginInfo.renderArea.extent = state.targetExtent;
	vkCmdBeginRenderPass(state.commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	state.legacyPassActive = true;
}

static void endLegacyPass()
{
	if (!state.legacyPassActive)
		return;
	vkCmdEndRenderPass(state.commandBuffer);
	state.legacyPassActive = false;
}

static void submitAndWait()
{
	if (!state.commandRecording)
		return;
	endLegacyPass();
	requireSuccess(vkEndCommandBuffer(state.commandBuffer), "vkEndCommandBuffer");
	flushStreamBuffers();
	requireSuccess(vkResetFences(state.device, 1, &state.frameFence), "vkResetFences");
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &state.commandBuffer;
	requireSuccess(vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, state.frameFence),
		"vkQueueSubmit");
	requireSuccess(vkWaitForFences(state.device, 1, &state.frameFence, VK_TRUE,
		std::numeric_limits<uint64_t>::max()), "vkWaitForFences");
	state.commandRecording = false;
	cleanupSubmittedResources();
}

static void retireImage(ImageResource &image)
{
	if (image.image == VK_NULL_HANDLE)
		return;
	if (state.commandRecording)
	{
		state.retiredImages.push_back(image);
		image = ImageResource();
	}
	else
	{
		destroyImage(image);
	}
}

static void destroyResources()
{
	if (state.device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(state.device);
		state.commandRecording = false;
		state.legacyPassActive = false;
		for (BufferResource &buffer : state.transientBuffers)
			destroyBuffer(buffer);
		state.transientBuffers.clear();
		for (StreamChunk &chunk : state.streamChunks)
			destroyBuffer(chunk.buffer);
		state.streamChunks.clear();
		for (ImageResource &image : state.retiredImages)
			destroyImage(image);
		state.retiredImages.clear();
		for (std::pair<const unsigned int, VulkanTexture> &entry : state.textures)
			destroyImage(entry.second.image);
		state.textures.clear();
		destroyImage(state.fallbackTexture);
		for (const std::pair<const SamplerKey, VkSampler> &entry : state.samplers)
			vkDestroySampler(state.device, entry.second, nullptr);
		state.samplers.clear();
		for (VkDescriptorPool pool : state.descriptorPools)
			vkDestroyDescriptorPool(state.device, pool, nullptr);
		state.descriptorPools.clear();
		destroyRenderTargets();
		destroySwapchain();
		if (state.presentSampler != VK_NULL_HANDLE)
			vkDestroySampler(state.device, state.presentSampler, nullptr);
		if (state.presentFragmentShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(state.device, state.presentFragmentShader, nullptr);
		if (state.presentVertexShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(state.device, state.presentVertexShader, nullptr);
		if (state.clearFragmentShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(state.device, state.clearFragmentShader, nullptr);
		if (state.clearVertexShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(state.device, state.clearVertexShader, nullptr);
		if (state.legacyFragmentShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(state.device, state.legacyFragmentShader, nullptr);
		if (state.legacyVertexShader != VK_NULL_HANDLE)
			vkDestroyShaderModule(state.device, state.legacyVertexShader, nullptr);
		if (state.presentPipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(state.device, state.presentPipelineLayout, nullptr);
		if (state.presentDescriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(state.device, state.presentDescriptorSetLayout, nullptr);
		if (state.clearPipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(state.device, state.clearPipelineLayout, nullptr);
		if (state.legacyPipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(state.device, state.legacyPipelineLayout, nullptr);
		if (state.legacyDescriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(state.device, state.legacyDescriptorSetLayout, nullptr);
		if (state.frameFence != VK_NULL_HANDLE)
			vkDestroyFence(state.device, state.frameFence, nullptr);
		if (state.renderingFinished != VK_NULL_HANDLE)
			vkDestroySemaphore(state.device, state.renderingFinished, nullptr);
		if (state.imageAvailable != VK_NULL_HANDLE)
			vkDestroySemaphore(state.device, state.imageAvailable, nullptr);
		if (state.commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(state.device, state.commandPool, nullptr);
		vkDestroyDevice(state.device, nullptr);
	}
	if (state.surface != VK_NULL_HANDLE && state.instance != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(state.instance, state.surface, nullptr);
	if (state.debugMessenger != VK_NULL_HANDLE && state.instance != VK_NULL_HANDLE)
	{
		PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger =
			reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(state.instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (destroyDebugMessenger != nullptr)
			destroyDebugMessenger(state.instance, state.debugMessenger, nullptr);
	}
	if (state.instance != VK_NULL_HANDLE)
		vkDestroyInstance(state.instance, nullptr);
}

static void initialize()
{
	if (state.initialized)
		return;
	try
	{
		platform::createWindow(platform::WindowGraphicsAPI::Vulkan);
		createInstance();
		platform::createVulkanSurface(reinterpret_cast<void *>(state.instance), &state.surface);
		selectPhysicalDevice();
		createDevice();
		createCommandResources();
		createRendererResources();
		if (createSwapchain())
			createRenderTargets();
		state.initialized = true;
		std::cout << "legacygl: selected backend vulkan\n";
	}
	catch (...)
	{
		destroyResources();
		state = State();
		platform::destroyWindow();
		throw;
	}
}

static void present()
{
	if (!state.initialized)
		return;
	if (!ensureRenderTargets())
		return;
	uint32_t imageIndex = 0;
	VkResult acquireResult = vkAcquireNextImageKHR(state.device, state.swapchain,
		std::numeric_limits<uint64_t>::max(), state.imageAvailable, VK_NULL_HANDLE, &imageIndex);
	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapchain();
		return;
	}
	if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
		requireSuccess(acquireResult, "vkAcquireNextImageKHR");

	endLegacyPass();
	beginCommandRecording();
	imageBarrier(state.colorTarget.image, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	VkDescriptorSet descriptorSet = allocateDescriptorSet(state.presentDescriptorSetLayout);
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = state.presentSampler;
	imageInfo.imageView = state.colorTarget.view;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSet;
	write.dstBinding = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(state.device, 1, &write, 0, nullptr);

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = state.renderPass;
	renderPassBeginInfo.framebuffer = state.framebuffers[imageIndex];
	renderPassBeginInfo.renderArea.extent = state.swapchainExtent;
	vkCmdBeginRenderPass(state.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(state.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentPipeline());
	setFullscreenViewport(state.swapchainExtent);
	vkCmdBindDescriptorSets(state.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		state.presentPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdDraw(state.commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(state.commandBuffer);
	imageBarrier(state.colorTarget.image, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	requireSuccess(vkEndCommandBuffer(state.commandBuffer), "vkEndCommandBuffer");

	flushStreamBuffers();
	requireSuccess(vkResetFences(state.device, 1, &state.frameFence), "vkResetFences");
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &state.imageAvailable;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &state.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &state.renderingFinished;
	requireSuccess(vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, state.frameFence),
		"vkQueueSubmit");
	state.commandRecording = false;

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &state.renderingFinished;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &state.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	VkResult presentResult = vkQueuePresentKHR(state.presentQueue, &presentInfo);
	requireSuccess(vkQueueWaitIdle(state.presentQueue), "vkQueueWaitIdle");
	requireSuccess(vkWaitForFences(state.device, 1, &state.frameFence, VK_TRUE,
		std::numeric_limits<uint64_t>::max()), "vkWaitForFences");
	cleanupSubmittedResources();
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR ||
		acquireResult == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapchain();
	}
	else
	{
		requireSuccess(presentResult, "vkQueuePresentKHR");
	}
}

static void shutdown()
{
	if (!state.initialized && state.instance == VK_NULL_HANDLE)
		return;
	if (state.initialized)
		submitAndWait();
	destroyResources();
	const unsigned int validationErrors = state.validationErrorCount;
	state = State();
	std::cout << "vulkan: shutdown, validation errors=" << validationErrors << '\n';
}

static unsigned char colorByte(float value)
{
	value = std::max(0.0f, std::min(1.0f, value));
	return static_cast<unsigned char>(std::floor(value * 255.0f + 0.5f));
}

static void copy4(float *destination, const float *source)
{
	std::memcpy(destination, source, sizeof(float) * 4);
}

static int pixelComponents(unsigned int format)
{
	switch (format)
	{
		case GL_RGBA:
		case GL_BGRA_EXT:
			return 4;
		case GL_RGB:
		case GL_BGR_EXT:
			return 3;
		case GL_LUMINANCE_ALPHA:
			return 2;
		case GL_ALPHA:
		case GL_LUMINANCE:
			return 1;
		default:
			return 0;
	}
}

static std::size_t alignedRowSize(std::size_t rowSize, int alignment)
{
	const std::size_t value = static_cast<std::size_t>(alignment);
	return (rowSize + value - 1) / value * value;
}

static void decodePixel(const unsigned char *source, unsigned int format, unsigned char *rgba)
{
	switch (format)
	{
		case GL_RGBA:
			rgba[0] = source[0]; rgba[1] = source[1]; rgba[2] = source[2]; rgba[3] = source[3];
			break;
		case GL_BGRA_EXT:
			rgba[0] = source[2]; rgba[1] = source[1]; rgba[2] = source[0]; rgba[3] = source[3];
			break;
		case GL_RGB:
			rgba[0] = source[0]; rgba[1] = source[1]; rgba[2] = source[2]; rgba[3] = 255;
			break;
		case GL_BGR_EXT:
			rgba[0] = source[2]; rgba[1] = source[1]; rgba[2] = source[0]; rgba[3] = 255;
			break;
		case GL_LUMINANCE_ALPHA:
			rgba[0] = source[0]; rgba[1] = source[0]; rgba[2] = source[0]; rgba[3] = source[1];
			break;
		case GL_ALPHA:
			rgba[0] = 255; rgba[1] = 255; rgba[2] = 255; rgba[3] = source[0];
			break;
		default:
			rgba[0] = source[0]; rgba[1] = source[0]; rgba[2] = source[0]; rgba[3] = 255;
			break;
	}
}

static void encodePixel(const unsigned char *rgba, unsigned int format, unsigned char *destination)
{
	switch (format)
	{
		case GL_RGBA:
			destination[0] = rgba[0]; destination[1] = rgba[1]; destination[2] = rgba[2]; destination[3] = rgba[3];
			break;
		case GL_BGRA_EXT:
			destination[0] = rgba[2]; destination[1] = rgba[1]; destination[2] = rgba[0]; destination[3] = rgba[3];
			break;
		case GL_RGB:
			destination[0] = rgba[0]; destination[1] = rgba[1]; destination[2] = rgba[2];
			break;
		case GL_BGR_EXT:
			destination[0] = rgba[2]; destination[1] = rgba[1]; destination[2] = rgba[0];
			break;
		case GL_LUMINANCE_ALPHA:
			destination[0] = rgba[0]; destination[1] = rgba[3];
			break;
		case GL_ALPHA:
			destination[0] = rgba[3];
			break;
		default:
			destination[0] = rgba[0];
			break;
	}
}

static unsigned int alphaFunction(unsigned int function)
{
	switch (function)
	{
		case GL_NEVER: return 0;
		case GL_LESS: return 1;
		case GL_EQUAL: return 2;
		case GL_LEQUAL: return 3;
		case GL_GREATER: return 4;
		case GL_NOTEQUAL: return 5;
		case GL_GEQUAL: return 6;
		default: return 7;
	}
}

static unsigned int fogMode(unsigned int mode, bool enabled)
{
	if (!enabled)
		return 0;
	if (mode == GL_LINEAR)
		return 1;
	if (mode == GL_EXP)
		return 2;
	return 3;
}

static unsigned int wrapMode(unsigned int mode)
{
	if (mode == GL_REPEAT)
		return 0;
	if (mode == GL_CLAMP)
		return 1;
	return 2;
}

static unsigned int colorMaterialFace(unsigned int face)
{
	if (face == GL_FRONT)
		return 1;
	if (face == GL_BACK)
		return 2;
	return 3;
}

static unsigned int colorMaterialMode(unsigned int mode)
{
	if (mode == GL_AMBIENT)
		return 0;
	if (mode == GL_DIFFUSE)
		return 1;
	if (mode == GL_AMBIENT_AND_DIFFUSE)
		return 2;
	if (mode == GL_SPECULAR)
		return 3;
	return 4;
}

static void copyMaterial(VulkanGPUMaterial &destination, const legacygl::MaterialState &source)
{
	copy4(destination.ambient, source.ambient);
	copy4(destination.diffuse, source.diffuse);
	copy4(destination.specular, source.specular);
	copy4(destination.emission, source.emission);
	destination.shininess[0] = source.shininess;
	destination.shininess[1] = 0.0f;
	destination.shininess[2] = 0.0f;
	destination.shininess[3] = 0.0f;
}

static VulkanGPUVertex makeGPUVertex(const legacygl::Vertex &vertex, const legacygl::Vertex &flat)
{
	VulkanGPUVertex result;
	result.position[0] = vertex.x; result.position[1] = vertex.y; result.position[2] = vertex.z;
	result.color[0] = vertex.r; result.color[1] = vertex.g; result.color[2] = vertex.b; result.color[3] = vertex.a;
	result.normal[0] = vertex.nx; result.normal[1] = vertex.ny; result.normal[2] = vertex.nz;
	result.texCoord[0] = vertex.s; result.texCoord[1] = vertex.t;
	result.flatPosition[0] = flat.x; result.flatPosition[1] = flat.y; result.flatPosition[2] = flat.z;
	result.flatColor[0] = flat.r; result.flatColor[1] = flat.g; result.flatColor[2] = flat.b; result.flatColor[3] = flat.a;
	result.flatNormal[0] = flat.nx; result.flatNormal[1] = flat.ny; result.flatNormal[2] = flat.nz;
	return result;
}

static void fillGPUState(const legacygl::ResolvedDraw &command, VulkanGPUState &gpuState)
{
	std::memcpy(gpuState.modelView, command.modelView.m, sizeof(gpuState.modelView));
	std::memcpy(gpuState.projection, command.projection.m, sizeof(gpuState.projection));
	std::memcpy(gpuState.texture, command.textureMatrix.m, sizeof(gpuState.texture));
	std::memcpy(gpuState.normal, command.normal.m, sizeof(gpuState.normal));
	copy4(gpuState.globalAmbient, command.lighting.modelAmbient);
	copyMaterial(gpuState.frontMaterial, command.lighting.frontMaterial);
	copyMaterial(gpuState.backMaterial, command.lighting.backMaterial);

	unsigned int lightMask = 0;
	for (int i = 0; i < 8; i++)
	{
		const legacygl::LightState &source = command.lighting.lights[i];
		VulkanGPULight &destination = gpuState.lights[i];
		copy4(destination.ambient, source.ambient);
		copy4(destination.diffuse, source.diffuse);
		copy4(destination.specular, source.specular);
		copy4(destination.positionEye, source.positionEye);
		destination.spotDirectionCutoff[0] = source.spotDirectionEye[0];
		destination.spotDirectionCutoff[1] = source.spotDirectionEye[1];
		destination.spotDirectionCutoff[2] = source.spotDirectionEye[2];
		destination.spotDirectionCutoff[3] = source.spotCutoff == 180.0f ? -1.0f :
			std::cos(source.spotCutoff * 0.01745329251994329577f);
		destination.attenuationExponent[0] = source.constantAttenuation;
		destination.attenuationExponent[1] = source.linearAttenuation;
		destination.attenuationExponent[2] = source.quadraticAttenuation;
		destination.attenuationExponent[3] = source.spotExponent;
		if (source.enabled)
			lightMask |= 1u << i;
	}

	copy4(gpuState.fogColor, command.fog.color);
	gpuState.fogParams[0] = command.fog.start;
	gpuState.fogParams[1] = command.fog.end;
	gpuState.fogParams[2] = command.fog.density;
	gpuState.fogParams[3] = command.pipeline.alphaReference;
	gpuState.normalParams[0] = command.normalRescaleFactor;
	gpuState.flags0[0] = command.enables.lighting ? 1u : 0u;
	gpuState.flags0[1] = command.enables.texture2D ? 1u : 0u;
	gpuState.flags0[2] = command.enables.colorMaterial ? 1u : 0u;
	gpuState.flags0[3] = command.pipeline.shadeModel == GL_FLAT ? 1u : 0u;
	gpuState.flags1[0] = command.enables.normalize ? 2u : (command.enables.rescaleNormal ? 1u : 0u);
	gpuState.flags1[1] = lightMask;
	gpuState.flags1[2] = fogMode(command.fog.mode, command.enables.fog);
	gpuState.flags1[3] = command.fog.distanceMode == GL_EYE_RADIAL_NV ? 2u :
		(command.fog.distanceMode == GL_EYE_PLANE ? 1u : 0u);
	gpuState.flags2[0] = command.enables.alphaTest ? 1u : 0u;
	gpuState.flags2[1] = alphaFunction(command.pipeline.alphaFunction);
	gpuState.flags2[2] = wrapMode(command.texture.wrapS);
	gpuState.flags2[3] = wrapMode(command.texture.wrapT);
	gpuState.flags3[0] = colorMaterialFace(command.lighting.colorMaterialFace);
	gpuState.flags3[1] = colorMaterialMode(command.lighting.colorMaterialMode);
	gpuState.flags3[2] = command.texture.complete ? 1u : 0u;
}

static VkPrimitiveTopology primitiveTopology(legacygl::Topology topology)
{
	if (topology == legacygl::Topology::Points)
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	if (topology == legacygl::Topology::Lines)
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

static VkCompareOp compareOp(unsigned int function)
{
	switch (function)
	{
		case GL_NEVER: return VK_COMPARE_OP_NEVER;
		case GL_LESS: return VK_COMPARE_OP_LESS;
		case GL_EQUAL: return VK_COMPARE_OP_EQUAL;
		case GL_LEQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case GL_GREATER: return VK_COMPARE_OP_GREATER;
		case GL_NOTEQUAL: return VK_COMPARE_OP_NOT_EQUAL;
		case GL_GEQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		default: return VK_COMPARE_OP_ALWAYS;
	}
}

static VkBlendFactor blendFactor(unsigned int factor)
{
	switch (factor)
	{
		case GL_ZERO: return VK_BLEND_FACTOR_ZERO;
		case GL_ONE: return VK_BLEND_FACTOR_ONE;
		case GL_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
		case GL_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case GL_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
		case GL_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case GL_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
		case GL_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case GL_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
		case GL_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case GL_SRC_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		default: return VK_BLEND_FACTOR_ONE;
	}
}

static VkCullModeFlags cullMode(unsigned int mode)
{
	if (mode == GL_FRONT)
		return VK_CULL_MODE_FRONT_BIT;
	if (mode == GL_FRONT_AND_BACK)
		return VK_CULL_MODE_FRONT_AND_BACK;
	return VK_CULL_MODE_BACK_BIT;
}

static VkFrontFace frontFace(unsigned int mode)
{
	return mode == GL_CW ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

static VkLogicOp logicOp(unsigned int operation)
{
	switch (operation)
	{
		case GL_CLEAR: return VK_LOGIC_OP_CLEAR;
		case GL_AND: return VK_LOGIC_OP_AND;
		case GL_AND_REVERSE: return VK_LOGIC_OP_AND_REVERSE;
		case GL_COPY: return VK_LOGIC_OP_COPY;
		case GL_AND_INVERTED: return VK_LOGIC_OP_AND_INVERTED;
		case GL_NOOP: return VK_LOGIC_OP_NO_OP;
		case GL_XOR: return VK_LOGIC_OP_XOR;
		case GL_OR: return VK_LOGIC_OP_OR;
		case GL_NOR: return VK_LOGIC_OP_NOR;
		case GL_EQUIV: return VK_LOGIC_OP_EQUIVALENT;
		case GL_INVERT: return VK_LOGIC_OP_INVERT;
		case GL_OR_REVERSE: return VK_LOGIC_OP_OR_REVERSE;
		case GL_COPY_INVERTED: return VK_LOGIC_OP_COPY_INVERTED;
		case GL_OR_INVERTED: return VK_LOGIC_OP_OR_INVERTED;
		case GL_NAND: return VK_LOGIC_OP_NAND;
		default: return VK_LOGIC_OP_SET;
	}
}

static unsigned int colorWriteMask(const bool write[4])
{
	unsigned int mask = 0;
	if (write[0]) mask |= VK_COLOR_COMPONENT_R_BIT;
	if (write[1]) mask |= VK_COLOR_COMPONENT_G_BIT;
	if (write[2]) mask |= VK_COLOR_COMPONENT_B_BIT;
	if (write[3]) mask |= VK_COLOR_COMPONENT_A_BIT;
	return mask;
}

static VkPipeline createLegacyPipeline(const PipelineKey &key)
{
	VkPipelineShaderStageCreateInfo stages[2] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = state.legacyVertexShader;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = state.legacyFragmentShader;
	stages[1].pName = "main";

	VkVertexInputBindingDescription binding = {};
	binding.binding = 0;
	binding.stride = sizeof(VulkanGPUVertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	VkVertexInputAttributeDescription attributes[7] = {};
	attributes[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(VulkanGPUVertex, position)) };
	attributes[1] = { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(VulkanGPUVertex, color)) };
	attributes[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(VulkanGPUVertex, normal)) };
	attributes[3] = { 3, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(VulkanGPUVertex, texCoord)) };
	attributes[4] = { 4, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(VulkanGPUVertex, flatPosition)) };
	attributes[5] = { 5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(VulkanGPUVertex, flatColor)) };
	attributes[6] = { 6, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(VulkanGPUVertex, flatNormal)) };
	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &binding;
	vertexInput.vertexAttributeDescriptionCount = 7;
	vertexInput.pVertexAttributeDescriptions = attributes;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = primitiveTopology(key.topology);

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	VkPipelineRasterizationStateCreateInfo rasterization = {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	VkPipelineRasterizationLineStateCreateInfoEXT lineRasterization = {};
	lineRasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT;
	lineRasterization.lineRasterizationMode = state.lineRasterizationMode;
	if (key.topology == legacygl::Topology::Lines &&
		state.lineRasterizationMode != VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT)
	{
		rasterization.pNext = &lineRasterization;
	}
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = key.cullFace ? cullMode(key.cullFaceMode) : VK_CULL_MODE_NONE;
	rasterization.frontFace = frontFace(key.frontFaceMode);
	rasterization.depthBiasEnable = key.depthBias ? VK_TRUE : VK_FALSE;
	rasterization.lineWidth = 1.0f;
	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = key.depthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = key.depthWrite ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = compareOp(key.depthFunction);
	depthStencil.stencilTestEnable = key.stencilTest ? VK_TRUE : VK_FALSE;
	depthStencil.front.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
	depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
	depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
	depthStencil.back = depthStencil.front;

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable = key.blend && !key.logicOp ? VK_TRUE : VK_FALSE;
	blendAttachment.srcColorBlendFactor = blendFactor(key.blendSource);
	blendAttachment.dstColorBlendFactor = blendFactor(key.blendDestination);
	blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = blendFactor(key.blendSource);
	blendAttachment.dstAlphaBlendFactor = blendFactor(key.blendDestination);
	blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.colorWriteMask = key.colorWriteMask;
	VkPipelineColorBlendStateCreateInfo colorBlend = {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = key.logicOp ? VK_TRUE : VK_FALSE;
	colorBlend.logicOp = logicOp(key.logicOpcode);
	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &blendAttachment;
	const VkDynamicState dynamicValues[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH,
		VK_DYNAMIC_STATE_DEPTH_BIAS
	};
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 4;
	dynamicState.pDynamicStates = dynamicValues;
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = stages;
	createInfo.pVertexInputState = &vertexInput;
	createInfo.pInputAssemblyState = &inputAssembly;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterization;
	createInfo.pMultisampleState = &multisample;
	createInfo.pDepthStencilState = &depthStencil;
	createInfo.pColorBlendState = &colorBlend;
	createInfo.pDynamicState = &dynamicState;
	createInfo.layout = state.legacyPipelineLayout;
	createInfo.renderPass = state.legacyRenderPass;
	VkPipeline pipeline = VK_NULL_HANDLE;
	requireSuccess(vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &createInfo, nullptr,
		&pipeline), "vkCreateGraphicsPipelines(legacy)");
	return pipeline;
}

static VkPipeline legacyPipeline(const legacygl::ResolvedDraw &command)
{
	PipelineKey key;
	key.topology = command.primitives->topology;
	key.depthTest = command.enables.depthTest;
	key.depthWrite = command.pipeline.depthWrite;
	key.depthFunction = command.pipeline.depthFunction;
	key.cullFace = command.enables.cullFace;
	key.cullFaceMode = command.pipeline.cullFaceMode;
	key.frontFaceMode = command.pipeline.frontFaceMode;
	key.logicOp = command.enables.colorLogicOp;
	key.logicOpcode = command.pipeline.logicOpcode;
	key.blend = command.enables.blend;
	key.blendSource = command.pipeline.blendSource;
	key.blendDestination = command.pipeline.blendDestination;
	key.colorWriteMask = colorWriteMask(command.pipeline.colorWrite);
	key.depthBias = command.enables.polygonOffsetFill &&
		command.primitives->topology == legacygl::Topology::Triangles;
	key.stencilTest = command.enables.stencilTest;
	if (key.logicOp && !state.logicOpSupported)
		throw std::runtime_error("Vulkan device cannot emulate enabled GL_COLOR_LOGIC_OP");
	auto found = state.pipelines.find(key);
	if (found != state.pipelines.end())
		return found->second;
	VkPipeline pipeline = createLegacyPipeline(key);
	state.pipelines.emplace(key, pipeline);
	return pipeline;
}

static void imageBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout,
	VkImageLayout newLayout, VkAccessFlags sourceAccess, VkAccessFlags destinationAccess,
	VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = sourceAccess;
	barrier.dstAccessMask = destinationAccess;
	vkCmdPipelineBarrier(state.commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0,
		nullptr, 1, &barrier);
}

static void uploadRGBAImage(ImageResource &image, const unsigned char *pixels, int width, int height)
{
	endLegacyPass();
	beginCommandRecording();
	retireImage(image);
	image = createImage(static_cast<uint32_t>(width), static_cast<uint32_t>(height),
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	const VkDeviceSize byteSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;
	BufferResource staging = createBuffer(byteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
	std::memcpy(staging.mapped, pixels, static_cast<std::size_t>(byteSize));
	flushBuffer(staging);
	VkBufferImageCopy copy = {};
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.layerCount = 1;
	copy.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
	imageBarrier(image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkCmdCopyBufferToImage(state.commandBuffer, staging.buffer, image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
	imageBarrier(image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	state.transientBuffers.push_back(staging);
}

static void ensureFallbackTexture()
{
	if (state.fallbackTexture.image != VK_NULL_HANDLE)
		return;
	const unsigned char black[4] = { 0, 0, 0, 255 };
	uploadRGBAImage(state.fallbackTexture, black, 1, 1);
}

static VkSampler textureSampler(unsigned int minFilterValue, unsigned int magFilterValue,
	unsigned int wrapSValue, unsigned int wrapTValue, bool useGutter)
{
	SamplerKey key;
	key.minFilter = minFilterValue == GL_NEAREST || minFilterValue == GL_NEAREST_MIPMAP_NEAREST ||
		minFilterValue == GL_NEAREST_MIPMAP_LINEAR ? 0u : 1u;
	key.magFilter = magFilterValue == GL_NEAREST ? 0u : 1u;
	key.wrapS = useGutter || wrapSValue != GL_REPEAT ? 1u : 0u;
	key.wrapT = useGutter || wrapTValue != GL_REPEAT ? 1u : 0u;
	auto found = state.samplers.find(key);
	if (found != state.samplers.end())
		return found->second;
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = key.magFilter == 0 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	createInfo.minFilter = key.minFilter == 0 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	createInfo.addressModeU = key.wrapS == 0 ? VK_SAMPLER_ADDRESS_MODE_REPEAT :
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.addressModeV = key.wrapT == 0 ? VK_SAMPLER_ADDRESS_MODE_REPEAT :
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	createInfo.maxLod = 0.0f;
	VkSampler sampler = VK_NULL_HANDLE;
	requireSuccess(vkCreateSampler(state.device, &createInfo, nullptr, &sampler),
		"vkCreateSampler(texture)");
	state.samplers.emplace(key, sampler);
	return sampler;
}

static bool ensureTextureImage(VulkanTexture &texture, const legacygl::ResolvedTextureState &textureState,
	bool useGutter)
{
	unsigned char border[4];
	for (int i = 0; i < 4; i++)
		border[i] = colorByte(textureState.borderColor[i]);
	if (!texture.derivedDirty && texture.image.image != VK_NULL_HANDLE &&
		texture.derivedHasGutter == useGutter && (!useGutter ||
		(texture.derivedWrapS == textureState.wrapS && texture.derivedWrapT == textureState.wrapT &&
		std::memcmp(texture.derivedBorder, border, 4) == 0)))
	{
		return true;
	}

	VulkanTextureLevel &source = texture.levels[0];
	if (!source.defined || source.width <= 0 || source.height <= 0)
		return false;
	if (!useGutter)
	{
		uploadRGBAImage(texture.image, source.rgba.data(), source.width, source.height);
		texture.derivedHasGutter = false;
		texture.derivedDirty = false;
		return true;
	}

	const int derivedWidth = source.width + 2;
	const int derivedHeight = source.height + 2;
	std::vector<unsigned char> derived(static_cast<std::size_t>(derivedWidth) *
		static_cast<std::size_t>(derivedHeight) * 4);
	for (int y = 0; y < derivedHeight; y++)
	{
		const bool outsideT = y == 0 || y == derivedHeight - 1;
		int sourceY = y - 1;
		if (y == 0)
			sourceY = textureState.wrapT == GL_REPEAT ? source.height - 1 : 0;
		else if (y == derivedHeight - 1)
			sourceY = textureState.wrapT == GL_REPEAT ? 0 : source.height - 1;
		for (int x = 0; x < derivedWidth; x++)
		{
			const bool outsideS = x == 0 || x == derivedWidth - 1;
			int sourceX = x - 1;
			if (x == 0)
				sourceX = textureState.wrapS == GL_REPEAT ? source.width - 1 : 0;
			else if (x == derivedWidth - 1)
				sourceX = textureState.wrapS == GL_REPEAT ? 0 : source.width - 1;
			unsigned char *destination = derived.data() +
				(static_cast<std::size_t>(y) * static_cast<std::size_t>(derivedWidth) +
				static_cast<std::size_t>(x)) * 4;
			if ((outsideS && textureState.wrapS == GL_CLAMP) ||
				(outsideT && textureState.wrapT == GL_CLAMP))
			{
				std::memcpy(destination, border, 4);
			}
			else
			{
				std::memcpy(destination, source.rgba.data() +
					(static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(source.width) +
					static_cast<std::size_t>(sourceX)) * 4, 4);
			}
		}
	}
	uploadRGBAImage(texture.image, derived.data(), derivedWidth, derivedHeight);
	texture.derivedWrapS = textureState.wrapS;
	texture.derivedWrapT = textureState.wrapT;
	std::memcpy(texture.derivedBorder, border, 4);
	texture.derivedHasGutter = true;
	texture.derivedDirty = false;
	return true;
}

struct TextureBinding
{
	VkImageView view = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
};

static TextureBinding bindTextureState(const legacygl::ResolvedDraw &command, VulkanGPUState &gpuState)
{
	ensureFallbackTexture();
	TextureBinding result;
	result.view = state.fallbackTexture.view;
	result.sampler = state.presentSampler;
	if (!command.enables.texture2D || !command.texture.complete)
		return result;
	VulkanTexture &texture = state.textures[command.texture.name];
	VulkanTextureLevel &level = texture.levels[0];
	if (!level.defined)
	{
		gpuState.flags3[2] = 0;
		return result;
	}
	const bool useGutter = command.texture.wrapS == GL_CLAMP || command.texture.wrapT == GL_CLAMP;
	if (!ensureTextureImage(texture, command.texture, useGutter))
	{
		gpuState.flags3[2] = 0;
		return result;
	}
	result.view = texture.image.view;
	result.sampler = textureSampler(command.texture.minFilter, command.texture.magFilter,
		command.texture.wrapS, command.texture.wrapT, useGutter);
	gpuState.textureSize[0] = static_cast<float>(level.width);
	gpuState.textureSize[1] = static_cast<float>(level.height);
	gpuState.textureSize[2] = static_cast<float>(level.width + 2);
	gpuState.textureSize[3] = static_cast<float>(level.height + 2);
	gpuState.flags3[3] = useGutter ? 1u : 0u;
	return result;
}

static VkPipeline createFullscreenPipeline(VkShaderModule vertexShader, VkShaderModule fragmentShader,
	VkPipelineLayout layout, VkRenderPass renderPass, VkColorComponentFlags writeMask)
{
	VkPipelineShaderStageCreateInfo stages[2] = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vertexShader;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fragmentShader;
	stages[1].pName = "main";
	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	VkPipelineRasterizationStateCreateInfo rasterization = {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_NONE;
	rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState attachment = {};
	attachment.colorWriteMask = writeMask;
	VkPipelineColorBlendStateCreateInfo colorBlend = {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &attachment;
	const VkDynamicState dynamicValues[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic = {};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = 2;
	dynamic.pDynamicStates = dynamicValues;
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = stages;
	createInfo.pVertexInputState = &vertexInput;
	createInfo.pInputAssemblyState = &inputAssembly;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterization;
	createInfo.pMultisampleState = &multisample;
	createInfo.pDepthStencilState = &depthStencil;
	createInfo.pColorBlendState = &colorBlend;
	createInfo.pDynamicState = &dynamic;
	createInfo.layout = layout;
	createInfo.renderPass = renderPass;
	VkPipeline pipeline = VK_NULL_HANDLE;
	requireSuccess(vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &createInfo, nullptr,
		&pipeline), "vkCreateGraphicsPipelines(fullscreen)");
	return pipeline;
}

static VkPipeline clearPipeline(unsigned int writeMask)
{
	auto found = state.clearPipelines.find(writeMask);
	if (found != state.clearPipelines.end())
		return found->second;
	VkPipeline pipeline = createFullscreenPipeline(state.clearVertexShader, state.clearFragmentShader,
		state.clearPipelineLayout, state.legacyRenderPass, writeMask);
	state.clearPipelines.emplace(writeMask, pipeline);
	return pipeline;
}

static VkPipeline presentPipeline()
{
	if (state.presentPipeline == VK_NULL_HANDLE)
	{
		state.presentPipeline = createFullscreenPipeline(state.presentVertexShader,
			state.presentFragmentShader, state.presentPipelineLayout, state.renderPass,
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
	}
	return state.presentPipeline;
}

static void setFullscreenViewport(VkExtent2D extent)
{
	VkViewport viewport = {};
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.maxDepth = 1.0f;
	VkRect2D scissor = {};
	scissor.extent = extent;
	vkCmdSetViewport(state.commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(state.commandBuffer, 0, 1, &scissor);
}

class VulkanSink final : public legacygl::Sink
{
public:
	void matrixMode(unsigned int) override {}
	void loadIdentity() override {}
	void pushMatrix() override {}
	void popMatrix() override {}
	void translatef(float, float, float) override {}
	void rotatef(float, float, float, float) override {}
	void scalef(float, float, float) override {}
	void scaled(double, double, double) override {}
	void ortho(double, double, double, double, double, double) override {}
	void frustum(double, double, double, double, double, double) override {}
	void enable(unsigned int) override {}
	void disable(unsigned int) override {}
	void blendFunc(unsigned int, unsigned int) override {}
	void alphaFunc(unsigned int, float) override {}
	void depthFunc(unsigned int) override {}
	void depthMask(unsigned char) override {}
	void colorMask(unsigned char, unsigned char, unsigned char, unsigned char) override {}
	void cullFace(unsigned int) override {}
	void shadeModel(unsigned int) override {}
	void logicOp(unsigned int) override {}
	void lineWidth(float) override {}
	void polygonOffset(float, float) override {}
	void viewport(int, int, int, int) override {}
	void pixelStorei(unsigned int, int) override {}
	void color4f(float, float, float, float) override {}
	void color3f(float, float, float) override {}
	void normal3f(float, float, float) override {}
	void normal3b(signed char, signed char, signed char) override {}
	void fogf(unsigned int, float) override {}
	void fogfv(unsigned int, const float *) override {}
	void fogi(unsigned int, int) override {}
	void lightfv(unsigned int, unsigned int, const float *) override {}
	void lightModelfv(unsigned int, const float *) override {}
	void colorMaterial(unsigned int, unsigned int) override {}

	void genTextures(int n, unsigned int *textures) override
	{
		if (n <= 0 || textures == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			textures[i] = textureNames.allocate();
			if (textures[i] == 0)
				throw std::runtime_error("Vulkan logical texture-name namespace exhausted");
		}
	}

	void deleteTextures(int n, const unsigned int *textures) override
	{
		if (n <= 0 || textures == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			const unsigned int name = textures[i];
			textureNames.release(name);
			auto found = state.textures.find(name);
			if (found != state.textures.end())
			{
				retireImage(found->second.image);
				state.textures.erase(found);
			}
		}
	}
	void bindTexture(unsigned int, unsigned int name) override { textureNames.reserve(name); }
	void texParameteri(unsigned int, unsigned int, int) override {}
	void texImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int,
		const void *) override {}
	void texSubImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int,
		const void *) override {}
	void enableClientState(unsigned int) override {}
	void disableClientState(unsigned int) override {}
	void vertexPointer(int, unsigned int, int, const void *) override {}
	void texCoordPointer(int, unsigned int, int, const void *) override {}
	void colorPointer(int, unsigned int, int, const void *) override {}
	void normalPointer(unsigned int, int, const void *) override {}
	void drawArrays(unsigned int, int, int) override {}
	void begin(unsigned int) override {}
	void end() override {}
	void vertex3f(float, float, float) override {}
	void texCoord2f(float, float) override {}

	void genBuffersARB(int n, unsigned int *buffers) override
	{
		if (n <= 0 || buffers == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			buffers[i] = bufferNames.allocate();
			if (buffers[i] == 0)
				throw std::runtime_error("Vulkan logical buffer-name namespace exhausted");
		}
	}

	void bindBufferARB(unsigned int, unsigned int name) override { bufferNames.reserve(name); }
	void bufferDataARB(unsigned int, std::ptrdiff_t, const void *, unsigned int) override {}

	unsigned int genLists(int range) override
	{
		if (range <= 0)
			return 0;
		const std::uint64_t count = static_cast<std::uint64_t>(range);
		const std::uint64_t maximumName = std::numeric_limits<unsigned int>::max();
		if (nextListName > maximumName || count > maximumName - nextListName + 1)
		{
			return 0;
		}
		const unsigned int base = static_cast<unsigned int>(nextListName);
		nextListName += count;
		return base;
	}

	void newList(unsigned int list, unsigned int) override
	{
		if (list != 0)
			nextListName = std::max(nextListName, static_cast<std::uint64_t>(list) + 1);
	}
	void endList() override {}
	void callList(unsigned int) override {}
	void callLists(int, unsigned int, const void *) override {}
	void deleteLists(unsigned int, int) override {}
	void clear(unsigned int) override {}
	void clearColor(float, float, float, float) override {}
	void clearDepth(double) override {}
	void readPixels(int, int, int, int, unsigned int, unsigned int, void *) override {}
	void finish() override
	{
		if (state.device != VK_NULL_HANDLE)
			submitAndWait();
	}

	bool wantsCanonicalGeometry() const override
	{
		return true;
	}

	void resolvedClear(const legacygl::ResolvedClear &command) override
	{
		if (!ensureRenderTargets())
			return;
		beginLegacyPass();
		VkClearAttachment attachments[2] = {};
		uint32_t attachmentCount = 0;
		const unsigned int writeMask = colorWriteMask(command.colorWrite);
		if ((command.mask & GL_COLOR_BUFFER_BIT) != 0 &&
			writeMask == (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT))
		{
			attachments[attachmentCount].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachments[attachmentCount].colorAttachment = 0;
			for (int i = 0; i < 4; i++)
				attachments[attachmentCount].clearValue.color.float32[i] = command.color[i];
			attachmentCount++;
		}
		if ((command.mask & GL_DEPTH_BUFFER_BIT) != 0 && command.depthWrite)
		{
			attachments[attachmentCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			attachments[attachmentCount].clearValue.depthStencil.depth =
				static_cast<float>(std::max(0.0, std::min(1.0, command.depth)));
			attachmentCount++;
		}
		if (attachmentCount != 0)
		{
			VkClearRect rectangle = {};
			rectangle.rect.extent = state.targetExtent;
			rectangle.layerCount = 1;
			vkCmdClearAttachments(state.commandBuffer, attachmentCount, attachments, 1, &rectangle);
		}
		if ((command.mask & GL_COLOR_BUFFER_BIT) != 0 && writeMask != 0 &&
			writeMask != (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT))
		{
			struct ClearPushConstants
			{
				float color[4];
				float depth;
				float padding[3];
			} push = {};
			for (int i = 0; i < 4; i++)
				push.color[i] = command.color[i];
			push.depth = static_cast<float>(command.depth);
			vkCmdBindPipeline(state.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				clearPipeline(writeMask));
			setFullscreenViewport(state.targetExtent);
			vkCmdPushConstants(state.commandBuffer, state.clearPipelineLayout,
				VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
			vkCmdDraw(state.commandBuffer, 3, 1, 0, 0);
		}
	}

	void resolvedDraw(const legacygl::ResolvedDraw &command) override
	{
		if (!ensureRenderTargets() || command.geometry == nullptr || command.primitives == nullptr ||
			command.geometry->vertices.empty())
		{
			return;
		}
		if (command.enables.lineSmooth)
			throw std::runtime_error("Vulkan backend does not emulate exercised GL_LINE_SMOOTH");

		std::vector<VulkanGPUVertex> vertices;
		const int verticesPerPrimitive = command.primitives->topology == legacygl::Topology::Points ? 1 :
			(command.primitives->topology == legacygl::Topology::Lines ? 2 : 3);
		vertices.reserve(command.primitives->primitives.size() *
			static_cast<std::size_t>(verticesPerPrimitive));
		for (const legacygl::CanonicalPrimitive &primitive : command.primitives->primitives)
		{
			const legacygl::Vertex &flat = command.geometry->vertices[
				static_cast<std::size_t>(primitive.provoking)];
			for (int i = 0; i < verticesPerPrimitive; i++)
			{
				const legacygl::Vertex &vertex = command.geometry->vertices[
					static_cast<std::size_t>(primitive.indices[i])];
				vertices.push_back(makeGPUVertex(vertex, flat));
			}
		}
		if (vertices.empty())
			return;

		VulkanGPUState gpuState = {};
		fillGPUState(command, gpuState);
		const TextureBinding texture = bindTextureState(command, gpuState);
		beginLegacyPass();
		BufferResource vertexBuffer = createBuffer(vertices.size() * sizeof(VulkanGPUVertex),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
		std::memcpy(vertexBuffer.mapped, vertices.data(), vertices.size() * sizeof(VulkanGPUVertex));
		flushBuffer(vertexBuffer);
		BufferResource uniformBuffer = createBuffer(sizeof(VulkanGPUState),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
		std::memcpy(uniformBuffer.mapped, &gpuState, sizeof(gpuState));
		flushBuffer(uniformBuffer);

		VkDescriptorSet descriptorSet = allocateDescriptorSet(state.legacyDescriptorSetLayout);
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffer.buffer;
		bufferInfo.range = sizeof(VulkanGPUState);
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.sampler = texture.sampler;
		imageInfo.imageView = texture.view;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet writes[2] = {};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = descriptorSet;
		writes[0].dstBinding = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].pBufferInfo = &bufferInfo;
		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSet;
		writes[1].dstBinding = 1;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[1].pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(state.device, 2, writes, 0, nullptr);

		vkCmdBindPipeline(state.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			legacyPipeline(command));
		VkViewport viewport = {};
		viewport.x = static_cast<float>(command.pipeline.viewport[0]);
		viewport.y = static_cast<float>(static_cast<int>(state.targetExtent.height) -
			command.pipeline.viewport[1]);
		if (command.primitives->topology == legacygl::Topology::Lines)
			// Select the GL diamond-exit side of exact pixel-boundary ties after the negative viewport flip.
			viewport.y += state.lineRasterizationBias;
		viewport.width = static_cast<float>(command.pipeline.viewport[2]);
		viewport.height = -static_cast<float>(command.pipeline.viewport[3]);
		viewport.maxDepth = 1.0f;
		VkRect2D scissor = {};
		scissor.extent = state.targetExtent;
		vkCmdSetViewport(state.commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(state.commandBuffer, 0, 1, &scissor);
		float lineWidth = command.pipeline.lineWidth;
		if (!state.wideLinesSupported || lineWidth < state.physicalProperties.limits.lineWidthRange[0] ||
			lineWidth > state.physicalProperties.limits.lineWidthRange[1])
		{
			lineWidth = 1.0f;
			if (!lineWidthFallbackReported)
			{
				std::fprintf(stderr, "LegacyGL vulkan: line width %.3g is unavailable; using classified fallback width 1\n",
					command.pipeline.lineWidth);
				lineWidthFallbackReported = true;
			}
		}
		vkCmdSetLineWidth(state.commandBuffer, lineWidth);
		vkCmdSetDepthBias(state.commandBuffer, command.pipeline.polygonOffsetUnits, 0.0f,
			command.pipeline.polygonOffsetFactor);
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(state.commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
		vkCmdBindDescriptorSets(state.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			state.legacyPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdDraw(state.commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		state.transientBuffers.push_back(vertexBuffer);
		state.transientBuffers.push_back(uniformBuffer);
	}

	void resolvedTextureUpload(const legacygl::ResolvedTextureUpload &command) override
	{
		if (command.level < 0 || command.level >= VULKAN_TEXTURE_LEVELS)
			return;
		VulkanTexture &texture = state.textures[command.texture];
		VulkanTextureLevel &level = texture.levels[command.level];
		if (!command.subImage)
		{
			level.width = command.width;
			level.height = command.height;
			level.defined = true;
			level.rgba.assign(static_cast<std::size_t>(command.width) *
				static_cast<std::size_t>(command.height) * 4, 0);
		}
		if (command.pixels != nullptr && command.width > 0 && command.height > 0)
		{
			const int components = pixelComponents(command.sourceFormat);
			const std::size_t sourceRow = alignedRowSize(static_cast<std::size_t>(command.width) *
				static_cast<std::size_t>(components), command.unpackAlignment);
			const unsigned char *source = static_cast<const unsigned char *>(command.pixels);
			for (int y = 0; y < command.height; y++)
			{
				for (int x = 0; x < command.width; x++)
				{
					unsigned char rgba[4];
					decodePixel(source + static_cast<std::size_t>(y) * sourceRow +
						static_cast<std::size_t>(x) * static_cast<std::size_t>(components),
						command.sourceFormat, rgba);
					const std::size_t destination = (static_cast<std::size_t>(command.y + y) *
						static_cast<std::size_t>(level.width) + static_cast<std::size_t>(command.x + x)) * 4;
					std::memcpy(level.rgba.data() + destination, rgba, 4);
				}
			}
		}
		texture.derivedDirty = true;
	}

	void resolvedReadback(const legacygl::ResolvedReadback &command) override
	{
		if (command.pixels == nullptr || command.width <= 0 || command.height <= 0 ||
			!ensureRenderTargets())
			return;
		endLegacyPass();
		beginCommandRecording();
		const VkDeviceSize byteSize = static_cast<VkDeviceSize>(command.width) *
			static_cast<VkDeviceSize>(command.height) * 4;
		BufferResource readback = createBuffer(byteSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
		imageBarrier(state.colorTarget.image, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		VkBufferImageCopy copy = {};
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.layerCount = 1;
		copy.imageOffset.x = command.x;
		copy.imageOffset.y = static_cast<int>(state.targetExtent.height) - (command.y + command.height);
		copy.imageExtent = { static_cast<uint32_t>(command.width),
			static_cast<uint32_t>(command.height), 1 };
		vkCmdCopyImageToBuffer(state.commandBuffer, state.colorTarget.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &copy);
		imageBarrier(state.colorTarget.image, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		submitAndWait();
		invalidateBuffer(readback);

		const int components = pixelComponents(command.format);
		const std::size_t stride = alignedRowSize(static_cast<std::size_t>(command.width) *
			static_cast<std::size_t>(components), command.packAlignment);
		unsigned char *destination = static_cast<unsigned char *>(command.pixels);
		for (int y = 0; y < command.height; y++)
		{
			for (int x = 0; x < command.width; x++)
			{
				const unsigned char *source = static_cast<const unsigned char *>(readback.mapped) +
					(static_cast<std::size_t>(command.height - 1 - y) *
					static_cast<std::size_t>(command.width) + static_cast<std::size_t>(x)) * 4;
				encodePixel(source, command.format, destination + static_cast<std::size_t>(y) * stride +
					static_cast<std::size_t>(x) * static_cast<std::size_t>(components));
			}
		}
		destroyBuffer(readback);
	}

private:
	LogicalNameAllocator textureNames;
	LogicalNameAllocator bufferNames;
	std::uint64_t nextListName = 1;
	bool lineWidthFallbackReported = false;
};

static VulkanSink sinkInstance;

}

namespace renderbackend
{

const Configuration &configuration()
{
	static const Configuration value = {
		"vulkan",
		0,
		0,
		OpenGLProfile::None,
		0,
		0,
		OpenGLProfile::None,
		true,
		false
	};
	return value;
}

void initialize()
{
	vulkanbackend::initialize();
}

void present()
{
	vulkanbackend::present();
}

void shutdown()
{
	vulkanbackend::shutdown();
}

bool hasCapability(const char *capability)
{
	return capability != nullptr && std::strcmp(capability, "GL_NV_fog_distance") == 0;
}

legacygl::Sink *sink()
{
	return &vulkanbackend::sinkInstance;
}

}
