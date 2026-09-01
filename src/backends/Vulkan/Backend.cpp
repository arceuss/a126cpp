#include "backends/Backend.h"
#include "backends/Vulkan/PipelineCacheFile.h"
#include "backends/Vulkan/Shaders.h"

#include <algorithm>
#include <array>
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
#include <chrono>
#endif
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "backends/Platform/Platform.h"
#include "legacygl/LegacyGL.h"
#include "legacygl/PixelFormat.h"
#include "legacygl/PhaseProfile.h"
#include "legacygl/Sink.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vulkanbackend
{

#define A126_VULKAN_GLOBAL_FUNCTIONS(X) \
	X(vkCreateInstance) \
	X(vkEnumerateInstanceExtensionProperties) \
	X(vkEnumerateInstanceLayerProperties)

#define A126_VULKAN_INSTANCE_FUNCTIONS(X) \
	X(vkDestroyInstance) \
	X(vkCreateDevice) \
	X(vkDestroySurfaceKHR) \
	X(vkEnumerateDeviceExtensionProperties) \
	X(vkEnumeratePhysicalDevices) \
	X(vkGetDeviceProcAddr) \
	X(vkGetPhysicalDeviceFeatures) \
	X(vkGetPhysicalDeviceFeatures2) \
	X(vkGetPhysicalDeviceFormatProperties) \
	X(vkGetPhysicalDeviceMemoryProperties) \
	X(vkGetPhysicalDeviceProperties) \
	X(vkGetPhysicalDeviceProperties2) \
	X(vkGetPhysicalDeviceQueueFamilyProperties) \
	X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
	X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
	X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
	X(vkGetPhysicalDeviceSurfaceSupportKHR)

#define A126_VULKAN_DEVICE_FUNCTIONS(X) \
	X(vkDestroyDevice) \
	X(vkAcquireNextImageKHR) \
	X(vkAllocateCommandBuffers) \
	X(vkAllocateDescriptorSets) \
	X(vkAllocateMemory) \
	X(vkBeginCommandBuffer) \
	X(vkBindBufferMemory) \
	X(vkBindImageMemory) \
	X(vkCmdBeginRenderPass) \
	X(vkCmdBindDescriptorSets) \
	X(vkCmdBindPipeline) \
	X(vkCmdBindVertexBuffers) \
	X(vkCmdClearAttachments) \
	X(vkCmdCopyBufferToImage) \
	X(vkCmdCopyImageToBuffer) \
	X(vkCmdDraw) \
	X(vkCmdEndRenderPass) \
	X(vkCmdPipelineBarrier) \
	X(vkCmdPushConstants) \
	X(vkCmdSetDepthBias) \
	X(vkCmdSetLineWidth) \
	X(vkCmdSetScissor) \
	X(vkCmdSetViewport) \
	X(vkCreateBuffer) \
	X(vkCreateCommandPool) \
	X(vkCreateDescriptorPool) \
	X(vkCreateDescriptorSetLayout) \
	X(vkCreateFence) \
	X(vkCreateFramebuffer) \
	X(vkCreateGraphicsPipelines) \
	X(vkCreateImage) \
	X(vkCreateImageView) \
	X(vkCreatePipelineCache) \
	X(vkCreatePipelineLayout) \
	X(vkCreateRenderPass) \
	X(vkCreateSampler) \
	X(vkCreateSemaphore) \
	X(vkCreateShaderModule) \
	X(vkCreateSwapchainKHR) \
	X(vkDestroyBuffer) \
	X(vkDestroyCommandPool) \
	X(vkDestroyDescriptorPool) \
	X(vkDestroyDescriptorSetLayout) \
	X(vkDestroyFence) \
	X(vkDestroyFramebuffer) \
	X(vkDestroyImage) \
	X(vkDestroyImageView) \
	X(vkDestroyPipeline) \
	X(vkDestroyPipelineCache) \
	X(vkDestroyPipelineLayout) \
	X(vkDestroyRenderPass) \
	X(vkDestroySampler) \
	X(vkDestroySemaphore) \
	X(vkDestroyShaderModule) \
	X(vkDestroySwapchainKHR) \
	X(vkDeviceWaitIdle) \
	X(vkEndCommandBuffer) \
	X(vkFlushMappedMemoryRanges) \
	X(vkFreeMemory) \
	X(vkGetBufferMemoryRequirements) \
	X(vkGetDeviceQueue) \
	X(vkGetImageMemoryRequirements) \
	X(vkGetPipelineCacheData) \
	X(vkGetSwapchainImagesKHR) \
	X(vkInvalidateMappedMemoryRanges) \
	X(vkMapMemory) \
	X(vkQueuePresentKHR) \
	X(vkQueueSubmit) \
	X(vkResetCommandBuffer) \
	X(vkResetDescriptorPool) \
	X(vkResetFences) \
	X(vkUnmapMemory) \
	X(vkUpdateDescriptorSets) \
	X(vkWaitForFences)

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;

#define A126_DEFINE_VULKAN_FUNCTION(name) static PFN_##name name = nullptr;
A126_VULKAN_GLOBAL_FUNCTIONS(A126_DEFINE_VULKAN_FUNCTION)
A126_VULKAN_INSTANCE_FUNCTIONS(A126_DEFINE_VULKAN_FUNCTION)
A126_VULKAN_DEVICE_FUNCTIONS(A126_DEFINE_VULKAN_FUNCTION)
#undef A126_DEFINE_VULKAN_FUNCTION

static const int VULKAN_TEXTURE_LEVELS = 16;
static const int VULKAN_FRAMES_IN_FLIGHT = 2;
static const unsigned int VULKAN_PIPELINE_KEY_ABI_VERSION = 1;
static const VkDeviceSize VULKAN_STREAM_CHUNK_SIZE = 4 * 1024 * 1024;
static const VkDeviceSize VULKAN_RESIDENT_PAGE_SIZE = 16 * 1024 * 1024;
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
	float currentColor[4];
	float currentNormal[4];
	float currentTexCoord[4];
	unsigned int flags4[4];
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
static_assert(offsetof(VulkanGPUState, currentColor) == 1328, "Vulkan shader current color ABI changed");
static_assert(offsetof(VulkanGPUState, currentNormal) == 1344, "Vulkan shader current normal ABI changed");
static_assert(offsetof(VulkanGPUState, currentTexCoord) == 1360, "Vulkan shader current texture coordinate ABI changed");
static_assert(offsetof(VulkanGPUState, flags4) == 1376, "Vulkan shader flags4 ABI changed");
static_assert(sizeof(VulkanGPUState) == 1392, "Vulkan shader block ABI changed");

// The only constants that change on every draw. Kept bit-identical to the
// matrices the semantic core resolved; nothing here is derived or compressed.
struct VulkanDrawPush
{
	float modelView[16];
	float normal[16];
};

static_assert(sizeof(VulkanDrawPush) == 128, "Vulkan push block must fit the guaranteed 128 bytes");
static_assert(offsetof(VulkanDrawPush, modelView) == 0, "Vulkan push model-view offset changed");
static_assert(offsetof(VulkanDrawPush, normal) == 64, "Vulkan push normal offset changed");

struct BufferResource
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize size = 0;
	VkDeviceSize allocationSize = 0;
	void *mapped = nullptr;
	uint32_t memoryTypeIndex = std::numeric_limits<uint32_t>::max();
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

struct ResidentFreeRange
{
	VkDeviceSize offset = 0;
	VkDeviceSize size = 0;
};

struct ResidentPage
{
	BufferResource buffer;
	std::vector<ResidentFreeRange> freeRanges;
};

struct ResidentAllocation
{
	ResidentPage *page = nullptr;
	VkDeviceSize offset = 0;
	VkDeviceSize size = 0;
};

// Display-list geometry is immutable, so its identity alone keys the resident
// copy. Attributes the draw omitted are supplied at execution time from the
// uniform block, which is what keeps a chunk from being duplicated once per
// distinct current colour, normal or texture coordinate.
struct ResidentGeometryEntry
{
	std::shared_ptr<ResidentAllocation> allocation;
	legacygl::Topology topology = legacygl::Topology::Triangles;
	uint32_t vertexCount = 0;
	bool hasColor = false;
	bool hasNormal = false;
	bool hasTexCoord = false;
};

struct LegacyDescriptorEntry
{
	VkBuffer uniformBuffer = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct ImageResource
{
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	uint32_t width = 0;
	uint32_t height = 0;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkAccessFlags access = 0;
	VkPipelineStageFlags stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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

	bool operator==(const PipelineKey &other) const
	{
		return std::tie(topology, depthTest, depthWrite, depthFunction, cullFace,
			cullFaceMode, frontFaceMode, blend, blendSource, blendDestination,
			logicOp, logicOpcode, colorWriteMask, depthBias, stencilTest) ==
			std::tie(other.topology, other.depthTest, other.depthWrite, other.depthFunction,
			other.cullFace, other.cullFaceMode, other.frontFaceMode, other.blend,
			other.blendSource, other.blendDestination, other.logicOp, other.logicOpcode,
			other.colorWriteMask, other.depthBias, other.stencilTest);
	}

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

enum class BoundPipelineDomain
{
	Legacy,
	MaskedClear,
	Present
};

struct CommandState
{
	VkPipeline pipeline = VK_NULL_HANDLE;
	bool pipelineValid = false;
	PipelineKey legacyPipelineKey;
	unsigned int maskedClearWriteMask = 0;
	BoundPipelineDomain pipelineDomain = BoundPipelineDomain::Legacy;
	bool logicalPipelineValid = false;
	float viewport[6] = {};
	bool viewportValid = false;
	int scissorOffset[2] = {};
	uint32_t scissorExtent[2] = {};
	bool scissorValid = false;
	float lineWidth = 0.0f;
	bool lineWidthValid = false;
	float depthBias[3] = {};
	bool depthBiasValid = false;
	// Environment constants and the descriptor binding that selects them. Both
	// are command-buffer scoped: a reset command buffer must rebind even when
	// the semantic state is unchanged from the previous frame.
	VulkanGPUState environment = {};
	VkBuffer environmentBuffer = VK_NULL_HANDLE;
	uint32_t environmentOffset = 0;
	bool environmentValid = false;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	uint32_t descriptorOffset = 0;
	bool descriptorValid = false;
};

#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
enum class ImageBarrierReason : std::size_t
{
	TargetInitialization,
	PresentToSample,
	PresentToRender,
	TextureNewToTransfer,
	TextureReuseToTransfer,
	TextureUploadToSample,
	ReadbackToTransfer,
	ReadbackToRender,
	Count
};

enum class LegacyPassBreakReason : std::size_t
{
	Submit,
	Present,
	TextureUpload,
	Readback,
	Finish,
	Shutdown,
	SwapchainRecreate,
	Count
};

enum class PipelineMetricDomain
{
	Legacy,
	MaskedClear,
	Present
};

enum class DescriptorMetricDomain
{
	Legacy,
	Present
};

struct PipelineMetrics
{
	std::uint64_t lookups = 0;
	std::uint64_t currentHits = 0;
	std::uint64_t cacheHits = 0;
	std::uint64_t creates = 0;
	std::uint64_t createNanoseconds = 0;
	std::uint64_t binds = 0;
	std::uint64_t redundantBindCandidates = 0;
};

struct DescriptorMetrics
{
	std::uint64_t lookups = 0;
	std::uint64_t hits = 0;
	std::uint64_t allocations = 0;
	std::uint64_t invalidations = 0;
};

struct CommandObservation
{
	VkPipeline pipeline = VK_NULL_HANDLE;
	bool pipelineValid = false;
	PipelineKey legacyPipelineKey;
	unsigned int maskedClearWriteMask = 0;
	PipelineMetricDomain pipelineDomain = PipelineMetricDomain::Legacy;
	bool logicalPipelineValid = false;
	float viewport[6] = {};
	bool viewportValid = false;
	int scissorOffset[2] = {};
	uint32_t scissorExtent[2] = {};
	bool scissorValid = false;
	float lineWidth = 0.0f;
	bool lineWidthValid = false;
	float depthBias[3] = {};
	bool depthBiasValid = false;
};

struct VulkanDiagnostics
{
	PipelineMetrics legacyPipelines;
	PipelineMetrics maskedClearPipelines;
	PipelineMetrics presentPipelines;
	DescriptorMetrics legacyDescriptors;
	DescriptorMetrics presentDescriptors;
	std::uint64_t viewportEmits = 0;
	std::uint64_t viewportRedundantCandidates = 0;
	std::uint64_t scissorEmits = 0;
	std::uint64_t scissorRedundantCandidates = 0;
	std::uint64_t lineWidthEmits = 0;
	std::uint64_t lineWidthRedundantCandidates = 0;
	std::uint64_t depthBiasEmits = 0;
	std::uint64_t depthBiasRedundantCandidates = 0;
	std::uint64_t imageBarriersEmitted = 0;
	std::uint64_t imageBarriersSkipped = 0;
	std::array<std::uint64_t, static_cast<std::size_t>(ImageBarrierReason::Count)>
		imageBarrierReasons = {};
	std::uint64_t legacyPassBegins = 0;
	std::uint64_t legacyPassEnds = 0;
	std::uint64_t presentPassBegins = 0;
	std::uint64_t presentPassEnds = 0;
	std::array<std::uint64_t, static_cast<std::size_t>(LegacyPassBreakReason::Count)>
		legacyPassBreakReasons = {};
	std::uint64_t imagesRetired = 0;
	std::uint64_t imagesReclaimed = 0;
	std::uint64_t residentAllocationsRetired = 0;
	std::uint64_t residentAllocationsReclaimed = 0;
	std::uint64_t transientBuffersRetired = 0;
	std::uint64_t transientBuffersReclaimed = 0;
	std::uint64_t fenceWaits = 0;
	std::uint64_t finishDrains = 0;
};
#endif

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

struct FrameResources
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkSemaphore imageAvailable = VK_NULL_HANDLE;
	VkFence fence = VK_NULL_HANDLE;
	std::vector<VkDescriptorPool> descriptorPools;
	std::size_t activeDescriptorPool = 0;
	std::vector<LegacyDescriptorEntry> legacyDescriptorCache;
	std::vector<StreamChunk> streamChunks;
	std::vector<std::shared_ptr<ResidentAllocation>> residentAllocations;
	std::vector<BufferResource> transientBuffers;
	std::vector<ImageResource> retiredImages;
	CommandState commandState;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	CommandObservation observation;
	std::size_t presentDescriptorAllocations = 0;
#endif
	bool commandRecording = false;
	bool legacyPassActive = false;
	bool inFlight = false;
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
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;
	PipelineCacheIdentity pipelineCacheIdentity;
	std::filesystem::path pipelineCachePath;
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
	std::unordered_map<std::uint64_t, ResidentGeometryEntry> residentGeometry;
	std::vector<std::unique_ptr<ResidentPage>> residentPages;
	ImageResource fallbackTexture;

	VkCommandPool commandPool = VK_NULL_HANDLE;
	FrameResources frames[VULKAN_FRAMES_IN_FLIGHT];
	std::size_t currentFrame = 0;
	std::vector<VkSemaphore> renderingFinished;
	std::uint64_t legacyDescriptorCacheHits = 0;
	std::uint64_t legacyDescriptorCacheMisses = 0;
	std::uint64_t textureImageCreates = 0;
	std::uint64_t textureImageReuses = 0;
	std::uint64_t textureUploadBytes = 0;
	std::uint64_t residentGeometryCacheHits = 0;
	std::uint64_t residentGeometryCacheMisses = 0;
	VkDeviceSize residentGeometryBytes = 0;
	VkDeviceSize residentGeometryPeakBytes = 0;
	std::uint64_t drawableSizeQueries = 0;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	VulkanDiagnostics diagnostics;
	std::set<PipelineKey> observedPipelineKeys;
	std::set<unsigned int> observedMaskedClearPipelineKeys;
	std::string pipelineKeyDumpPath;
	bool diagnosticsEnabled = false;
	bool collectPipelineKeys = false;
#endif
	bool streamMemoryTypeReported = false;
	bool residentMemoryTypeReported = false;
	int drawableWidth = 0;
	int drawableHeight = 0;
	bool drawableSizeCheckedForFrame = false;
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

static FrameResources &currentFrame()
{
	return state.frames[state.currentFrame];
}

static void resetCommandState(FrameResources &frame)
{
	frame.commandState = CommandState();
}

static VkPipeline currentLegacyPipeline(const PipelineKey &key)
{
	const CommandState &command = currentFrame().commandState;
	if (!command.pipelineValid || !command.logicalPipelineValid ||
		command.pipelineDomain != BoundPipelineDomain::Legacy ||
		!(command.legacyPipelineKey == key))
	{
		return VK_NULL_HANDLE;
	}
	return command.pipeline;
}

static void bindGraphicsPipeline(VkPipeline pipeline, BoundPipelineDomain domain,
	const PipelineKey *legacyKey = nullptr, unsigned int maskedClearWriteMask = 0)
{
	CommandState &command = currentFrame().commandState;
	if (!command.pipelineValid || command.pipeline != pipeline)
	{
		vkCmdBindPipeline(currentFrame().commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline);
	}
	command.pipeline = pipeline;
	command.pipelineValid = true;
	command.pipelineDomain = domain;
	command.logicalPipelineValid = true;
	if (legacyKey != nullptr)
		command.legacyPipelineKey = *legacyKey;
	command.maskedClearWriteMask = maskedClearWriteMask;
}

static void setViewportState(const VkViewport &viewport)
{
	CommandState &command = currentFrame().commandState;
	const float values[6] = { viewport.x, viewport.y, viewport.width, viewport.height,
		viewport.minDepth, viewport.maxDepth };
	if (!command.viewportValid || !std::equal(values, values + 6, command.viewport))
	{
		vkCmdSetViewport(currentFrame().commandBuffer, 0, 1, &viewport);
		std::copy(values, values + 6, command.viewport);
		command.viewportValid = true;
	}
}

static void setScissorState(const VkRect2D &scissor)
{
	CommandState &command = currentFrame().commandState;
	if (!command.scissorValid || command.scissorOffset[0] != scissor.offset.x ||
		command.scissorOffset[1] != scissor.offset.y ||
		command.scissorExtent[0] != scissor.extent.width ||
		command.scissorExtent[1] != scissor.extent.height)
	{
		vkCmdSetScissor(currentFrame().commandBuffer, 0, 1, &scissor);
		command.scissorOffset[0] = scissor.offset.x;
		command.scissorOffset[1] = scissor.offset.y;
		command.scissorExtent[0] = scissor.extent.width;
		command.scissorExtent[1] = scissor.extent.height;
		command.scissorValid = true;
	}
}

static void setLineWidthState(float lineWidth)
{
	CommandState &command = currentFrame().commandState;
	if (!command.lineWidthValid || command.lineWidth != lineWidth)
	{
		vkCmdSetLineWidth(currentFrame().commandBuffer, lineWidth);
		command.lineWidth = lineWidth;
		command.lineWidthValid = true;
	}
}

static void setDepthBiasState(float constantFactor, float clamp, float slopeFactor)
{
	CommandState &command = currentFrame().commandState;
	if (!command.depthBiasValid || command.depthBias[0] != constantFactor ||
		command.depthBias[1] != clamp || command.depthBias[2] != slopeFactor)
	{
		vkCmdSetDepthBias(currentFrame().commandBuffer, constantFactor, clamp, slopeFactor);
		command.depthBias[0] = constantFactor;
		command.depthBias[1] = clamp;
		command.depthBias[2] = slopeFactor;
		command.depthBiasValid = true;
	}
}

static void appendShaderABI(std::vector<unsigned char> &bytes, unsigned char stage,
	const std::uint32_t *code, std::size_t byteSize)
{
	bytes.push_back(stage);
	const std::uint64_t encodedSize = static_cast<std::uint64_t>(byteSize);
	for (int i = 0; i < 8; i++)
		bytes.push_back(static_cast<unsigned char>((encodedSize >> (i * 8)) & 0xff));
	const unsigned char *shaderBytes = reinterpret_cast<const unsigned char *>(code);
	bytes.insert(bytes.end(), shaderBytes, shaderBytes + byteSize);
}

static std::uint64_t pipelineShaderABI()
{
	std::vector<unsigned char> bytes;
	std::size_t byteSize = 0;
	const std::uint32_t *code = legacyVertexShaderCode(byteSize);
	appendShaderABI(bytes, 0, code, byteSize);
	code = legacyFragmentShaderCode(byteSize);
	appendShaderABI(bytes, 1, code, byteSize);
	code = clearVertexShaderCode(byteSize);
	appendShaderABI(bytes, 2, code, byteSize);
	code = clearFragmentShaderCode(byteSize);
	appendShaderABI(bytes, 3, code, byteSize);
	code = presentVertexShaderCode(byteSize);
	appendShaderABI(bytes, 4, code, byteSize);
	code = presentFragmentShaderCode(byteSize);
	appendShaderABI(bytes, 5, code, byteSize);
	return pipelineCacheChecksum(bytes.data(), bytes.size());
}

static PipelineCacheIdentity currentPipelineCacheIdentity()
{
	static_assert(VK_UUID_SIZE == PIPELINE_CACHE_UUID_SIZE,
		"Vulkan pipeline cache UUID size changed");
	PipelineCacheIdentity identity;
	identity.apiVersion = state.physicalProperties.apiVersion;
	identity.keySchemaVersion = VULKAN_PIPELINE_KEY_ABI_VERSION;
	identity.shaderABI = pipelineShaderABI();
	identity.vendorID = state.physicalProperties.vendorID;
	identity.deviceID = state.physicalProperties.deviceID;
	identity.driverVersion = state.physicalProperties.driverVersion;
	std::memcpy(identity.uuid, state.physicalProperties.pipelineCacheUUID,
		PIPELINE_CACHE_UUID_SIZE);
	return identity;
}

static std::filesystem::path configuredPipelineCachePath()
{
	const char *configured = std::getenv("A126_VULKAN_PIPELINE_CACHE");
	if (configured != nullptr && std::strcmp(configured, "0") == 0)
		return std::filesystem::path();
	if (configured != nullptr && configured[0] != '\0')
		return std::filesystem::path(configured);
	return std::filesystem::path(platform::getCachePath("vulkan-pipeline-cache.bin"));
}

static void createPipelineCache()
{
	try
	{
		state.pipelineCachePath = configuredPipelineCachePath();
	}
	catch (const std::exception &)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
			std::cout << "vulkan diagnostics: pipeline_cache.load=path-error\n";
#endif
		return;
	}
	if (state.pipelineCachePath.empty())
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
			std::cout << "vulkan diagnostics: pipeline_cache.load=disabled\n";
#endif
		return;
	}

	state.pipelineCacheIdentity = currentPipelineCacheIdentity();
	PipelineCacheFileLoad loaded;
	try
	{
		loaded = loadPipelineCacheFile(state.pipelineCachePath, state.pipelineCacheIdentity);
	}
	catch (const std::exception &)
	{
		loaded.status = PipelineCacheFileStatus::ReadError;
		loaded.payload.clear();
	}
	VkPipelineCacheCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	if (loaded.status == PipelineCacheFileStatus::Accepted)
	{
		createInfo.initialDataSize = loaded.payload.size();
		createInfo.pInitialData = loaded.payload.empty() ? nullptr : loaded.payload.data();
	}
	VkResult result = vkCreatePipelineCache(state.device, &createInfo, nullptr,
		&state.pipelineCache);
	if (result != VK_SUCCESS && createInfo.initialDataSize != 0)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
		{
			std::cout << "vulkan diagnostics: pipeline_cache.driver_rejected=" <<
				static_cast<int>(result) << "\n";
		}
#endif
		state.pipelineCache = VK_NULL_HANDLE;
		createInfo.initialDataSize = 0;
		createInfo.pInitialData = nullptr;
		result = vkCreatePipelineCache(state.device, &createInfo, nullptr,
			&state.pipelineCache);
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
	{
		std::cout << "vulkan diagnostics: pipeline_cache.load=" <<
			pipelineCacheFileStatusName(loaded.status) <<
			", bytes=" << loaded.payload.size() <<
			", create_result=" << static_cast<int>(result) << '\n';
	}
#endif
	if (result != VK_SUCCESS)
	{
		state.pipelineCache = VK_NULL_HANDLE;
		state.pipelineCachePath.clear();
	}
}

static void savePipelineCache()
{
	if (state.pipelineCache == VK_NULL_HANDLE || state.pipelineCachePath.empty())
		return;

	std::vector<unsigned char> payload;
	VkResult result = VK_INCOMPLETE;
	for (int attempt = 0; attempt < 4 && result == VK_INCOMPLETE; attempt++)
	{
		std::size_t byteSize = 0;
		result = vkGetPipelineCacheData(state.device, state.pipelineCache, &byteSize, nullptr);
		if (result != VK_SUCCESS)
			break;
		if (byteSize > PIPELINE_CACHE_MAX_PAYLOAD_SIZE)
		{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			if (state.diagnosticsEnabled)
				std::cout << "vulkan diagnostics: pipeline_cache.save=payload-too-large\n";
#endif
			return;
		}
		payload.resize(byteSize);
		result = vkGetPipelineCacheData(state.device, state.pipelineCache, &byteSize,
			payload.empty() ? nullptr : payload.data());
		if (result == VK_SUCCESS)
			payload.resize(byteSize);
	}
	if (result != VK_SUCCESS)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
		{
			std::cout << "vulkan diagnostics: pipeline_cache.save=driver-error, result=" <<
				static_cast<int>(result) << '\n';
		}
#endif
		return;
	}

	PipelineCacheFileStatus status = PipelineCacheFileStatus::WriteError;
	try
	{
		status = savePipelineCacheFile(state.pipelineCachePath, state.pipelineCacheIdentity,
			payload.data(), payload.size());
	}
	catch (const std::exception &)
	{
		status = PipelineCacheFileStatus::WriteError;
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
	{
		std::cout << "vulkan diagnostics: pipeline_cache.save=" <<
			pipelineCacheFileStatusName(status) << ", bytes=" << payload.size() << '\n';
	}
#endif
}

#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
static PipelineMetrics &pipelineMetrics(PipelineMetricDomain domain)
{
	switch (domain)
	{
		case PipelineMetricDomain::Legacy: return state.diagnostics.legacyPipelines;
		case PipelineMetricDomain::MaskedClear: return state.diagnostics.maskedClearPipelines;
		case PipelineMetricDomain::Present: return state.diagnostics.presentPipelines;
	}
	return state.diagnostics.legacyPipelines;
}

static DescriptorMetrics &descriptorMetrics(DescriptorMetricDomain domain)
{
	return domain == DescriptorMetricDomain::Legacy ? state.diagnostics.legacyDescriptors :
		state.diagnostics.presentDescriptors;
}

static void resetCommandObservation(FrameResources &frame)
{
	if (state.diagnosticsEnabled)
		frame.observation = CommandObservation();
}

static void observeLegacyPipelineLookup(const PipelineKey &key)
{
	if (!state.diagnosticsEnabled)
		return;
	PipelineMetrics &metrics = state.diagnostics.legacyPipelines;
	metrics.lookups++;
	const CommandObservation &observation = currentFrame().observation;
	if (observation.logicalPipelineValid &&
		observation.pipelineDomain == PipelineMetricDomain::Legacy &&
		observation.legacyPipelineKey == key)
	{
		metrics.currentHits++;
	}
}

static void observeMaskedClearPipelineLookup(unsigned int writeMask)
{
	if (!state.diagnosticsEnabled)
		return;
	PipelineMetrics &metrics = state.diagnostics.maskedClearPipelines;
	metrics.lookups++;
	const CommandObservation &observation = currentFrame().observation;
	if (observation.logicalPipelineValid &&
		observation.pipelineDomain == PipelineMetricDomain::MaskedClear &&
		observation.maskedClearWriteMask == writeMask)
	{
		metrics.currentHits++;
	}
	state.observedMaskedClearPipelineKeys.insert(writeMask);
}

static void observePresentPipelineLookup()
{
	if (!state.diagnosticsEnabled)
		return;
	PipelineMetrics &metrics = state.diagnostics.presentPipelines;
	metrics.lookups++;
	const CommandObservation &observation = currentFrame().observation;
	if (observation.logicalPipelineValid &&
		observation.pipelineDomain == PipelineMetricDomain::Present)
	{
		metrics.currentHits++;
	}
}

static void observePipelineBind(VkPipeline pipeline, PipelineMetricDomain domain,
	const PipelineKey *legacyKey = nullptr, unsigned int maskedClearWriteMask = 0)
{
	if (!state.diagnosticsEnabled)
		return;
	CommandObservation &observation = currentFrame().observation;
	PipelineMetrics &metrics = pipelineMetrics(domain);
	metrics.binds++;
	if (observation.pipelineValid && observation.pipeline == pipeline)
		metrics.redundantBindCandidates++;
	observation.pipeline = pipeline;
	observation.pipelineValid = true;
	observation.pipelineDomain = domain;
	observation.logicalPipelineValid = true;
	if (legacyKey != nullptr)
		observation.legacyPipelineKey = *legacyKey;
	observation.maskedClearWriteMask = maskedClearWriteMask;
}

static void observeViewport(const VkViewport &viewport)
{
	if (!state.diagnosticsEnabled)
		return;
	CommandObservation &observation = currentFrame().observation;
	state.diagnostics.viewportEmits++;
	const float values[6] = { viewport.x, viewport.y, viewport.width, viewport.height,
		viewport.minDepth, viewport.maxDepth };
	if (observation.viewportValid &&
		std::equal(values, values + 6, observation.viewport))
	{
		state.diagnostics.viewportRedundantCandidates++;
	}
	std::copy(values, values + 6, observation.viewport);
	observation.viewportValid = true;
}

static void observeScissor(const VkRect2D &scissor)
{
	if (!state.diagnosticsEnabled)
		return;
	CommandObservation &observation = currentFrame().observation;
	state.diagnostics.scissorEmits++;
	if (observation.scissorValid && observation.scissorOffset[0] == scissor.offset.x &&
		observation.scissorOffset[1] == scissor.offset.y &&
		observation.scissorExtent[0] == scissor.extent.width &&
		observation.scissorExtent[1] == scissor.extent.height)
	{
		state.diagnostics.scissorRedundantCandidates++;
	}
	observation.scissorOffset[0] = scissor.offset.x;
	observation.scissorOffset[1] = scissor.offset.y;
	observation.scissorExtent[0] = scissor.extent.width;
	observation.scissorExtent[1] = scissor.extent.height;
	observation.scissorValid = true;
}

static void observeLineWidth(float lineWidth)
{
	if (!state.diagnosticsEnabled)
		return;
	CommandObservation &observation = currentFrame().observation;
	state.diagnostics.lineWidthEmits++;
	if (observation.lineWidthValid && observation.lineWidth == lineWidth)
		state.diagnostics.lineWidthRedundantCandidates++;
	observation.lineWidth = lineWidth;
	observation.lineWidthValid = true;
}

static void observeDepthBias(float constantFactor, float clamp, float slopeFactor)
{
	if (!state.diagnosticsEnabled)
		return;
	CommandObservation &observation = currentFrame().observation;
	state.diagnostics.depthBiasEmits++;
	if (observation.depthBiasValid && observation.depthBias[0] == constantFactor &&
		observation.depthBias[1] == clamp && observation.depthBias[2] == slopeFactor)
	{
		state.diagnostics.depthBiasRedundantCandidates++;
	}
	observation.depthBias[0] = constantFactor;
	observation.depthBias[1] = clamp;
	observation.depthBias[2] = slopeFactor;
	observation.depthBiasValid = true;
}

static void observeImageBarriers(ImageBarrierReason reason, std::uint64_t count)
{
	if (!state.diagnosticsEnabled)
		return;
	state.diagnostics.imageBarriersEmitted += count;
	state.diagnostics.imageBarrierReasons[static_cast<std::size_t>(reason)] += count;
}
#endif

#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
#define A126_VULKAN_DIAGNOSTIC_PARAMETER(type, name) type name
#define A126_VULKAN_DIAGNOSTIC_ARGUMENT(value) value
#define A126_VULKAN_DIAGNOSTIC_TRAILING_PARAMETER(type, name) , type name
#define A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(value) , value
#else
#define A126_VULKAN_DIAGNOSTIC_PARAMETER(type, name)
#define A126_VULKAN_DIAGNOSTIC_ARGUMENT(value)
#define A126_VULKAN_DIAGNOSTIC_TRAILING_PARAMETER(type, name)
#define A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(value)
#endif

static void submitAndWait(
	A126_VULKAN_DIAGNOSTIC_PARAMETER(LegacyPassBreakReason, reason));
static void imageBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout,
	VkImageLayout newLayout, VkAccessFlags sourceAccess, VkAccessFlags destinationAccess,
	VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage
	A126_VULKAN_DIAGNOSTIC_TRAILING_PARAMETER(ImageBarrierReason, reason));
static void transitionImage(ImageResource &image, VkImageAspectFlags aspect,
	VkImageLayout newLayout, VkAccessFlags destinationAccess,
	VkPipelineStageFlags destinationStage
	A126_VULKAN_DIAGNOSTIC_TRAILING_PARAMETER(ImageBarrierReason, reason));
static VkPipeline presentPipeline();
static void setFullscreenViewport(VkExtent2D extent);

static void requireSuccess(VkResult result, const char *operation)
{
	if (result == VK_SUCCESS)
		return;
	throw std::runtime_error(std::string(operation) + " failed with VkResult " +
		std::to_string(static_cast<int>(result)));
}

template <typename Function>
static Function requiredVulkanFunction(PFN_vkVoidFunction address, const char *name)
{
	if (address == nullptr)
		throw std::runtime_error(std::string("required Vulkan function is unavailable: ") + name);
	return reinterpret_cast<Function>(address);
}

static void loadGlobalFunctions()
{
	vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
		platform::getVulkanInstanceProcAddress());
	if (vkGetInstanceProcAddr == nullptr)
		throw std::runtime_error("Vulkan loader does not export vkGetInstanceProcAddr");

#define A126_LOAD_VULKAN_GLOBAL_FUNCTION(name) \
	name = requiredVulkanFunction<PFN_##name>( \
		vkGetInstanceProcAddr(VK_NULL_HANDLE, #name), #name);
	A126_VULKAN_GLOBAL_FUNCTIONS(A126_LOAD_VULKAN_GLOBAL_FUNCTION)
#undef A126_LOAD_VULKAN_GLOBAL_FUNCTION
}

static void loadInstanceFunctions()
{
	vkDestroyInstance = nullptr;
#define A126_LOAD_VULKAN_INSTANCE_FUNCTION(name) \
	name = requiredVulkanFunction<PFN_##name>( \
		vkGetInstanceProcAddr(state.instance, #name), #name);
	A126_VULKAN_INSTANCE_FUNCTIONS(A126_LOAD_VULKAN_INSTANCE_FUNCTION)
#undef A126_LOAD_VULKAN_INSTANCE_FUNCTION
}

static void loadDeviceFunctions()
{
	vkDestroyDevice = nullptr;
#define A126_LOAD_VULKAN_DEVICE_FUNCTION(name) \
	name = requiredVulkanFunction<PFN_##name>( \
		vkGetDeviceProcAddr(state.device, #name), #name);
	A126_VULKAN_DEVICE_FUNCTIONS(A126_LOAD_VULKAN_DEVICE_FUNCTION)
#undef A126_LOAD_VULKAN_DEVICE_FUNCTION
}

#undef A126_VULKAN_GLOBAL_FUNCTIONS
#undef A126_VULKAN_INSTANCE_FUNCTIONS
#undef A126_VULKAN_DEVICE_FUNCTIONS

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
	try
	{
		loadInstanceFunctions();
	}
	catch (...)
	{
		if (vkDestroyInstance != nullptr)
			vkDestroyInstance(state.instance, nullptr);
		state.instance = VK_NULL_HANDLE;
		throw;
	}

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
	VkFormatProperties colorFormatProperties = {};
	vkGetPhysicalDeviceFormatProperties(device, VK_FORMAT_R8G8B8A8_UNORM,
		&colorFormatProperties);
	const VkFormatFeatureFlags requiredColorFeatures =
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
		VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
		VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
		VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
		VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
	if ((colorFormatProperties.optimalTilingFeatures & requiredColorFeatures) !=
		requiredColorFeatures)
	{
		return false;
	}
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
			"swapchain, logic-op, and required RGBA8 format support was found");
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
	try
	{
		loadDeviceFunctions();
	}
	catch (...)
	{
		if (vkDestroyDevice != nullptr)
			vkDestroyDevice(state.device, nullptr);
		state.device = VK_NULL_HANDLE;
		throw;
	}
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
	const bool legacyDitherAvailable =
		deviceExtensionAvailable(state.physicalDevice, "VK_EXT_legacy_dithering");
	std::cout << "vulkan: capability_report color_target=VK_FORMAT_R8G8B8A8_UNORM"
		" texture_storage=VK_FORMAT_R8G8B8A8_UNORM sampled=native"
		" linear_filter=native transfer_src=native transfer_dst=native"
		" logic_op=native line_rasterization=" <<
		(state.lineRasterizationMode == VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT ?
			"bresenham" : "default-fallback") <<
		" wide_lines=" << (state.wideLinesSupported ? "native" : "width1-fallback") <<
		" legacy_dither=" <<
		(legacyDitherAvailable ? "available-not-enabled" : "unavailable-no-emulation") <<
		" dynamic_state=core\n";
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
	result.allocationSize = requirements.size;
	const uint32_t memoryType = findMemoryType(requirements.memoryTypeBits, required, preferred);
	result.memoryTypeIndex = memoryType;
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

static void flushBufferRange(const BufferResource &buffer, VkDeviceSize offset, VkDeviceSize size)
{
	if (buffer.coherent || buffer.memory == VK_NULL_HANDLE || size == 0)
		return;
	if (offset > buffer.allocationSize || size > buffer.allocationSize - offset)
		throw std::runtime_error("Vulkan mapped flush range exceeds its allocation");
	const VkDeviceSize atomSize = std::max<VkDeviceSize>(1,
		state.physicalProperties.limits.nonCoherentAtomSize);
	const VkDeviceSize flushOffset = offset - offset % atomSize;
	VkDeviceSize flushEnd = alignDeviceSize(offset + size, atomSize);
	flushEnd = std::min(flushEnd, buffer.allocationSize);
	VkMappedMemoryRange range = {};
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.memory = buffer.memory;
	range.offset = flushOffset;
	range.size = flushEnd - flushOffset;
	requireSuccess(vkFlushMappedMemoryRanges(state.device, 1, &range),
		"vkFlushMappedMemoryRanges(resident geometry)");
}

static bool allocateResidentRange(ResidentPage &page, VkDeviceSize size,
	VkDeviceSize alignment, VkDeviceSize &offset)
{
	for (std::size_t i = 0; i < page.freeRanges.size(); i++)
	{
		const ResidentFreeRange range = page.freeRanges[i];
		const VkDeviceSize alignedOffset = alignDeviceSize(range.offset, alignment);
		if (alignedOffset < range.offset)
			continue;
		const VkDeviceSize padding = alignedOffset - range.offset;
		if (padding > range.size || size > range.size - padding)
			continue;

		page.freeRanges.erase(page.freeRanges.begin() + static_cast<std::ptrdiff_t>(i));
		std::size_t insertion = i;
		if (padding != 0)
		{
			page.freeRanges.insert(page.freeRanges.begin() + static_cast<std::ptrdiff_t>(insertion),
				{ range.offset, padding });
			insertion++;
		}
		const VkDeviceSize remaining = range.size - padding - size;
		if (remaining != 0)
		{
			page.freeRanges.insert(page.freeRanges.begin() + static_cast<std::ptrdiff_t>(insertion),
				{ alignedOffset + size, remaining });
		}
		offset = alignedOffset;
		return true;
	}
	return false;
}

static void releaseResidentRange(ResidentPage &page, VkDeviceSize offset, VkDeviceSize size)
{
	ResidentFreeRange released = { offset, size };
	auto found = std::lower_bound(page.freeRanges.begin(), page.freeRanges.end(), offset,
		[](const ResidentFreeRange &range, VkDeviceSize value)
		{
			return range.offset < value;
		});
	found = page.freeRanges.insert(found, released);
	if (found != page.freeRanges.begin())
	{
		auto previous = found - 1;
		if (previous->offset + previous->size == found->offset)
		{
			previous->size += found->size;
			found = page.freeRanges.erase(found);
			found = previous;
		}
	}
	auto next = found + 1;
	if (next != page.freeRanges.end() && found->offset + found->size == next->offset)
	{
		found->size += next->size;
		page.freeRanges.erase(next);
	}
}

static void destroyResidentAllocation(ResidentAllocation *allocation)
{
	if (allocation == nullptr)
		return;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.residentAllocationsReclaimed++;
#endif
	if (allocation->page != nullptr)
		releaseResidentRange(*allocation->page, allocation->offset, allocation->size);
	if (state.residentGeometryBytes >= allocation->size)
		state.residentGeometryBytes -= allocation->size;
	else
		state.residentGeometryBytes = 0;
	delete allocation;
}

static std::shared_ptr<ResidentAllocation> allocateResidentGeometry(VkDeviceSize size,
	VkDeviceSize alignment)
{
	ResidentPage *selectedPage = nullptr;
	VkDeviceSize offset = 0;
	VkDeviceSize reservedSize = 0;
	const VkDeviceSize atomSize = std::max<VkDeviceSize>(1,
		state.physicalProperties.limits.nonCoherentAtomSize);
	for (const std::unique_ptr<ResidentPage> &page : state.residentPages)
	{
		const VkDeviceSize pageAlignment = page->buffer.coherent ? alignment :
			std::max(alignment, atomSize);
		const VkDeviceSize pageSize = page->buffer.coherent ? size :
			alignDeviceSize(size, atomSize);
		if (allocateResidentRange(*page, pageSize, pageAlignment, offset))
		{
			selectedPage = page.get();
			reservedSize = pageSize;
			break;
		}
	}
	if (selectedPage == nullptr)
	{
		std::unique_ptr<ResidentPage> page(new ResidentPage());
		const VkDeviceSize pageSize = std::max(VULKAN_RESIDENT_PAGE_SIZE,
			alignDeviceSize(size, std::max(alignment, atomSize)));
		page->buffer = createBuffer(pageSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
		page->freeRanges.push_back({ 0, pageSize });
		if (!state.residentMemoryTypeReported)
		{
			const VkMemoryType &memoryType =
				state.memoryProperties.memoryTypes[page->buffer.memoryTypeIndex];
			std::cout << "vulkan: resident geometry memory type=" << page->buffer.memoryTypeIndex <<
				", flags=0x" << std::hex << memoryType.propertyFlags << std::dec <<
				", heap=" << memoryType.heapIndex << '\n';
			state.residentMemoryTypeReported = true;
		}
		selectedPage = page.get();
		state.residentPages.push_back(std::move(page));
		const VkDeviceSize pageAlignment = selectedPage->buffer.coherent ? alignment :
			std::max(alignment, atomSize);
		reservedSize = selectedPage->buffer.coherent ? size : alignDeviceSize(size, atomSize);
		if (!allocateResidentRange(*selectedPage, reservedSize, pageAlignment, offset))
			throw std::runtime_error("Vulkan resident geometry page allocation failed");
	}

	ResidentAllocation *allocation = new ResidentAllocation();
	allocation->page = selectedPage;
	allocation->offset = offset;
	allocation->size = reservedSize;
	state.residentGeometryBytes += reservedSize;
	state.residentGeometryPeakBytes = std::max(state.residentGeometryPeakBytes,
		state.residentGeometryBytes);
	return std::shared_ptr<ResidentAllocation>(allocation, destroyResidentAllocation);
}

static void releaseResidentGeometry(std::uint64_t residencyId)
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled && state.residentGeometry.count(residencyId) != 0)
		state.diagnostics.residentAllocationsRetired++;
#endif
	state.residentGeometry.erase(residencyId);
}

static StreamAllocation allocateStreamBuffer(VkDeviceSize size, VkDeviceSize alignment)
{
	FrameResources &frame = currentFrame();
	for (StreamChunk &chunk : frame.streamChunks)
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
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
	if (!state.streamMemoryTypeReported)
	{
		const VkMemoryType &memoryType =
			state.memoryProperties.memoryTypes[chunk.buffer.memoryTypeIndex];
		std::cout << "vulkan: stream memory type=" << chunk.buffer.memoryTypeIndex <<
			", flags=0x" << std::hex << memoryType.propertyFlags << std::dec <<
			", heap=" << memoryType.heapIndex << '\n';
		state.streamMemoryTypeReported = true;
	}
	frame.streamChunks.push_back(chunk);
	StreamChunk &stored = frame.streamChunks.back();
	stored.used = size;
	stored.dirty = true;
	StreamAllocation allocation;
	allocation.buffer = stored.buffer.buffer;
	allocation.mapped = stored.buffer.mapped;
	return allocation;
}

static void flushStreamBuffers(FrameResources &frame)
{
	for (const StreamChunk &chunk : frame.streamChunks)
	{
		if (chunk.dirty)
			flushBuffer(chunk.buffer);
	}
}

static void resetStreamBuffers(FrameResources &frame)
{
	for (StreamChunk &chunk : frame.streamChunks)
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
	result.width = width;
	result.height = height;
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
	for (VkSemaphore semaphore : state.renderingFinished)
	{
		if (semaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(state.device, semaphore, nullptr);
	}
	state.renderingFinished.clear();
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
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	const char *presentModeName = "fifo";
	for (VkPresentModeKHR supportedMode : support.presentModes)
	{
		if (supportedMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			presentMode = supportedMode;
			presentModeName = "immediate";
			break;
		}
		if (supportedMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			presentMode = supportedMode;
			presentModeName = "mailbox";
		}
	}

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
	createInfo.presentMode = presentMode;
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
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	state.renderingFinished.resize(state.swapchainImages.size(), VK_NULL_HANDLE);
	for (VkSemaphore &semaphore : state.renderingFinished)
	{
		requireSuccess(vkCreateSemaphore(state.device, &semaphoreCreateInfo, nullptr,
			&semaphore), "vkCreateSemaphore(renderingFinished)");
	}

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
		state.swapchainExtent.height << ", images=" << state.swapchainImages.size() <<
		", present=" << presentModeName << '\n';
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
	submitAndWait(A126_VULKAN_DIAGNOSTIC_ARGUMENT(
		LegacyPassBreakReason::SwapchainRecreate));
	requireSuccess(vkDeviceWaitIdle(state.device), "vkDeviceWaitIdle");
	destroyRenderTargets();
	destroySwapchain();
	if (createSwapchain())
		createRenderTargets();
}

static bool ensureRenderTargets()
{
	if (!state.drawableSizeCheckedForFrame)
	{
		platform::getDrawableSize(state.drawableWidth, state.drawableHeight);
		state.drawableSizeQueries++;
		state.drawableSizeCheckedForFrame = true;
	}
	if (state.drawableWidth <= 0 || state.drawableHeight <= 0)
		return false;
	if (state.swapchain == VK_NULL_HANDLE ||
		state.targetExtent.width != static_cast<uint32_t>(state.drawableWidth) ||
		state.targetExtent.height != static_cast<uint32_t>(state.drawableHeight))
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

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (FrameResources &frame : state.frames)
	{
		requireSuccess(vkAllocateCommandBuffers(state.device, &allocateInfo, &frame.commandBuffer),
			"vkAllocateCommandBuffers");
		requireSuccess(vkCreateSemaphore(state.device, &semaphoreCreateInfo, nullptr,
			&frame.imageAvailable), "vkCreateSemaphore(imageAvailable)");
		requireSuccess(vkCreateFence(state.device, &fenceCreateInfo, nullptr, &frame.fence),
			"vkCreateFence");
	}
}

static void createRendererResources()
{
	VkDescriptorSetLayoutBinding legacyBindings[2] = {};
	legacyBindings[0].binding = 0;
	legacyBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
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

	// The model-view and normal matrices are the only per-draw constants, so
	// they travel as 128 bytes of push data instead of forcing a fresh dynamic
	// uniform offset and a descriptor rebind on every draw. Vulkan 1.0
	// guarantees at least 128 bytes, so no extension or limit check is needed.
	VkPushConstantRange legacyPushRange = {};
	legacyPushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	legacyPushRange.offset = 0;
	legacyPushRange.size = sizeof(VulkanDrawPush);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &state.legacyDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &legacyPushRange;
	requireSuccess(vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr,
		&state.legacyPipelineLayout), "vkCreatePipelineLayout(legacy)");
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

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
	sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
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

static VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout
	A126_VULKAN_DIAGNOSTIC_TRAILING_PARAMETER(DescriptorMetricDomain, domain))
{
	FrameResources &frame = currentFrame();
	for (;;)
	{
		if (frame.activeDescriptorPool == frame.descriptorPools.size())
			frame.descriptorPools.push_back(createDescriptorPool());
		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = frame.descriptorPools[frame.activeDescriptorPool];
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &layout;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		const VkResult result = vkAllocateDescriptorSets(state.device, &allocateInfo, &descriptorSet);
		if (result == VK_SUCCESS)
		{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			if (state.diagnosticsEnabled)
			{
				descriptorMetrics(domain).allocations++;
				if (domain == DescriptorMetricDomain::Present)
					frame.presentDescriptorAllocations++;
			}
#endif
			return descriptorSet;
		}
		if (result != VK_ERROR_OUT_OF_POOL_MEMORY && result != VK_ERROR_FRAGMENTED_POOL)
			requireSuccess(result, "vkAllocateDescriptorSets");
		frame.activeDescriptorPool++;
	}
}

static void cleanupSubmittedResources(FrameResources &frame)
{
	frame.residentAllocations.clear();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
	{
		state.diagnostics.transientBuffersRetired += frame.transientBuffers.size();
		state.diagnostics.transientBuffersReclaimed += frame.transientBuffers.size();
	}
#endif
	for (BufferResource &buffer : frame.transientBuffers)
		destroyBuffer(buffer);
	frame.transientBuffers.clear();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.imagesReclaimed += frame.retiredImages.size();
#endif
	for (ImageResource &image : frame.retiredImages)
		destroyImage(image);
	frame.retiredImages.clear();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
	{
		state.diagnostics.legacyDescriptors.invalidations += frame.legacyDescriptorCache.size();
		state.diagnostics.presentDescriptors.invalidations += frame.presentDescriptorAllocations;
	}
	frame.presentDescriptorAllocations = 0;
#endif
	frame.legacyDescriptorCache.clear();
	for (VkDescriptorPool pool : frame.descriptorPools)
		requireSuccess(vkResetDescriptorPool(state.device, pool, 0), "vkResetDescriptorPool");
	frame.activeDescriptorPool = 0;
	resetStreamBuffers(frame);
}

static void waitForFrame(FrameResources &frame)
{
	if (!frame.inFlight)
		return;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.fenceWaits++;
#endif
	requireSuccess(vkWaitForFences(state.device, 1, &frame.fence, VK_TRUE,
		std::numeric_limits<uint64_t>::max()), "vkWaitForFences");
	cleanupSubmittedResources(frame);
	frame.inFlight = false;
}

static void beginCommandRecording()
{
	FrameResources &frame = currentFrame();
	if (frame.commandRecording)
		return;
	waitForFrame(frame);
	requireSuccess(vkResetCommandBuffer(frame.commandBuffer, 0), "vkResetCommandBuffer");
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	requireSuccess(vkBeginCommandBuffer(frame.commandBuffer, &beginInfo), "vkBeginCommandBuffer");
	frame.commandRecording = true;
	resetCommandState(frame);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	resetCommandObservation(frame);
#endif

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
		vkCmdPipelineBarrier(frame.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		observeImageBarriers(ImageBarrierReason::TargetInitialization, 2);
#endif
		state.colorTarget.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		state.colorTarget.access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		state.colorTarget.stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		state.depthTarget.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		state.depthTarget.access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		state.depthTarget.stages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		state.targetsNeedTransition = false;
	}
}

static void beginLegacyPass()
{
	beginCommandRecording();
	FrameResources &frame = currentFrame();
	if (frame.legacyPassActive)
		return;
	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = state.legacyRenderPass;
	beginInfo.framebuffer = state.legacyFramebuffer;
	beginInfo.renderArea.extent = state.targetExtent;
	vkCmdBeginRenderPass(frame.commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	frame.legacyPassActive = true;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.legacyPassBegins++;
#endif
}

static void endLegacyPass(
	A126_VULKAN_DIAGNOSTIC_PARAMETER(LegacyPassBreakReason, reason))
{
	FrameResources &frame = currentFrame();
	if (!frame.legacyPassActive)
		return;
	vkCmdEndRenderPass(frame.commandBuffer);
	frame.legacyPassActive = false;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
	{
		state.diagnostics.legacyPassEnds++;
		state.diagnostics.legacyPassBreakReasons[static_cast<std::size_t>(reason)]++;
	}
#endif
}

static void submitAndWait(
	A126_VULKAN_DIAGNOSTIC_PARAMETER(LegacyPassBreakReason, reason))
{
	FrameResources &frame = currentFrame();
	if (frame.commandRecording)
	{
		endLegacyPass(A126_VULKAN_DIAGNOSTIC_ARGUMENT(reason));
		requireSuccess(vkEndCommandBuffer(frame.commandBuffer), "vkEndCommandBuffer");
		flushStreamBuffers(frame);
		requireSuccess(vkResetFences(state.device, 1, &frame.fence), "vkResetFences");
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frame.commandBuffer;
		requireSuccess(vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, frame.fence),
			"vkQueueSubmit");
		frame.commandRecording = false;
		frame.inFlight = true;
	}
	for (FrameResources &submittedFrame : state.frames)
		waitForFrame(submittedFrame);
}

static void retireImage(ImageResource &image)
{
	if (image.image == VK_NULL_HANDLE)
		return;
	beginCommandRecording();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.imagesRetired++;
#endif
	currentFrame().retiredImages.push_back(image);
	image = ImageResource();
}

static void destroyResources()
{
	if (state.device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(state.device);
		if (state.initialized)
			savePipelineCache();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
			state.diagnostics.residentAllocationsRetired += state.residentGeometry.size();
#endif
		state.residentGeometry.clear();
		for (FrameResources &frame : state.frames)
		{
			frame.commandRecording = false;
			frame.legacyPassActive = false;
			frame.inFlight = false;
			frame.residentAllocations.clear();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			if (state.diagnosticsEnabled)
			{
				state.diagnostics.transientBuffersRetired += frame.transientBuffers.size();
				state.diagnostics.transientBuffersReclaimed += frame.transientBuffers.size();
			}
#endif
			for (BufferResource &buffer : frame.transientBuffers)
				destroyBuffer(buffer);
			frame.transientBuffers.clear();
			for (StreamChunk &chunk : frame.streamChunks)
				destroyBuffer(chunk.buffer);
			frame.streamChunks.clear();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			if (state.diagnosticsEnabled)
			{
				state.diagnostics.imagesReclaimed += frame.retiredImages.size();
				state.diagnostics.legacyDescriptors.invalidations +=
					frame.legacyDescriptorCache.size();
				state.diagnostics.presentDescriptors.invalidations +=
					frame.presentDescriptorAllocations;
			}
			frame.presentDescriptorAllocations = 0;
#endif
			for (ImageResource &image : frame.retiredImages)
				destroyImage(image);
			frame.retiredImages.clear();
			frame.legacyDescriptorCache.clear();
			for (VkDescriptorPool pool : frame.descriptorPools)
				vkDestroyDescriptorPool(state.device, pool, nullptr);
			frame.descriptorPools.clear();
			if (frame.fence != VK_NULL_HANDLE)
				vkDestroyFence(state.device, frame.fence, nullptr);
			if (frame.imageAvailable != VK_NULL_HANDLE)
				vkDestroySemaphore(state.device, frame.imageAvailable, nullptr);
		}
		for (const std::unique_ptr<ResidentPage> &page : state.residentPages)
			destroyBuffer(page->buffer);
		state.residentPages.clear();
		for (std::pair<const unsigned int, VulkanTexture> &entry : state.textures)
			destroyImage(entry.second.image);
		state.textures.clear();
		destroyImage(state.fallbackTexture);
		for (const std::pair<const SamplerKey, VkSampler> &entry : state.samplers)
			vkDestroySampler(state.device, entry.second, nullptr);
		state.samplers.clear();
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
		if (state.commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(state.device, state.commandPool, nullptr);
		if (state.pipelineCache != VK_NULL_HANDLE)
			vkDestroyPipelineCache(state.device, state.pipelineCache, nullptr);
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
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	const char *diagnostics = std::getenv("A126_RENDER_DIAGNOSTICS");
	state.diagnosticsEnabled = diagnostics != nullptr && std::strcmp(diagnostics, "1") == 0;
	const char *pipelineKeyDump = std::getenv("A126_PIPELINE_KEY_DUMP");
	if (pipelineKeyDump != nullptr && pipelineKeyDump[0] != '\0')
		state.pipelineKeyDumpPath = pipelineKeyDump;
	state.collectPipelineKeys = state.diagnosticsEnabled || !state.pipelineKeyDumpPath.empty();
#endif
	state.residentGeometry.reserve(8192);
	try
	{
		platform::createWindow(platform::WindowGraphicsAPI::Vulkan);
		loadGlobalFunctions();
		createInstance();
		platform::createVulkanSurface(reinterpret_cast<void *>(state.instance), &state.surface);
		selectPhysicalDevice();
		createDevice();
		createPipelineCache();
		createCommandResources();
		createRendererResources();
		if (createSwapchain())
			createRenderTargets();
		state.initialized = true;
		std::cout << "legacygl: selected backend vulkan\n";
		std::cout << "vulkan: frames in flight=" << VULKAN_FRAMES_IN_FLIGHT << '\n';
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
	const bool targetsReady = ensureRenderTargets();
	state.drawableSizeCheckedForFrame = false;
	if (!targetsReady)
		return;
	FrameResources &frame = currentFrame();
	waitForFrame(frame);
	uint32_t imageIndex = 0;
	VkResult acquireResult = vkAcquireNextImageKHR(state.device, state.swapchain,
		std::numeric_limits<uint64_t>::max(), frame.imageAvailable, VK_NULL_HANDLE, &imageIndex);
	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapchain();
		return;
	}
	if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
		requireSuccess(acquireResult, "vkAcquireNextImageKHR");
	VkSemaphore renderingFinished = state.renderingFinished[imageIndex];

	endLegacyPass(A126_VULKAN_DIAGNOSTIC_ARGUMENT(LegacyPassBreakReason::Present));
	beginCommandRecording();
	transitionImage(state.colorTarget, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(ImageBarrierReason::PresentToSample));
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.presentDescriptors.lookups++;
#endif
	VkDescriptorSet descriptorSet = allocateDescriptorSet(state.presentDescriptorSetLayout
		A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(DescriptorMetricDomain::Present));
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
	vkCmdBeginRenderPass(frame.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.presentPassBegins++;
#endif
	const VkPipeline fullscreenPresentPipeline = presentPipeline();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observePipelineBind(fullscreenPresentPipeline, PipelineMetricDomain::Present);
#endif
	bindGraphicsPipeline(fullscreenPresentPipeline, BoundPipelineDomain::Present);
	setFullscreenViewport(state.swapchainExtent);
	vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		state.presentPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdDraw(frame.commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(frame.commandBuffer);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.presentPassEnds++;
#endif
	transitionImage(state.colorTarget, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(ImageBarrierReason::PresentToRender));
	requireSuccess(vkEndCommandBuffer(frame.commandBuffer), "vkEndCommandBuffer");

	flushStreamBuffers(frame);
	requireSuccess(vkResetFences(state.device, 1, &frame.fence), "vkResetFences");
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.imageAvailable;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderingFinished;
	requireSuccess(vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, frame.fence),
		"vkQueueSubmit");
	frame.commandRecording = false;
	frame.inFlight = true;

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderingFinished;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &state.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	VkResult presentResult = vkQueuePresentKHR(state.presentQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR ||
		acquireResult == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapchain();
	}
	else
	{
		requireSuccess(presentResult, "vkQueuePresentKHR");
		state.currentFrame = (state.currentFrame + 1) % VULKAN_FRAMES_IN_FLIGHT;
	}
}

#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
static const char *pipelineTopologyName(legacygl::Topology topology)
{
	switch (topology)
	{
		case legacygl::Topology::Points: return "points";
		case legacygl::Topology::Lines: return "lines";
		case legacygl::Topology::Triangles: return "triangles";
	}
	return "unknown";
}

static const char *formatName(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8G8B8A8_UNORM: return "VK_FORMAT_R8G8B8A8_UNORM";
		case VK_FORMAT_D24_UNORM_S8_UINT: return "VK_FORMAT_D24_UNORM_S8_UINT";
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return "VK_FORMAT_D32_SFLOAT_S8_UINT";
		case VK_FORMAT_D32_SFLOAT: return "VK_FORMAT_D32_SFLOAT";
		case VK_FORMAT_UNDEFINED: return "VK_FORMAT_UNDEFINED";
		default: return "VK_FORMAT_UNKNOWN";
	}
}

static void writePipelineKeyDump()
{
	if (state.pipelineKeyDumpPath.empty())
		return;
	std::FILE *output = std::fopen(state.pipelineKeyDumpPath.c_str(), "wb");
	if (output == nullptr)
	{
		std::fprintf(stderr, "vulkan: could not open pipeline-key dump '%s'\n",
			state.pipelineKeyDumpPath.c_str());
		return;
	}
	std::fprintf(output, "pipeline_key_dump_version=1\n");
	std::fprintf(output, "backend=vulkan\n");
	std::fprintf(output, "backend_abi=%u\n", VULKAN_PIPELINE_KEY_ABI_VERSION);
	std::fprintf(output, "target_format=%s\n", formatName(VK_FORMAT_R8G8B8A8_UNORM));
	std::fprintf(output, "target_format_value=%d\n", static_cast<int>(VK_FORMAT_R8G8B8A8_UNORM));
	std::fprintf(output, "depth_format=%s\n", formatName(state.depthFormat));
	std::fprintf(output, "depth_format_value=%d\n", static_cast<int>(state.depthFormat));
	std::fprintf(output, "samples=1\n");
	std::fprintf(output, "unique_keys=%zu\n", state.observedPipelineKeys.size());
	std::size_t index = 0;
	for (const PipelineKey &key : state.observedPipelineKeys)
	{
		std::fprintf(output,
			"key[%zu] topology=%s depth_test=%u depth_write=%u depth_function=0x%x "
			"cull_face=%u cull_face_mode=0x%x front_face_mode=0x%x blend=%u "
			"blend_source=0x%x blend_destination=0x%x logic_op=%u logic_opcode=0x%x "
			"color_write_mask=0x%x depth_bias=%u stencil_test=%u\n",
			index, pipelineTopologyName(key.topology), key.depthTest ? 1u : 0u,
			key.depthWrite ? 1u : 0u, key.depthFunction, key.cullFace ? 1u : 0u,
			key.cullFaceMode, key.frontFaceMode, key.blend ? 1u : 0u, key.blendSource,
			key.blendDestination, key.logicOp ? 1u : 0u, key.logicOpcode,
			key.colorWriteMask, key.depthBias ? 1u : 0u, key.stencilTest ? 1u : 0u);
		index++;
	}
	if (std::fclose(output) != 0)
	{
		std::fprintf(stderr, "vulkan: failed to finish pipeline-key dump '%s'\n",
			state.pipelineKeyDumpPath.c_str());
	}
}

static void reportDiagnostics()
{
	if (!state.diagnosticsEnabled)
		return;
	static const char *imageBarrierReasonNames[] = {
		"target_initialization",
		"present_to_sample",
		"present_to_render",
		"texture_new_to_transfer",
		"texture_reuse_to_transfer",
		"texture_upload_to_sample",
		"readback_to_transfer",
		"readback_to_render"
	};
	static const char *legacyPassBreakReasonNames[] = {
		"submit",
		"present",
		"texture_upload",
		"readback",
		"finish",
		"shutdown",
		"swapchain_recreate"
	};
	const VulkanDiagnostics &diagnostics = state.diagnostics;
	const PipelineMetrics *pipelineDomains[] = {
		&diagnostics.legacyPipelines,
		&diagnostics.maskedClearPipelines,
		&diagnostics.presentPipelines
	};
	const char *pipelineDomainNames[] = { "legacy", "masked_clear", "present" };
	const std::size_t pipelineUniqueKeys[] = {
		state.observedPipelineKeys.size(),
		state.observedMaskedClearPipelineKeys.size(),
		diagnostics.presentPipelines.lookups == 0 ? 0u : 1u
	};
	for (std::size_t i = 0; i < 3; i++)
	{
		const PipelineMetrics &metrics = *pipelineDomains[i];
		std::cout << "vulkan diagnostics: pipeline." << pipelineDomainNames[i] <<
			".lookups=" << metrics.lookups <<
			", pipeline." << pipelineDomainNames[i] << ".current_hits=" << metrics.currentHits <<
			", pipeline." << pipelineDomainNames[i] << ".cache_hits=" << metrics.cacheHits <<
			", pipeline." << pipelineDomainNames[i] << ".creates=" << metrics.creates <<
			", pipeline." << pipelineDomainNames[i] << ".create_ns_total=" <<
				metrics.createNanoseconds <<
			", pipeline." << pipelineDomainNames[i] << ".unique_keys=" << pipelineUniqueKeys[i] <<
			", pipeline." << pipelineDomainNames[i] << ".bind_requests=" << metrics.binds <<
			", pipeline." << pipelineDomainNames[i] << ".binds_emitted=" <<
				metrics.binds - metrics.redundantBindCandidates <<
			", pipeline." << pipelineDomainNames[i] << ".binds_suppressed=" <<
				metrics.redundantBindCandidates << '\n';
	}
	const DescriptorMetrics *descriptorDomains[] = {
		&diagnostics.legacyDescriptors,
		&diagnostics.presentDescriptors
	};
	const char *descriptorDomainNames[] = { "legacy", "present" };
	for (std::size_t i = 0; i < 2; i++)
	{
		const DescriptorMetrics &metrics = *descriptorDomains[i];
		std::cout << "vulkan diagnostics: descriptor." << descriptorDomainNames[i] <<
			".lookups=" << metrics.lookups <<
			", descriptor." << descriptorDomainNames[i] << ".hits=" << metrics.hits <<
			", descriptor." << descriptorDomainNames[i] << ".allocations=" << metrics.allocations <<
			", descriptor." << descriptorDomainNames[i] << ".invalidations=" <<
				metrics.invalidations << '\n';
	}
	std::cout << "vulkan diagnostics: dynamic.viewport_requests=" << diagnostics.viewportEmits <<
		", dynamic.viewport_emits=" << diagnostics.viewportEmits -
			diagnostics.viewportRedundantCandidates <<
		", dynamic.viewport_suppressed=" << diagnostics.viewportRedundantCandidates <<
		", dynamic.scissor_requests=" << diagnostics.scissorEmits <<
		", dynamic.scissor_emits=" << diagnostics.scissorEmits -
			diagnostics.scissorRedundantCandidates <<
		", dynamic.scissor_suppressed=" << diagnostics.scissorRedundantCandidates <<
		", dynamic.line_width_requests=" << diagnostics.lineWidthEmits <<
		", dynamic.line_width_emits=" << diagnostics.lineWidthEmits -
			diagnostics.lineWidthRedundantCandidates <<
		", dynamic.line_width_suppressed=" << diagnostics.lineWidthRedundantCandidates <<
		", dynamic.depth_bias_requests=" << diagnostics.depthBiasEmits <<
		", dynamic.depth_bias_emits=" << diagnostics.depthBiasEmits -
			diagnostics.depthBiasRedundantCandidates <<
		", dynamic.depth_bias_suppressed=" << diagnostics.depthBiasRedundantCandidates << '\n';
	std::cout << "vulkan diagnostics: barrier.image_count=" << diagnostics.imageBarriersEmitted <<
		", barrier.image_skipped=" << diagnostics.imageBarriersSkipped <<
		", barrier.buffer_count=0\n";
	for (std::size_t i = 0; i < static_cast<std::size_t>(ImageBarrierReason::Count); i++)
	{
		std::cout << "vulkan diagnostics: barrier.by_reason[" << imageBarrierReasonNames[i] <<
			"]=" << diagnostics.imageBarrierReasons[i] << '\n';
	}
	std::cout << "vulkan diagnostics: render_pass.legacy.begin_count=" <<
		diagnostics.legacyPassBegins << ", render_pass.legacy.end_count=" <<
		diagnostics.legacyPassEnds << '\n';
	std::cout << "vulkan diagnostics: render_pass.present.begin_count=" <<
		diagnostics.presentPassBegins << ", render_pass.present.end_count=" <<
		diagnostics.presentPassEnds << '\n';
	for (std::size_t i = 0; i < static_cast<std::size_t>(LegacyPassBreakReason::Count); i++)
	{
		std::cout << "vulkan diagnostics: render_pass.legacy.break_reason[" <<
			legacyPassBreakReasonNames[i] << "]=" << diagnostics.legacyPassBreakReasons[i] << '\n';
	}
	std::cout << "vulkan diagnostics: retire.images_queued=" << diagnostics.imagesRetired <<
		", retire.images_reclaimed=" << diagnostics.imagesReclaimed <<
		", retire.resident_allocations_queued=" << diagnostics.residentAllocationsRetired <<
		", retire.resident_allocations_reclaimed=" <<
			diagnostics.residentAllocationsReclaimed <<
		", retire.transient_buffers_queued=" << diagnostics.transientBuffersRetired <<
		", retire.transient_buffers_reclaimed=" << diagnostics.transientBuffersReclaimed << '\n';
	std::cout << "vulkan diagnostics: retire.objects_queued=" << diagnostics.imagesRetired +
		diagnostics.residentAllocationsRetired + diagnostics.transientBuffersRetired <<
		", retire.objects_reclaimed=" << diagnostics.imagesReclaimed +
		diagnostics.residentAllocationsReclaimed + diagnostics.transientBuffersReclaimed << '\n';
	std::cout << "vulkan diagnostics: sync.fence_waits=" << diagnostics.fenceWaits <<
		", sync.finish_drains=" << diagnostics.finishDrains << '\n';
}
#endif

static void shutdown()
{
	if (!state.initialized && state.instance == VK_NULL_HANDLE)
		return;
	if (state.initialized)
		submitAndWait(A126_VULKAN_DIAGNOSTIC_ARGUMENT(LegacyPassBreakReason::Shutdown));
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	writePipelineKeyDump();
#endif
	const std::uint64_t residentCacheHits = state.residentGeometryCacheHits;
	const std::uint64_t residentCacheMisses = state.residentGeometryCacheMisses;
	const VkDeviceSize residentBytes = state.residentGeometryPeakBytes;
	const std::size_t residentPages = state.residentPages.size();
	destroyResources();
	const unsigned int validationErrors = state.validationErrorCount;
	const std::uint64_t descriptorCacheHits = state.legacyDescriptorCacheHits;
	const std::uint64_t descriptorCacheMisses = state.legacyDescriptorCacheMisses;
	const std::uint64_t textureImageCreates = state.textureImageCreates;
	const std::uint64_t textureImageReuses = state.textureImageReuses;
	const std::uint64_t textureUploadBytes = state.textureUploadBytes;
	const std::uint64_t drawableSizeQueries = state.drawableSizeQueries;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	reportDiagnostics();
#endif
	state = State();
	std::cout << "vulkan: shutdown, validation errors=" << validationErrors <<
		", descriptor cache hits=" << descriptorCacheHits <<
		", misses=" << descriptorCacheMisses <<
		", texture images=" << textureImageCreates << " created/" << textureImageReuses << " reused" <<
		", upload bytes=" << textureUploadBytes <<
		", resident cache hits=" << residentCacheHits <<
		", misses=" << residentCacheMisses <<
		", resident bytes=" << residentBytes <<
		", pages=" << residentPages <<
		", drawable size queries=" << drawableSizeQueries << '\n';
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

static std::size_t alignedRowSize(std::size_t rowSize, int alignment)
{
	const std::size_t value = static_cast<std::size_t>(alignment);
	return (rowSize + value - 1) / value * value;
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

static void writeGPUVertices(const legacygl::ResolvedDraw &command, int verticesPerPrimitive,
	void *destination)
{
	unsigned char *write = static_cast<unsigned char *>(destination);
	for (const legacygl::CanonicalPrimitive &primitive : command.primitives->primitives)
	{
		const legacygl::Vertex &flat = command.geometry->vertices[
			static_cast<std::size_t>(primitive.provoking)];
		for (int i = 0; i < verticesPerPrimitive; i++)
		{
			const legacygl::Vertex &vertex = command.geometry->vertices[
				static_cast<std::size_t>(primitive.indices[i])];
			const VulkanGPUVertex gpuVertex = makeGPUVertex(vertex, flat);
			std::memcpy(write, &gpuVertex, sizeof(gpuVertex));
			write += sizeof(gpuVertex);
		}
	}
}

static const ResidentGeometryEntry &residentGeometryEntry(
	const legacygl::ResolvedDraw &command, int verticesPerPrimitive,
	std::size_t vertexCount, VkDeviceSize vertexBytes)
{
	auto found = state.residentGeometry.find(command.geometryResidencyId);
	if (found != state.residentGeometry.end())
	{
		if (found->second.topology != command.primitives->topology ||
			found->second.vertexCount != vertexCount ||
			found->second.hasColor != command.geometry->hasColor ||
			found->second.hasNormal != command.geometry->hasNormal ||
			found->second.hasTexCoord != command.geometry->hasTexCoord)
		{
			throw std::runtime_error("Vulkan resident geometry identity changed while cached");
		}
		state.residentGeometryCacheHits++;
		return found->second;
	}

	if (vertexCount > std::numeric_limits<uint32_t>::max())
		throw std::runtime_error("Vulkan resident geometry exceeds the draw-count range");
	state.residentGeometryCacheMisses++;
	ResidentGeometryEntry entry;
	entry.allocation = allocateResidentGeometry(vertexBytes,
		static_cast<VkDeviceSize>(alignof(VulkanGPUVertex)));
	entry.topology = command.primitives->topology;
	entry.vertexCount = static_cast<uint32_t>(vertexCount);
	entry.hasColor = command.geometry->hasColor;
	entry.hasNormal = command.geometry->hasNormal;
	entry.hasTexCoord = command.geometry->hasTexCoord;
	void *destination = static_cast<unsigned char *>(entry.allocation->page->buffer.mapped) +
		entry.allocation->offset;
	writeGPUVertices(command, verticesPerPrimitive, destination);
	flushBufferRange(entry.allocation->page->buffer, entry.allocation->offset, vertexBytes);
	return state.residentGeometry.emplace(command.geometryResidencyId,
		std::move(entry)).first->second;
}

static void fillGPUState(const legacygl::ResolvedDraw &command, VulkanGPUState &gpuState)
{
	// modelView and normal now arrive as push constants. Their slots stay in the
	// block so every following std140 offset is unchanged, but they are left
	// zeroed so two draws that differ only by transform compare equal here.
	std::memcpy(gpuState.projection, command.projection.m, sizeof(gpuState.projection));
	std::memcpy(gpuState.texture, command.textureMatrix.m, sizeof(gpuState.texture));
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
	gpuState.currentColor[0] = command.currentAttributes.r;
	gpuState.currentColor[1] = command.currentAttributes.g;
	gpuState.currentColor[2] = command.currentAttributes.b;
	gpuState.currentColor[3] = command.currentAttributes.a;
	gpuState.currentNormal[0] = command.currentAttributes.nx;
	gpuState.currentNormal[1] = command.currentAttributes.ny;
	gpuState.currentNormal[2] = command.currentAttributes.nz;
	gpuState.currentTexCoord[0] = command.currentAttributes.s;
	gpuState.currentTexCoord[1] = command.currentAttributes.t;
	gpuState.flags4[0] = command.geometry->hasColor ? 1u : 0u;
	gpuState.flags4[1] = command.geometry->hasNormal ? 1u : 0u;
	gpuState.flags4[2] = command.geometry->hasTexCoord ? 1u : 0u;
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
	requireSuccess(vkCreateGraphicsPipelines(state.device, state.pipelineCache, 1, &createInfo, nullptr,
		&pipeline), "vkCreateGraphicsPipelines(legacy)");
	return pipeline;
}

static PipelineKey legacyPipelineKey(const legacygl::ResolvedDraw &command)
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
	return key;
}

static VkPipeline legacyPipeline(const PipelineKey &key)
{
	if (key.logicOp && !state.logicOpSupported)
		throw std::runtime_error("Vulkan device cannot emulate enabled GL_COLOR_LOGIC_OP");
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeLegacyPipelineLookup(key);
	if (state.collectPipelineKeys)
		state.observedPipelineKeys.insert(key);
#endif
	const VkPipeline current = currentLegacyPipeline(key);
	if (current != VK_NULL_HANDLE)
		return current;
	auto found = state.pipelines.find(key);
	if (found != state.pipelines.end())
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
			state.diagnostics.legacyPipelines.cacheHits++;
#endif
		return found->second;
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	std::chrono::steady_clock::time_point creationStart;
	if (state.diagnosticsEnabled)
		creationStart = std::chrono::steady_clock::now();
#endif
	VkPipeline pipeline = createLegacyPipeline(key);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
	{
		const std::chrono::steady_clock::time_point creationEnd =
			std::chrono::steady_clock::now();
		state.diagnostics.legacyPipelines.creates++;
		state.diagnostics.legacyPipelines.createNanoseconds += static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(
			creationEnd - creationStart).count());
	}
#endif
	state.pipelines.emplace(key, pipeline);
	return pipeline;
}

static void imageBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout,
	VkImageLayout newLayout, VkAccessFlags sourceAccess, VkAccessFlags destinationAccess,
	VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage
	A126_VULKAN_DIAGNOSTIC_TRAILING_PARAMETER(ImageBarrierReason, reason))
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
	vkCmdPipelineBarrier(currentFrame().commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0,
		nullptr, 1, &barrier);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeImageBarriers(reason, 1);
#endif
}

static void transitionImage(ImageResource &image, VkImageAspectFlags aspect,
	VkImageLayout newLayout, VkAccessFlags destinationAccess,
	VkPipelineStageFlags destinationStage
	A126_VULKAN_DIAGNOSTIC_TRAILING_PARAMETER(ImageBarrierReason, reason))
{
	if (image.image == VK_NULL_HANDLE)
		throw std::runtime_error("Vulkan image transition received a null resource");
	if (image.layout == newLayout && image.access == destinationAccess &&
		image.stages == destinationStage)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
			state.diagnostics.imageBarriersSkipped++;
#endif
		return;
	}
	imageBarrier(image.image, aspect, image.layout, newLayout, image.access,
		destinationAccess, image.stages, destinationStage
		A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(reason));
	image.layout = newLayout;
	image.access = destinationAccess;
	image.stages = destinationStage;
}

static void uploadRGBAImage(ImageResource &image, const unsigned char *pixels, int width, int height)
{
	endLegacyPass(A126_VULKAN_DIAGNOSTIC_ARGUMENT(LegacyPassBreakReason::TextureUpload));
	beginCommandRecording();
	const uint32_t imageWidth = static_cast<uint32_t>(width);
	const uint32_t imageHeight = static_cast<uint32_t>(height);
	const bool reuseImage = image.image != VK_NULL_HANDLE && image.width == imageWidth &&
		image.height == imageHeight;
	if (!reuseImage)
	{
		retireImage(image);
		image = createImage(imageWidth, imageHeight, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT);
		state.textureImageCreates++;
	}
	else
	{
		state.textureImageReuses++;
	}
	const VkDeviceSize byteSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;
	state.textureUploadBytes += static_cast<std::uint64_t>(byteSize);
	StreamAllocation staging = allocateStreamBuffer(byteSize, 4);
	std::memcpy(staging.mapped, pixels, static_cast<std::size_t>(byteSize));
	VkBufferImageCopy copy = {};
	copy.bufferOffset = staging.offset;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.layerCount = 1;
	copy.imageExtent = { imageWidth, imageHeight, 1 };
	transitionImage(image, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
		A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(reuseImage ?
			ImageBarrierReason::TextureReuseToTransfer : ImageBarrierReason::TextureNewToTransfer));
	vkCmdCopyBufferToImage(currentFrame().commandBuffer, staging.buffer, image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
	transitionImage(image, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(ImageBarrierReason::TextureUploadToSample));
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

static VkDescriptorSet legacyDescriptorSet(VkBuffer uniformBuffer, const TextureBinding &texture)
{
	FrameResources &frame = currentFrame();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
		state.diagnostics.legacyDescriptors.lookups++;
#endif
	for (const LegacyDescriptorEntry &entry : frame.legacyDescriptorCache)
	{
		if (entry.uniformBuffer == uniformBuffer && entry.imageView == texture.view &&
			entry.sampler == texture.sampler)
		{
			state.legacyDescriptorCacheHits++;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			if (state.diagnosticsEnabled)
				state.diagnostics.legacyDescriptors.hits++;
#endif
			return entry.descriptorSet;
		}
	}

	state.legacyDescriptorCacheMisses++;
	LegacyDescriptorEntry entry;
	entry.uniformBuffer = uniformBuffer;
	entry.imageView = texture.view;
	entry.sampler = texture.sampler;
	entry.descriptorSet = allocateDescriptorSet(state.legacyDescriptorSetLayout
		A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(DescriptorMetricDomain::Legacy));
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.range = sizeof(VulkanGPUState);
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = texture.sampler;
	imageInfo.imageView = texture.view;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkWriteDescriptorSet writes[2] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = entry.descriptorSet;
	writes[0].dstBinding = 0;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writes[0].pBufferInfo = &bufferInfo;
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = entry.descriptorSet;
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(state.device, 2, writes, 0, nullptr);
	frame.legacyDescriptorCache.push_back(entry);
	return entry.descriptorSet;
}

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
	requireSuccess(vkCreateGraphicsPipelines(state.device, state.pipelineCache, 1, &createInfo, nullptr,
		&pipeline), "vkCreateGraphicsPipelines(fullscreen)");
	return pipeline;
}

static VkPipeline clearPipeline(unsigned int writeMask)
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeMaskedClearPipelineLookup(writeMask);
#endif
	auto found = state.clearPipelines.find(writeMask);
	if (found != state.clearPipelines.end())
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
			state.diagnostics.maskedClearPipelines.cacheHits++;
#endif
		return found->second;
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	std::chrono::steady_clock::time_point creationStart;
	if (state.diagnosticsEnabled)
		creationStart = std::chrono::steady_clock::now();
#endif
	VkPipeline pipeline = createFullscreenPipeline(state.clearVertexShader, state.clearFragmentShader,
		state.clearPipelineLayout, state.legacyRenderPass, writeMask);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnosticsEnabled)
	{
		const std::chrono::steady_clock::time_point creationEnd =
			std::chrono::steady_clock::now();
		state.diagnostics.maskedClearPipelines.creates++;
		state.diagnostics.maskedClearPipelines.createNanoseconds += static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(
			creationEnd - creationStart).count());
	}
#endif
	state.clearPipelines.emplace(writeMask, pipeline);
	return pipeline;
}

static VkPipeline presentPipeline()
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observePresentPipelineLookup();
#endif
	if (state.presentPipeline == VK_NULL_HANDLE)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		std::chrono::steady_clock::time_point creationStart;
		if (state.diagnosticsEnabled)
			creationStart = std::chrono::steady_clock::now();
#endif
		state.presentPipeline = createFullscreenPipeline(state.presentVertexShader,
			state.presentFragmentShader, state.presentPipelineLayout, state.renderPass,
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnosticsEnabled)
		{
			const std::chrono::steady_clock::time_point creationEnd =
				std::chrono::steady_clock::now();
			state.diagnostics.presentPipelines.creates++;
			state.diagnostics.presentPipelines.createNanoseconds += static_cast<std::uint64_t>(
				std::chrono::duration_cast<std::chrono::nanoseconds>(
				creationEnd - creationStart).count());
		}
#endif
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	else if (state.diagnosticsEnabled)
	{
		state.diagnostics.presentPipelines.cacheHits++;
	}
#endif
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
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeViewport(viewport);
#endif
	setViewportState(viewport);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeScissor(scissor);
#endif
	setScissorState(scissor);
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
			if (name == 0)
				continue;
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
		{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			if (state.diagnosticsEnabled)
				state.diagnostics.finishDrains++;
#endif
			submitAndWait(A126_VULKAN_DIAGNOSTIC_ARGUMENT(LegacyPassBreakReason::Finish));
		}
	}

	bool wantsCanonicalGeometry() const override
	{
		return true;
	}

	void releaseCanonicalGeometry(std::uint64_t residencyId) override
	{
		releaseResidentGeometry(residencyId);
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
		VkImageAspectFlags depthStencilAspect = 0;
		if ((command.mask & GL_DEPTH_BUFFER_BIT) != 0 && command.depthWrite)
			depthStencilAspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
		if ((command.mask & GL_STENCIL_BUFFER_BIT) != 0 &&
			(state.depthFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
			state.depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT))
		{
			depthStencilAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		if (depthStencilAspect != 0)
		{
			attachments[attachmentCount].aspectMask = depthStencilAspect;
			attachments[attachmentCount].clearValue.depthStencil.depth =
				static_cast<float>(std::max(0.0, std::min(1.0, command.depth)));
			attachments[attachmentCount].clearValue.depthStencil.stencil = 0;
			attachmentCount++;
		}
		if (attachmentCount != 0)
		{
			VkClearRect rectangle = {};
			rectangle.rect.extent = state.targetExtent;
			rectangle.layerCount = 1;
			vkCmdClearAttachments(currentFrame().commandBuffer, attachmentCount, attachments, 1, &rectangle);
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
			const VkPipeline maskedClearPipeline = clearPipeline(writeMask);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			observePipelineBind(maskedClearPipeline, PipelineMetricDomain::MaskedClear,
				nullptr, writeMask);
#endif
			bindGraphicsPipeline(maskedClearPipeline, BoundPipelineDomain::MaskedClear,
				nullptr, writeMask);
			setFullscreenViewport(state.targetExtent);
			vkCmdPushConstants(currentFrame().commandBuffer, state.clearPipelineLayout,
				VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
			vkCmdDraw(currentFrame().commandBuffer, 3, 1, 0, 0);
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

		const int verticesPerPrimitive = command.primitives->topology == legacygl::Topology::Points ? 1 :
			(command.primitives->topology == legacygl::Topology::Lines ? 2 : 3);
		const std::size_t vertexCount = command.primitives->primitives.size() *
			static_cast<std::size_t>(verticesPerPrimitive);
		if (vertexCount == 0)
			return;
		if (vertexCount > std::numeric_limits<uint32_t>::max() ||
			vertexCount > std::numeric_limits<VkDeviceSize>::max() / sizeof(VulkanGPUVertex))
		{
			throw std::runtime_error("Vulkan geometry exceeds the draw-count range");
		}

		VulkanGPUState gpuState = {};
		const TextureBinding texture = [&]()
		{
			legacygl::PhaseScope phase(legacygl::DrawPhase::StatePack);
			fillGPUState(command, gpuState);
			return bindTextureState(command, gpuState);
		}();
		beginLegacyPass();
		const VkDeviceSize vertexBytes = static_cast<VkDeviceSize>(
			vertexCount * sizeof(VulkanGPUVertex));
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkDeviceSize vertexOffset = 0;
		uint32_t drawVertexCount = static_cast<uint32_t>(vertexCount);
		{
			legacygl::PhaseScope phase(legacygl::DrawPhase::Geometry);
			if (command.geometryResidencyId == 0)
			{
				const StreamAllocation vertexUpload = allocateStreamBuffer(vertexBytes,
					static_cast<VkDeviceSize>(alignof(VulkanGPUVertex)));
				writeGPUVertices(command, verticesPerPrimitive, vertexUpload.mapped);
				vertexBuffer = vertexUpload.buffer;
				vertexOffset = vertexUpload.offset;
			}
			else
			{
				const ResidentGeometryEntry &entry = residentGeometryEntry(command,
					verticesPerPrimitive, vertexCount, vertexBytes);
				currentFrame().residentAllocations.push_back(entry.allocation);
				vertexBuffer = entry.allocation->page->buffer.buffer;
				vertexOffset = entry.allocation->offset;
				drawVertexCount = entry.vertexCount;
			}
		}
		// The environment block excludes the model-view and normal matrices, so
		// consecutive draws under the same lights/fog/texture state compare
		// equal and reuse the previous upload instead of a fresh stream slot.
		CommandState &commandState = currentFrame().commandState;
		VkBuffer uniformBuffer = commandState.environmentBuffer;
		uint32_t uniformOffset = commandState.environmentOffset;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		{
			legacygl::PhaseScope phase(legacygl::DrawPhase::StateUpload);
			if (!commandState.environmentValid ||
				std::memcmp(&commandState.environment, &gpuState, sizeof(gpuState)) != 0)
			{
				const VkDeviceSize uniformAlignment = std::max<VkDeviceSize>(
					static_cast<VkDeviceSize>(alignof(VulkanGPUState)),
					state.physicalProperties.limits.minUniformBufferOffsetAlignment);
				const StreamAllocation uniformUpload = allocateStreamBuffer(sizeof(VulkanGPUState),
					uniformAlignment);
				std::memcpy(uniformUpload.mapped, &gpuState, sizeof(gpuState));
				if (uniformUpload.offset > std::numeric_limits<uint32_t>::max())
					throw std::runtime_error("Vulkan uniform stream offset exceeds the dynamic-offset range");
				uniformBuffer = uniformUpload.buffer;
				uniformOffset = static_cast<uint32_t>(uniformUpload.offset);
				std::memcpy(&commandState.environment, &gpuState, sizeof(gpuState));
				commandState.environmentBuffer = uniformBuffer;
				commandState.environmentOffset = uniformOffset;
				commandState.environmentValid = true;
			}
			descriptorSet = legacyDescriptorSet(uniformBuffer, texture);
		}

		{
		legacygl::PhaseScope bindPhase(legacygl::DrawPhase::Bind);
		const PipelineKey key = legacyPipelineKey(command);
		const VkPipeline pipeline = legacyPipeline(key);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		observePipelineBind(pipeline, PipelineMetricDomain::Legacy, &key);
#endif
		bindGraphicsPipeline(pipeline, BoundPipelineDomain::Legacy, &key);
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
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		observeViewport(viewport);
#endif
		setViewportState(viewport);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		observeScissor(scissor);
#endif
		setScissorState(scissor);
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
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		observeLineWidth(lineWidth);
#endif
		setLineWidthState(lineWidth);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		observeDepthBias(command.pipeline.polygonOffsetUnits, 0.0f,
			command.pipeline.polygonOffsetFactor);
#endif
		setDepthBiasState(command.pipeline.polygonOffsetUnits, 0.0f,
			command.pipeline.polygonOffsetFactor);
		vkCmdBindVertexBuffers(currentFrame().commandBuffer, 0, 1, &vertexBuffer, &vertexOffset);
		if (!commandState.descriptorValid || commandState.descriptorSet != descriptorSet ||
			commandState.descriptorOffset != uniformOffset)
		{
			vkCmdBindDescriptorSets(currentFrame().commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				state.legacyPipelineLayout, 0, 1, &descriptorSet, 1, &uniformOffset);
			commandState.descriptorSet = descriptorSet;
			commandState.descriptorOffset = uniformOffset;
			commandState.descriptorValid = true;
		}
		VulkanDrawPush push;
		std::memcpy(push.modelView, command.modelView.m, sizeof(push.modelView));
		std::memcpy(push.normal, command.normal.m, sizeof(push.normal));
		vkCmdPushConstants(currentFrame().commandBuffer, state.legacyPipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
		}
		{
			legacygl::PhaseScope drawPhase(legacygl::DrawPhase::Draw);
			vkCmdDraw(currentFrame().commandBuffer, drawVertexCount, 1, 0, 0);
		}
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
		const legacygl::PixelTransferFormat *transfer =
			legacygl::unsignedBytePixelTransferFormat(command.sourceFormat);
		legacygl::PixelStorageFormat storage;
		if (command.sourceType != GL_UNSIGNED_BYTE || transfer == nullptr ||
			!legacygl::pixelStorageFormat(command.internalFormat, storage) ||
			storage.physical != legacygl::PhysicalPixelFormat::RGBA8)
		{
			throw std::runtime_error("Vulkan texture upload received an unsupported pixel format");
		}
		if (command.pixels != nullptr && command.width > 0 && command.height > 0)
		{
			const int components = transfer->components;
			const std::size_t sourceRow = alignedRowSize(static_cast<std::size_t>(command.width) *
				static_cast<std::size_t>(components), command.unpackAlignment);
			const unsigned char *source = static_cast<const unsigned char *>(command.pixels);
			for (int y = 0; y < command.height; y++)
			{
				for (int x = 0; x < command.width; x++)
				{
					unsigned char rgba[4];
					if (!legacygl::decodeUnsignedBytePixel(
						source + static_cast<std::size_t>(y) * sourceRow +
							static_cast<std::size_t>(x) * static_cast<std::size_t>(components),
						command.sourceFormat, rgba) ||
						!legacygl::applyIntendedPixelFormat(storage.intended, rgba))
					{
						throw std::runtime_error("Vulkan texture upload conversion failed");
					}
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
		const legacygl::PixelTransferFormat *transfer =
			legacygl::unsignedBytePixelTransferFormat(command.format);
		if (command.type != GL_UNSIGNED_BYTE || transfer == nullptr)
			throw std::runtime_error("Vulkan readback received an unsupported pixel format");

		endLegacyPass(A126_VULKAN_DIAGNOSTIC_ARGUMENT(LegacyPassBreakReason::Readback));
		beginCommandRecording();
		const VkDeviceSize byteSize = static_cast<VkDeviceSize>(command.width) *
			static_cast<VkDeviceSize>(command.height) * 4;
		BufferResource readback = createBuffer(byteSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
		transitionImage(state.colorTarget, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
			A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(ImageBarrierReason::ReadbackToTransfer));
		VkBufferImageCopy copy = {};
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.layerCount = 1;
		copy.imageOffset.x = command.x;
		copy.imageOffset.y = static_cast<int>(state.targetExtent.height) - (command.y + command.height);
		copy.imageExtent = { static_cast<uint32_t>(command.width),
			static_cast<uint32_t>(command.height), 1 };
		vkCmdCopyImageToBuffer(currentFrame().commandBuffer, state.colorTarget.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &copy);
		transitionImage(state.colorTarget, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT(ImageBarrierReason::ReadbackToRender));
		submitAndWait(A126_VULKAN_DIAGNOSTIC_ARGUMENT(LegacyPassBreakReason::Readback));
		invalidateBuffer(readback);

		const int components = transfer->components;
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
				if (!legacygl::encodeUnsignedBytePixel(source, command.format,
					destination + static_cast<std::size_t>(y) * stride +
						static_cast<std::size_t>(x) * static_cast<std::size_t>(components)))
				{
					throw std::runtime_error("Vulkan readback conversion failed");
				}
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

#undef A126_VULKAN_DIAGNOSTIC_PARAMETER
#undef A126_VULKAN_DIAGNOSTIC_ARGUMENT
#undef A126_VULKAN_DIAGNOSTIC_TRAILING_PARAMETER
#undef A126_VULKAN_DIAGNOSTIC_TRAILING_ARGUMENT

}

namespace renderbackend
{

static const Configuration &vulkanConfiguration()
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

static void vulkanInitialize()
{
	vulkanbackend::initialize();
}

static void vulkanPresent()
{
	vulkanbackend::present();
}

static void vulkanShutdown()
{
	vulkanbackend::shutdown();
}

static bool vulkanHasCapability(const char *capability)
{
	return capability != nullptr && std::strcmp(capability, "GL_NV_fog_distance") == 0;
}

static legacygl::Sink *vulkanSink()
{
	return &vulkanbackend::sinkInstance;
}

const Backend &vulkanBackend()
{
	static const Backend backend = {
		"vulkan",
		vulkanConfiguration,
		vulkanInitialize,
		vulkanPresent,
		vulkanShutdown,
		vulkanHasCapability,
		vulkanSink
	};
	return backend;
}

}
